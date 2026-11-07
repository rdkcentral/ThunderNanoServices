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

#include "PrivateComRPCTest.h"

#include <chrono>
#include <thread>

namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<PrivateComRPCTest> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

    PrivateComRPCTest::PrivateComRPCTest()
        : _service(nullptr)
    {
        Register<JsonObject, JsonObject>(
        _T("processLargeData"),
        static_cast<uint32_t (PrivateComRPCTest::*)(
            const JsonObject&,
            JsonObject&)>(
            &PrivateComRPCTest::ProcessLargeData),
        this);
    }

    PrivateComRPCTest::~PrivateComRPCTest()
    {
        Unregister(_T("processLargeData"));
    }

    const string PrivateComRPCTest::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        _service = service;
        _service->AddRef();

        return string();
    }

    void PrivateComRPCTest::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service == _service);

        _service->Release();
        _service = nullptr;
    }

    string PrivateComRPCTest::Information() const
    {
        return string();
    }

    Core::hresult PrivateComRPCTest::ProcessLargeData(const string& payload, const uint32_t delayMs, const bool echo, string& response)
    {
        if (payload.empty() == true) {
            return Core::ERROR_BAD_REQUEST;
        }

        if (delayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }

        if (echo == true) {
            response = payload;
        } else {
            response.clear();
        }

        return Core::ERROR_NONE;
    }

    uint32_t PrivateComRPCTest::ProcessLargeData(const JsonObject& parameters, JsonObject& response)
    {
        if (parameters.HasLabel(_T("payload")) == false) {
            return Core::ERROR_BAD_REQUEST;
        }

        const string payload = parameters[_T("payload")].String();

        uint32_t delayMs = 0;
        bool echo = false;

        if (parameters.HasLabel(_T("delayMs")) == true) {
            delayMs = parameters[_T("delayMs")].Number();
        }

        if (parameters.HasLabel(_T("echo")) == true) {
            echo = parameters[_T("echo")].Boolean();
        }

        string result;

        const Core::hresult status = ProcessLargeData(payload, delayMs, echo, result);

        if (status != Core::ERROR_NONE) {
            return status;
        }

        response[_T("success")] = true;
        response[_T("sentBytes")] = static_cast<uint32_t>(payload.length());
        response[_T("receivedBytes")] = static_cast<uint32_t>(result.length());

        if (echo == true) {
            response[_T("payload")] = result;
        }

        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace Thunder