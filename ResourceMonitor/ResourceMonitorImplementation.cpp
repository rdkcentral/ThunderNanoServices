#include "Module.h"
#include <core/ProcessInfo.h>
#include <interfaces/IMemory.h>
#include <interfaces/IResourceMonitor.h>
#include <sstream>
#include <vector>

using std::endl;
using std::list;
using std::stringstream;
using std::vector;

namespace WPEFramework {
namespace Plugin {
   // TODO: read name via link from "/usr/bin/WPEFramework" ?
   const string g_parentProcessName = _T("WPEFramework-1.0.0");

   class ResourceMonitorImplementation : public Exchange::IResourceMonitor {
  private:
      class Config : public Core::JSON::Container {
     private:
         Config& operator=(const Config&) = delete;

     public:
         Config()
             : Core::JSON::Container()
             , Path()
             , Interval()
         {
            Add(_T("path"), &Path);
            Add(_T("interval"), &Interval);
         }
         Config(const Config& copy)
             : Core::JSON::Container()
             , Path(copy.Path)
             , Interval(copy.Interval)
         {
            Add(_T("path"), &Path);
            Add(_T("interval"), &Interval);
         }
         ~Config()
         {
         }

     public:
         Core::JSON::String Path;
         Core::JSON::DecUInt32 Interval;
      };

      class ProcessThread : public Core::Thread {
     public:
         explicit ProcessThread(const Config& config)
             : _binFile(nullptr)
             , _otherMap(nullptr)
             , _ourMap(nullptr)
             , _bufferEntries(0)
             , _interval(0)
         {
            _binFile = fopen(config.Path.Value().c_str(), "w");

            uint32_t pageCount = Core::SystemInfo::Instance().GetPhysicalPageCount();
            const uint32_t bitPersUint32 = 32;
            _bufferEntries = pageCount / bitPersUint32;
            if ((pageCount % bitPersUint32) != 0) {
               _bufferEntries++;
            }

            _ourMap = new uint32_t[_bufferEntries];
            _otherMap = new uint32_t[_bufferEntries];
            _interval = config.Interval.Value();

            _namesLock.Lock();
            if (_processNames.empty()) {
                // TODO: can check empty without locking mutex?
                _processNames.push_back(g_parentProcessName);
            }
            _namesLock.Unlock();

            list<Core::ProcessInfo> parentProcesses;
            Core::ProcessInfo::FindByName(g_parentProcessName, false, parentProcesses);

            if (parentProcesses.empty()) {
               TRACE_L1("ERROR: Found no WPEFramework processes!!");
               // TODO: is this the best way to cancel thread creation?
               Core::Thread::Stop();
               Core::Thread::Wait(STOPPED, Core::infinite);
               return;
            }

            if (parentProcesses.size() > 1) {
               TRACE_L1("Warning: found more than one WPEFramework process, only looking at first");
            }

            _parentProcess = parentProcesses.front();
         }

