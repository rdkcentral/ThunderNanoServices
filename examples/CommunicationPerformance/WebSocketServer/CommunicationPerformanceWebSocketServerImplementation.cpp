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

#include "CommunicationPerformanceWebSocketServerImplementation.h"

// Pull in 'Common' template definitions
#include "../Common/CommunicationPerformanceImplementation.cpp"

namespace Thunder {
namespace Plugin {

template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::WebSocketServer(const string& localNode)
    : _localNode{ static_cast<const TCHAR*>(localNode.c_str()) } // listening node
    , _server{ _localNode }
{}


template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::Server(const SOCKET& socket, const ::Thunder::Core::NodeId& remoteNode, VARIABLE_IS_NOT_USED Core::SocketServerType<Server>* socketServer /* our creator */)
    // A server should nevwer apply masking when sending frames to the client
    : Web::WebSocketServerType<STREAMTYPE>(false /* binary */, false /* masking */, /* <Arguments SocketStream> */ false /* use raw socket */, socket, remoteNode, SENDBUFFERSIZE, RECEIVEBUFFERSIZE /* </Arguments SocketStream> */)
{}

template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
bool WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::IsIdle() const
{
    _guard.Lock();

    bool result = _post.size() == 0;

    _guard.Unlock();

    return result;
}

template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
void WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::StateChange()
{}

template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint16_t WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
{
// TODO: probably the location to use Exchange
    size_t count = 0;

    if (   dataFrame != nullptr
        && maxSendSize > 0
        && !IsIdle()
        && Web::WebSocketServerType<STREAMTYPE>::IsOpen()
        && Web::WebSocketServerType<STREAMTYPE>::IsWebSocket() // Redundant, covered by IsOpen
        && Web::WebSocketServerType<STREAMTYPE>::IsCompleted()
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
            Web::WebSocketServerType<STREAMTYPE>::Link().Trigger();
        }

        _guard.Unlock();
    }

    return count;
}

template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint16_t WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
{
// TODO: probably the location to use Exchange
    if (receivedSize > 0) {
        _guard.Lock();

        _response.emplace_back(std::basic_string<uint8_t>{ dataFrame, receivedSize });

        _guard.Unlock();
    }

    return receivedSize;
}
#ifdef _0
template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
bool WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::Submit(const std::basic_string<uint8_t>& message)
{
    _guard.Lock();

    size_t count = _post.size();

    _post.emplace_back(message);

    bool result =  count < _post.size();

    _guard.Unlock();

    // Trigger a call to SendData
    Web::WebSocketServerType<STREAMTYPE>::Link().Trigger();

    return result;
}

template <size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
std::basic_string<uint8_t> WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::Response()
{
    std::basic_string<uint8_t> message;

    _guard.Lock();

    if (_response.size() > 0) {
        message = _response.front();
        _response.erase(_response.begin());
    } 

    _guard.Unlock();

    return message;
}
#endif


SimplePluginWebSocketServerImplementation::SimplePluginWebSocketServerImplementation()
    : _server{ _T("127.0.0.1:8081") /* listening node */ }
{}

uint32_t SimplePluginWebSocketServerImplementation::Start(VARIABLE_IS_NOT_USED uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;
    return result;
}

 uint32_t SimplePluginWebSocketServerImplementation::Stop(VARIABLE_IS_NOT_USED uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;
    return result;
}

uint32_t SimplePluginWebSocketServerImplementation::Task(VARIABLE_IS_NOT_USED STATE& state, VARIABLE_IS_NOT_USED uint32_t& waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;
    return result;
}  


// Inform Thunder this out-of-service (module) implements this service (class)
// Arguments are module (service) name, major minor and optional patch version
// Use after the service has been defined / declared

SERVICE_REGISTRATION(SimplePluginWebSocketServerImplementation, 1, 0)

} } // namespace Thunder::Plugin
