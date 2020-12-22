#include "Module.h"
#include <core/ProcessInfo.h>
#include <interfaces/IMemory.h>
#include <interfaces/IResourceMonitor.h>
#include <sstream>
#include <vector>

using std::endl;
using std::cerr; // TODO: temp
using std::list;
using std::stringstream;
using std::vector;

// TODO: don't create our own thread, use threadpool from WPEFramework

namespace WPEFramework {
namespace Plugin {
   class ResourceMonitorImplementation : public Exchange::IResourceMonitor {
  private:
      class Config : public Core::JSON::Container {
      public:
         enum class CollectMode {
            Single,    // Expect one process that can come and go
            Multiple,  // Expect several processes with the same name
            Callsign,  // Log Thunder process by callsign
            ClassName, // Log Thunder process by class name
            Invalid
         };

     private:
         Config& operator=(const Config&) = delete;
         
     public:
         Config()
             : Core::JSON::Container()
             , Path()
             , Interval()
             , Mode()
             , ParentName()
         {
            Add(_T("path"), &Path);
            Add(_T("interval"), &Interval);
            Add(_T("mode"), &Mode);
            Add(_T("parent-name"), &ParentName);
         }
         Config(const Config& copy)
             : Core::JSON::Container()
             , Path(copy.Path)
             , Interval(copy.Interval)
             , Mode(copy.Mode)
             , ParentName(copy.ParentName)
         {
            Add(_T("path"), &Path);
            Add(_T("interval"), &Interval);
            Add(_T("mode"), &Mode);
            Add(_T("parent-name"), &ParentName);
         }
         ~Config()
         {
         }
         
         CollectMode GetCollectMode() const
         {
             if (Mode == "single") {
                return CollectMode::Single;
             } else if(Mode == "multiple") {
                return CollectMode::Multiple;
             } else if (Mode == "callsign") {
                return CollectMode::Callsign;
             } else if (Mode == "classname") {
                return CollectMode::ClassName;
             }

             // TODO: no assert, should be logged
             ASSERT(!"Not a valid collect mode!");
             return CollectMode::Invalid;
         }

     public:
         Core::JSON::String Path;
         Core::JSON::DecUInt32 Interval;
         Core::JSON::String Mode;
         Core::JSON::String ParentName;
      };

      class StatCollecter {
     public:
         explicit StatCollecter(const Config& config)
             : _binFile(nullptr)
             , _otherMap(nullptr)
             , _ourMap(nullptr)
             , _bufferEntries(0)
             , _interval(0)
             , _collectMode(Config::CollectMode::Invalid)
             , _activity(*this)
         {
            _binFile = fopen(config.Path.Value().c_str(), "w");

            uint32_t pageCount = Core::SystemInfo::Instance().GetPhysicalPageCount();
            const uint32_t bitPersUint32 = 32;
            _bufferEntries = pageCount / bitPersUint32;
            if ((pageCount % bitPersUint32) != 0) {
               _bufferEntries++;
            }

            // Because linux doesn't report the first couple of pages it uses itself,
            //    allocate a little extra to make sure we don't miss the highest ones.
            _bufferEntries += _bufferEntries / 10;

            _ourMap = new uint32_t[_bufferEntries];
            _otherMap = new uint32_t[_bufferEntries];
            _interval = config.Interval.Value();
            _collectMode = config.GetCollectMode();
            _parentName = config.ParentName.Value();

            _activity.Submit();
         }

         ~StatCollecter()
         {
            fclose(_binFile);

            delete [] _ourMap;
            delete [] _otherMap;
         }

         void GetProcessNames(vector<string>& processNames)
         {
            _namesLock.Lock();
            processNames = _processNames;
            _namesLock.Unlock();
         }

