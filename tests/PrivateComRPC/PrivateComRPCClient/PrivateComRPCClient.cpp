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

#include "PrivateComRPCClient.h"

#include <qa_interfaces/IPrivateComRPC.h>

namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<PrivateComRPCClient> metadata(
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

    PrivateComRPCClient::PrivateComRPCClient()
        : _service(nullptr)
        , _callsign()
    {
        Register<ProcessLargeDataParams, ProcessLargeDataResult>(_T("processLargeData"), &PrivateComRPCClient::ProcessLargeData, this);
    }

    PrivateComRPCClient::~PrivateComRPCClient()
    {
        Unregister(_T("processLargeData"));
    }

    const string PrivateComRPCClient::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);

        _service = service;
        _service->AddRef();

        Config config;
        config.FromString(_service->ConfigLine());

        // Fall back to the default callsign if none is configured.
        _callsign = (config.Callsign.IsSet() && (config.Callsign.Value().empty() == false)) ? config.Callsign.Value() : string(_T("PrivateComRPCTest"));

        return string();
    }

    void PrivateComRPCClient::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service == _service);

        _service->Release();
        _service = nullptr;
        _callsign.clear();
    }

    string PrivateComRPCClient::Information() const
    {
        return string();
    }

    Core::hresult PrivateComRPCClient::ProcessLargeData(const ProcessLargeDataParams& params, ProcessLargeDataResult& response)
    {
        ASSERT(_service != nullptr);

        Core::hresult result = Core::ERROR_UNAVAILABLE;

        QualityAssurance::IPrivateComRPC* interface =
        _service->QueryInterfaceByCallsign<QualityAssurance::IPrivateComRPC>(_callsign);

        if (interface != nullptr) {
            string output;

            result = interface->ProcessLargeData(params.Payload.Value(), params.DelayMs.Value(), params.Echo.Value(), output);

            if (result == Core::ERROR_NONE) {
                response.Response = output;
            }

            interface->Release();
        }

        return result;
    }

} // namespace Plugin
} // namespace Thunder