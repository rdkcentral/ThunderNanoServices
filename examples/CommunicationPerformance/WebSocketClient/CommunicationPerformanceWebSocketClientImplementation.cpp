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

#include "CommunicationPerformanceWebSocketClientImplementation.h"

// Pull in 'Common' template definitions
#include "../Common/CommunicationPerformanceImplementation.cpp"

namespace Thunder {
namespace Plugin {

template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::WebSocketClient(  const string& remoteNode
                                                                                , VARIABLE_IS_NOT_USED const string& uriPath     // HTTP URI part, empty path allowed
                                                                                , VARIABLE_IS_NOT_USED const string& protocol    // Optional HTTP field, WebSocket sub-protocol, ie, Sec-WebSocket-Protocol
                                                                                , VARIABLE_IS_NOT_USED const string& uriQuery    // HTTP URI part, absent query allowed
                                                                                , VARIABLE_IS_NOT_USED const string& origin      // Optional, set by browser client
)
    : _remoteNode{ static_cast<const TCHAR*>(remoteNode.c_str()) }
    // The minimum send and receive buffer sizes are determined by the HTTP upgrade process. The limit here is above that threshold.
    , _client{ uriPath, protocol, uriQuery, origin, false /* use raw socket */ , _remoteNode.AnyInterface(), _remoteNode, SENDBUFFERSIZE /* send buffer size */, RECEIVEBUFFERSIZE /* receive buffer size */ }
{}


template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
template <typename... Args>
WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::Client( const string& path
                                                                                     , const string& protocol
                                                                                     , const string& query
                                                                                     , const string& origin
                                                                                     , Args&&... args
)
    // A client should always apply masking when sending frames to the server
    : Web::WebSocketClientType<STREAMTYPE>(path, protocol, query, origin, false /* binary */, true /* masking */, /* <Arguments SocketStream> */ std::forward<Args>(args)... /* </Arguments SocketStream> */)
{
}


template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
bool WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::IsIdle() const
{
    _guard.Lock();

    bool result = _post.size() == 0;

    _guard.Unlock();

    return result;
}

template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
void WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::StateChange()
{}

template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint16_t WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
{
    size_t count = 0;

    if (   dataFrame != nullptr
        && maxSendSize > 0
        && !IsIdle()
        && Web::WebSocketClientType<STREAMTYPE>::IsOpen()
        && Web::WebSocketClientType<STREAMTYPE>::IsWebSocket() // Redundant, covered by IsOpen
        && Web::WebSocketClientType<STREAMTYPE>::IsCompleted()
       ) {
        _guard.Lock();

        std::basic_string<uint8_t>& message = _post.front();

        count = std::min(message.size(), static_cast<size_t>(maxSendSize));

        /* void* */ memcpy(dataFrame, message.data(), count);

        if (count == message.size()) {
            /* iterator */ _post.erase(_post.begin());
        } else {
            /* this */ message.erase(0, count);

            // Trigger a call to SendData for remaining data
            Web::WebSocketClientType<STREAMTYPE>::Link().Trigger();
        }

        _guard.Unlock();
    }

    return count;
}

template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint16_t WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
{
    // Echo the data in reverse order
// TODO: probably the location to use Exchange
    std::basic_string<uint8_t> message{ dataFrame, receivedSize };

    std::reverse(message.begin(), message.end());

    return Submit(message) ? message.size() : 0;
}

template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
bool WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::Submit(const std::basic_string<uint8_t>& message)
{
// TODO: probably the location to use Exchange
    _guard.Lock();

    size_t count = _post.size();

    _post.emplace_back(message);

    bool result = count < _post.size();

    _guard.Unlock();

    Web::WebSocketClientType<STREAMTYPE>::Link().Trigger();

    return result;
}


SimplePluginWebSocketClientImplementation::SimplePluginWebSocketClientImplementation()
// TODO: add node string here
    : _client { _T("127.0.0.1:8081") /* remote node */, "", "", "", "" }
{}

uint32_t SimplePluginWebSocketClientImplementation::Start(VARIABLE_IS_NOT_USED uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;
    return result;
}

 uint32_t SimplePluginWebSocketClientImplementation::Stop(VARIABLE_IS_NOT_USED uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;
    return result;
}

uint32_t SimplePluginWebSocketClientImplementation::Task(VARIABLE_IS_NOT_USED STATE& state, VARIABLE_IS_NOT_USED uint32_t& waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;
    return result;
}


// Inform Thunder this out-of-service (module) implements this service (class)
// Arguments are module (service) name, major minor and optional patch version
// Use after the service has been defined / declared

SERVICE_REGISTRATION(SimplePluginWebSocketClientImplementation, 1, 0)

} } // namespace Thunder::Plugin
