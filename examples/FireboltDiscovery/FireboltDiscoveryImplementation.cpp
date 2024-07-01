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

#include "FireboltDiscoveryImplementation.h"

namespace WPEFramework {

namespace Plugin
{
    class FireboltDiscoveryImplementation : public Exchange::IFDiscovery,
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
                , DictionaryCallSign()
            {
                Add(_T("callsign"), &DictionaryCallSign);
            }
            ~Config() override = default;
        public:
            Core::JSON::String DictionaryCallSign;
        };


    public:

        FireboltDiscoveryImplementation(const FireboltDiscoveryImplementation&) = delete;
        FireboltDiscoveryImplementation(FireboltDiscoveryImplementation&&) = delete;
        FireboltDiscoveryImplementation& operator= (const FireboltDiscoveryImplementation&) = delete;
        FireboltDiscoveryImplementation& operator= (FireboltDiscoveryImplementation&&) = delete;

        FireboltDiscoveryImplementation()
            : _service(nullptr)
            , _adminLock()
        {
        }
        ~FireboltDiscoveryImplementation() override {
        }
        Core::hresult watched(const string& appId)  {
            std::cout<<"Received Watch request for appId"<<appId<<'\n';    
            return Core::ERROR_NONE;
        }

#if 0
        Core::hresult Register(Exchange::IFireboltDiscovery::INotification* notification) override
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

        Core::hresult Unregister(Exchange::IFireboltDiscovery::INotification* notification) override
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
#endif



        uint32_t Configure(PluginHost::IShell* service)  override {

	        ASSERT( service != nullptr);
            uint32_t result = Core::ERROR_NONE;
            Config config;
            config.FromString(service->ConfigLine());
            _service = service;
            return result;
        }

        BEGIN_INTERFACE_MAP(FireboltDiscoveryImplementation)
        INTERFACE_ENTRY(Exchange::IFDiscovery)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service;
        mutable Core::CriticalSection _adminLock;
        bool _allowResumePoints;
    };

    SERVICE_REGISTRATION(FireboltDiscoveryImplementation, 1, 0, 0)
} // namespace Plugin
} // namespace WPEFramework

