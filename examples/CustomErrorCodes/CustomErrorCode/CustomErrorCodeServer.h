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
#pragma once

#include "Module.h"
#include <interfaces/ICustomErrorCode.h>

namespace Thunder {
namespace Plugin {

    class CustomErrorCodeServer : public Exchange::ICustomErrorCode, public PluginHost::IPlugin, public PluginHost::JSONRPC {
    public:
        CustomErrorCodeServer(const CustomErrorCodeServer&) = delete;
        CustomErrorCodeServer& operator=(const CustomErrorCodeServer&) = delete;
        CustomErrorCodeServer(CustomErrorCodeServer&&) = delete;
        CustomErrorCodeServer& operator=(CustomErrorCodeServer&&) = delete;

        CustomErrorCodeServer()
            : Exchange::ICustomErrorCode()
            , PluginHost::IPlugin()
            , PluginHost::JSONRPC()
        {
        }

        ~CustomErrorCodeServer() override = default;

    public:
        //Service Registration
        BEGIN_INTERFACE_MAP(CustomErrorCodeServer)
            INTERFACE_ENTRY(IPlugin)
            INTERFACE_ENTRY(IDispatcher)
            INTERFACE_ENTRY(Exchange::ICustomErrorCode)
        END_INTERFACE_MAP

        const string Initialize(PluginHost::IShell* service);
        void Deinitialize(PluginHost::IShell* service);
        string Information() const;

        Core::hresult TriggerCustomError(const int32_t errorcode) const override 
        {
            Core::hresult result = SimulatedComRPCCall(errorcode);

            if (result != Core::ERROR_NONE) {
                ExampleHandlingHResultWhichCouldContainCustomCode(result);
            }

            return Core::CustomCode(errorcode);
        }

        Core::hresult TriggerNonCustomError(const uint32_t errorcode) const override 
        {
            Core::hresult result = SimulatedComRPCCall(errorcode);

            if (result != Core::ERROR_NONE) {
                ExampleHandlingHResultWhichCouldContainCustomCode(result);
            }

            return errorcode;
        }

    private:
        // simulate comrpc call that returns an error
        Core::hresult SimulatedComRPCCall(const Core::hresult errorcode) const
        {
            return errorcode;
        }
        Core::hresult SimulatedComRPCCall(const int32_t customerrorcode) const
        {
            return Core::CustomCode(customerrorcode);
        }
        void ExampleHandlingHResultWhichCouldContainCustomCode(const Core::hresult errorcode) const
        {
            int32_t customcode = Core::IsCustomCode(errorcode);

            if (customcode == 0) {
                TRACE(Trace::Information, (_T("Thunder Error received: %u"), errorcode));
            } else {
                TRACE(Trace::Information, (_T("Custom Code received: %i"), customcode));
            }

            TRACE(Trace::Information, (_T("Error received: %s"), Core::ErrorToStringExtended(errorcode).c_str()));
        }

    };
}
}
