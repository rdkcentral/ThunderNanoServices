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

#include "CommunicationPerformanceCyclicBufferServerImplementation.h"

// Pull in 'Common' template definitions
#include "../Common/CommunicationPerformanceImplementation.cpp"

#ifdef _USE_CHRONO_HIGH_RESOLUTION_CLOCK_
#include <chrono>
#endif

#include <random>

namespace Thunder {
namespace Plugin {

template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::CyclicBufferServer(const std::string& fileName)
    : _fileName{ fileName }
    , _buffer{ _fileName, ACCESSMODE | Core::File::CREATE | Core::File::USER_WRITE | Core::File::SHAREABLE /* required for multiple processes to share data */, FILESIZE /* server controls size */ , OVERWRITE }
    , _lock{}
    // Listening node
    , _batonListeningNode{ Core::NodeId{ std::string{ _fileName + "Baton" }.c_str(), Core::NodeId::TYPE_DOMAIN } }
{}

template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::~CyclicBufferServer()
{
    /* uint32_t result = */ Stop(Core::infinite);
}

template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
uint32_t CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::Start(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    result = Open(waitTime);

    return result;
}

template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
uint32_t CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::Stop(uint32_t waitTime)
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

template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
uint32_t CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::Exchange(uint8_t buffer[], uint16_t& bufferSize, VARIABLE_IS_NOT_USED uint16_t bufferMaxSize, uint64_t& duration)
{
    ASSERT(bufferSize <= bufferMaxSize);

    uint32_t result = Core::ERROR_GENERAL;

    constexpr bool dataPresent{ false };

    // Testing value
//    duration = Core::infinite;

// TODO: narrowing conversion
    uint32_t waitTime{ static_cast<uint32_t>(duration) };

    if (_buffer.IsValid() != false) {
        auto it = _batonListeningNode.Clients();

#ifndef _USE_CHRONO_HIGH_RESOLUTION_CLOCK_
        Core::StopWatch timer;

        /* uint64_t */ timer.Reset();
#else
        auto start = std::chrono::high_resolution_clock::now();
#endif

        if ((result = _buffer.Lock(dataPresent, duration)) == Core::ERROR_NONE) {

            // Buffer writers should make a reservation for space before 'Write(...)' to avoid data corruption
            if (   _buffer.Free() >= bufferSize
                && _buffer.Reserve(bufferSize) == bufferSize
            ) {
                bufferSize = _buffer.Write(buffer, bufferSize);

                ASSERT(_buffer.Used() > 0);

                // The 'client' can read

                it.Reset();

                result = _buffer.Unlock();

                while (   it.Next() != false
                       && result == Core::ERROR_NONE
                       && (result = it.Client()->Signal(waitTime)) == Core::ERROR_NONE
                ) {
                    result = it.Client()->Wait(waitTime);
                }

                // The 'client' has written

                if (   result == Core::ERROR_NONE
                    && (result = _buffer.Lock(waitTime)) == Core::ERROR_NONE
                ) {
                    // The 'server' can read the written data 

                    // The read size should match the write size
                    VARIABLE_IS_NOT_USED int16_t bufferWriteSize = bufferSize;

                    bufferSize = _buffer.Used();

                    ASSERT(bufferSize <= bufferMaxSize);

                    bufferSize =_buffer.Read(buffer, bufferSize, false /* partial read if true, eg insufficient data to fill the input buffer avaialble */);

                    ASSERT(_buffer.Used() <= 0);

                    // The read size should match the write size
                    ASSERT(bufferWriteSize == bufferSize);

                    result = _buffer.Unlock();
                }

                // Enforce start condition
                _buffer.Flush();
            } else {
                result = _buffer.Unlock();
            }
        }

#ifndef _USE_CHRONO_HIGH_RESOLUTION_CLOCK_
        duration = timer.Elapsed();
#else
        auto end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif
    } else {
        duration = 0;
    }

    ASSERT((result == Core::ERROR_NONE) || (result == Core::ERROR_TIMEDOUT));

    return result;
}

template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
uint32_t CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::Open(uint32_t waitTime)
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
    ) {
        result = _batonListeningNode.Open(waitTime);
    }

