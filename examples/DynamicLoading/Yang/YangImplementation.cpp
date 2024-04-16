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

    class YangImplementation: public Exchange::IYang {
    private:
        class Config : public Core::JSON::Container {
        public:

            Config()
                : Core::JSON::Container()
                , YinCallsign()
                , Etymology()
                , Color()
            {
                Init();
            }

            Config(const string& configLine)
                : Core::JSON::Container()
                , YinCallsign()
                , Etymology()
                , Color()
            {
                Init();
                FromString(configLine);
            }

            ~Config() = default;

            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        private:
            void Init()
            {
                Add(_T("yincallsign"), &YinCallsign);
                Add(_T("etymology"), &Etymology);
                Add(_T("color"), &Color);
            }

        public:
            Core::JSON::String YinCallsign;
            Core::JSON::String Etymology;
            Core::JSON::EnumType<Exchange::IYang::color> Color;
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
        YangImplementation()
            : _service(nullptr)
            , _lock()
            , _observers()
            , _decoupledJob()
            , _settingsFileName(_T("config.json"))
            , _yinCallsign()
            , _etymology(_T("undefined"))
            , _color(Exchange::IYang::color::BLACK)
            , _balance(50)
        {
        }
        ~YangImplementation() override
        {
            if (_service != nullptr) {
                _service->Release();
            }
        }

        YangImplementation(const YangImplementation&) = delete;
        YangImplementation& operator=(const YangImplementation&) = delete;

    private:
        void Configure(const Config& config)
        {
            _yinCallsign = config.YinCallsign;
            _etymology = config.Etymology;
            _color = config.Color;

            LoadSettings();

            TRACE(Trace::Information, (_T("Yang means '%s'"), _etymology.c_str()));
            TRACE(Trace::Information, (_T("Yang's color is '%s'"), config.Color.Data()));
        }
        void LoadSettings()
        {
            Core::File settingsFile(_service->PersistentPath() + _settingsFileName);
            if (settingsFile.Open(true) == true) {
                Config config;
                config.IElement::FromFile(settingsFile);

                if (config.Color.IsSet() == true) {
                    _color = config.Color;
                }

                settingsFile.Close();
            }
        }
        void SaveSettings() const
        {
            Core::File settingsFile(_service->PersistentPath() + _settingsFileName);
            if (settingsFile.Create() == true) {
                Config config;
                config.Color = _color;
                config.IElement::ToFile(settingsFile);
                settingsFile.Close();
            }
            else {
                TRACE(Trace::Error, (_T("Failed to save settings")));
            }
        }

    public:
        // IYang overrides
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
        uint32_t Color(color& value) const override
        {
            _lock.Lock();
            value = _color;
            _lock.Unlock();

            return (Core::ERROR_NONE);
        }
        uint32_t Color(const color& value) override
        {
            _lock.Lock();

            if (_color != value) {
                _color = value;

                TRACE(Trace::Information, (_T("Yang's color is now '%s'"),
                        Core::EnumerateType<decltype(_color)>(_color).Data()));

                SaveSettings();
            }

            _lock.Unlock();

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
                Exchange::IYin* yin = (_yinCallsign.empty()? nullptr
                                            : _service->QueryInterfaceByCallsign<Exchange::IYin>(_yinCallsign));

                if (yin == nullptr) {
                    TRACE(Trace::Error, (_T("Unable to modify yang if yin is not present")));
                    result = Core::ERROR_UNAVAILABLE;
                }
                else {
                    _lock.Lock();

                    if (percentage != _balance) {
                        _balance = percentage;
                        BalanceChanged(percentage);

                        yin->AddRef();

                        _decoupledJob.Submit([yin, percentage]() {
                            yin->Balance(100 - percentage);
                            yin->Release();
                        });
                    }

                    _lock.Unlock();
                }

                yin->Release();
            }

            return (result);
        }
        uint32_t Register(Exchange::IYang::INotification* notification)
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
        uint32_t Unregister(const Exchange::IYang::INotification* notification)
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
            TRACE(Trace::Information, (_T("Yang balance changed to %u%%"), percentage));

            // Called interlocked!
            for (Exchange::IYang::INotification*& observer : _observers) {
                observer->BalanceChanged(percentage);
            }
        }

    public:
        BEGIN_INTERFACE_MAP(YangImplementation)
            INTERFACE_ENTRY(Exchange::IYang)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service;
        mutable Core::CriticalSection _lock;
        std::list<Exchange::IYang::INotification*> _observers;
        DecoupledJob _decoupledJob;
        string _settingsFileName;
        string _yinCallsign;
        string _etymology;
        Exchange::IYang::color _color;
        uint8_t _balance;
    }; // class YangImplementation

    SERVICE_REGISTRATION(YangImplementation, 1, 0)

} // namespace Plugin

}
