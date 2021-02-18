#include "Module.h"
#include <bitset>
#include <core/ProcessInfo.h>
#include <fstream>
#include <interfaces/IMemory.h>
#include <interfaces/IResourceMonitor.h>
#include <numeric>
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
                , _interval(5)
                , _worker(Core::ProxyType<Worker>::Create(this))
                , _uss(0)
                , _rss(0)
                , _pss(0)
                , _vss(0)
                , _userCpuTime(0)
                , _systemCpuTime(0)

            {
                _logfile.Append("Time[s]", "Name", "USS[KiB]", "PSS[KiB]", "RSS[KiB]", "VSS[KiB]", "UserCPU[%]", "SystemCPU[%]");

                _interval = config.Interval.Value();
                _filterName = config.FilterName.Value();

                Core::IWorkerPool::Instance().Schedule(Core::Time::Now(), Core::ProxyType<Core::IDispatch>(_worker));
            }

            ~StatCollecter()
            {
                Core::IWorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(_worker), Core::infinite);
            }

        private:
            void Skip(std::istream& is, uint32_t n, char delim)
            {
                // ignores n lines or words with max 300chars - stops ignoring after delim
                for (uint32_t i = 0; i < n; i++) {
                    is.ignore(std::numeric_limits<std::streamsize>::max(), delim);
                }
            }
            
            void Dispatch()
            {

                std::list<Core::ProcessInfo> processes;
                Core::ProcessInfo::FindByName(_filterName, false, processes);

                for (const Core::ProcessInfo& process : processes) {
                    if (Collect(process.Id())) {
                        LogProcess(process);
                    }
                    _uss = 0;
                    _pss = 0;
                    _rss = 0;
                    _vss = 0;
                }
            }

            bool Collect(Core::process_t pid)
            {

                std::ostringstream pathToSmaps;
                pathToSmaps << "/proc/" << pid << "/smaps";

                std::ifstream smaps(pathToSmaps.str());
                if (!smaps.is_open()) {
                    TRACE(Trace::Error, (_T("Could not open %s. Skipping monitoring this pid in this iteration."), pathToSmaps.str()));
                    return false;
                }

                std::istringstream iss;
                std::string fieldName;
                std::string line;
                //values from single memory range in /proc/pid/smaps
                uint64_t rss = 0;
                uint64_t pss = 0;
                uint64_t privateClean = 0;
                uint64_t privateDirty = 0;
                uint64_t vss = 0;

                std::getline(smaps, line);
                if (!smaps.good()) {
                    TRACE(Trace::Error, (_T("Failed to read from %s. Skipping monitoring this pid in this iteration."), pathToSmaps.str()));
                    return false;
                }
                do {
                    std::getline(smaps, line);
                    iss.str(line);
                    iss >> fieldName >> vss;

                    Skip(smaps, 2, '\n');

                    std::getline(smaps, line);
                    iss.str(line);
                    iss >> fieldName >> rss;

                    std::getline(smaps, line);
                    iss.str(line);
                    iss >> fieldName >> pss;

                    Skip(smaps, 2, '\n');

                    std::getline(smaps, line);
                    iss.str(line);
                    iss >> fieldName >> privateClean;

                    std::getline(smaps, line);
                    iss.str(line);
                    iss >> fieldName >> privateDirty;

                    _rss += rss;
                    _pss += pss;
                    _uss += privateDirty + privateClean;
                    _vss += vss;

                    Skip(smaps, 13, '\n');
                    //try to read next memory range to set eof flag if not accessible
                    std::getline(smaps, line);
                } while (!smaps.eof());

                return true;
            }

            void LogProcess(const Core::ProcessInfo& process)
            {
                auto timestamp = static_cast<uint32_t>(Core::Time::Now().Ticks() / 1000 / 1000);
                string name = process.Name() + " (" + std::to_string(process.Id()) + ")";

                uint64_t jiffies = process.Jiffies();
                uint64_t totalJiffies = Core::SystemInfo::Instance().GetJiffies();

                _logfile.Append(timestamp, name, _uss, _pss, _rss, _vss, _userCpuTime, _systemCpuTime);
                _logfile.Store();
            }

        private:
            CSVFile _logfile;

            std::vector<string> _processNames;
            Core::CriticalSection _guard;
            uint32_t _interval;
            string _filterName;

            Core::ProxyType<Worker> _worker;

            uint64_t _uss;
            uint64_t _rss;
            uint64_t _pss;
            uint64_t _vss;

            uint64_t _userCpuTime;
            uint64_t _systemCpuTime;

        
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
