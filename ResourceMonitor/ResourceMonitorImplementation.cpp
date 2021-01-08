#include "Module.h"
#include <core/ProcessInfo.h>
#include <interfaces/IMemory.h>
#include <interfaces/IResourceMonitor.h>
#include <sstream>
#include <vector>

namespace WPEFramework
{
   namespace Plugin
   {
      class ResourceMonitorImplementation : public Exchange::IResourceMonitor
      {
      private:
         class Config : public Core::JSON::Container
         {
         public:
            Config &operator=(const Config &) = delete;

            Config()
                : Core::JSON::Container(), Path(), Seperator(), Interval(), FilterName()
            {
               Add(_T("csv_filepath"), &Path);
               Add(_T("csv_sep"), &Seperator);
               Add(_T("interval"), &Interval);
               Add(_T("name"), &FilterName);
            }

            Config(const Config &copy)
                : Core::JSON::Container(), Path(copy.Path), Interval(copy.Interval), FilterName(copy.FilterName)
            {
            }

            ~Config()
            {
            }

         public:
            Core::JSON::String Path;
            Core::JSON::String Seperator;
            Core::JSON::DecUInt32 Interval;
            Core::JSON::String FilterName;
         };

         class StatCollecter
         {
         public:
            explicit StatCollecter(const Config &config)
                : _binFile(nullptr), _otherMap(nullptr), _ourMap(nullptr), _bufferEntries(0), _interval(0), _activity(*this)
            {
               _binFile = fopen(config.Path.Value().c_str(), "w");

               uint32_t pageCount = Core::SystemInfo::Instance().GetPhysicalPageCount();
               const uint32_t bitPersUint32 = 32;
               _bufferEntries = pageCount / bitPersUint32;
               if ((pageCount % bitPersUint32) != 0)
               {
                  _bufferEntries++;
               }

               // Because linux doesn't report the first couple of pages it uses itself,
               //    allocate a little extra to make sure we don't miss the highest ones.
               _bufferEntries += _bufferEntries / 10;

               _ourMap = new uint32_t[_bufferEntries];
               _otherMap = new uint32_t[_bufferEntries];
               _interval = config.Interval.Value();
               _filterName = config.FilterName.Value();

               _activity.Submit();
            }

            ~StatCollecter()
            {
               fclose(_binFile);

               delete[] _ourMap;
               delete[] _otherMap;
            }

            void GetProcessNames(std::vector<string> &processNames)
            {
               _namesLock.Lock();
               processNames = _processNames;
               _namesLock.Unlock();
            }

         private:
            void Collect()
            {
               std::list<Core::ProcessInfo> processes;
               Core::ProcessInfo::FindByName(_filterName, false, processes);

               StartLogLine(processes.size());

               for (const Core::ProcessInfo &processInfo : processes)
               {
                  uint32_t mapBufferSize = sizeof(_ourMap[0]) * _bufferEntries;
                  memset(_ourMap, 0, mapBufferSize);
                  memset(_otherMap, 0, mapBufferSize);

                  string processName = processInfo.Name() + " (" + std::to_string(processInfo.Id()) + ")";

                  _namesLock.Lock();
                  if (find(_processNames.begin(), _processNames.end(), processName) == _processNames.end())
                  {
                     _processNames.push_back(processName);
                  }
                  _namesLock.Unlock();

                  Core::ProcessTree processTree(processInfo.Id());

                  processTree.MarkOccupiedPages(_ourMap, mapBufferSize);

                  std::list<Core::ProcessInfo> otherProcesses;
                  Core::ProcessInfo::Iterator otherIterator;
                  while (otherIterator.Next())
                  {
                     ::ThreadId otherId = otherIterator.Current().Id();
                     if (!processTree.ContainsProcess(otherId))
                     {
                        otherIterator.Current().MarkOccupiedPages(_otherMap, mapBufferSize);
                     }
                  }

                  LogProcess(processName, processInfo);
               }
            }

         protected:
            void Dispatch()
            {
               Collect();
               _activity.Schedule(Core::Time::Now().Add(_interval * 1000));
            }

         private:
            uint32_t CountSetBits(uint32_t pageBuffer[], const uint32_t *inverseMask)
            {
               uint32_t count = 0;

               if (inverseMask == nullptr)
               {
                  for (uint32_t index = 0; index < _bufferEntries; index++)
                  {
                     count += __builtin_popcount(pageBuffer[index]);
                  }
               }
               else
               {
                  for (uint32_t index = 0; index < _bufferEntries; index++)
                  {
                     count += __builtin_popcount(pageBuffer[index] & (~inverseMask[index]));
                  }
               }

               return count;
            }

