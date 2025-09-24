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
class WebSocketClient {
public :
    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient(WebSocketClient&&) = delete;

    WebSocketClient& operator=(const WebSocketClient&) = delete;
    WebSocketClient& operator=(WebSocketClient&&) = delete;

    WebSocketClient(  const string& remoteNode
                    , const string& uriPath     // HTTP URI part, empty path allowed
                    , const string& protocol    // Optional HTTP field, WebSocket sub-protocol, ie, Sec-WebSocket-Protocol
                    , const string& uriQuery    // HTTP URI part, absent query allowed
                    , const string& origin      // Optional, set by browser client
    );
    ~WebSocketClient();

    uint32_t Start(uint32_t waitTime);
    uint32_t Stop(uint32_t waitTime);

    uint32_t Exchange(uint8_t buffer[], uint16_t bufferSize, uint16_t bufferMaxSize, uint64_t& duration);

private :

    class Client : public Web::WebSocketClientType<STREAMTYPE> {
    public :
        Client(const Client&) = delete;
        Client(Client&&) = delete;

        Client operator=(const Client&) = delete;
        Client operator=(Client&&) = delete;

        template <typename... Args>
        Client( const string& path
              , const string& protocol
              , const string& query
              , const string& origin
              , Args&&... args
        );
        ~Client() override;

        bool Submit(const std::basic_string<uint8_t>& message, uint32_t waitTime = Core::infinite);
        bool Response(std::basic_string<uint8_t>& message, uint32_t waitTime = Core::infinite);


        // Web::WebSocketClientType methods
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

        std::vector<std::basic_string<uint8_t>> _post; // Send message queue
        std::vector<std::basic_string<uint8_t>> _response; // Receive message queue

        mutable Core::CriticalSection _guardPost;
        mutable Core::CriticalSection _guardResponse;
    };
 
    const Core::NodeId _remoteNode;

    Client _client;

    uint32_t Open(uint32_t waitTime);
    uint32_t Close(uint32_t waitTime);
};

class SimplePluginWebSocketClientImplementation : public SimplePluginImplementation<SimplePluginWebSocketClientImplementation> {
public :
    SimplePluginWebSocketClientImplementation(const SimplePluginWebSocketClientImplementation&) = delete;
    SimplePluginWebSocketClientImplementation(SimplePluginWebSocketClientImplementation&&) = delete;

    SimplePluginWebSocketClientImplementation& operator=(const SimplePluginWebSocketClientImplementation&) = delete;
    SimplePluginWebSocketClientImplementation& operator=(SimplePluginWebSocketClientImplementation&&) = delete;

    SimplePluginWebSocketClientImplementation();
    ~SimplePluginWebSocketClientImplementation();

    uint32_t Start(uint32_t waitTime);
    uint32_t Stop(uint32_t waitTime);

private :
    WebSocketClient<1024, 1024, Core::SocketStream> _client;

    friend SimplePluginImplementation;
    uint32_t Task(STATE& state, uint32_t& waitTime);
};

} } // namespace Thunder::Plugin