      private:
         // TODO: combine these "Collect*" methods
         void CollectSingle()
         {
            list<Core::ProcessInfo> processes;
            Core::ProcessInfo::FindByName(_parentName, false, processes);

            // TODO: check if only one, warning otherwise?
            // TOOD: what if none found? will cause segfault when using .front() later on.
            if (processes.empty()) {
               TRACE(Trace::Error, (_T("Failed to find process %s"), _parentName));
               return;
            }

            if (processes.size() > 1) {
               TRACE(Trace::Information, (_T("Found more than one process named %s, only tracking first"), _parentName));
            }

            uint32_t mapBufferSize = sizeof(_ourMap[0]) * _bufferEntries;
            memset(_ourMap, 0, mapBufferSize);
            memset(_otherMap, 0, mapBufferSize);

            vector<::ThreadId> processIds;

            for (const Core::ProcessInfo& processInfo : processes) {
               string processName = processInfo.Name();

               _namesLock.Lock();
               if (find(_processNames.begin(), _processNames.end(), _parentName) == _processNames.end()) {
                  _processNames.push_back(_parentName);
               }
               _namesLock.Unlock();

               Core::ProcessTree processTree(processInfo.Id());

               processTree.MarkOccupiedPages(_ourMap, mapBufferSize);

               std::list<::ThreadId> addedProcessIds;
               processTree.GetProcessIds(addedProcessIds);
               processIds.insert(processIds.end(), addedProcessIds.begin(), addedProcessIds.end());
            }

            memset(_otherMap, 0, mapBufferSize);

            // Find other processes
            list<Core::ProcessInfo> otherProcesses;
            Core::ProcessInfo::Iterator otherIterator;
            while (otherIterator.Next()) {
               ::ThreadId otherId = otherIterator.Current().Id();
               if (find(processIds.begin(), processIds.end(), otherId) == processIds.end()) {
                  otherIterator.Current().MarkOccupiedPages(_otherMap, mapBufferSize);
               }
            }

            StartLogLine(1);
            LogProcess(_parentName, processes.front());
         }

         void CollectMultiple()
         {
            list<Core::ProcessInfo> processes;
            Core::ProcessInfo::FindByName(_parentName, false, processes);

            StartLogLine(processes.size());

            for (const Core::ProcessInfo& processInfo : processes) {
               uint32_t mapBufferSize = sizeof(_ourMap[0]) * _bufferEntries;
               memset(_ourMap, 0, mapBufferSize);
               memset(_otherMap, 0, mapBufferSize);

               string processName = processInfo.Name() + " (" + std::to_string(processInfo.Id()) + ")";

               _namesLock.Lock();
               if (find(_processNames.begin(), _processNames.end(), processName) == _processNames.end()) {
                  _processNames.push_back(processName);
               }
               _namesLock.Unlock();

               Core::ProcessTree processTree(processInfo.Id());

               processTree.MarkOccupiedPages(_ourMap, mapBufferSize);

               list<Core::ProcessInfo> otherProcesses;
               Core::ProcessInfo::Iterator otherIterator;
               while (otherIterator.Next()) {
                  ::ThreadId otherId = otherIterator.Current().Id();
                  if (!processTree.ContainsProcess(otherId)) {
                     otherIterator.Current().MarkOccupiedPages(_otherMap, mapBufferSize);
                  }
               }

               LogProcess(processName, processInfo);
            }
         }

         void CollectWPEProcess(const string& argument)
         {
            const string processName = "WPEProcess-1.0.0";

            list<Core::ProcessInfo> processes;
            Core::ProcessInfo::FindByName(processName, false, processes);

            vector<std::pair<Core::ProcessInfo, string> > processIds;
            for (const Core::ProcessInfo& processInfo : processes) {
               std::list<string> commandLine = processInfo.CommandLine();

               bool shouldTrack = false;
               string columnName;

               // Get callsign/classname
               std::list<string>::const_iterator i = std::find(commandLine.cbegin(), commandLine.cend(), argument);
               if (i != commandLine.cend()) {
                  i++;
                  if (i != commandLine.cend()) {
                     if (*i == _parentName) {
                        columnName = _parentName + " (" + std::to_string(processInfo.Id()) + ")";
                        processIds.push_back(std::pair<::ThreadId, string>(processInfo.Id(), columnName));
                        shouldTrack = true;
                     }
                  }
               }

               if (!shouldTrack) {
                  continue;
               }

               _namesLock.Lock();
               if (std::find(_processNames.cbegin(), _processNames.cend(), columnName) == _processNames.cend()) {
                  _processNames.push_back(columnName);
               }
               _namesLock.Unlock();
            }

            StartLogLine(processIds.size());
            for (std::pair<Core::ProcessInfo, string> processDesc : processIds) {
               Core::ProcessTree tree(processDesc.first);

               uint32_t mapBufferSize = sizeof(_ourMap[0]) * _bufferEntries;
               memset(_ourMap, 0, mapBufferSize);
               memset(_otherMap, 0, mapBufferSize);

               tree.MarkOccupiedPages(_ourMap, mapBufferSize);

               list<Core::ProcessInfo> otherProcesses;
               Core::ProcessInfo::Iterator otherIterator;
               while (otherIterator.Next()) {
                  ::ThreadId otherId = otherIterator.Current().Id();
                  if (!tree.ContainsProcess(otherId)) {
                     otherIterator.Current().MarkOccupiedPages(_otherMap, mapBufferSize);
                  }
               }

               LogProcess(processDesc.second, processDesc.first);
            }
         }

