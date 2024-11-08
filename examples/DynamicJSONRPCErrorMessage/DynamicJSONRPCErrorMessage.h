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

#pragma once

#include "Module.h"

#include <interfaces/IMath.h>

namespace Thunder {

namespace Plugin {


// some examples:
// use 2nd line to call a simple (static) funtion
// use 3rd line to call a more complex function (lamda, bind())
// use 4th line to have the normal errorhandling
// of course enable the correct accompanying line/part in the c'tor

    class DynamicJSONRPCErrorMessage : public PluginHost::IPlugin
                                     , public PluginHost::JSONRPCErrorAssessor<PluginHost::JSONRPCErrorAssessorTypes::FunctionCallbackType>
//                                     , public PluginHost::JSONRPCErrorAssessor<PluginHost::JSONRPCErrorAssessorTypes::StdFunctionCallbackType>
//                                     , public PluginHost::JSONRPC
                                     , public Exchange::IMath {

    public:
        DynamicJSONRPCErrorMessage() 
            : PluginHost::IPlugin()
            , PluginHost::JSONRPCErrorAssessor<PluginHost::JSONRPCErrorAssessorTypes::FunctionCallbackType>(DynamicJSONRPCErrorMessage::OnJSONRPCError)
/*            , PluginHost::JSONRPCErrorAssessor<PluginHost::JSONRPCErrorAssessorTypes::StdFunctionCallbackType>([this](const Core::JSONRPC::Context& context, const string& method, const string& params, const uint32_t errorcode, string& result) -> uint32_t
            {
                return OnJSONRPCErrorMethod(context, method, params, errorcode, result);
            }) */
//            , PluginHost::JSONRPC()
            , Exchange::IMath() 
        {
        }
        ~DynamicJSONRPCErrorMessage() override = default;

        DynamicJSONRPCErrorMessage(const DynamicJSONRPCErrorMessage&) = delete;
        DynamicJSONRPCErrorMessage &operator=(const DynamicJSONRPCErrorMessage&) = delete;
        DynamicJSONRPCErrorMessage(DynamicJSONRPCErrorMessage&&) = delete;
        DynamicJSONRPCErrorMessage &operator=(DynamicJSONRPCErrorMessage&&) = delete;

    public:
        // PluginHost::IPlugin overrides
        const string Initialize(PluginHost::IShell *service) override;
        void Deinitialize(PluginHost::IShell *service) override;
        string Information() const override;

        // IMath overrides
        uint32_t Add(const uint16_t a, const uint16_t b , uint16_t&) const override {
            uint32_t result = Core::ERROR_NOT_SUPPORTED;
            if( a + b == 4 ) {
                result = Core::ERROR_GENERAL;
            }
            return result;
        };

        uint32_t Sub(const uint16_t a, const uint16_t b, uint16_t& result) const override {
            result = a - b;
            return Core::ERROR_NONE;
        }

        uint32_t OnJSONRPCErrorMethod(const Core::JSONRPC::Context& context, const string& method, const string& parameters, const uint32_t errorcode, string& errormessage);
        static uint32_t OnJSONRPCError(const Core::JSONRPC::Context& context, const string& method, const string& parameters, const uint32_t errorcode, string& errormessage);

    public:
        BEGIN_INTERFACE_MAP(DynamicJSONRPCErrorMessage)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::IMath)
        END_INTERFACE_MAP

    }; // class DynamicJSONRPCErrorMessage

} // namespace Plugin

}
