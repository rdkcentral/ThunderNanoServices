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

#include "CommunicationPerformanceCOMRPCClientImplementation.h"

// Pull in 'Common' template definitions
#include "../Common/CommunicationPerformanceImplementation.cpp"

namespace Thunder {
namespace Plugin {

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
COMRPCClient<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::COMRPCClient(const std::string& remoteNode)
    : _remoteNode{ remoteNode.c_str() }
    , _messageHandler{}
    , _communicatorClient{}
    , _lock{}
    , _performanceImplementation{ nullptr }
    , _referenceCount{ 0 }
{}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
COMRPCClient<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::~COMRPCClient()
{
    ASSERT(_referenceCount == 0);
    /* uint32_t result = */ Stop(Core::infinite);
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
uint32_t COMRPCClient<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Start(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (_communicatorClient.IsValid() != true) {
        _messageHandler = Core::ProxyType<RPC::InvokeServerType<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>>::Create();

        if (_messageHandler.IsValid() != false) {
            _communicatorClient = Core::ProxyType<RPC::CommunicatorClient>::Create(_remoteNode, Core::ProxyType<Core::IIPCServer>(_messageHandler));
        }
    }

    if (_communicatorClient.IsValid() != false) {
        result = Open(waitTime);
    }

    return result;
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
uint32_t COMRPCClient<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Stop(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    /* uint32_t */ Revoke(waitTime);

    ASSERT(_performanceImplementation == nullptr);

    if (   _communicatorClient.IsValid() != false
        && (result = Close(waitTime)) == Core::ERROR_NONE
       ) {
        result = _communicatorClient.Release();
        ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
    } 

    if (   _communicatorClient.IsValid() != true
        && _messageHandler.IsValid() != false
       ) {
        result = _messageHandler.Release();
        ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
    }

    return result;
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
Performance<COMRPCClient<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>&>* COMRPCClient<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Acquire(uint32_t waitTime)
{
    _lock.Lock();

    ASSERT(_performanceImplementation != nullptr);

    _lock.Unlock();

    return _performanceImplementation;
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
uint32_t COMRPCClient<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Offer(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    // Replace any exisitng interface
    /* uint32_t */ Revoke(waitTime);

    _lock.Lock();

    ASSERT(_performanceImplementation == nullptr);

    // Implicit 2x AddRef (?)
    _performanceImplementation = Core::ServiceType< Performance< COMRPCClient< THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS >& > >:: template Create< Performance< COMRPCClient< THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>& > >();

    if (   _performanceImplementation != nullptr
        && _communicatorClient.IsValid() != false
        ) {
        constexpr uint32_t version = static_cast<uint32_t>(~0);
        // Implicit AddRef
        result = _communicatorClient->Offer<Exchange::IPerformance>(_performanceImplementation, version, waitTime);
    }

    _lock.Unlock();

    return result;
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
uint32_t COMRPCClient<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Revoke(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    _lock.Lock();

    if (   _communicatorClient.IsValid() != false
        && _performanceImplementation != nullptr
    ) {
        constexpr uint32_t version = static_cast<uint32_t>(~0);

        // No implicit Release
        result = _communicatorClient->Revoke<Exchange::IPerformance>(_performanceImplementation, version, waitTime);
        ASSERT(result == Core::ERROR_NONE);

        // The missing Release
        result = _performanceImplementation->Release();
        ASSERT((result == Core::ERROR_NONE) || (result == Core::ERROR_DESTRUCTION_SUCCEEDED));
    }

    // Release for the implicit AddRef of Create()
    if (_performanceImplementation != nullptr
        && (    result == Core::ERROR_DESTRUCTION_SUCCEEDED
            || (result = _performanceImplementation->Release()) == Core::ERROR_DESTRUCTION_SUCCEEDED
           )
       ) {
        _performanceImplementation = nullptr;
    }

    ASSERT(_performanceImplementation == nullptr);

    _lock.Unlock();

    return result;
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
uint32_t COMRPCClient<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Open(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_NONE;

    if (_communicatorClient->IsOpen() != true) {
        if (   (result = _communicatorClient->Open(waitTime)) != Core::ERROR_NONE
            || _communicatorClient->IsOpen() != true
           ) {
            result = Core::ERROR_CONNECTION_CLOSED;
        }
    }

    return result;
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
uint32_t COMRPCClient<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Close(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_NONE;

    if (_communicatorClient->IsOpen() != false) {
        if (   (result = _communicatorClient->Close(waitTime)) != Core::ERROR_NONE
            && _communicatorClient->IsOpen() != false
           ) {
            result = Core::ERROR_CLOSING_FAILED;
        }
    }

    return result;
}


SimplePluginCOMRPCClientImplementation::SimplePluginCOMRPCClientImplementation()
#ifndef _USE_UNIX_DOMAIN_SOCKET_
    : _client{ _T("127.0.0.1:8080") /* remoteNode */ }
#else
    : _client{ _T("/tmp/CommunicationPerformanceCOMRPC") /* remoteNode */ }
#endif
{}

SimplePluginCOMRPCClientImplementation::~SimplePluginCOMRPCClientImplementation()
{
    /* uint32_t */ Stop(Core::infinite);
}

uint32_t SimplePluginCOMRPCClientImplementation::Start(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if ((result = _client.Start(waitTime)) != Core::ERROR_NONE) {
        /* uint32_t */ Stop(waitTime);
    } else {
        result = _client.Offer(waitTime);
    }

    return result;
}

uint32_t SimplePluginCOMRPCClientImplementation::Stop(uint32_t waitTime)
{
    uint32_t result = Core ::ERROR_GENERAL;

   if ((result = _client.Revoke(waitTime)) != Core::ERROR_NONE) {
        /* uint32_t */ _client.Stop(waitTime);
   } else {
        result = _client.Stop(waitTime);
   }

    return result;
}

uint32_t SimplePluginCOMRPCClientImplementation::Task(STATE& state, uint32_t& waitTime)
{
    uint32_t result = Core::ERROR_NONE;

PUSH_WARNING(DISABLE_WARNING_IMPLICIT_FALLTHROUGH);
    switch (state) {
    case STATE::IDLE    :   state = STATE::RUN;
    case STATE::RUN     :   state = STATE::EXECUTE;
    case STATE::EXECUTE :   state = STATE::STOP;
                            break;
    case STATE::ERROR   :   result = Core::ERROR_GENERAL;
    case STATE::STOP    :   waitTime = Core::infinite;
    default             :   ;
    }
POP_WARNING();

    return result;
}


// Inform Thunder this out-of-service (module) implements this service (class)
// Arguments are module (service) name, major minor and optional patch version
// Use after the service has been defined / declared

SERVICE_REGISTRATION(SimplePluginCOMRPCClientImplementation, 1, 0)

} } // namespace Thunder::Plugin
