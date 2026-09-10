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

namespace Thunder {
namespace Plugin {

    class PrivateComRPCClient : public PluginHost::IPlugin
                               , public PluginHost::JSONRPC {
    private:
        class ProcessLargeDataParams : public Core::JSON::Container {
        public:
            ProcessLargeDataParams()
                : Core::JSON::Container()
                , Payload()
                , DelayMs(0)
                , Echo(true)
            {
                Add(_T("payload"), &Payload);
                Add(_T("delayMs"), &DelayMs);
                Add(_T("echo"), &Echo);
            }

            ProcessLargeDataParams(const ProcessLargeDataParams&) = delete;
            ProcessLargeDataParams& operator=(const ProcessLargeDataParams&) = delete;

        public:
            Core::JSON::String Payload;
            Core::JSON::DecUInt32 DelayMs;
            Core::JSON::Boolean Echo;
        };

        class ProcessLargeDataResult : public Core::JSON::Container {
        public:
            ProcessLargeDataResult()
                : Core::JSON::Container()
                , Response()
            {
                Add(_T("response"), &Response);
            }

            ProcessLargeDataResult(const ProcessLargeDataResult&) = delete;
            ProcessLargeDataResult& operator=(const ProcessLargeDataResult&) = delete;

        public:
            Core::JSON::String Response;
        };

    private:
        PrivateComRPCClient(const PrivateComRPCClient&) = delete;
        PrivateComRPCClient& operator=(const PrivateComRPCClient&) = delete;

    public:
        PrivateComRPCClient();
        ~PrivateComRPCClient() override;

        BEGIN_INTERFACE_MAP(PrivateComRPCClient)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        Core::hresult ProcessLargeData(const ProcessLargeDataParams& params, ProcessLargeDataResult& response);

    private:
        PluginHost::IShell* _service;
    };

} // namespace Plugin
} // namespace Thunder