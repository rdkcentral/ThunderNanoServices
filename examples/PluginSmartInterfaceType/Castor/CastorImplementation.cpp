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

#include <interfaces/IConfiguration.h>
#include <interfaces/IMath.h>

namespace Thunder {

namespace Plugin {

    class CastorImplementation: public Exchange::IMath, 
                                public Exchange::IConfiguration,
                                public Thunder::RPC::PluginSmartInterfaceType<Thunder::Exchange::IMath> {
    private:
        using BaseClass = Thunder::RPC::PluginSmartInterfaceType<Thunder::Exchange::IMath>;

        class Config : Core::JSON::Container {
        public:
            Config(const string& configLine)
                : Core::JSON::Container()
                , PolluxCallsign()
            {
                Add(_T("polluxcallsign"), &PolluxCallsign);
                FromString(configLine);
            }

            ~Config() = default;

            Config() = delete;
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config(Config&&) = delete;
            Config& operator=(Config&&) = delete;

        public:
            Core::JSON::String PolluxCallsign;
        }; // class Config

    public:
        CastorImplementation()
            : _service(nullptr)
            , _polluxCallsign()
        {
        }
        ~CastorImplementation() override
        {
            if (_service != nullptr) {
                BaseClass::Close();
                _service->Release();
            }
        }

        CastorImplementation(const CastorImplementation&) = delete;
        CastorImplementation& operator=(const CastorImplementation&) = delete;
        CastorImplementation(CastorImplementation&&) = delete;
        CastorImplementation& operator=(CastorImplementation&&) = delete;

    public:
        void Configure(const Config& config)
        {
            _polluxCallsign = config.PolluxCallsign;
        }

        uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const override 
        {
            uint32_t result = Core::ERROR_NONE;
            const IMath* math = BaseClass::Interface();
            if(math != nullptr) {
                result = math->Add(A, B, sum);
                math->Release();
                if(result == Core::ERROR_NONE) {
                    TRACE(Trace::Information, (_T("Called Pollux::Add on behalf of Castor")));
                } else {
                    TRACE(Trace::Error, (_T("Error %u calling Pollux Add"), result));
                }
            } else {
                TRACE(Trace::Error, (_T("Could not call Add, Pollux was not available")));
                result = Core::ERROR_UNAVAILABLE;
            }
            return (result);
        }
        uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const override 
        {
            uint32_t result = Core::ERROR_NONE;
            const IMath* math = BaseClass::Interface();
            if(math != nullptr) {
                result = math->Sub(A, B, sum);
                math->Release();
                if(result == Core::ERROR_NONE) {
                    TRACE(Trace::Information, (_T("Called Pollux::Sub on behalf of Castor")));
                } else {
                    TRACE(Trace::Error, (_T("Error %u calling Pollux Sub"), result));
                }
            } else {
                TRACE(Trace::Error, (_T("Could not call Add, Pollux was not available")));
                result = Core::ERROR_UNAVAILABLE;
            }
            return (result);
        }

    public:
        uint32_t Configure(PluginHost::IShell* service) override
        {
            ASSERT(_service == nullptr);
            ASSERT(service != nullptr);
            _service = service;
            _service->AddRef();

            Configure(Config(_service->ConfigLine()));

            uint32_t result = BaseClass::Open(_service, _polluxCallsign);
            if( result != Core::ERROR_NONE ) {
                TRACE(Trace::Error, (_T("Failed to open pollux link")));
            }
            return (result);
        }

    public:
        BEGIN_INTERFACE_MAP(CastorImplementation)
            INTERFACE_ENTRY(Exchange::IMath)
            INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service;
        string _polluxCallsign;
    }; // class CastorImplementation

    SERVICE_REGISTRATION(CastorImplementation, 1, 0)

} // namespace Plugin

}
