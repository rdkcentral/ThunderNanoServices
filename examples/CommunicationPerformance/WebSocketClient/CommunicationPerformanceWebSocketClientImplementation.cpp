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

#include <algorithm>

namespace Thunder {
namespace Plugin {

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::WebSocketClient(  const string& remoteNode
                                                                                , const string& uriPath     // HTTP URI part, empty path allowed
                                                                                , const string& protocol    // Optional HTTP field, WebSocket sub-protocol, ie, Sec-WebSocket-Protocol
                                                                                , const string& uriQuery    // HTTP URI part, absent query allowed
                                                                                , const string& origin      // Optional, set by browser client
)
    : _remoteNode{ static_cast<const TCHAR*>(remoteNode.c_str()) }
    // The minimum send and receive buffer sizes are determined by the HTTP upgrade process. The limit here is above that threshold.
    , _client{ uriPath, protocol, uriQuery, origin, false /* use raw socket */ , _remoteNode.AnyInterface(), _remoteNode, SENDBUFFERSIZE /* send buffer size */, RECEIVEBUFFERSIZE /* receive buffer size */ }
{}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::~WebSocketClient()
{
    /* uint32_t result = */ Stop(Core::infinite);
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint32_t WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Start(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    result = Open(waitTime); 

    return result;
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint32_t WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Stop(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    result = Close(waitTime); 

    return result;
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint32_t WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Exchange(uint8_t buffer[], VARIABLE_IS_NOT_USED const uint16_t bufferSize, const uint16_t bufferMaxSize, uint64_t& duration)
{
    return Core::ERROR_NOT_SUPPORTED;
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint32_t WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Open(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (_client.IsOpen() != true) {
        result = _client.Open(waitTime);
    }

    return result;
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint32_t WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Close(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (_client.IsClosed() != true) {
        result = _client.Close(waitTime);
    }

    return result;
}


template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
template <typename... Args>
WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::Client( const string& path
                                                                                     , const string& protocol
                                                                                     , const string& query
                                                                                     , const string& origin
                                                                                     , Args&&... args
)
    // A client should always apply masking when sending frames to the server
    : Web::WebSocketClientType<STREAMTYPE>(path, protocol, query, origin, false /* binary */, true /* masking */, /* <Arguments SocketStream> */ std::forward<Args>(args)... /* </Arguments SocketStream> */)
    , _post{}
    , _response{}
    , _guardPost{}
    , _guardResponse{}
{}


template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::~Client()
{}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
bool WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::IsIdle() const
{
    _guardPost.Lock();

    bool result = _post.size() == 0;

    _guardPost.Unlock();

    return result;
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
void WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::StateChange()
{}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint16_t WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
{
    size_t count{ 0 };
    
    if (   dataFrame != nullptr
        && maxSendSize > 0
        // Is there anything available in the post queue?
        && !(this->IsIdle())
        && this->IsOpen()
        && this->IsWebSocket() // Redundant, covered by IsOpen
       ) {
        // Put the message of the _post queue into dataFrame
        // Update count with the message size

        _guardPost.Lock();

        std::basic_string<uint8_t>& message = _post.front();

        // size_t is mininum 16 bit integral since c++11
        static_assert(std::is_same<std::basic_string<uint8_t>::size_type, std::size_t>::value, "size_type is not aliasing std::size_t");

        // Do not exceed bounds
        count = std::min(static_cast<size_t>(maxSendSize), message.size());

        // memmove expects unsigned char
        static_assert(sizeof(unsigned char) == sizeof(uint8_t), "");

        /* void* */ memmove(dataFrame, message.data(), count);

        if (maxSendSize >= count) {
            // All fits
          
            _post.erase(_post.begin()); 

            // message has become invalid
        } else {
            // Only remove data that was send and keep the remaining portion to be send later

            message.erase(0, count);

            // Trigger a call to SendData to send the remainder
            Web::WebSocketClientType<STREAMTYPE>::Link().Trigger();
        }

        _guardPost.Unlock();
    }

    return static_cast<uint16_t>(count);
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint16_t WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
{
    uint16_t count{ receivedSize };

    if (   dataFrame != nullptr
        && this->IsOpen()
        && this->IsWebSocket() // Redundant, covered by IsOpen
       ) {
        _guardResponse.Lock();

        if (_response.empty() != false) {
            _response.emplace_back(static_cast<const uint8_t*>(nullptr), 0);
        }

        _response.back().append(dataFrame, count);

        _guardResponse.Unlock();

        if (this->IsCompleteMessage() != false) {
            // Entire message has been received
            // Append data to message in queue

            _guardResponse.Lock();

            /* bool */ Submit(_response.back());

            // Create the next entry
            _response.emplace_back(static_cast<const uint8_t*>(nullptr), 0);

            _guardResponse.Unlock();
        }
    }

    // Probably not used by the caller
    return count;
}


template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
bool WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::Submit(const std::basic_string<uint8_t>& message, VARIABLE_IS_NOT_USED const uint32_t waitTime)
{
    _guardPost.Lock();

    // Put data in the queue for sending

    size_t count = _post.size();

    if (message.size() > 0) {
        _post.emplace_back(message);
    }

    bool result =    message.size()
                  && count < _post.size()
                  ;

    _guardPost.Unlock();

    if (result != false) {
        // Trigger a call to SendData to get the message from the queue to send
        Web::WebSocketClientType<STREAMTYPE>::Link().Trigger();
    }

    return result;
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
bool WebSocketClient<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Client::Response(std::basic_string<uint8_t>& message, VARIABLE_IS_NOT_USED const uint32_t waitTime)
{
// TODO : Wait for complete message to be available, but do not block the running thread
    uint32_t result = Core::ERROR_NONE;

    message.clear();

    _guardResponse.Lock();

    if (_response.size() > 1) {
        message = _response.front();
        _response.erase(_response.begin());
    } 

    _guardResponse.Unlock();

    return (message.size() > 0);
}


SimplePluginWebSocketClientImplementation::SimplePluginWebSocketClientImplementation()
// TODO: add node string here
    : _client { _T("127.0.0.1:8081") /* remote node */, "", "", "", "" }
{}


SimplePluginWebSocketClientImplementation::~SimplePluginWebSocketClientImplementation()
{
    /* uint32_t */ Stop(Core::infinite);
}

uint32_t SimplePluginWebSocketClientImplementation::Start(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    result = _client.Start(waitTime);

    return result;
}

 uint32_t SimplePluginWebSocketClientImplementation::Stop(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    result = _client.Stop(waitTime);

    return result;
}

uint32_t SimplePluginWebSocketClientImplementation::Task(STATE& state, uint32_t& waitTime)
{
    uint32_t result = Core::ERROR_NONE;

PUSH_WARNING(DISABLE_WARNING_IMPLICIT_FALLTHROUGH);
    switch (state) {
    case STATE::IDLE    :   state = STATE::RUN;
    case STATE::RUN     :   state = STATE::EXECUTE;
    case STATE::EXECUTE :   state = STATE::STOP;
                            waitTime = Core::infinite;
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

SERVICE_REGISTRATION(SimplePluginWebSocketClientImplementation, 1, 0)

} } // namespace Thunder::Plugin
