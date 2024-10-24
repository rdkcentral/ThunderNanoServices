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

    class DynamicJSONRPCErrorMessage : public PluginHost::IPlugin
                                     , public PluginHost::JSONRPCErrorAssessor
                                     , Exchange::IMath {

    public:
        DynamicJSONRPCErrorMessage() = default;
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
        uint32_t Add(const uint16_t, const uint16_t, uint16_t&) const override {
            return Core::ERROR_USER_DEFINED_JSONRPC;
        };

        uint32_t Sub(const uint16_t, const uint16_t, uint16_t&) const override {
            return Core::ERROR_USER_DEFINED_JSONRPC;
        }

        // JSONRPCErrorAssessor overrides
        uint32_t OnJSONRPCError(const Core::JSONRPC::Context& context, const string& designator, const string& parameters, string& errormessage) override;

    public:
        BEGIN_INTERFACE_MAP(DynamicJSONRPCErrorMessage)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::IMath)
        END_INTERFACE_MAP

    }; // class DynamicJSONRPCErrorMessage

} // namespace Plugin

}
