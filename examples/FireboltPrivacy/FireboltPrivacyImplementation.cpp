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

#include "FireboltPrivacyImplementation.h"

namespace WPEFramework {

namespace Plugin
{
    class FireboltPrivacyImplementation : public Exchange::IFireboltPrivacy,
                                         public Exchange::IConfiguration {

    private:
        class Dictionary : public WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IDictionary > {

        private:
            using BaseClass = WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IDictionary >;

        public:
            Dictionary(string& callsign)
                : BaseClass() {
                BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), _T(callsign.c_str()));
            }

            ~Dictionary() {
                BaseClass::Close(WPEFramework::Core::infinite);
            }
        
        public:
            bool Get(const string& nameSpace, const string& key, string& value ) const {
                bool result = false;
                const WPEFramework::Exchange::IDictionary* impl = BaseClass::Interface();
        
                if (impl != nullptr) {
                    result = impl->Get(nameSpace, key, value);
                    impl->Release();
                } else {
                    SYSLOG(Logging::Error, (_T("Unable to get Dictionary implementation:")));
                }

                return (result);
            }

            bool Set(const string& nameSpace, const string& key, const string& value) {
                bool result = false;
                WPEFramework::Exchange::IDictionary* impl = BaseClass::Interface();
        
                if (impl != nullptr) {
                    result = impl->Set(nameSpace, key, value);
                    impl->Release();
                } else {
                    SYSLOG(Logging::Error, (_T("Unable to get Dictionary implementation:")));
                }
        
                return (result);
            }
        
        private:

            void Operational(const bool upAndRunning) {
                SYSLOG(Logging::Notification , (_T("Operational state of Dictionary : %s"), upAndRunning ? _T("true"): _T("false")));
            }
        };

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config(Config&&) = delete;
            Config& operator=(const Config&) = delete;
            Config& operator=(Config&&) = delete;

