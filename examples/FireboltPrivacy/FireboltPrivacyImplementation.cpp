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

#define MYSMARTINTERFACE
namespace WPEFramework {

namespace Plugin
{
    class FireboltPrivacyImplementation : public Exchange::IFireboltPrivacy,
                                         public Exchange::IConfiguration {

    private:
#ifdef MYSMARTINTERFACE 
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
            #if 0
            uint32_t connect(const string& callsign) {
                return BaseClass::Open(RPC::CommunicationTimeOut, BaseClass::Connector(), _T(callsign.c_str()));
            }
            #endif
            bool Get(const string& nameSpace, const string& key, string& value ) const {
                SYSLOG(Logging::Error, (_T("Entry: %s"), __FUNCTION__ ));
                bool result = false;
                const WPEFramework::Exchange::IDictionary* impl = BaseClass::Interface();
        
                if (impl != nullptr) {
                    SYSLOG(Logging::Error, (_T("Able to get Dictionary implementation:")));
                    result = impl->Get(nameSpace, key, value);
                    impl->Release();
                } else {
                    SYSLOG(Logging::Error, (_T("Unable to get Dictionary implementation:")));
                }

                return (result);
            }
            bool Set(const string& nameSpace, const string& key, const string& value) {
                SYSLOG(Logging::Error, (_T("Entry: %s"), __FUNCTION__ ));
                bool result = false;
                WPEFramework::Exchange::IDictionary* impl = BaseClass::Interface();
        
                if (impl != nullptr) {
                    SYSLOG(Logging::Error, (_T("Able to get Dictionary implementation:")));
                    result = impl->Set(nameSpace, key, value);
                    impl->Release();
                } else {
                    SYSLOG(Logging::Error, (_T("Unable to get Dictionary implementation:")));
                }
        
                return (result);
            }
        
        private:
            void Operational(const bool upAndRunning) {
                SYSLOG(Logging::Error, (_T("Operational state of Dictionary : %s"), upAndRunning ? _T("true"): _T("false")));
            }
        };
#endif
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
                SYSLOG(Logging::Error, (_T("Entry: %s"), __FUNCTION__ ));
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
            NotificationDispatcher(FireboltPrivacyImplementation&  parent, bool allowResumePoints):_parent(parent), _allowResumePoints(allowResumePoints) {
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
            , _notifications()
            , _inMemory(true)
       #ifdef MYSMARTINTERFACE
            , _localStore()
       #endif
        {
            SYSLOG(Logging::Error, (_T("Entry: %s"), __FUNCTION__ ));
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
        Core::hresult GetStorageLocation(StorageLocation& value /* @out */) const override {
            SYSLOG(Logging::Error, (_T("Entering: %s"), __FUNCTION__));
            if (_inMemory) {
                value = StorageLocation::InMemory;
            } else {
                value = StorageLocation::Disk;
            }
            return Core::ERROR_NONE;
        }
        Core::hresult GetAllowResumePoints(bool& allowResumePoints /* @out */) const override {
            SYSLOG(Logging::Error, (_T("Entering: %s"), __FUNCTION__));
            if (_inMemory) {
                _adminLock.Lock();
                allowResumePoints = _allowResumePoints;
                _adminLock.Unlock();
            } else {
               #if 0 
                if (_localStore == nullptr) {
                    _localStore = _service->QueryInterfaceByCallsign<Exchange::IDictionary>("Dictionary");
                }
                if (_localStore != nullptr) {
                    string value;
                    _localStore->Get("FireboltPrivacy", "AllowResumePoints", value);
                    if (value.empty() || value == "false" ) {
                        allowResumePoints = false;
                    } else {
                        allowResumePoints = true;
                    }
                } else {
                    SYSLOG(Logging::Error,(_T("UNABLE TO GET DICTIONARY INTERFACE")) ); 
                    return Core::ERROR_RPC_CALL_FAILED;
                }
                #endif
                #ifdef MYSMARTINTERFACE

                SYSLOG(Logging::Error, (_T("Operations: %s"), _localStore->IsOperational() ? _T("true"): _T("false")));
                string value;
                //_localStore.Get("FireboltPrivacy", "AllowResumePoints", value);
                _localStore->Get(PrivacyNamespace, PrivacyKeyAllowResume, value);
                if (value.empty() || value == "false" ) {
                    allowResumePoints = false;
                } else {
                    allowResumePoints = true;
                }
                #else
                    allowResumePoints = true;
                #endif
            }
            return Core::ERROR_NONE;
        }
        Core::hresult SetAllowResumePoints(const bool& allowResumePoints ) override {
            SYSLOG(Logging::Error, (_T("Entering: %s"), __FUNCTION__));
            if (_inMemory) {
                _adminLock.Lock();
                _allowResumePoints = allowResumePoints;
                _adminLock.Unlock();
                Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(Core::ProxyType<NotificationDispatcher>::Create(*this, allowResumePoints)));
            } else {
                #if 0
                if (_localStore == nullptr) {
                    _localStore = _service->QueryInterfaceByCallsign<Exchange::IDictionary>("Dictionary");
                }
                if (_localStore != nullptr) {
                    if (allowResumePoints) {
                        _localStore->Set("FireboltPrivacy", "AllowResumePoints", "true");
                    } else {
                        _localStore->Set("FireboltPrivacy", "AllowResumePoints", "false");
                    }
                    _adminLock.Lock();
                    for(auto notification: _notifications){
                        notification->OnAllowResumePointsChanged(allowResumePoints);
                    }
                    _adminLock.Unlock();
                } else {
                      SYSLOG(Logging::Error,(_T("UNABLE TO GET DICTIONARY INTERFACE")) ); 
                    return Core::ERROR_RPC_CALL_FAILED;
                }
                #else
                #ifdef MYSMARTINTERFACE
                SYSLOG(Logging::Error, (_T("Operations: %s"), _localStore->IsOperational() ? _T("true"): _T("false")));
                string value;
                bool ret_val;
                if (allowResumePoints) {
                    ret_val = _localStore->Set(PrivacyNamespace, PrivacyKeyAllowResume, "true");
                } else {
                    ret_val = _localStore->Set(PrivacyNamespace, PrivacyKeyAllowResume, "false");
                }
                Core::IWorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(Core::ProxyType<NotificationDispatcher>::Create(*this, allowResumePoints)));
                SYSLOG(Logging::Error, (_T("Set Operation result: %s"), ret_val ? _T("true"): _T("false")));
                #endif
                #endif
                SYSLOG(Logging::Error, (_T("Set Operation Completed")));

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
            SYSLOG(Logging::Error, (_T("Callsign is null: %d"), config.DictionaryCallSign.IsNull()));
            if (config.DictionaryCallSign.IsNull() == false && config.DictionaryCallSign.Value().empty() == false) { 
                _inMemory = false;
                string callsign = config.DictionaryCallSign.Value();
                SYSLOG(Logging::Error, (_T("Callsign : %s"), callsign.c_str()));
                if (_localStore.IsValid() == false) {
                    _localStore = Core::ProxyType<Dictionary>::Create(callsign);
                }
                //result = _localStore.connect(config.DictionaryCallSign.Value());
                //SYSLOG(Logging::Error, (_T("Connect result : %d"), result));
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
        Notifications _notifications;
        bool _inMemory;
        #ifdef MYSMARTINTERFACE
        Core::ProxyType<Dictionary> _localStore;
        #endif
    };

    SERVICE_REGISTRATION(FireboltPrivacyImplementation, 1, 0, 0)
} // namespace Plugin
} // namespace WPEFramework

