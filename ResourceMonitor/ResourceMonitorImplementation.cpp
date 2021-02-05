#include "Module.h"
#include <core/ProcessInfo.h>
#include <fstream>
#include <interfaces/IMemory.h>
#include <interfaces/IResourceMonitor.h>
#include <sstream>
#include <vector>

namespace WPEFramework {
namespace Plugin {
    class ResourceMonitorImplementation : public Exchange::IResourceMonitor {
    private:
        class CSVFile {
        public:
            CSVFile(string filepath, string seperator)
                : _file(filepath, false)
                , _seperator()
            {
                _file.Create();
                if (!_file.IsOpen()) {
                    TRACE(Trace::Error, (_T("Could not open file <%s>. Full resource monitoring unavailable."), filepath));
                } else {
                    if (seperator.empty() || seperator.size() > 1) {
                        TRACE(Trace::Error, (_T("Invalid seperator, falling back to ';'.")));
                        _seperator = ';';
                    } else {
                        _seperator = seperator[0];
                    }
                }
            }

            ~CSVFile()
            {
                if (_file.IsOpen()) {
                    _file.Close();
                }
            }

            void Store()
            {
                if (_file.IsOpen()) {
                    std::string output = _stringstream.str();
                    _file.Write(reinterpret_cast<const uint8_t*>(output.c_str()), output.length());
                }
                _stringstream.str("");
            }

            template <typename FIRST, typename... Args>
            void Append(const FIRST& first, Args... rest)
            {
                _stringstream << first;
                Append(true, rest...);
            }

        private:
            template <typename FIRST, typename... Args>
            void Append(bool dummy, const FIRST& first, Args... rest)
            {
                _stringstream << _seperator << first;
                Append(dummy, rest...);
            }

            void Append(bool dummy)
            {
                _stringstream << std::endl;
            }

            Core::File _file;
            std::stringstream _stringstream;
            char _seperator;
        };

        class Config : public Core::JSON::Container {
        public:
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , Path()
                , Seperator()
                , Interval()
                , FilterName()
            {
                Add(_T("csv_filepath"), &Path);
                Add(_T("csv_sep"), &Seperator);
                Add(_T("interval"), &Interval);
                Add(_T("name"), &FilterName);
            }

            Config(const Config& copy)
                : Core::JSON::Container()
                , Path(copy.Path)
                , Interval(copy.Interval)
                , FilterName(copy.FilterName)
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

        class StatCollecter {
        public:
            explicit StatCollecter(const Config& config)
                : _logfile(config.Path.Value(), config.Seperator.Value())
                , _otherMap(nullptr)
                , _ourMap(nullptr)
                , _bufferEntries(0)
                , _interval(0)
                , _activity(*this)
            {
                _logfile.Append("Time[s]", "Name", "VSS[KiB]", "USS[KiB]", "Jiffies", "TotalJiffies");
                uint32_t pageCount = Core::SystemInfo::Instance().GetPhysicalPageCount();
                _memoryPageSize = Core::SystemInfo::Instance().GetPageSize();
                _pluginStartupTime = static_cast<uint32_t>(Core::Time::Now().Ticks() / 1000 / 1000);

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
                _filterName = config.FilterName.Value();

                _activity.Submit();
            }

            ~StatCollecter()
            {
                _activity.Revoke();

                delete[] _ourMap;
                delete[] _otherMap;
            }

            void GetProcessNames(std::vector<string>& processNames)
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

                    std::list<Core::ProcessInfo> otherProcesses;
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

        protected:
            void Dispatch()
            {
                Collect();
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
                auto timestamp = static_cast<uint32_t>(Core::Time::Now().Ticks() / 1000 / 1000) - _pluginStartupTime;
                uint32_t vss = CountSetBits(_ourMap, nullptr);
                uint32_t uss = CountSetBits(_ourMap, _otherMap);
                uint64_t ussInKilobytes = (_memoryPageSize / 1024) * uss;
                uint64_t vssInKilobytes = (_memoryPageSize / 1024) * vss;
                uint64_t jiffies = info.Jiffies();
                uint64_t totalJiffies = Core::SystemInfo::Instance().GetJiffies();

                _logfile.Append(timestamp, name, vssInKilobytes, ussInKilobytes, jiffies, totalJiffies);
                _logfile.Store();
            }

            CSVFile _logfile;
            uint32_t _memoryPageSize; //size of the device memory page
            uint32_t _pluginStartupTime; //time in which the resurcemonitor started

            std::vector<string> _processNames; // Seen process names.
            Core::CriticalSection _namesLock;
            uint32_t* _otherMap; // Buffer used to mark other processes pages.
            uint32_t* _ourMap; // Buffer for pages used by our process (tree).
            uint32_t _bufferEntries; // Numer of entries in each buffer.
            uint32_t _interval; // Seconds between measurement.
            string _filterName; // Process/plugin name we are looking for.
            Core::WorkerPool::JobType<StatCollecter&> _activity;

            friend Core::ThreadPool::JobType<StatCollecter&>;
        };

    private:
        ResourceMonitorImplementation(const ResourceMonitorImplementation&) = delete;
        ResourceMonitorImplementation& operator=(const ResourceMonitorImplementation&) = delete;

    public:
        ResourceMonitorImplementation()
            : _processThread(nullptr)
            , _csvFilePath()
        {
        }

        ~ResourceMonitorImplementation() override = default;

        virtual uint32_t Configure(PluginHost::IShell* service) override
        {
            uint32_t result(Core::ERROR_INCOMPLETE_CONFIG);

            ASSERT(service != nullptr);

            Config config;
            config.FromString(service->ConfigLine());
            if (config.Path.IsSet() == true) {
                _csvFilePath = config.Path.Value().c_str();
            }

            result = Core::ERROR_NONE;
            _processThread.reset(new StatCollecter(config));

            return (result);
        }

        string CompileMemoryCsv() override
        {
            const uint32_t lastMeasurementsHistory = Core::infinite;

            std::ifstream csvFile(_csvFilePath, std::fstream::binary);
            std::ostringstream result;
            std::string line;

            //get the first line - header
            std::getline(csvFile, line);
            result << line << std::endl;

            if (lastMeasurementsHistory == Core::infinite) {
                while (std::getline(csvFile, line)) {
                    result << line << std::endl;
                }
            } else {
                std::vector<std::string> allLines;
                while (std::getline(csvFile, line)) {
                    allLines.push_back(line);
                }
                if (allLines.size() > lastMeasurementsHistory) {
                    for (auto currentLine = allLines.end() - lastMeasurementsHistory; currentLine != allLines.end(); ++currentLine) {
                        result << *currentLine << std::endl;
                    }
                } else {
                    result << "Not enough measurements yet!" << std::endl;
                }
            }

            return result.str();
        }

        BEGIN_INTERFACE_MAP(ResourceMonitorImplementation)
        INTERFACE_ENTRY(Exchange::IResourceMonitor)
        END_INTERFACE_MAP

    private:
        std::unique_ptr<StatCollecter> _processThread;
        string _csvFilePath;
    };

    SERVICE_REGISTRATION(ResourceMonitorImplementation, 1, 0);
} /* namespace Plugin */
} // namespace WPEFramework