         ~ProcessThread()
         {
            Core::Thread::Stop();
            Core::Thread::Wait(STOPPED, Core::infinite);

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

     protected:
         virtual uint32_t Worker()
         {
            Core::ProcessTree parentTree(_parentProcess.Id());

            list<Core::ProcessTree> childTrees;

            Core::ProcessInfo::Iterator childIterator = _parentProcess.Children();
            while (childIterator.Next()) {
               childTrees.emplace_back(childIterator.Current().Id());
            }

            // Find non-WPEFramework processes
            list<Core::ProcessInfo> otherProcesses;
            Core::ProcessInfo::Iterator otherIterator;
            while (otherIterator.Next()) {
               uint32_t otherId = otherIterator.Current().Id();
               if (!parentTree.ContainsProcess(otherId)) {
                  otherProcesses.push_back(otherIterator.Current());
               }
            }

            uint32_t mapBufferSize = sizeof(_otherMap[0]) * _bufferEntries;
            memset(_otherMap, 0, mapBufferSize);

            for (const Core::ProcessInfo& otherProcess : otherProcesses) {
               otherProcess.MarkOccupiedPages(_otherMap, mapBufferSize);
            }

            // We are only interested in pages NOT used by any other process.
            for (uint32_t i = 0; i < _bufferEntries; i++) {
               _otherMap[i] = ~_otherMap[i];
            }

            uint32_t totalProcessCount = 1 + childTrees.size();
            StartLogLine(totalProcessCount);

            _parentProcess.MarkOccupiedPages(_ourMap, mapBufferSize);
            LogProcess(g_parentProcessName);

            for (const Core::ProcessTree& childTree : childTrees) {
               memset(_ourMap, 0, mapBufferSize);
               childTree.MarkOccupiedPages(_ourMap, mapBufferSize);
               uint32_t rootId = childTree.RootId();
               Core::ProcessInfo rootProcess(rootId);
               std::list<string> commandLine = rootProcess.CommandLine();

               string printedName = "<invalid>";
               if (!commandLine.empty()) {
                  printedName = commandLine.front();
               }

               if (printedName == "WPEProcess") {
                  // Get callsign (following "-C")
                  std::list<string>::const_iterator i = std::find(commandLine.cbegin(), commandLine.cend(), _T("-C"));
                  if (i != commandLine.cend()) {
                     i++;
                     if (i != commandLine.cend()) {
                        printedName += " (" + *i + ")";
                     }
                  }
               }

               _namesLock.Lock();
               if (std::find(_processNames.cbegin(), _processNames.cend(), printedName) == _processNames.cend()) {
                  _processNames.push_back(printedName);
               }
               _namesLock.Unlock();

               LogProcess(printedName);
            }

            Thread::Block();
            return _interval * 1000;
         }

    private:
         uint32_t CountSetBits(uint32_t pageBuffer[], const uint32_t* mask)
         {
            uint32_t count = 0;

            if (mask == nullptr) {
               for (uint32_t index = 0; index < _bufferEntries; index++) {
                  count += __builtin_popcount(pageBuffer[index]);
               }
            } else {
               for (uint32_t index = 0; index < _bufferEntries; index++) {
                  count += __builtin_popcount(pageBuffer[index] & mask[index]);
               }
            }

            return count;
         }

         void LogProcess(const string& name)
         {
            uint32_t vss = CountSetBits(_ourMap, nullptr);
            uint32_t uss = CountSetBits(_ourMap, _otherMap);

            uint32_t nameSize = name.length();
            fwrite(&nameSize, sizeof(nameSize), 1, _binFile);
            fwrite(name.c_str(), sizeof(name[0]), name.length(), _binFile);
            fwrite(&vss, 1, sizeof(vss), _binFile);
            fwrite(&uss, 1, sizeof(uss), _binFile);
            fflush(_binFile);
         }

         void StartLogLine(uint32_t processCount)
         {
            // TODO: no simple time_t alike in Thunder?
            uint32_t timestamp = static_cast<uint32_t>(Core::Time::Now().Ticks() / 1000 / 1000);

            fwrite(&timestamp, 1, sizeof(timestamp), _binFile);
            fwrite(&processCount, 1, sizeof(processCount), _binFile);
         }

         FILE *_binFile;
         vector<string> _processNames; // Seen process names.
         Core::CriticalSection _namesLock;
         uint32_t * _otherMap; // Buffer used to mark other processes pages.
         uint32_t * _ourMap;   // Buffer for pages used by our process (tree).
         uint32_t _bufferEntries; // Numer of entries in each buffer.
         uint32_t _interval; // Seconds between measurement.
         Core::ProcessInfo _parentProcess; // Parent process of our tree ("WPEFramework").
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

         result = Core::ERROR_NONE;

         _processThread = new ProcessThread(config);
         _processThread->Run();

         return (result);
      }

      string CompileMemoryCsv() override
      {
         // TODO: should we worry about doing this as repsonse to RPC (could take too long?)
         FILE* inFile = fopen(_binPath.c_str(), "rb");
         stringstream output;

         vector<string> processNames;
         _processThread->GetProcessNames(processNames);

         output << _T("time (s)");
         for (const string& processName : processNames) {
            output << _T("\t") << processName << _T(" (VSS)\t") << processName << _T(" (USS)");
         }
         output << endl;

         vector<uint32_t> pageVector(processNames.size() * 2);
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
               fread(&vss, sizeof(vss), 1, inFile);
               fread(&uss, sizeof(uss), 1, inFile);
               if (nameIterator == processNames.cend()) {
                   continue;
               }

               int index = nameIterator - processNames.cbegin();

               pageVector[index * 2] = vss;
               pageVector[index * 2 + 1] = uss;
            }

            output << (timestamp - firstTimestamp);
            for (uint32_t pageEntry : pageVector) {
               output << "\t" << pageEntry;
            }
            output << endl;
         }

         fclose(inFile);

         return output.str();
      }

      BEGIN_INTERFACE_MAP(ResourceMonitorImplementation)
      INTERFACE_ENTRY(Exchange::IResourceMonitor)
      END_INTERFACE_MAP

  private:
      ProcessThread* _processThread;
      string _binPath;
   };

   SERVICE_REGISTRATION(ResourceMonitorImplementation, 1, 0);
} /* namespace Plugin */
} // namespace ResourceMonitor
