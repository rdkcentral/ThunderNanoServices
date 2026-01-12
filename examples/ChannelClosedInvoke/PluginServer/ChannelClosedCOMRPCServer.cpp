/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "ChannelClosedCOMRPCServer.h"


namespace Thunder {

namespace Plugin {

    namespace {

        static Metadata<ChannelClosedCOMRPCServer> metadata(
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


    void* ChannelClosedCOMRPCServer::COMServer::Acquire(const string& className VARIABLE_IS_NOT_USED, const uint32_t interfaceId, const uint32_t versionId) 
    {
        void* result = nullptr;

        // Currently we only support version 1 of the ITestChannelClosed
        if ((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) {

            if (interfaceId == Exchange::ITestChannelClosed::ID) {

                // Allright, request a new object that implements the requested interface.
                result = Core::ServiceType<ChannelClosedCOMRPCServer::TestChannelClosedService>::Create<Exchange::ITestChannelClosed>();
            } else if (interfaceId == Core::IUnknown::ID) {

                // Allright, request a new object that implements the requested interface.
                result = Core::ServiceType<ChannelClosedCOMRPCServer::TestChannelClosedService>::Create<Core::IUnknown>();
            }
        }
        return (result);
    }

    // IPlugin
    const string ChannelClosedCOMRPCServer::Initialize(PluginHost::IShell* service) /* override */
    {
        ASSERT(service != nullptr);
        _comserver = Core::ProxyType<COMServer>::Create(Core::NodeId("127.0.0.1:62010"), service->ProxyStubPath());
        return (EMPTY_STRING);
    }

    void ChannelClosedCOMRPCServer::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED) /* override */
    {
        ASSERT(service != nullptr);
        _comserver = std::move(Core::ProxyType<COMServer>());
    }

    string ChannelClosedCOMRPCServer::Information() const /* override */
    {
        return (EMPTY_STRING);
    }

}
} // namespace Plugin