    return result;
}

template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
uint32_t CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::Close(uint32_t waitTime)
{
    uint32_t result = _batonListeningNode.Close(waitTime);

    if (_buffer.IsValid() != false) {
        /* void */ _buffer.Close();
    }

    if (   result == Core:: ERROR_NONE
        && _buffer.IsValid() != false
    ) {
        result = Core::ERROR_GENERAL;
    }

    return result;
}


template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::CustomCyclicBuffer::CustomCyclicBuffer(const string& fileName, const uint32_t mode, const uint32_t bufferSize, const bool overwrite)
    : CyclicBuffer(fileName, mode, bufferSize, overwrite)
{}

template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::CustomCyclicBuffer::CustomCyclicBuffer(Core::DataElementFile& buffer, const bool initiator, const uint32_t offset, const uint32_t bufferSize, const bool overwrite)
    : CyclicBuffer(buffer, initiator, offset, bufferSize, overwrite)
{}

template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::CustomCyclicBuffer::~CustomCyclicBuffer()
{}

template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
void CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::CustomCyclicBuffer::DataAvailable()
{}


template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::BatonAcceptConnectionType::BatonAcceptConnectionType(const SOCKET localSocket, const Core::NodeId& remoteNodeId, Core::SocketServerType<BatonAcceptConnectionType>* server)
    : BatonConnectionType( false, localSocket, remoteNodeId, 1024, 2024 )
    , _server{ *server }
{}

template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE, bool OVERWRITE>
CyclicBufferServer<FILESIZE, ACCESSMODE, OVERWRITE>::BatonAcceptConnectionType::~BatonAcceptConnectionType()
{}


SimplePluginCyclicBufferServerImplementation::SimplePluginCyclicBufferServerImplementation()
    : _server{ _T("/tmp/CommunicationPerformanceCyclicBuffer") }
{}

SimplePluginCyclicBufferServerImplementation::~SimplePluginCyclicBufferServerImplementation()
{
    /* uint32_t */ Stop(Core::infinite);
}

uint32_t SimplePluginCyclicBufferServerImplementation::Start(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if ((result = _server.Start(waitTime)) != Core::ERROR_NONE) {
        /* uint32_t */ Stop(waitTime);
    }

    return result;
}

uint32_t SimplePluginCyclicBufferServerImplementation::Stop(uint32_t waitTime)
{
    return _server.Stop(waitTime);
}

uint32_t SimplePluginCyclicBufferServerImplementation::Task(STATE& state, uint32_t& waitTime)
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

                            // Educated guess, system dependent, required for distribution
                            constexpr uint64_t upperBoundDuration = CommunicationPerformanceHelpers::UpperBoundDuration;
                            constexpr uint64_t lowerBoundDuration = CommunicationPerformanceHelpers::LowerBoundDuration;
                            constexpr uint16_t bufferMinSize = 0;

                            // Initial value used as waitTime in Exchange()
                            // Educated guess
                            // Return value has the round trip time in ticks
                            uint64_t duration = 1000;

                            // Add some randomness

                            std::random_device rd; // Generate values between 0 and std:numeric_limits<unsigned int>::max()

                            using common_t = std::common_type<unsigned int, uint16_t>::type;

                            static_assert(static_cast<common_t>(std::numeric_limits<unsigned int>::max()) >= static_cast<common_t>(bufferMaxSize), "");

                            uint16_t bufferSize = static_cast<uint16_t>(static_cast<common_t>(rd()) % static_cast<common_t>(bufferMaxSize));

                            // With no mistakes this always holds
                            ASSERT(bufferSize <= bufferMaxSize);

                            // This fails if the client is missing so it is not typically an error
                            if (   bufferSize > 0
                                && _server.Exchange(buffer.data(), bufferSize, bufferMaxSize, duration) == Core::ERROR_NONE
                                && bufferSize > 0
                            ) {

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

SERVICE_REGISTRATION(SimplePluginCyclicBufferServerImplementation, 1, 0)

} } // namespace Thunder::Plugin
