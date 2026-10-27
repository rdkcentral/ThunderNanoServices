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
#include <qa_interfaces/ISmartConsumer.h>
#include <qa_interfaces/json/JSmartConsumer.h>

namespace Thunder {
namespace Plugin {

class TestSmartConsumer
    : public PluginHost::IPlugin
    , public PluginHost::JSONRPC
    , public QualityAssurance::ISmartConsumer {
private:
    using SmartMath = RPC::PluginSmartInterfaceType<Exchange::IMath>;

    class Config : public Core::JSON::Container {
    public:
        Config();

        Core::JSON::String ProviderCallsign;
    };

public:
    TestSmartConsumer();
    ~TestSmartConsumer() override;

    TestSmartConsumer(const TestSmartConsumer&) = delete;
    TestSmartConsumer& operator=(const TestSmartConsumer&) = delete;

    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

    uint32_t Calculate(const uint16_t a, const uint16_t b, uint16_t& addResult, uint16_t& subResult) override;
    BEGIN_INTERFACE_MAP(TestSmartConsumer)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::JSONRPC)
        INTERFACE_ENTRY(QualityAssurance::ISmartConsumer)
    END_INTERFACE_MAP

private:
    PluginHost::IShell* _service;
    string _providerCallsign;
    SmartMath _smartMath;
};

} // namespace Plugin
} // namespace Thunder