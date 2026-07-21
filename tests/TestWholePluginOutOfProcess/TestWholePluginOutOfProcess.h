/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2026 Metrological
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

    class TestWholePluginOutOfProcess : public PluginHost::IPlugin, public PluginHost::JSONRPC, public Exchange::IMath {
    public:
        TestWholePluginOutOfProcess(const TestWholePluginOutOfProcess&) = delete;
        TestWholePluginOutOfProcess& operator=(const TestWholePluginOutOfProcess&) = delete;
        TestWholePluginOutOfProcess(TestWholePluginOutOfProcess&&) = delete;
        TestWholePluginOutOfProcess& operator=(TestWholePluginOutOfProcess&&) = delete;

        TestWholePluginOutOfProcess()
            : PluginHost::IPlugin()
            , PluginHost::JSONRPC()
            , Exchange::IMath()
        {
        }

        ~TestWholePluginOutOfProcess() override = default;
        // IPlugin Methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        // IMath methods
        uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */) const override;
        uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& difference /* @out */) const override;

        BEGIN_INTERFACE_MAP(TestWholePluginOutOfProcess)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::IMath)
        END_INTERFACE_MAP

    };
} // Plugin
} // Thunder