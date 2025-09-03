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

// Test data for Histogram2D
//#define _USE_TESTDATA
#ifdef _USE_TESTDATA
#include "TestData.h"
#endif

#define _USE_UNIX_DOMAIN_SOCKET_
//#ifdef _USE_CHRONO_HIGH_RESOLUTION_CLOCK_

namespace Thunder {
namespace Plugin {

// A single server to serve one to N clients
template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
class COMRPCServer {
public :
    COMRPCServer() = delete;
    COMRPCServer(const COMRPCServer&) = delete;
    COMRPCServer(COMRPCServer&&) = delete;

    COMRPCServer& operator=(const COMRPCServer&) = delete;
    COMRPCServer& operator=(COMRPCServer&&) = delete;

    COMRPCServer(const std::string& remoteNode, const string& proxyStubPath, const std::string& sourceName);

    ~COMRPCServer();

    uint32_t Start(uint32_t waitTime);

    uint32_t Stop(uint32_t waitTime);

    uint32_t Exchange(uint8_t buffer[], uint16_t bufferSize, uint16_t bufferMaxSize, uint64_t& duration) const;

private :

    class Communicator : public RPC::Communicator {
    public :
        Communicator() = delete;
        Communicator(const Communicator&) = delete;
        Communicator(Communicator&&) = delete;

        Communicator& operator=(const Communicator&) = delete;
        Communicator& operator=(Communicator&&) = delete;

        Communicator(const Core::NodeId& remoteNode, const std::string& proxyStubPath, const Core::ProxyType<Core::IIPCServer>& messageHandler, const TCHAR* sourceName);

        ~Communicator();

        void Lock() const;

        void Unlock() const;

        // RPC::Communicator methods
        // -------------------------

        // This implementation is asked for a handle to the requested interface
        void* Acquire(const string& className, const uint32_t interfaceId, const uint32_t version) override;

    private:

        friend uint32_t COMRPCServer::Stop(uint32_t);

        mutable Core::CriticalSection _lock;

        Exchange::IPerformance* _performanceInterface;

        // The remote offered an interface
        void Offer(Core::IUnknown* object, const uint32_t interfaceId) override;

        // The remote revoked an interface
        void Revoke(const Core::IUnknown* object, const uint32_t interfaceId) override;

        // The remote probably forgot to revoke the interface 
        void Dangling(RPC::Communicator::Danglings&& proxies) override;
    };

    Core::NodeId _remoteNode;

    const std::string _proxyStubPath;
    const std::string _sourceName;

    Core::ProxyType< RPC::InvokeServerType<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS> > _messageHandler;

    Core::ProxyType<Communicator> _communicator;

    mutable Core::CriticalSection _lock;

    uint32_t Open(uint32_t waitTime);

    uint32_t Close(uint32_t waitTime);
};

class SimplePluginCOMRPCServerImplementation : public SimplePluginImplementation<SimplePluginCOMRPCServerImplementation> {
public :
    SimplePluginCOMRPCServerImplementation(const SimplePluginCOMRPCServerImplementation&) = delete;
    SimplePluginCOMRPCServerImplementation(SimplePluginCOMRPCServerImplementation&&) = delete;

    SimplePluginCOMRPCServerImplementation& operator=(const SimplePluginCOMRPCServerImplementation&) = delete;
    SimplePluginCOMRPCServerImplementation& operator=(SimplePluginCOMRPCServerImplementation&&) = delete;

    // SERVICE_REGISTRATION requires a constructor that takes no arguments
    SimplePluginCOMRPCServerImplementation();

    ~SimplePluginCOMRPCServerImplementation();

    uint32_t Start(uint32_t waitTime);

    uint32_t Stop(uint32_t waitTime);

private :
// TODO:
    // Some arbitrary values
    COMRPCServer<1, 0, 4> _server;

#ifdef _USE_TESTDATA
    // Select series 1, 2, 3, 4 or 5
    Plugin::CommunicationPerformanceHelpers::TestData<1> _data;
#endif

    friend SimplePluginImplementation;
    uint32_t Task(STATE& state, uint32_t& waitTime);
};

} } // namespace Thunder::Plugin