     protected:
         void Dispatch()
         {
            switch(_collectMode) {
               case Config::CollectMode::Single:
                  CollectSingle();
                  break;
               case Config::CollectMode::Multiple: 
                  CollectMultiple();
                  break;
               case Config::CollectMode::Callsign: 
                  CollectWPEProcess("-C");
                  break;
               case Config::CollectMode::ClassName:
                  CollectWPEProcess("-c");
                  break;
               case Config::CollectMode::Invalid:
                  // TODO: ASSERT?
                  break;
            }

            _activity.Schedule(Core::Time::Now().Add(_interval * 1000));
         }

    private:
         uint32_t CountSetBits(uint32_t pageBuffer[], const uint32_t* inverseMask)
         {
            uint32_t count = 0;

            if (inverseMask == nullptr) {
               for (uint32_t index = 0; index < _bufferEntries; index++) {
                  count += __builtin_popcount(pageBuffer[index]);
               }
            } else {
               for (uint32_t index = 0; index < _bufferEntries; index++) {
                  count += __builtin_popcount(pageBuffer[index] & (~inverseMask[index]));
               }
            }

            return count;
         }

         void LogProcess(const string& name, const Core::ProcessInfo& info)
         {
            uint32_t vss = CountSetBits(_ourMap, nullptr);
            uint32_t uss = CountSetBits(_ourMap, _otherMap);
            uint64_t jiffies = info.Jiffies();

            uint32_t nameSize = name.length();
            fwrite(&nameSize, sizeof(nameSize), 1, _binFile);
            fwrite(name.c_str(), sizeof(name[0]), name.length(), _binFile);
            fwrite(&vss, 1, sizeof(vss), _binFile);
            fwrite(&uss, 1, sizeof(uss), _binFile);
            fwrite(&jiffies, 1, sizeof(jiffies), _binFile);
            fflush(_binFile);
         }

         void StartLogLine(uint32_t processCount)
         {
            // TODO: no simple time_t alike in Thunder?
            uint32_t timestamp = static_cast<uint32_t>(Core::Time::Now().Ticks() / 1000 / 1000);
            uint64_t jiffies = Core::SystemInfo::Instance().GetJiffies();

            fwrite(&timestamp, 1, sizeof(timestamp), _binFile);
            fwrite(&processCount, 1, sizeof(processCount), _binFile);
            fwrite(&jiffies, 1, sizeof(jiffies), _binFile);
         }

         FILE *_binFile;
         vector<string> _processNames; // Seen process names.
         Core::CriticalSection _namesLock;
         uint32_t * _otherMap; // Buffer used to mark other processes pages.
         uint32_t * _ourMap;   // Buffer for pages used by our process (tree).
         uint32_t _bufferEntries; // Numer of entries in each buffer.
         uint32_t _interval; // Seconds between measurement.
         Config::CollectMode _collectMode; // Collection style.
         string _parentName; // Process/plugin name we are looking for.
         Core::WorkerPool::JobType<StatCollecter&> _activity;

         friend Core::ThreadPool::JobType<StatCollecter&>;
      };

