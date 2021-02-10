#include "Module.h"
#include <bitset>
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
        private:
            class Worker : public Core::IDispatch {
            public:
                Worker(StatCollecter* parent)
                    : _parent(*parent)
                {
                }

                void Dispatch() override
                {
                    _parent._guard.Lock();
                    _parent.Dispatch();
                    _parent._guard.Unlock();

                    Core::IWorkerPool::Instance().Schedule(Core::Time::Now().Add(_parent._interval * 1000), Core::ProxyType<Core::IDispatch>(*this));
                }

            private:
                StatCollecter& _parent;
            };

        public:
            explicit StatCollecter(const Config& config)
                : _logfile(config.Path.Value(), config.Seperator.Value())
                , _memoryPageSize(0)
                , _bufferEntries(0)
                , _pageCount(0)
                , _interval(0)
                , _worker(Core::ProxyType<Worker>::Create(this))

            {
                _logfile.Append("Time[s]", "Name", "VSS[KiB]", "USS[KiB]", "Jiffies", "TotalJiffies");
                _memoryPageSize = Core::SystemInfo::Instance().GetPageSize();

                _pageCount = Core::SystemInfo::Instance().GetPhysicalPageCount();
                _bufferEntries = DivideAndCeil(_pageCount, 32); //divide by bit size of uint32_t

                // Because linux doesn't report the first couple of pages it uses itself,
                // allocate a little extra to make sure we don't miss the highest ones.
                // The number here is selected arbitrarily
                _bufferEntries += _bufferEntries / 10;

                _ourProcessPages.resize(_bufferEntries);
                _otherProcessesPages.resize(_bufferEntries);
                _interval = config.Interval.Value();
                _filterName = config.FilterName.Value();

                Core::IWorkerPool::Instance().Schedule(Core::Time::Now(), Core::ProxyType<Core::IDispatch>(_worker));
            }

            ~StatCollecter()
            {
                Core::IWorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(_worker), Core::infinite);
            }

        private:
            uint32_t DivideAndCeil(uint32_t dividend, uint32_t divisor)
            {
                return dividend / divisor + (dividend % divisor != 0);
            }

            void Collect()
            {
                std::list<Core::ProcessInfo> processes;
                Core::ProcessInfo::FindByName(_filterName, false, processes);

                for (const Core::ProcessInfo& processInfo : processes) {
                    //reset maps
                    std::fill(std::begin(_ourProcessPages), std::end(_ourProcessPages), 0);
                    std::fill(std::begin(_otherProcessesPages), std::end(_otherProcessesPages), 0);

                    string processName = processInfo.Name() + " (" + std::to_string(processInfo.Id()) + ")";

                    if (find(_processNames.begin(), _processNames.end(), processName) == _processNames.end()) {
                        _processNames.push_back(processName);
                    }

                    Core::ProcessTree processTree(processInfo.Id());

                    processTree.MarkOccupiedPages(_ourProcessPages.data(), _pageCount);

                    std::list<Core::ProcessInfo> otherProcesses;
                    Core::ProcessInfo::Iterator otherIterator;
                    while (otherIterator.Next()) {
                        ::ThreadId otherId = otherIterator.Current().Id();
                        if (!processTree.ContainsProcess(otherId)) {
                            otherIterator.Current().MarkOccupiedPages(_otherProcessesPages.data(), _pageCount);
                        }
                    }

                    LogProcess(processName, processInfo);
                }
            }

            void Dispatch()
            {
                Collect();
            }

            uint32_t CountSetBits(const bool calculateUSS, const std::vector<uint32_t>& pageBuffer, const std::vector<uint32_t>& inverseMask)
            {
                uint32_t count = 0;
                if (calculateUSS) {
                    for (uint32_t index = 0; index < _bufferEntries; index++) {
                        count += std::bitset<32>(pageBuffer[index] & (~inverseMask[index])).count();
                    }
                } else {
                    for (uint32_t index = 0; index < _bufferEntries; index++) {
                        count += std::bitset<32>(pageBuffer[index]).count();
                    }
                }
                return count;
            }

            void LogProcess(const string& name, const Core::ProcessInfo& info)
            {
                auto timestamp = static_cast<uint32_t>(Core::Time::Now().Ticks() / 1000 / 1000);
                uint32_t uss = CountSetBits(true, _ourProcessPages, _ourProcessPages);
                uint32_t vss = CountSetBits(false, _ourProcessPages, _ourProcessPages);

                //multiply uss and vss with size of page map (in kilobytes)
                uint64_t ussInKilobytes = (_memoryPageSize / 1024) * uss;
                uint64_t vssInKilobytes = (_memoryPageSize / 1024) * vss;

                uint64_t jiffies = info.Jiffies();
                uint64_t totalJiffies = Core::SystemInfo::Instance().GetJiffies();

                _logfile.Append(timestamp, name, vssInKilobytes, ussInKilobytes, jiffies, totalJiffies);
                _logfile.Store();
            }

        private:
            CSVFile _logfile;
            uint32_t _memoryPageSize;

            std::vector<string> _processNames;
            Core::CriticalSection _guard;
            std::vector<uint32_t> _otherProcessesPages;
            std::vector<uint32_t> _ourProcessPages;
            uint32_t _bufferEntries;
            uint32_t _pageCount;
            uint32_t _interval;
            string _filterName;

            Core::ProxyType<Worker> _worker;
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

            if (config.Path.IsSet()) {
                _csvFilePath = config.Path.Value().c_str();

                if (!config.Interval.IsSet()) {
                    TRACE(Trace::Error, (_T("Interval not specified!")));
                } else {
                    if (config.Interval.Value() == 0) {
                        TRACE(Trace::Error, (_T("Interval must be greater than 0!")));
                    } else {
                        _processThread.reset(new StatCollecter(config));
                        result = Core::ERROR_NONE;
                    }
                }

            } else {
                TRACE(Trace::Error, (_T(".csv filepath not specified!")));
            }

            return (result);
        }

        string CompileMemoryCsv() override
        {
            const uint8_t lastMeasurementsHistory = 10;

            std::ifstream csvFile(_csvFilePath, std::fstream::binary);
            std::ostringstream result;
            std::string line;
            std::vector<std::string> allLines;

            while (std::getline(csvFile, line)) {
                allLines.push_back(line);
            }
            if (allLines.size() > lastMeasurementsHistory) {

                result << allLines.front() << std::endl; //write header

                for (auto currentLine = allLines.end() - lastMeasurementsHistory; currentLine != allLines.end(); ++currentLine) {
                    result << *currentLine << std::endl;
                }
            } else {
                result << "Not enough measurements yet!" << std::endl;
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
