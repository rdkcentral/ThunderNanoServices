/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#include <interfaces/IYin.h>
#include <interfaces/IYang.h>


namespace Thunder {

namespace Plugin {

    class YinImplementation: public Exchange::IYin {
    private:
        class Config : Core::JSON::Container {
        public:
            Config(const string& configLine)
                : Core::JSON::Container()
                , YangCallsign()
                , Etymology()
                , DataFile()
            {
                Add(_T("yangcallsign"), &YangCallsign);
                Add(_T("etymology"), &Etymology);
                Add(_T("datafile"), &DataFile);
                FromString(configLine);
            }

            ~Config() = default;

            Config() = delete;
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Core::JSON::String YangCallsign;
            Core::JSON::String Etymology;
            Core::JSON::String DataFile;
        }; // class Config

    private:
        class DecoupledJob {
            friend Core::ThreadPool::JobType<DecoupledJob&>;

        public:
            DecoupledJob()
                : _job(*this)
            {
            }
            ~DecoupledJob() = default;

            DecoupledJob(const DecoupledJob&) = delete;
            DecoupledJob& operator=(const DecoupledJob&) = delete;

        public:
            void Submit(const std::function<void(void)>& fn, const Core::Time& time = 0)
            {
                _fn = fn;
                _job.Reschedule(time);
            }

        private:
            void Dispatch()
            {
                _fn();
            }

        private:
            Core::WorkerPool::JobType<DecoupledJob&> _job;
            std::function<void(void)> _fn;
        }; // class DecoupledJob

    public:
        YinImplementation()
            : _service(nullptr)
            , _lock()
            , _observers()
            , _decoupledJob()
            , _yangCallsign()
            , _etymology(_T("undefined"))
            , _dataFile()
            , _balance(50)
        {
        }
        ~YinImplementation() override
        {
            if (_service != nullptr) {
                _service->Release();
            }
        }

        YinImplementation(const YinImplementation&) = delete;
        YinImplementation& operator=(const YinImplementation&) = delete;

    public:
        void Configure(const Config& config)
        {
            _yangCallsign = config.YangCallsign;
            _etymology = config.Etymology;
            _dataFile = config.DataFile;

            TRACE(Trace::Information, (_T("Yin means '%s'"), _etymology.c_str()));
            TRACE(Trace::Information, (_T("Yin's color is 'White")));
        }

    public:
        // IYin overrides
        void Configure(PluginHost::IShell* service) override
        {
            ASSERT(_service == nullptr);
            ASSERT(service != nullptr);
            _service = service;
            _service->AddRef();

            Configure(Config(_service->ConfigLine()));
        }
        uint32_t Etymology(string& meaning) const override
        {
            meaning = _etymology;
            return (Core::ERROR_NONE);
        }
        uint32_t Balance(uint8_t& percentage) const override
        {
            _lock.Lock();
            percentage = _balance;
            _lock.Unlock();

            return (Core::ERROR_NONE);
        }
        uint32_t Balance(const uint8_t& percentage) override
        {
            uint32_t result = Core::ERROR_NONE;

            if (percentage > 100) {
                TRACE(Trace::Error, (_T("Invalid percentage value: %u"), percentage));
                result = Core::ERROR_BAD_REQUEST;
            }
            else {
                ASSERT(_service != nullptr);
                Exchange::IYang* yang = (_yangCallsign.empty()? nullptr
                                            : _service->QueryInterfaceByCallsign<Exchange::IYang>(_yangCallsign));

                if (yang == nullptr) {
                    TRACE(Trace::Error, (_T("Unable to modify yin if yang is not present")));
                    result = Core::ERROR_UNAVAILABLE;
                }
                else {
                    _lock.Lock();

                    if (percentage != _balance) {
                        _balance = percentage;
                        BalanceChanged(percentage);

                        yang->AddRef();

                        _decoupledJob.Submit([yang, percentage]() {
                            yang->Balance(100 - percentage);
                            yang->Release();
                        });
                    }

                    _lock.Unlock();
                }

                yang->Release();
            }

            return (result);
        }
        uint32_t Symbol() override
        {
            uint32_t result = Core::ERROR_NONE;

            if (_balance != 50) {
                result = Core::ERROR_NOT_SUPPORTED;
            }
            else if (_dataFile.empty() == true) {
                TRACE(Trace::Error, (_T("Data file not specified in config")));
                result = Core::ERROR_UNAVAILABLE;
            }
            else {
                ASSERT(_service != nullptr);
                Core::File dataFile(_service->DataPath() + _dataFile);

                if (dataFile.Open(true) == true) {
                    const uint64_t size = std::min(dataFile.Size(), 4096ULL);
                    uint8_t *data = static_cast<uint8_t*>(ALLOCA(size + 1));
                    ASSERT(data != nullptr);
                    dataFile.Read(data, size);
                    dataFile.Close();
                    data[size] = '\0';

                    printf("%s\n", data);
                }
                else {
                    TRACE(Trace::Error, (_T("Data file not available")));
                    result = Core::ERROR_UNAVAILABLE;
                }
            }

            return (result);
        }
        uint32_t Register(Exchange::IYin::INotification* notification)
        {
            ASSERT(notification != nullptr);

            uint32_t result = Core::ERROR_BAD_REQUEST;

            _lock.Lock();

            if ((notification != nullptr) && (std::find(_observers.begin(), _observers.end(), notification) == _observers.end())) {
                _observers.push_back(notification);
                notification->AddRef();
                result = Core::ERROR_NONE;
            }

            _lock.Unlock();

            return (result);
        }
        uint32_t Unregister(const Exchange::IYin::INotification* notification)
        {
            ASSERT(notification != nullptr);

            uint32_t result = Core::ERROR_BAD_REQUEST;

            _lock.Lock();

            if (notification != nullptr) {
                auto it = std::find(_observers.begin(), _observers.end(), notification);
                if (it != _observers.end()) {
                    (*it)->Release();
                    _observers.erase(it);
                    result = Core::ERROR_NONE;
                }
            }

            _lock.Unlock();

            return (result);
        }

    private:
        void BalanceChanged(const uint8_t percentage)
        {
            TRACE(Trace::Information, (_T("Yin balance changed to %u%%"), percentage));

            // Called interlocked!
            for (Exchange::IYin::INotification* observer : _observers) {
                observer->BalanceChanged(percentage);
            }
        }

    public:
        BEGIN_INTERFACE_MAP(YinImplementation)
            INTERFACE_ENTRY(Exchange::IYin)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service;
        mutable Core::CriticalSection _lock;
        std::list<Exchange::IYin::INotification*> _observers;
        DecoupledJob _decoupledJob;
        string _yangCallsign;
        string _etymology;
        string _dataFile;
        uint8_t _balance;
    }; // class YinImplementation

    SERVICE_REGISTRATION(YinImplementation, 1, 0)

} // namespace Plugin

}
