/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include <com/com.h>

// Common (shared)
#include "../Common/CommunicationPerformanceImplementation.h"

#define _USE_UNIX_DOMAIN_SOCKET_

namespace Thunder {
namespace Plugin {

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
class COMRPCClient {
public:
    COMRPCClient() = delete;
    COMRPCClient(const COMRPCClient&) = delete;
    COMRPCClient(COMRPCClient&&) = delete;

    COMRPCClient& operator=(const COMRPCClient&) = delete;
    COMRPCClient& operator=(COMRPCClient&&) = delete;

    COMRPCClient(const std::string& remoteNode);

    ~COMRPCClient(); // no override !
    
    uint32_t Start(uint32_t waitTime);

    uint32_t Stop(uint32_t waitTime);

    // This implementation is asked for a handle to the requested interface
    Performance< COMRPCClient< THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS >& >* Acquire(uint32_t waitTime);

    // RPC::CommunicatorClient methods
    // -------------------------------

    // Offer the remote our implementation of the interface, e.g., inform a server this is implementing the given interface
    uint32_t Offer(uint32_t waitTime);

    // Revoke the available (earlier) offered interface
    uint32_t Revoke(uint32_t waitTime);

private:

    Core::NodeId _remoteNode;

    Core::ProxyType< RPC::InvokeServerType<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS> > _messageHandler;

    Core::ProxyType< RPC::CommunicatorClient> _communicatorClient;

    mutable Core::CriticalSection _lock;

    Performance<COMRPCClient<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>&>* _performanceImplementation;

    mutable std::atomic<uint32_t> _referenceCount;

    uint32_t Open(uint32_t waitTime);

    uint32_t Close(uint32_t waitTime);
};

class SimplePluginCOMRPCClientImplementation : public SimplePluginImplementation<SimplePluginCOMRPCClientImplementation> {
public :
    SimplePluginCOMRPCClientImplementation(const SimplePluginCOMRPCClientImplementation&) = delete;
    SimplePluginCOMRPCClientImplementation(SimplePluginCOMRPCClientImplementation&&) = delete;

    SimplePluginCOMRPCClientImplementation& operator=(const SimplePluginCOMRPCClientImplementation&) = delete;
    SimplePluginCOMRPCClientImplementation& operator=(SimplePluginCOMRPCClientImplementation&&) = delete;

    // SERVICE_REGISTRATION requires a constructor that takes no arguments
    SimplePluginCOMRPCClientImplementation();

    ~SimplePluginCOMRPCClientImplementation();

    uint32_t Start(uint32_t waitTime);

    uint32_t Stop(uint32_t waitTime);

private :
// TODO:
    // Some arbitrary values
    COMRPCClient<1, 0, 4> _client;

    friend SimplePluginImplementation;
    uint32_t Task(STATE& state, uint32_t& waitTime);
};

} } // namespace Thunder::Plugin
