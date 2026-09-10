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
#include <qa_interfaces/IPrivateComRPC.h>

namespace Thunder {
namespace Plugin {

    class PrivateComRPCTest : public PluginHost::IPlugin, public PluginHost::JSONRPC, public QualityAssurance::IPrivateComRPC {
    private:
        PrivateComRPCTest(const PrivateComRPCTest&) = delete;
        PrivateComRPCTest& operator=(const PrivateComRPCTest&) = delete;

    public:
        PrivateComRPCTest();
        ~PrivateComRPCTest() override;

        BEGIN_INTERFACE_MAP(PrivateComRPCTest)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_ENTRY(QualityAssurance::IPrivateComRPC)
        END_INTERFACE_MAP

    public:
        // PluginHost::IPlugin overrides.
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        // QualityAssurance::IPrivateComRPC overrides.
        Core::hresult ProcessLargeData(const string& payload, const uint32_t delayMs, const bool echo, string& response /* @out */) override;

    private:
        uint32_t ProcessLargeData(const JsonObject& parameters, JsonObject& response);

    private:
        PluginHost::IShell* _service;
    };

} // namespace Plugin
} // namespace Thunder