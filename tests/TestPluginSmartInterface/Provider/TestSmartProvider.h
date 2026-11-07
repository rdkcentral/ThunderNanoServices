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

    class TestSmartProvider : public PluginHost::IPlugin, public Exchange::IMath {
    public:
        TestSmartProvider() = default;
        ~TestSmartProvider() override = default;

        TestSmartProvider(const TestSmartProvider&) = delete;
        TestSmartProvider& operator=(const TestSmartProvider&) = delete;

        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        uint32_t Add(const uint16_t a, const uint16_t b, uint16_t& sum) const override;
        uint32_t Sub(const uint16_t a, const uint16_t b, uint16_t& sum) const override;

        BEGIN_INTERFACE_MAP(TestSmartProvider)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(Exchange::IMath)
        END_INTERFACE_MAP
    };

} // namespace Plugin
} // namespace Thunder
