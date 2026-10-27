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

#include "TestSmartProvider.h"

namespace Thunder {
namespace Plugin {

    namespace {
        static Metadata<TestSmartProvider> metadata(1, 0, 0, {}, {}, {});
    }

    const string TestSmartProvider::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        return {};
    }

    void TestSmartProvider::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
    }

    string TestSmartProvider::Information() const
    {
        return {};
    }

    uint32_t TestSmartProvider::Add(const uint16_t a, const uint16_t b, uint16_t& sum) const
    {
        sum = static_cast<uint16_t>(a + b);
        return Core::ERROR_NONE;
    }

    uint32_t TestSmartProvider::Sub(const uint16_t a, const uint16_t b, uint16_t& sum) const
    {
        sum = static_cast<uint16_t>(a - b);
        return Core::ERROR_NONE;
    }

    SERVICE_REGISTRATION(TestSmartProvider, 1, 0)

} // namespace Plugin
} // namespace Thunder