  private:
      ResourceMonitorImplementation(const ResourceMonitorImplementation&) = delete;
      ResourceMonitorImplementation& operator=(const ResourceMonitorImplementation&) = delete;

  public:
      ResourceMonitorImplementation()
          : _processThread(nullptr)
          , _binPath(_T("/tmp/resource-log.bin"))
      {
      }

      virtual ~ResourceMonitorImplementation()
      {
         if (_processThread != nullptr) {
            delete _processThread;
            _processThread = nullptr;
         }
      }

      virtual uint32_t Configure(PluginHost::IShell* service) override
      {
         uint32_t result(Core::ERROR_INCOMPLETE_CONFIG);

         ASSERT(service != nullptr);

         Config config;
         config.FromString(service->ConfigLine());
         if (config.Path.IsSet() == true) {
             _binPath = config.Path.Value().c_str();
         }

         result = Core::ERROR_NONE;

         _processThread = new StatCollecter(config);

         return (result);
      }

      string CompileMemoryCsv() override
      {
         // TODO: should we worry about doing this as repsonse to RPC (could take too long?)
         FILE* inFile = fopen(_binPath.c_str(), "rb");

         stringstream output;
         if (inFile != nullptr) {

            vector<string> processNames;
            _processThread->GetProcessNames(processNames);

            output << _T("time (s)\tJiffies");
            for (const string& processName : processNames) {
               output << _T("\t") << processName << _T(" (VSS)\t") << processName << _T(" (USS)\t") << processName << _T(" (jiffies)");
            }
            output << endl;

            vector<uint64_t> pageVector(processNames.size() * 3);
            bool seenFirstTimestamp = false;
            uint32_t firstTimestamp = 0;

            while (true) {
               std::fill(pageVector.begin(), pageVector.end(), 0);

               uint32_t timestamp = 0;
               size_t readCount = fread(&timestamp, sizeof(timestamp), 1, inFile);
               if (readCount != 1) {
                  break;
               }

               if (!seenFirstTimestamp) {
                  firstTimestamp = timestamp;
                  seenFirstTimestamp = true;
               }

               uint32_t processCount = 0;
               fread(&processCount, sizeof(processCount), 1, inFile);

               uint64_t totalJiffies = 0;
               fread(&totalJiffies, sizeof(totalJiffies), 1, inFile);

               for (uint32_t processIndex = 0; processIndex < processCount; processIndex++) {
                  uint32_t nameLength = 0;
                  fread(&nameLength, sizeof(nameLength), 1, inFile);
                  // TODO: unicode?
                  char nameBuffer[nameLength + 1];
                  fread(nameBuffer, sizeof(char), nameLength, inFile);

                  nameBuffer[nameLength] = '\0';
                  string name(nameBuffer);

                  vector<string>::const_iterator nameIterator = std::find(processNames.cbegin(), processNames.cend(), name);

                  uint32_t vss, uss;
                  uint64_t jiffies;
                  fread(&vss, sizeof(vss), 1, inFile);
                  fread(&uss, sizeof(uss), 1, inFile);
                  fread(&jiffies, sizeof(jiffies), 1, inFile);
                  if (nameIterator == processNames.cend()) {
                     continue;
                  }

                  int index = nameIterator - processNames.cbegin();

                  pageVector[index * 3] = static_cast<uint64_t>(vss);
                  pageVector[index * 3 + 1] = static_cast<uint64_t>(uss);
                  pageVector[index * 3 + 2] = jiffies;
               }

               output << (timestamp - firstTimestamp) << "\t" << totalJiffies;
               for (uint32_t pageEntry : pageVector) {
                  output << "\t" << pageEntry;
               }
               output << endl;
            }

            fclose(inFile);
         }

         return output.str();
      }

      BEGIN_INTERFACE_MAP(ResourceMonitorImplementation)
      INTERFACE_ENTRY(Exchange::IResourceMonitor)
      END_INTERFACE_MAP

  private:
      StatCollecter* _processThread;
      string _binPath;
   };

   SERVICE_REGISTRATION(ResourceMonitorImplementation, 1, 0);
} /* namespace Plugin */
} // namespace ResourceMonitor
