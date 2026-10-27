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

#include "Module.h"
#include "TestDelegatedRelease.h"

namespace Thunder {
namespace Plugin {

    namespace {
        static Metadata<TestDelegatedRelease> metadata(
            1, 0, 0,
            {},
            {},
            {}
        );
    }

    const string TestDelegatedRelease::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        _service = service;
        _service->AddRef();

        return {};
    }

    void TestDelegatedRelease::Deinitialize(PluginHost::IShell* service)
    {
        // The peer references are intentionally left for channel teardown to release.
        if (_service != nullptr) {
            ASSERT(_service == service);
            _service->Release();
            _service = nullptr;
        }
    }

    Core::hresult TestDelegatedRelease::Ping(uint32_t& value)
    {
        value = 0x12345678;
        return Core::ERROR_NONE;
    }

    Core::hresult TestDelegatedRelease::HoldPeer(QualityAssurance::ITestDelegatedReleasePeer* peer)
    {
        ASSERT(peer != nullptr);

        if (peer == nullptr) {
            return Core::ERROR_BAD_REQUEST;
        }

        peer->AddRef();

        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace Thunder