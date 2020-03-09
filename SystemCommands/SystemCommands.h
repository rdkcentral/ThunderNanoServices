/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include <interfaces/ISystemCommands.h>
#include <interfaces/json/JsonData_SystemCommands.h>

#include "SystemCommandsImplementation.h"

namespace WPEFramework {
namespace Plugin {

    class SystemCommands : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::ISystemCommands, public PluginHost::JSONRPC {
    public:
        SystemCommands()
        {
            RegisterAll();
        }

        ~SystemCommands() override
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(SystemCommands)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(Exchange::ISystemCommands)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        //   IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IWeb methods
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        // SystemCommands methods
        uint32_t USBReset(const std::string& device) override;

    private:
        SystemCommands(const SystemCommands&) = delete;
        SystemCommands& operator=(const SystemCommands&) = delete;

        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_usbreset(const JsonData::SystemCommands::UsbresetParamsData& params);

        SystemCommandsImplementation _implemetation;

        uint8_t _skipURL = 0;
    };

} // namespace Plugin
} // namespace WPEFramework
