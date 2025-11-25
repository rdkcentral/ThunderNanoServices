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

#include <algorithm>

#ifdef _USE_CHRONO_HIGH_RESOLUTION_CLOCK_
#include <chrono>
#endif

#include <random>

namespace Thunder {
namespace Plugin {

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::WebSocketServer(const string& localNode)
    : _localNode{ static_cast<const TCHAR*>(localNode.c_str()) } // listening node
    , _server{ _localNode }
{}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::~WebSocketServer()
{
    /* uint32_t result = */ Stop(Core::infinite);
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint32_t WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Start(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    result = Open(waitTime); 

    return result;
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint32_t WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Stop(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    result = Close(waitTime);

    return result;
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint32_t WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Exchange(uint8_t buffer[], const uint16_t bufferSize, const uint16_t bufferMaxSize, uint64_t& duration)
{
    ASSERT(bufferSize < bufferMaxSize);

    uint32_t result = Core::ERROR_GENERAL;

    std::basic_string<uint8_t> message{ buffer, bufferSize };

    uint32_t waitTime = std::min(static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()), duration); 

#ifndef _USE_CHRONO_HIGH_RESOLUTION_CLOCK_
    Core::StopWatch timer;

    /* uint64_t */ timer.Reset();
#else
        auto start = std::chrono::high_resolution_clock::now();
#endif

    // Obtain the endpoint at the server side for each (remotely) connected client, eg, an object of Class Server
    auto it = _server.Clients();

    // Assume a single Client
    if (it.Next() != false) {
        // Unless a client has sent an upgrade request, and, the request completed, no data can be send out

        // It should have been upgraded to a websocket
        if (   it.Client()->IsWebSocket() != false
        // This is the reversed order compared to the client side
          && it.Client()->Submit(message, waitTime) != false
          && it.Client()->Response(message, waitTime) != false
        ) {
          ASSERT(bufferSize == message.size());

          result = Core::ERROR_NONE;
        }
    }

#ifndef _USE_CHRONO_HIGH_RESOLUTION_CLOCK_
    duration = timer.Elapsed();
#else
        auto end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif

    using common_t = std::common_type<std::basic_string<uint8_t>::size_type, uint16_t>::type;

    /* void* */ memcpy(buffer, message.c_str(), std::min(static_cast<common_t>(message.size()), static_cast<common_t>(bufferMaxSize)));

    return result;
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint32_t WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Open(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    // Safe to call repeatedly
    result = _server.Open(waitTime);

    return result;
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint32_t WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Close(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;
   
    // Safe to call repeatedly
    result = _server.Close(waitTime);

    return result;
}


template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::Server(const SOCKET& socket, const ::Thunder::Core::NodeId& remoteNode, VARIABLE_IS_NOT_USED Core::SocketServerType<Server>* socketServer /* our creator */)
    // A server should nevwer apply masking when sending frames to the client
    : Web::WebSocketServerType<STREAMTYPE>(false /* binary */, false /* masking */, /* <Arguments SocketStream> */ false /* use raw socket */, socket, remoteNode, SENDBUFFERSIZE, RECEIVEBUFFERSIZE /* </Arguments SocketStream> */)
    , _post{}
    , _response{}
    , _guardPost{}
    , _guardResponse{}
    , _messageAvailable{ false, false }
{}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::~Server()
{
    /* uint32_t */ _messageAvailable.Unlock();
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
bool WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::Submit(const std::basic_string<uint8_t>& message, VARIABLE_IS_NOT_USED const uint32_t waitTime)
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

    if (   result != false
        && this->IsOpen() != false
       ) {
        // Trigger a call to SendData to get the message from the queue to send
        Web::WebSocketServerType<STREAMTYPE>::Link().Trigger();
     }

    return result;
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
bool WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::Response(std::basic_string<uint8_t>& message, const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_NONE;

    message.clear();

    do {
        _guardResponse.Lock();

        if (_response.size() > 1) {
            _messageAvailable.Unlock();
        } 

        _guardResponse.Unlock();
    } while (   // New messages will be accepted so engage in / enable another 'run' and expect data to be received 
                this->IsSuspended() != true
                // A connection is open, and, it is still WebSocket type
             && this->IsOpen() != false
             && _messageAvailable.IsSet() != true
             && (result = _messageAvailable.Lock(waitTime)) != Core::ERROR_GENERAL
            );

    _guardResponse.Lock();

    if (_response.size() > 1) {
        message = _response.front();
        _response.erase(_response.begin());
    } 

    _guardResponse.Unlock();

    _messageAvailable.ResetEvent();

    return (message.size() > 0);
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
bool WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::IsIdle() const
{
    _guardPost.Lock();

    bool result = _post.size() == 0;

    _guardPost.Unlock();

    return result;
}

template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
void WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::StateChange()
{
// TODO: transition IsOpen true to false, and, IsSupended false to true

    if (   this->IsOpen() != true 
        && this->IsSuspended() != false
       ) {
        /* uint32_t */ _messageAvailable.Unlock();
    }
}

// Reflects payload, effective after upgrade
template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint16_t WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
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
            Web::WebSocketServerType<STREAMTYPE>::Link().Trigger();
        }

        _guardPost.Unlock();
    }

    return static_cast<uint16_t>(count);
}

// Reflects payload, effective after upgrade
template <const size_t SENDBUFFERSIZE, const size_t RECEIVEBUFFERSIZE, typename STREAMTYPE>
uint16_t WebSocketServer<SENDBUFFERSIZE, RECEIVEBUFFERSIZE, STREAMTYPE>::Server::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
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

            // Create the next entry
            _response.emplace_back(static_cast<const uint8_t*>(nullptr), 0);

            _guardResponse.Unlock();

            /* uint32_t */ _messageAvailable.Unlock();
        }
    }
 
    // Probably not used by the caller
    return count;
}


SimplePluginWebSocketServerImplementation::SimplePluginWebSocketServerImplementation()
    : _server{ _T("127.0.0.1:8081") /* listening node */ }
{}

SimplePluginWebSocketServerImplementation::~SimplePluginWebSocketServerImplementation()
{
    /* uint32_t */ Stop(Core::infinite);
}

uint32_t SimplePluginWebSocketServerImplementation::Start(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    result = _server.Start(waitTime);

    return result;
}

