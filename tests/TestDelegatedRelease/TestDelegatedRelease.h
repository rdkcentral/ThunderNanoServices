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

#include <qa_interfaces/ITestDelegatedRelease.h>

namespace Thunder {
namespace Plugin {

    class TestDelegatedRelease : public PluginHost::IPlugin,
                                 public QualityAssurance::ITestDelegatedRelease {
    public:
        TestDelegatedRelease() = default;
        ~TestDelegatedRelease() override = default;

        TestDelegatedRelease(const TestDelegatedRelease&) = delete;
        TestDelegatedRelease& operator=(const TestDelegatedRelease&) = delete;

        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override { return {}; }

        Core::hresult Ping(uint32_t& value) override;
        Core::hresult HoldPeer(QualityAssurance::ITestDelegatedReleasePeer* peer) override;

        BEGIN_INTERFACE_MAP(TestDelegatedRelease)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(QualityAssurance::ITestDelegatedRelease)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service = nullptr;
    };

} // namespace Plugin
} // namespace Thunder