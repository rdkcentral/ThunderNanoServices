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

// !!!! IMPORTANT !!!!
// This plugin should never be started as outofprocess!

#include "Module.h"
#include "interfaces/json/JsonData_Containers.h"

namespace WPEFramework {
namespace Plugin {

    class Containers : public PluginHost::IPlugin, PluginHost::JSONRPC {
    public:
        Containers(const Containers&) = delete;
        Containers& operator=(const Containers&) = delete;

        Containers()
        {
            RegisterAll();
        }

        virtual ~Containers()
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(Containers)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

    private:
        //   JSONRPC methods
        // -------------------------------------------------------------------------------------------------------
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_start(const JsonData::Containers::StartParamsData& params);
        uint32_t endpoint_stop(const JsonData::Containers::StopParamsData& params);
        uint32_t get_containers(Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_networks(const string& index, Core::JSON::ArrayType<JsonData::Containers::NetworksData>& response) const;
        uint32_t get_memory(const string& index, JsonData::Containers::MemoryData& response) const;
        uint32_t get_cpu(const string& index, JsonData::Containers::CpuData& response) const;
    };

} // namespace Plugin
} // namespace WPEFramework