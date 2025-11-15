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

        // the interface allows the json rpc call to provide the error code used which provides for easy testing
        Core::hresult TriggerCustomError(const int32_t errorcode) const override 
        {
            // we simulate calling aanother COMRPC call which might return a custom error code (and in this case it will return a custom code)
            Core::hresult result = SimulatedComRPCCall(errorcode);

            // we check if there was an error, comparing to Core::ERROR_NONE is the only thing needed also with custom codes
            // only if you want to deal with some specific error you need to handle that situation specifically which we wil do here as en example
            if (result != Core::ERROR_NONE) {
                ExampleHandlingHResultWhichCouldContainCustomCode(result);
            }

            return Core::CustomCode(errorcode == 0 ? Core::ERROR_NONE : errorcode); // of course we know Core::ERROR_NONE is also 0 but semantically it is more correct like this (and will continue to work might ERROR_NONE ever be a different value) 
        }

        // the interface allows the json rpc call to provide the error code used which provides for easy testing
        Core::hresult TriggerNonCustomError(const uint32_t errorcode) const override 
        {
            // we simulate calling aanother COMRPC call which might return a custom error code (and in this case it wil return a non custom code)
            Core::hresult result = SimulatedComRPCCall(errorcode);

            // we check if there was an error, comparing to Core::ERROR_NONE is the only thing needed also with custom codes
            // only if you want to deal with some specific error you need to handle that situation specifically which we wil do here as en example
            if (result != Core::ERROR_NONE) {
                ExampleHandlingHResultWhichCouldContainCustomCode(result);
            }

            return errorcode;
        }

    private:
        // simulate a COMRPC call that returns an Thunder defined error
        Core::hresult SimulatedComRPCCall(const Core::hresult errorcode) const
        {
            return errorcode;
        }
        // simulate a COMRPC call that returns an Custom Code 
        Core::hresult SimulatedComRPCCall(const int32_t customerrorcode) const
        {
            // when there is no error, just feed Core::ERROR_NONE into the Core::CustomCode(...) function to indicate there is no error as you would do with a normal hresult
            int32_t result = Core::ERROR_NONE;

            // suppose you detect an error situation and you want to set a custom code, just set the value assigned for this error
            if (customerrorcode != 0) {
                result = customerrorcode;
            }

            return Core::CustomCode(customerrorcode); // calling Core::CustomCode will convert the custom code into the correct hresult value so the custom code value gets stored appropriately in the hresult  
        }
        void ExampleHandlingHResultWhichCouldContainCustomCode(const Core::hresult errorcode) const
        {
            // here we handle an hresult in a detailed way in case we would like to extract the custom code for some reason (again not required, if you just want to handle an error situation you can just compare the hresult with Core::ERROR_NOE, nothing more to do 
            // (and if you need a text representation feed the hresult to ErrorToString or ErrorToStringExtended that will internnly handle the custom code correclty)
            int32_t customcode = Core::IsCustomCode(errorcode);

            if (customcode == 0) {
                TRACE(Trace::Information, (_T("Thunder defined error received: %u"), errorcode));
            } else if (customcode == std::numeric_limits<int32_t>::min()) {
                TRACE(Trace::Information, (_T("Invalid custom code was passed")));
            } else {
                TRACE(Trace::Information, (_T("Custom Code received: %i"), customcode));
            }

            // You can just feed any hresult to ErrorToString or ErrorToStringExtended, it will deal with the Custom Code internally
            TRACE(Trace::Information, (_T("Error received: %s"), Core::ErrorToStringExtended(errorcode).c_str()));
        }

    };
}
}
