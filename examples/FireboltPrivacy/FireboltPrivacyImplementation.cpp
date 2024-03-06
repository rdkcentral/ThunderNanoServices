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
#include <interfaces/IConfiguration.h>
#include <interfaces/IFireboltPrivacy.h>
#include <interfaces/IDictionary.h>
#include <interfaces/IStore.h>
#include <interfaces/json/JsonData_PersistentStore.h>

namespace WPEFramework {

namespace Plugin
{
    class FireboltPrivacyImplementation : public Exchange::IFireboltPrivacy,
                                         public Exchange::IConfiguration {

    private:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config(Config&&) = delete;
            Config& operator=(const Config&) = delete;
            Config& operator=(Config&&) = delete;

            Config()
                : Core::JSON::Container()
                , StorageLocation(Exchange::IFireboltPrivacy::StorageLocation::InMemory)
            {
                Add(_T("storagelocation"), &StorageLocation);
            }
            ~Config() override = default;
        public:
            Core::JSON::EnumType<Exchange::IFireboltPrivacy::StorageLocation> StorageLocation;
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
            , _inMemory(false)
            , _localStore(nullptr)
        {
        }
        ~FireboltPrivacyImplementation() override = default;

        Core::hresult Register(Exchange::IFireboltPrivacy::INotification* notification) override
        {
            ASSERT(notification != nullptr);
            ASSERT(std::find(_notifications.begin(), _notifications.end(), notification) == _notifications.end());
            _adminLock.Lock();
            notification->AddRef();
            _notifications.push_back(notification);
            _adminLock.Unlock();

            return Core::ERROR_NONE;
        }

        Core::hresult Unregister(Exchange::IFireboltPrivacy::INotification* notification) override
        {
            ASSERT(notification != nullptr);
            _adminLock.Lock();
            auto item = std::find(_notifications.begin(), _notifications.end(), notification);
            if(item != _notifications.end()){
                notification->Release();
                _notifications.erase(item);
            }
            _adminLock.Unlock();

            return Core::ERROR_NONE;
        }
        Core::hresult GetStorageLocation(StorageLocation& value /* @out */) const override {
            if (_inMemory) {
                value = StorageLocation::InMemory;
            } else {
                value = StorageLocation::Disk;
            }
            return Core::ERROR_NONE;
        }
        Core::hresult AllowResumePoints(bool& allowResumePoints /* @out */) override {
            if (_inMemory) {
                _adminLock.Lock();
                allowResumePoints = _allowResumePoints;
                _adminLock.Unlock();
            } else {
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
            }
            return Core::ERROR_NONE;
        }
        Core::hresult SetAllowResumePoints(const bool& allowResumePoints ) override {
            if (_inMemory) {
                _adminLock.Lock();
                _allowResumePoints = allowResumePoints;
                for(auto notification: _notifications){
                    notification->OnAllowResumePointsChanged(allowResumePoints);
                }
                _adminLock.Unlock();
            } else {
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
            }
            return Core::ERROR_NONE;
        }
        uint32_t Configure(PluginHost::IShell* service)  override {
	        ASSERT( service != nullptr);
            Config config;
            config.FromString(service->ConfigLine());
            _service = service;
            Core::JSON::EnumType<Exchange::IFireboltPrivacy::StorageLocation> storageLocation = config.StorageLocation.Value();
            if (storageLocation == Exchange::IFireboltPrivacy::StorageLocation::InMemory) {
                _inMemory = true;
                
                _localStore = _service->QueryInterfaceByCallsign<Exchange::IDictionary>("Dictionary");
            }
            return Core::ERROR_NONE;
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
        Exchange::IDictionary* _localStore;
    };

    SERVICE_REGISTRATION(FireboltPrivacyImplementation, 1, 0, 0)
} // namespace Plugin
} // namespace WPEFramework

