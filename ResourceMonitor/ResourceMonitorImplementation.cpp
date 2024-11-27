/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "Module.h"
#include <bitset>
#include <core/ProcessInfo.h>
#include <core/SystemInfo.h>
#include <fstream>
#include <interfaces/IMemory.h>
#include <interfaces/IResourceMonitor.h>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace WPEFramework {
    namespace Plugin {
        class ResourceMonitorImplementation : public Exchange::IResourceMonitor {
            public:
                ResourceMonitorImplementation()
                    : _processThread(nullptr) {
                }

                ~ResourceMonitorImplementation() override = default;

                void Register(IResourceMonitor::INotification* sink) {
                    _monitorLock.Lock();
                    auto item = std::find(_notifications.begin(), _notifications.end(), sink);
                    ASSERT(item == _notifications.end());
                    if (item == _notifications.end()) {
                        sink->AddRef();
                        _notifications.push_back(sink);
                    }
                    _monitorLock.Unlock();
                }

                void Unregister(IResourceMonitor::INotification* sink) {          
                    _monitorLock.Lock();
                    auto item = std::find(_notifications.begin(), _notifications.end(), sink);
                    ASSERT(item != _notifications.end());
            
                    if (item != _notifications.end()) {
                        _notifications.erase(item);
                        (*item)->Release();
                    }
                    _monitorLock.Unlock();
                }

                uint32_t Configure(PluginHost::IShell* service) override {
                    uint32_t result(Core::ERROR_INCOMPLETE_CONFIG);
                    ASSERT(service != nullptr);
                    Config config;
                    config.FromString( ->ConfigLine());

                    if (Core::File(service->PersistentPath()).IsDirectory() == false) {
                        if (Core::Directory(service->PersistentPath().c_str()).CreatePath() == false) {
                            TRACE(Trace::Error, (_T("Failed to create persistent storage folder [%s]"), service->PersistentPath().c_str()));
                        }
                    }

                    _processThread.reset(new MemAndCpuMonitor(service->PersistentPath(), config, *this));
                    result = Core::ERROR_NONE;

                    return result;
                }

                uint32_t Start(const uint32_t interval, const IResourceMonitor::Mode mode, const string& processPrefix) override {
                    auto result = _processThread->Start(interval, mode, processPrefix);
                    if (result) {
                        return Core::ERROR_NONE;
                    } else {
                        return Core::ERROR_GENERAL;
                    }
                }

                uint32_t Stop() override {
                    auto result = _processThread->Stop();

                    if (result) {
                        return Core::ERROR_NONE;
                    } else {
                        return Core::ERROR_GENERAL;
                    }
                }

                uint32_t GetCurrentUsage(const string& processPrefix) override {
                    auto result = _processThread->GetCurrentUsage(processPrefix);

                    if (result) {
                        return Core::ERROR_NONE;
                    } else {
                        return Core::ERROR_GENERAL;
                    }
                }
                
                uint32_t AddLabel(const string& annotation) override {
                    auto result = _processThread->AddLabel(annotation);

                    if (result) {
                        return Core::ERROR_NONE;
                    } else {
                        return Core::ERROR_GENERAL;
                    }
                }
            
                void Notify(const string& data) {
                    Exchange::IResourceMonitor::EventData payload;
                    payload.samples = data;
                    for (auto* notification : _notifications) {
                        notification->OnResourceMonitorData(payload);
                    }
                }

                BEGIN_INTERFACE_MAP(ResourceMonitorImplementation)
                    INTERFACE_ENTRY(Exchange::IResourceMonitor)
                END_INTERFACE_MAP


            private:
                ResourceMonitorImplementation(const ResourceMonitorImplementation&) = delete;
                ResourceMonitorImplementation& operator=(const ResourceMonitorImplementation&) = delete;

                Core::CriticalSection _monitorLock;

                class MemAndCpuMonitor;
                std::unique_ptr<MemAndCpuMonitor> _processThread;
                std::vector<Exchange::IResourceMonitor::INotification*> _notifications;


                class DataSample {
                    public:
                        template <typename... Args>
                        std::string Set(Args... args) {
                            Clean(); // Make sure that stream is empty
                            if (sizeof...(args) > 0) {
                                _content << '{';
                                (void)std::initializer_list<int>{(_content << args << ",", 0)...};  
                                _content.seekp(_content.tellp() - std::streamoff(1));  // Move back one position to overwrite the last comma
                                _content << "}\n";
                            }
                            return _content.str();
                        }

                        std::string Get() {
                            return _content.str();
                        }

                        void Clean() {
                            _content.str("");
                            _content.clear();
                        }

                        DataSample() {
                            _content.precision(2);
                        }

                        ~DataSample() {
                        }

                    private:
                        std::stringstream _content;
                };

                class JsonlFile {
                    public:
                        JsonlFile() {
                        }

                        ~JsonlFile() {
                            if (_file.IsOpen()) {
                                _file.Close();
                            }
                        }

                        bool Create(const string& storageDirPath) {
                            auto filePath = GenerateTimestampedFileName(storageDirPath, "MemAndCpuMeasurements", "jsonl");

                            if (_file.IsOpen()) {
                                _file.Close();
                            }

                            _file = Core::File(filePath);
                            bool success = _file.Create();

                            if (!success) {
                                TRACE(Trace::Error, (_T("Failed to create file <%s>."), filePath));
                                TRACE(Trace::Error, (_T("Failed to create file <%s>."), filePath));
                            } else {
                                TRACE(Trace::Information, (_T("File created successfully: <%s>"), filePath));
                            }
                            _path = filePath;
                            return success;
                        }

                        void Save(std::string data) {
                            if (_file.IsOpen()) {
                                _file.Write(reinterpret_cast<const uint8_t*>(data.c_str()), data.length());
                            }
                        }

                        std::string Load() {
                            uint8_t buffer[1024] = {};
                            uint32_t size;
                            std::string content;
                            if (_file.Open()) {
                                while ((size = _file.Read(buffer, sizeof(buffer))) != 0) {
                                    content.append(string(reinterpret_cast<char *>(buffer), size));
                                }
                                _file.Close();
                            }
                            return content;
                        }

                        bool Exists() {
                            return _file.Exists();
                        }

                        bool IsOpen() {
                            return _file.IsOpen();
                        }

                        std::string GetPath() {
                            return _path;
                        }

                    private:
                        Core::File _file;
                        std::string _path;

                        string GenerateTimestampedFileName(const string& dirPath, const string& description, const string& extension) {
                            auto t = std::time(nullptr);
                            auto tm = *std::localtime(&t);

                            std::ostringstream oss;
                            oss << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_" << description << "." << extension;

                            string filePath = dirPath;
                            if (!filePath.empty() && filePath.back() != '/') {
                                filePath += '/';
                            }
                            filePath += oss.str();

                            return filePath;
                        }
                };
        
                class Config : public Core::JSON::Container {
                    public:
                        Core::JSON::String Path;
                        Core::JSON::String Name;

                        Config& operator=(const Config&) = delete;

                        ~Config() {
                        }

                        Config()
                            : Core::JSON::Container()
                            , Path()
                            , Name("Dummy") {
                            Add(_T("jsonlFilePath"), &Path);
                            Add(_T("jsonlFileName"), &Name);
                        }

                        Config(const Config& copy)
                            : Core::JSON::Container()
                            , Path(copy.Path)
                            , Name(copy.Name) {
                            }
                    };

                class NotificationWorker {
                    public:
                        explicit NotificationWorker(ResourceMonitorImplementation& parent)
                        : _parent(parent)
                        , _job(*this) {
                        }

                        ~NotificationWorker() {
                            _job.Revoke();
                        }

                        void Dispatch() {
                            if (_file.Exists()) {
                                auto payload = Load();
                                _parent.Notify(payload);
                            } else {
                                 TRACE(Trace::Error, (_T("ResourceMonitor could not send summary notification. Input file does not exist")));
                            }
                        }

                        void Submit(std::string filePath) {
                            _file = Core::File(filePath);
                            _job.Submit();
                        }

                    private:
                        friend Core::ThreadPool::JobType<NotificationWorker&>;
                        Core::File _file;
                        ResourceMonitorImplementation &_parent;
                        Core::WorkerPool::JobType<NotificationWorker&> _job;

                        std::string Load() {
                            uint8_t buffer[1024] = {};
                            uint32_t size;
                            std::string content;
                            if (_file.Open()) {
                                while ((size = _file.Read(buffer, sizeof(buffer))) != 0) {
                                    content.append(string(reinterpret_cast<char *>(buffer), size));
                                }
                                _file.Close();
                            }
                            return content;
                        }                       
                };

                class MemAndCpuMonitor {
                    public:
                        explicit MemAndCpuMonitor(const string& defaultStoragePath, const Config& config, ResourceMonitorImplementation& parent)
                            : _userCpuTime(0)
                            , _systemCpuTime(0)
                            , _storagePath(defaultStoragePath)
                            , _resultFile()
                            , _resultData()
                            , _interval(0)
                            , _filterNames()
                            , _job(*this)
                            , _mode(IResourceMonitor::Mode::NONE)
                            , _isCaptureInProgress(false)
                            , _isSingleSample(false)
                            , _parent(parent)
                            , _notificationWorker(parent) {
                        }

                        ~MemAndCpuMonitor() {
                            _job.Revoke();
                        }

                        bool Start(const uint32_t interval, const IResourceMonitor::Mode mode, const string& processPrefix) {
                            ASSERT(interval > 0);
                            _stateGuard.Lock();
                            bool result = false;
                            std::cerr << "FMELKA start, interval: " << interval << "\n";
                            std::cerr << "FMELKA start, mode: " << static_cast<uint8_t>(mode) << "\n";
                            std::cerr << "FMELKA start, processPrefix: " << processPrefix << "\n";
                            std::cerr << "FMELKA start, _isCapture: " << _isCaptureInProgress << "\n";
                            std::cerr << "FMELKA start, _mode: " << static_cast<uint8_t>(_mode) << "\n";
                            std::cerr << "FMELKA start, _interval: " << _interval << "\n";
                            std::cerr << "FMELKA start, _filterNames.size: " << _filterNames.size() << "\n";
                            if(_isCaptureInProgress) {
                                TRACE(Trace::Warning, (_T("ResourceMonitor is capturing data now. Can not start new session")));
                                std::cerr << "FMELKA start, isCapture=true\n";
                                return result;
                            }

                            _isCaptureInProgress = true;
                            _interval = interval;
                            _mode = mode;
                            _filterNames.clear();
                            _filterNames.push_back(processPrefix);

                            switch(_mode) {
                                case IResourceMonitor::Mode::LIVE:
                                    result = true;
                                    _job.Submit();
                                    break;
                                case IResourceMonitor::Mode::SUMMARY:
                                case IResourceMonitor::Mode::MIXED: 
                                    result = _resultFile.Create(_storagePath);
                                    if(result) {
                                        _job.Submit();
                                    }
                                    break;
                                default: 
                                    TRACE(Trace::Error, (_T("ResourceMonitor invalid start mode")));
                                    break;
                            };
                            _stateGuard.Unlock();
                            return result;
                        }

                        bool Stop() {
                            _stateGuard.Lock();
                            bool result = false;
                            std::cerr << "FMELKA stop, _isCapture: " << _isCaptureInProgress << "\n";
                            std::cerr << "FMELKA stop, _mode: " << static_cast<uint8_t>(_mode) << "\n";
                            std::cerr << "FMELKA stop, _interval: " << _interval << "\n";
                            std::cerr << "FMELKA stop, _filterNames.size: " << _filterNames.size() << "\n";
                            if(!_isCaptureInProgress) {
                                TRACE(Trace::Warning, (_T("ResourceMonitor is not active. Can not stop it")));
                                return result;
                            }

                            if(IResourceMonitor::Mode::NONE == _mode){
                                return result;
                            }

                            if((IResourceMonitor::Mode::SUMMARY == _mode) || (IResourceMonitor::Mode::MIXED == _mode)){
                                _notificationWorker.Submit(_resultFile.GetPath());
                            }

                            _mode = IResourceMonitor::Mode::NONE;
                            _isCaptureInProgress = false;
                            _job.Revoke();
                            result = true;
                            _stateGuard.Unlock();
                            return result;
                        }
                        
                        bool GetCurrentUsage(const string& processPrefix) {
                            _stateGuard.Lock();
                            bool result = false;

                            if(_isCaptureInProgress) {
                                TRACE(Trace::Warning, (_T("ResourceMonitor is capturing data now. Can not get current usage")));
                                return result;
                            }
                            std::cerr << "FMELKA getCurr, _isCapture: " << _isCaptureInProgress << "\n";
                            std::cerr << "FMELKA getCurr, _mode: " << static_cast<uint8_t>(_mode) << "\n";
                            std::cerr << "FMELKA getCurr, _interval: " << _interval << "\n";
                            std::cerr << "FMELKA getCurr, _filterNames.size: " << _filterNames.size() << "\n";
                            _filterNames.clear();
                            _filterNames.push_back(processPrefix);
                            _mode = IResourceMonitor::Mode::LIVE;
                            _isSingleSample = true;
                            _job.Submit();
                            result = true;
                            _stateGuard.Unlock();
                            return result;
                        }

                        bool AddLabel(const string& annotation) {
                            _stateGuard.Lock();
                            auto result = false;
                            if(!_isCaptureInProgress) {
                                TRACE(Trace::Warning, (_T("ResourceMonitor is not capturing data now. Can not add tag")));
                                return result;
                            } else {
                                result = true;
                            }

                            auto timestamp = Core::Time::Now().Ticks() / 1000; // ms from epoch
                            DataSample _tagData;
                            auto data = _tagData.Set(CreateKeyValuePair("time", timestamp, "ms"), 
                                                        CreateKeyValuePair("comment", annotation));
                            switch(_mode) {
                                case IResourceMonitor::Mode::LIVE:
                                    _parent.Notify(data);
                                    break;
                                case IResourceMonitor::Mode::SUMMARY:
                                    _resultFile.Save(data); 
                                    break;
                                case IResourceMonitor::Mode::MIXED: 
                                    _parent.Notify(data);
                                    _resultFile.Save(data);
                                    break;
                                default: 
                                    TRACE(Trace::Error, (_T("ResourceMonitor: can not tag in currently running mode")));
                                    break;
                            };
                            _stateGuard.Unlock();
                            return result;
                        }

                        void Dispatch() {
                            _stateGuard.Lock();
                                for (const auto& filterName : _filterNames) {
                                    std::list<Core::ProcessInfo> processes;
                                    Core::ProcessInfo::FindByName(filterName, false, processes);

                                    std::stringstream sample;
                                    if(processes.empty()){
                                    } else {
                                        for (const Core::ProcessInfo& process : processes) {
                                            CalculateCpuUsage(process.Id());
                                            auto result = LogProcess(process);
                                            switch(_mode) {
                                                case IResourceMonitor::Mode::LIVE:
                                                    sample << result;
                                                    break;
                                                case IResourceMonitor::Mode::SUMMARY:
                                                    _resultFile.Save(result); 
                                                    break;
                                                case IResourceMonitor::Mode::MIXED: 
                                                    sample << result;
                                                    _resultFile.Save(result);
                                                    break;
                                                default: 
                                                    TRACE(Trace::Error, (_T("ResourceMonitor invalid dispatch mode")));
                                                    break;
                                            };
                                        }
                                    }
                                    if ((IResourceMonitor::Mode::LIVE == _mode) || (IResourceMonitor::Mode::MIXED == _mode)){
                                        _parent.Notify(sample.str());
                                    }
                                }

                            if(_isSingleSample){
                                _mode = IResourceMonitor::Mode::NONE;
                                _isSingleSample = false;
                                _job.Revoke();
                            } else {
                                // Reschedule if started
                                _job.Reschedule(Core::Time::Now().Add(_interval * 1000));
                            }
                            std::cerr << "FMELKA dispatch, _isCapture: " << _isCaptureInProgress << "\n";
                            std::cerr << "FMELKA dispatch, _mode: " << static_cast<uint8_t>(_mode) << "\n";
                            std::cerr << "FMELKA dispatch, _interval: " << _interval << "\n";
                            std::cerr << "FMELKA dispatch, _filterNames.size: " << _filterNames.size() << "\n";
                            _stateGuard.Unlock();
                        }

                    private:
                        friend Core::ThreadPool::JobType<MemAndCpuMonitor&>;
                        struct Time {
                            Time()
                                : totalTime(0)
                                , sTime(0)
                                , uTime(0)
                                , prevTotalTime(0)
                                , prevUTime(0)
                                , prevSTime(0) {
                                }

                            uint64_t totalTime;
                            uint64_t sTime;
                            uint64_t uTime;
                            uint64_t prevTotalTime;
                            uint64_t prevUTime;
                            uint64_t prevSTime;
                        };

                        //NOTE: these two indicate usage of the whole CPU, not the single core (as 'top' shows by default)
                        double _userCpuTime;
                        double _systemCpuTime;

                        std::map<Core::process_t, Time> _processTimeInfo;

                        std::string _storagePath;
                        JsonlFile _resultFile;
                        DataSample _resultData;
                        uint32_t _interval;
                        std::list<std::string> _filterNames;

                        Core::CriticalSection _guard;
                        Core::WorkerPool::JobType<MemAndCpuMonitor&> _job;
                        IResourceMonitor::Mode _mode;
                        bool _isCaptureInProgress;
                        bool _isSingleSample;
                        ResourceMonitorImplementation &_parent;
                        NotificationWorker _notificationWorker;
                        Core::CriticalSection _stateGuard;

                        void GetTotalTime(Core::process_t pid) {
                            std::ifstream stat("/proc/stat");
                            if (!stat.is_open()) {
                            TRACE(Trace::Error, (_T("Could not open /proc/stat.")));

                                return;
                            }
                            std::string line;
                            std::istringstream iss;

                            std::getline(stat, line);
                            iss.str(line);

                            std::string dummy;
                            uint64_t user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0, guest = 0;

                            iss >> dummy >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal >> guest;
                            _processTimeInfo[pid].totalTime = user + nice + system + idle + iowait + irq + softirq + steal + guest;
                        }

                        void GetProcessTimes(Core::process_t pid) {
                            std::ostringstream pathToSmaps;
                            pathToSmaps << "/proc/" << pid << "/stat";

                            std::ifstream stat(pathToSmaps.str());
                            if (!stat.is_open()) {
                                TRACE(Trace::Error, (_T("Could not open %s"), pathToSmaps.str()));
                                return;
                            }

                            std::string uTimeString, sTimeString;

                            //ignoring 13 words spearated by space
                            for (uint32_t i = 0; i < 13; ++i) {
                                stat.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
                            }

                            stat >> _processTimeInfo[pid].uTime >> _processTimeInfo[pid].sTime;
                        }

                        void CalculateCpuUsage(Core::process_t pid) {
                            GetTotalTime(pid);
                            GetProcessTimes(pid);

                            _userCpuTime = 100.0 * (_processTimeInfo[pid].uTime - _processTimeInfo[pid].prevUTime) / (_processTimeInfo[pid].totalTime - _processTimeInfo[pid].prevTotalTime);

                            _systemCpuTime = 100.0 * (_processTimeInfo[pid].sTime - _processTimeInfo[pid].prevSTime) / (_processTimeInfo[pid].totalTime - _processTimeInfo[pid].prevTotalTime);

                            _processTimeInfo[pid].prevTotalTime = _processTimeInfo[pid].totalTime;
                            _processTimeInfo[pid].prevSTime = _processTimeInfo[pid].sTime;
                            _processTimeInfo[pid].prevUTime = _processTimeInfo[pid].uTime;
                        }

                        std::string DivideWithPrecision(double numerator, double denominator, uint32_t precisision) {
                            ASSERT(denominator != 0);
                            ASSERT(precisision != 0);

                            double result = numerator / denominator;
                            std::ostringstream oss;
                            oss << std::fixed << std::setprecision(precisision) << result;
                            
                            return oss.str();
                        }

                        std::string KbToMb(uint32_t kb) {
                            return DivideWithPrecision(kb, 1024, 3);
                        }

                        std::string CpuAvgLoadToFloat(uint64_t avgLoad) {
                            return DivideWithPrecision(static_cast<double>(avgLoad), 65536.0, 2);
                        }

                        std::string StringsListToString(const std::list<std::string>& stringList, const std::string& delimiter = " ") {
                            std::ostringstream oss;
                            for (auto it = stringList.begin(); it != stringList.end(); ++it) {
                                if (it != stringList.begin()) {
                                    oss << delimiter;
                                }
                                oss << *it;
                            }
                            return oss.str();
                        }

                        std::string CreateKeyValuePair(const std::string& key, const std::string& value) {
                            return "\"" + key + "\":\"" + value + "\"";
                        }

                        std::string CreateKeyValuePair(const std::string& key, uint64_t value) {
                            return "\"" + key + "\":" + std::to_string(value);
                        }

                        std::string CreateKeyValuePair(const std::string& key, const std::string& value, const std::string& unit) {
                            return "\"" + key + "\":{\"value\":\"" + value + "\",\"unit\":\"" + unit + "\"}";
                        }

                        std::string CreateKeyValuePair(const std::string& key, uint64_t value, const std::string& unit) {
                            return "\"" + key + "\":{\"value\":" + std::to_string(value) + ",\"unit\":\"" + unit + "\"}";
                        }


                        std::string LogProcess(const Core::ProcessInfo& process) {
                            auto timestamp = Core::Time::Now().Ticks() / 1000; // ms from epoch
                            process.MemoryStats();
                            auto memSnapshot = Core::SystemInfo::Instance().TakeMemorySnapshot();
                            auto totalRam = memSnapshot.Total();
                            auto freeRam = memSnapshot.Free();
                            auto availableRam = memSnapshot.Available();
                            auto cpuLoad = Core::SystemInfo::Instance().GetCpuLoad();
                            auto cpuLoadAvgPtr = Core::SystemInfo::Instance().GetCpuLoadAvg();
                            auto cmdLine = StringsListToString(process.CommandLine());
                            cmdLine.erase(std::remove(cmdLine.begin(), cmdLine.end(), '"'), cmdLine.end());

                            auto result = _resultData.Set(CreateKeyValuePair("time", timestamp, "ms"), 
                                                CreateKeyValuePair("name", process.Name()),
                                                CreateKeyValuePair("pid", process.Id()), 
                                                CreateKeyValuePair("uss", KbToMb(process.USS()), "MB"), 
                                                CreateKeyValuePair("pss", KbToMb(process.PSS()), "MB"), 
                                                CreateKeyValuePair("rss", KbToMb(process.RSS()), "MB"), 
                                                CreateKeyValuePair("userTotalCpu", _userCpuTime, "%"), 
                                                CreateKeyValuePair("systemTotalCpu", _systemCpuTime, "%"), 
                                                CreateKeyValuePair("cpuLoad", cpuLoad, "%"),
                                                CreateKeyValuePair("cpuLoadAvg1m", CpuAvgLoadToFloat(cpuLoadAvgPtr[0]), "%"), 
                                                CreateKeyValuePair("cpuLoadAvg5m", CpuAvgLoadToFloat(cpuLoadAvgPtr[1]), "%"), 
                                                CreateKeyValuePair("cpuLoadAvg15m", CpuAvgLoadToFloat(cpuLoadAvgPtr[2]), "%"),
                                                CreateKeyValuePair("totalRam", KbToMb(totalRam), "MB"), 
                                                CreateKeyValuePair("freeRam", KbToMb(freeRam), "MB"), 
                                                CreateKeyValuePair("availableRam", KbToMb(availableRam),"MB"),
                                                CreateKeyValuePair("command", cmdLine));               
                            return result;
                        }
                    };
            };

        SERVICE_REGISTRATION(ResourceMonitorImplementation, 1, 0)
    } /* namespace Plugin */
} /* namespace WPEFramework */ 