 uint32_t SimplePluginWebSocketServerImplementation::Stop(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    result = _server.Stop(waitTime);

    return result;
}

uint32_t SimplePluginWebSocketServerImplementation::Task(STATE& state, uint32_t& waitTime)
{
    uint32_t result = Core::ERROR_NONE;

PUSH_WARNING(DISABLE_WARNING_IMPLICIT_FALLTHROUGH);
    switch (state) {
    case STATE::IDLE    :   state = STATE::RUN;
    case STATE::RUN     :   state = STATE::EXECUTE;
    case STATE::EXECUTE :   {
                            // Set to a low value for quick builds
                            constexpr uint16_t bufferMaxSize = CommunicationPerformanceHelpers::TemplateRecursionDepth;

                            constexpr size_t numberOfBins = CommunicationPerformanceHelpers::NumberOfBins;

                            std::array<uint8_t, bufferMaxSize> buffer = CommunicationPerformanceHelpers::ConstexprArray<uint8_t, bufferMaxSize>::values;

                            constexpr uint64_t upperBoundDuration = CommunicationPerformanceHelpers::UpperBoundDuration;
                            constexpr uint64_t lowerBoundDuration = CommunicationPerformanceHelpers::LowerBoundDuration;
                            constexpr uint16_t bufferMinSize = 0;

                            // Round trip time in ticks, initialize with value for maximum wait
                            // Educated guess
                            uint64_t duration = 2000;

                            // Add some randomness

                            std::random_device rd; // Generate values between 0 and std:numeric_limits<unsigned int>::max()

                            using common_t = std::common_type<unsigned int, uint16_t>::type;

                            static_assert(static_cast<common_t>(std::numeric_limits<unsigned int>::max()) >= static_cast<common_t>(bufferMaxSize), "");

                            const uint16_t bufferSize = static_cast<uint16_t>(static_cast<common_t>(rd()) % static_cast<common_t>(bufferMaxSize));

                            // With no mistakes this always holds 
                            ASSERT(bufferSize <= bufferMaxSize);

                            // This fails if the client is missing so it is not typically an error
                            if (_server.Exchange(buffer.data(), bufferSize, bufferMaxSize, duration) == Core::ERROR_NONE) {
                                using measurements_t = Measurements<uint16_t, uint64_t>;
                                using distribution_t = measurements_t::Histogram2D<numberOfBins, bufferMaxSize, upperBoundDuration, bufferMinSize, lowerBoundDuration>;

                                distribution_t& measurements = measurements_t::Instance<distribution_t>();

                                // Do not continue if values cannot be inserted without error (remainder too large, numerical instability)
                                if (/* data corruption
                                    || */ measurements.Insert(std::pair<uint16_t, uint64_t>(bufferSize, duration)) != true
                                ) {
                                    state = STATE::STOP;
                                }
                            }
                            }
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

SERVICE_REGISTRATION(SimplePluginWebSocketServerImplementation, 1, 0)

} } // namespace Thunder::Plugin
