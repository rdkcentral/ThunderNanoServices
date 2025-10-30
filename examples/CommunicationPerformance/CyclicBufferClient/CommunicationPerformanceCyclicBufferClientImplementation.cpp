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

#include "CommunicationPerformanceCyclicBufferClientImplementation.h"

// Pull in 'Common' template definitions
#include "../Common/CommunicationPerformanceImplementation.cpp"

namespace Thunder {
namespace Plugin {

template <Core::File::Mode ACCESSMODE>
CyclicBufferClient<ACCESSMODE>::CyclicBufferClient(const std::string& fileName)
    : _fileName{ fileName }
    , _buffer{ _fileName, ACCESSMODE | Core::File::USER_WRITE | Core::File::SHAREABLE /* required for multiple processes to share data */, 0 /* ineffective,  server controls size */ , false /* ineffective, server controls flag */ }
    , _lock{}
    , _batonConnectingNode{ Core::NodeId{ std::string{ _fileName + "Baton", Core::NodeId::TYPE_DOMAIN }.c_str() } }
{}

template <Core::File::Mode ACCESSMODE>
CyclicBufferClient<ACCESSMODE>::~CyclicBufferClient()
{
    /* uint32_t result = */ Stop(Core::infinite);
}

template <Core::File::Mode ACCESSMODE>
uint32_t CyclicBufferClient<ACCESSMODE>::Start(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    result = Open(waitTime);

    return result;
}

template <Core::File::Mode ACCESSMODE>
uint32_t CyclicBufferClient<ACCESSMODE>::Stop(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (_buffer.IsValid() != false) {
        _buffer.Alert();

        _buffer.Flush();

        result = Close(waitTime);
    } else {
        // No valid buffer present
    }

    return result;
}

template <Core::File::Mode ACCESSMODE>
uint32_t CyclicBufferClient<ACCESSMODE>::Exchange(uint8_t buffer[], uint16_t& bufferSize, uint16_t bufferMaxSize, uint64_t& duration)
{
    ASSERT(bufferSize <= bufferMaxSize);

    uint32_t result = Core::ERROR_GENERAL;

    constexpr bool dataPresent{ false };

    // Testing value
//    duration = Core::infinite;

// TODO: narrowing conversion
    uint32_t waitTime{ static_cast<uint32_t>(duration) };

    if (_buffer.IsValid() != false) {
        Core::StopWatch timer;

        /* uint64_t */ timer.Reset();

        if (   (result = _batonConnectingNode.Wait(waitTime)) == Core::ERROR_NONE
            && (result = _buffer.Lock(dataPresent, duration)) == Core::ERROR_NONE
        ) {
            // The 'client side' can read the written data 

            bufferSize = _buffer.Used();

            ASSERT(bufferSize <= bufferMaxSize);

            bufferSize =_buffer.Read(buffer, bufferSize, false /* partial read if true, eg insufficient data to fill the input buffer avaialble */);

            ASSERT(_buffer.Used() <= 0);

            // The 'client side' can write data

            // Buffer writers should make a reservation for space before 'Write(...)' to avoid data corruption
            if (   _buffer.Free() >= bufferSize
                && _buffer.Reserve(bufferSize) == bufferSize
               ) {
                // The written size should match the read size
                uint16_t bufferReadSize = bufferSize;

                bufferSize = _buffer.Write(buffer, bufferSize);

                // The written size should match the read size

                ASSERT(bufferReadSize == bufferSize);

                ASSERT(_buffer.Used() > 0);

                if ((result = _buffer.Unlock()) == Core::ERROR_NONE) {
                    result = _batonConnectingNode.Signal(waitTime);
                }
            } else {
                result = _buffer.Unlock();
            }
      }

      duration = timer.Elapsed();
    } else {
        duration = 0;
    }

    ASSERT((result == Core::ERROR_NONE) || (result == Core::ERROR_TIMEDOUT));

    return result;

}

template <Core::File::Mode ACCESSMODE>
uint32_t CyclicBufferClient<ACCESSMODE>::Open(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (_buffer.IsValid() != true) {

        if (   _buffer.Open() != false
            && _buffer.IsValid() != false
        ) {
            result = Core::ERROR_NONE;
        }

    } else {
        result = Core::ERROR_NONE;            
    }
        
    if (   result == Core::ERROR_NONE
        && _buffer.IsValid() != false
        && _batonConnectingNode.IsOpen() != true
    ) {
        result = _batonConnectingNode.Open(waitTime);
    }

    return result;
}

template <Core::File::Mode ACCESSMODE>
uint32_t CyclicBufferClient<ACCESSMODE>::Close(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (_batonConnectingNode.IsOpen() != false) {
        result = _batonConnectingNode.Close(waitTime);
    }

    if (_buffer.IsValid() != false) {
        /* void */ _buffer.Close();
    }

    if (    result == Core::ERROR_NONE
        && _buffer.IsValid() != false
    ) {
        result = Core::ERROR_GENERAL;
    }

    return result;
}


template <Core::File::Mode ACCESSMODE>
CyclicBufferClient<ACCESSMODE>::CustomCyclicBuffer::CustomCyclicBuffer(const string& fileName, const uint32_t mode, const uint32_t bufferSize, const bool overwrite)
    : CyclicBuffer(fileName, mode, bufferSize, overwrite)
{}

template <Core::File::Mode ACCESSMODE>
CyclicBufferClient<ACCESSMODE>::CustomCyclicBuffer::CustomCyclicBuffer(Core::DataElementFile& buffer, const bool initiator, const uint32_t offset, const uint32_t bufferSize, const bool overwrite)
    : CyclicBuffer(buffer, initiator, offset, bufferSize, overwrite)
{}

template <Core::File::Mode ACCESSMODE>
CyclicBufferClient<ACCESSMODE>::CustomCyclicBuffer::~CustomCyclicBuffer()
{}

template <Core::File::Mode ACCESSMODE>
void CyclicBufferClient<ACCESSMODE>::CustomCyclicBuffer::DataAvailable()
{}


template <Core::File::Mode ACCESSMODE>
CyclicBufferClient<ACCESSMODE>::BatonConnectedType::BatonConnectedType(const Core::NodeId& remoteNodeId)
    // Use identical Type() of the remoteNodeId also locally
    : BatonConnectionType(false, remoteNodeId.AnyInterface(), remoteNodeId, 1024, 1024)
{}

template <Core::File::Mode ACCESSMODE>
CyclicBufferClient<ACCESSMODE>::BatonConnectedType::~BatonConnectedType()
{}


SimplePluginCyclicBufferClientImplementation::SimplePluginCyclicBufferClientImplementation()
    : _client{ _T("/tmp/CommunicationPerformanceCyclicBuffer") }
{}

SimplePluginCyclicBufferClientImplementation::~SimplePluginCyclicBufferClientImplementation()
{
    /* uint32_t */ Stop(Core::infinite);
}

uint32_t SimplePluginCyclicBufferClientImplementation::Start(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if ((result = _client.Start(waitTime)) != Core::ERROR_NONE) {
        /* uint32_t */ Stop(waitTime);
    }

    static_assert(  std::is_integral<decltype(std::time(nullptr))>::value
                  , "'std::srand' requires an integral type as seed"
    );

    std::srand(std::time(nullptr));

    return result;
}

uint32_t SimplePluginCyclicBufferClientImplementation::Stop(uint32_t waitTime)
{
    return _client.Stop(waitTime);
}

uint32_t SimplePluginCyclicBufferClientImplementation::Task(STATE& state, VARIABLE_IS_NOT_USED uint32_t& waitTime)
{
    uint32_t result = Core::ERROR_NONE;

PUSH_WARNING(DISABLE_WARNING_IMPLICIT_FALLTHROUGH);
    switch (state) {
    case STATE::IDLE    :   state = STATE::RUN;
    case STATE::RUN     :   state = STATE::EXECUTE;
    case STATE::EXECUTE :   {
                            // Set to a low value for quick builds
                            constexpr uint16_t bufferMaxSize = CommunicationPerformanceHelpers::TemplateRecursionDepth;

                            std::array<uint8_t, bufferMaxSize> buffer = CommunicationPerformanceHelpers::ConstexprArray<uint8_t, bufferMaxSize>::values;

                            // Initial value used as waitTime in Exchange()
                            // Educated guess
                            // Return value has execution time in ticks
                            uint64_t duration = 1000;

                            uint16_t bufferSize = bufferMaxSize;

                            // With no mistakes this always holds
                            ASSERT(bufferSize <= bufferMaxSize);

                            // This fails if the client is missing so it is not typically an error
                            if (   bufferSize > 0
                                && _client.Exchange(buffer.data(), bufferSize, bufferMaxSize, duration) == Core::ERROR_NONE
// TODO: distinghuish return values for partial and non-partial read allowed
                                && bufferSize > 0
                            ) {
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

SERVICE_REGISTRATION(SimplePluginCyclicBufferClientImplementation, 1, 0)

} } // namespace Thunder::Plugin
