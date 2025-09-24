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

// Common (shared)
#include "../Common/CommunicationPerformanceImplementation.h"

namespace Thunder {
namespace Plugin {

// TODO: buffer size is uint16_t
template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE = Core::SocketStream>
class WebSocketServer {
public :
    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer(WebSocketServer&&) = delete;

    WebSocketServer& operator=(const WebSocketServer&) = delete;
    WebSocketServer& operator=(WebSocketServer&&) = delete;

    WebSocketServer(const string& localNode);
    ~WebSocketServer();

    uint32_t Start(uint32_t waitTime);
    uint32_t Stop(uint32_t waitTime);

    uint32_t Exchange(uint8_t buffer[], uint16_t bufferSize, uint16_t bufferMaxSize, uint64_t& duration); 

private :

    class Server : public Web::WebSocketServerType<STREAMTYPE> {
    public :
        Server(const Server&) = delete;
        Server(Server&&) = delete;

        Server& operator=(const Server&) = delete;
        Server& operator=(Server&&) = delete;

        // The constructor expected by SocketServerType to exist
        Server(const SOCKET& socket, const ::Thunder::Core::NodeId& remoteNode, Core::SocketServerType<Server>* socketServer /* our creator */);
        ~Server() override;

        bool Submit(const std::basic_string<uint8_t>& message, uint32_t waitTime = Core::infinite);
        bool Response(std::basic_string<uint8_t>& message, uint32_t waitTime = Core::infinite);

        // Web::WebSocketServerType methods
        // --------------------------------

        // Non-idle then data available to send
        bool IsIdle() const override;

        // Allow for eventfull state updates in this class
        void StateChange() override;

        // Reflects payload, effective after upgrade
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override;

        // Reflects payload, effective after upgrade
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override;

    private:

// TODO : type uint8_t part of custom STREAMTYPE, eg, STREAMTYPE::underlying_type 
        std::vector<std::basic_string<uint8_t>> _post; // Send message queue
        std::vector<std::basic_string<uint8_t>> _response; // Receive message queue

        mutable Core::CriticalSection _guardPost;
        mutable Core::CriticalSection _guardResponse;

        Core::Event _messageAvailable;
    };

// TODO: currently not used
    bool _enableBinary;
    bool _enableMasking;

    const Core::NodeId _localNode;

    // Enable listening with SocketServerType
    Core::SocketServerType<Server> _server;

    uint32_t Open(uint32_t waitTime);
    uint32_t Close(uint32_t waitTime);
};

class SimplePluginWebSocketServerImplementation : public SimplePluginImplementation<SimplePluginWebSocketServerImplementation> {
public :
    SimplePluginWebSocketServerImplementation(const SimplePluginWebSocketServerImplementation&) = delete;
    SimplePluginWebSocketServerImplementation(SimplePluginWebSocketServerImplementation&&) = delete;

    SimplePluginWebSocketServerImplementation& operator=(const SimplePluginWebSocketServerImplementation&) = delete;
    SimplePluginWebSocketServerImplementation& operator=(SimplePluginWebSocketServerImplementation&&) = delete;

    SimplePluginWebSocketServerImplementation();
    ~SimplePluginWebSocketServerImplementation();

    uint32_t Start(uint32_t waitTime);
    uint32_t Stop(uint32_t waitTime);

private :
    WebSocketServer<1024, 1024, Core::SocketStream> _server;

    friend SimplePluginImplementation;
    uint32_t Task(STATE& state, uint32_t& waitTime);
};

} } // namespace Thunder::Plugin