            void LogProcess(const string &name, const Core::ProcessInfo &info)
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
            std::vector<string> _processNames; // Seen process names.
            Core::CriticalSection _namesLock;
            uint32_t *_otherMap;               // Buffer used to mark other processes pages.
            uint32_t *_ourMap;                 // Buffer for pages used by our process (tree).
            uint32_t _bufferEntries;           // Numer of entries in each buffer.
            uint32_t _interval;                // Seconds between measurement.
            string _filterName; // Process/plugin name we are looking for.
            Core::WorkerPool::JobType<StatCollecter &> _activity;

            friend Core::ThreadPool::JobType<StatCollecter &>;
         };

      private:
         ResourceMonitorImplementation(const ResourceMonitorImplementation &) = delete;
         ResourceMonitorImplementation &operator=(const ResourceMonitorImplementation &) = delete;

      public:
         ResourceMonitorImplementation()
             : _processThread(nullptr), _binPath(_T("/tmp/resource-log.bin"))
         {
         }

         ~ResourceMonitorImplementation() override = default;

         virtual uint32_t Configure(PluginHost::IShell *service) override
         {
            uint32_t result(Core::ERROR_INCOMPLETE_CONFIG);

            ASSERT(service != nullptr);

            Config config;
            config.FromString(service->ConfigLine());
            if (config.Path.IsSet() == true)
            {
               _binPath = config.Path.Value().c_str();
            }

            result = Core::ERROR_NONE;

            _processThread.reset(new StatCollecter(config));

            return (result);
         }

         string CompileMemoryCsv() override
         {
            // TODO: should we worry about doing this as repsonse to RPC (could take too long?)
            FILE *inFile = fopen(_binPath.c_str(), "rb");

            std::stringstream output;
            if (inFile != nullptr)
            {

               std::vector<string> processNames;
               _processThread->GetProcessNames(processNames);

               output << _T("time (s)\tJiffies");
               for (const string &processName : processNames)
               {
                  output << _T("\t") << processName << _T(" (VSS)\t") << processName << _T(" (USS)\t") << processName << _T(" (jiffies)");
               }
               output << std::endl;

               std::vector<uint64_t> pageVector(processNames.size() * 3);
               bool seenFirstTimestamp = false;
               uint32_t firstTimestamp = 0;

               while (true)
               {
                  std::fill(pageVector.begin(), pageVector.end(), 0);

                  uint32_t timestamp = 0;
                  size_t readCount = fread(&timestamp, sizeof(timestamp), 1, inFile);
                  if (readCount != 1)
                  {
                     break;
                  }

                  if (!seenFirstTimestamp)
                  {
                     firstTimestamp = timestamp;
                     seenFirstTimestamp = true;
                  }

                  uint32_t processCount = 0;
                  fread(&processCount, sizeof(processCount), 1, inFile);

                  uint64_t totalJiffies = 0;
                  fread(&totalJiffies, sizeof(totalJiffies), 1, inFile);

                  for (uint32_t processIndex = 0; processIndex < processCount; processIndex++)
                  {
                     uint32_t nameLength = 0;
                     fread(&nameLength, sizeof(nameLength), 1, inFile);
                     // TODO: unicode?
                     char nameBuffer[nameLength + 1];
                     fread(nameBuffer, sizeof(char), nameLength, inFile);

                     nameBuffer[nameLength] = '\0';
                     string name(nameBuffer);

                     std::vector<string>::const_iterator nameIterator = std::find(processNames.cbegin(), processNames.cend(), name);

                     uint32_t vss, uss;
                     uint64_t jiffies;
                     fread(&vss, sizeof(vss), 1, inFile);
                     fread(&uss, sizeof(uss), 1, inFile);
                     fread(&jiffies, sizeof(jiffies), 1, inFile);
                     if (nameIterator == processNames.cend())
                     {
                        continue;
                     }

                     int index = nameIterator - processNames.cbegin();

                     pageVector[index * 3] = static_cast<uint64_t>(vss);
                     pageVector[index * 3 + 1] = static_cast<uint64_t>(uss);
                     pageVector[index * 3 + 2] = jiffies;
                  }

                  output << (timestamp - firstTimestamp) << "\t" << totalJiffies;
                  for (uint32_t pageEntry : pageVector)
                  {
                     output << "\t" << pageEntry;
                  }
                  output << std::endl;
               }

               fclose(inFile);
            }

            return output.str();
         }

         BEGIN_INTERFACE_MAP(ResourceMonitorImplementation)
         INTERFACE_ENTRY(Exchange::IResourceMonitor)
         END_INTERFACE_MAP

      private:
         std::unique_ptr<StatCollecter> _processThread;
         string _binPath;
      };

      SERVICE_REGISTRATION(ResourceMonitorImplementation, 1, 0);
   } /* namespace Plugin */
} // namespace WPEFramework