            Config()
                : Core::JSON::Container()
                , DictionaryCallSign()
            {
                Add(_T("callsign"), &DictionaryCallSign);
            }
            ~Config() override = default;
        public:
            Core::JSON::String DictionaryCallSign;
        };

        class NotificationDispatcher: public Core::IDispatch {
        public:
            NotificationDispatcher(const NotificationDispatcher&) = delete;
            NotificationDispatcher(NotificationDispatcher&&) = delete;
            NotificationDispatcher& operator= (const NotificationDispatcher&) = delete;
            NotificationDispatcher& operator= (NotificationDispatcher&&) = delete;
            NotificationDispatcher(FireboltPrivacyImplementation&  parent, bool allowResumePoints): _parent(parent)
                                                                                                  , _allowResumePoints(allowResumePoints) {
            }

            void Dispatch() override {
                _parent.NotifySink(_allowResumePoints);
            }

        private:
            FireboltPrivacyImplementation& _parent;
            bool _allowResumePoints;
        };

    public:
        using Notifications = std::vector<Exchange::IFireboltPrivacy::INotification*>;

        FireboltPrivacyImplementation(const FireboltPrivacyImplementation&) = delete;
        FireboltPrivacyImplementation(FireboltPrivacyImplementation&&) = delete;
        FireboltPrivacyImplementation& operator= (const FireboltPrivacyImplementation&) = delete;
        FireboltPrivacyImplementation& operator= (FireboltPrivacyImplementation&&) = delete;

        FireboltPrivacyImplementation()
            : _service(nullptr)
            , _adminLock()
            , _allowResumePoints(false)
            , _inMemory(true)
            , _notifications()
            , _localStore()
        {
        }
        ~FireboltPrivacyImplementation() override {
            if (_localStore.IsValid() == true) {
                _localStore.Release();
            }
        }

        Core::hresult Register(Exchange::IFireboltPrivacy::INotification* notification) override
        {
            ASSERT(notification != nullptr);

            _adminLock.Lock();
            ASSERT(std::find(_notifications.begin(), _notifications.end(), notification) == _notifications.end());
            if(std::find(_notifications.begin(), _notifications.end(), notification) == _notifications.end()){
                _notifications.push_back(notification);
                notification->AddRef();
            }
            _adminLock.Unlock();

            return Core::ERROR_NONE;
        }

        Core::hresult Unregister(Exchange::IFireboltPrivacy::INotification* notification) override
        {
            ASSERT(notification != nullptr);
            Core::hresult result = Core::ERROR_NONE;

            _adminLock.Lock();
            ASSERT(std::find(_notifications.begin(), _notifications.end(), notification) == _notifications.end());
            auto item = std::find(_notifications.begin(), _notifications.end(), notification);
            if(item != _notifications.end()){
                notification->Release();
                _notifications.erase(item);
            }
            else {
                result = Core::ERROR_INVALID_PARAMETER;
            }
            _adminLock.Unlock();

            return result;
        }

        Core::hresult GetAllowResumePoints(const string& appId, bool& allowResumePoints /* @out */) const override {
            SYSLOG(Logging::Error, (_T("Getting allow resume points: %s"), appId.c_str()));
            if (_inMemory) {
                _adminLock.Lock();
                allowResumePoints = _allowResumePoints;
                _adminLock.Unlock();
            } else {
                string value;
                _localStore->Get(PrivacyNamespace, PrivacyKeyAllowResume, value);
                if (value.empty() || strcasecmp(value.c_str(), "false") == 0 ) {
                    allowResumePoints = false;
                } else {
                    allowResumePoints = true;
                }
            }
            return Core::ERROR_NONE;
        }

        Core::hresult SetAllowResumePoints(const string& appId, const bool& allowResumePoints ) override {
            SYSLOG(Logging::Error, (_T("Setting allow resume points: %s"), appId.c_str()));
            if (_inMemory) {
                _adminLock.Lock();
                _allowResumePoints = allowResumePoints;
                _adminLock.Unlock();
                Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(Core::ProxyType<NotificationDispatcher>::Create(*this, allowResumePoints)));
            } else {
                string value;
                bool ret_val;
                if (allowResumePoints) {
                    ret_val = _localStore->Set(PrivacyNamespace, PrivacyKeyAllowResume, "true");
                } else {
                    ret_val = _localStore->Set(PrivacyNamespace, PrivacyKeyAllowResume, "false");
                }
                if (ret_val == true) {
                    Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(Core::ProxyType<NotificationDispatcher>::Create(*this, allowResumePoints)));
                }
            }
            return Core::ERROR_NONE;
        }

        void NotifySink(const bool allowResumePoints) {
            _adminLock.Lock();
            for (auto notification: _notifications) {
                notification->OnAllowResumePointsChanged(allowResumePoints);
            }
            _adminLock.Unlock();
        }

        uint32_t Configure(PluginHost::IShell* service)  override {

	        ASSERT( service != nullptr);
            uint32_t result = Core::ERROR_NONE;
            Config config;
            config.FromString(service->ConfigLine());
            _service = service;

            if (config.DictionaryCallSign.IsNull() == false && config.DictionaryCallSign.Value().empty() == false) { 
                _inMemory = false;
                string callsign = config.DictionaryCallSign.Value();
                if (_localStore.IsValid() == false) {
                    _localStore = Core::ProxyType<Dictionary>::Create(callsign);
                }
            }

            return result;
        }

        BEGIN_INTERFACE_MAP(FireboltPrivacyImplementation)
        INTERFACE_ENTRY(Exchange::IFireboltPrivacy)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service;
        mutable Core::CriticalSection _adminLock;
        bool _allowResumePoints;
        bool _inMemory;
        Notifications _notifications;
        Core::ProxyType<Dictionary> _localStore;
    };

    SERVICE_REGISTRATION(FireboltPrivacyImplementation, 1, 0, 0)
} // namespace Plugin
} // namespace WPEFramework

