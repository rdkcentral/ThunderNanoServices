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

#include "Castor.h"
#include <interfaces/json/JMath.h>

namespace Thunder {

namespace Plugin {

    namespace {
        static Metadata<Castor> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { },
            // Terminations
            { },
            // Controls
            { }
        );
    }

    const string Castor::Initialize(PluginHost::IShell *service)
    {
        string message{};

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        _service = service;
        _service->AddRef();

        Configure(Config(_service->ConfigLine()));

        uint32_t result = BaseClass::Open(_service, _polluxCallsign);
        if( result != Core::ERROR_NONE ) {
            message =  _T("Failed to open pollux link");
        }

        Exchange::JMath::Register(*this, this);

        return (message);
    }

    void Castor::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {

            ASSERT(_service == service);

            Exchange::JMath::Unregister(*this);

            BaseClass::Close();

            _service->Release();
            _service = nullptr;
        }
    }

    string Castor::Information() const
    {
        return {};
    }
    
    uint32_t Castor::Add(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const  
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

    uint32_t Castor::Sub(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const 
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


} // namespace Plugin

}
