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
    , _buffer{ _fileName, ACCESSMODE | Core::File::SHAREABLE /* required for multiple processes to share data */, 0 /* ineffective,  server controls size */ , false /* ineffective, server controls flag */ }
    , _lock{}
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

    if (_buffer.IsValid() != true) {
        result = Open(waitTime);
    } else {
        // Already a valid buffer present
        result = Core::ERROR_NONE;
    }

    return result;
}

template <Core::File::Mode ACCESSMODE>
uint32_t CyclicBufferClient<ACCESSMODE>::Stop(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (_buffer.IsValid() != false) {
        result = Close(waitTime);
    } else {
        // No valid buffer present
    }

    return result;
}

template <Core::File::Mode ACCESSMODE>
uint32_t CyclicBufferClient<ACCESSMODE>::ReceiveData(uint8_t buffer[], uint16_t& bufferSize, uint16_t bufferMaxSize, uint64_t& duration) const
{
    ASSERT(bufferSize <= bufferMaxSize);

    uint32_t result = Core::ERROR_GENERAL;

    if (_buffer.IsValid() != false) {
        Core::StopWatch timer;

        /* uint64_t */ timer.Reset();

        // Only full reads are considered when processing the data
        if ((result = _buffer.Lock()) == Core::ERROR_NONE) {
            bufferSize =_buffer.Read(buffer, bufferMaxSize, bufferSize != bufferMaxSize /* partial read if true, eg insufficient data to fill the input buffer avaialble */);

            result = _buffer.Unlock();
        } else {
            // The lock could not be obtained
            ASSERT(false);
        }

        duration = timer.Elapsed();
    } else {
        duration = 0;
    }

    return result;
}

template <Core::File::Mode ACCESSMODE>
uint32_t CyclicBufferClient<ACCESSMODE>::Open(VARIABLE_IS_NOT_USED uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (   _buffer.IsValid() != true
        && _buffer.Open() != false
        && _buffer.IsValid() != false
       ) {
        result = Core::ERROR_NONE;
    }

    return result;
}

template <Core::File::Mode ACCESSMODE>
uint32_t CyclicBufferClient<ACCESSMODE>::Close(VARIABLE_IS_NOT_USED uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (_buffer.IsValid() != false) {

        /* void */ _buffer.Close();
    }

    if (_buffer.IsValid() != true) {
        result = Core::ERROR_NONE;
    }

    return result;
}


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

uint32_t SimplePluginCyclicBufferClientImplementation::Task(VARIABLE_IS_NOT_USED STATE& state, VARIABLE_IS_NOT_USED uint32_t& waitTime)
{
    uint32_t result = Core::ERROR_NONE;

PUSH_WARNING(DISABLE_WARNING_IMPLICIT_FALLTHROUGH);
    switch (state) {
    case STATE::IDLE    :   state = STATE::RUN;
    case STATE::RUN     :   state = STATE::EXECUTE;
    case STATE::EXECUTE :   {
                            // Set to a low value for quick builds
                            constexpr uint16_t bufferMaxSize = 8999;

                            constexpr size_t numberOfBins = 20;

                            std::array<uint8_t, bufferMaxSize> buffer = CommunicationPerformanceHelpers::ConstexprArray<uint8_t, bufferMaxSize>::values;

                            // Educated guess, system dependent, required for distribution
                            constexpr uint64_t upperBoundDuration = 250;

                            // Round trip time in ticks
                            uint64_t duration = 0;

                            // Add some randomness

                            using common_t = std::common_type<int, uint16_t>::type;
                            uint16_t bufferSize = static_cast<uint16_t>(static_cast<common_t>(std::rand()) % static_cast<common_t>(bufferMaxSize));

                            // With no mistakes this always holds
                            ASSERT(bufferSize <= bufferMaxSize);

                            // This fails if the client is missing so it is not typically an error
                            if (   bufferSize > 0
                                && _client.ReceiveData(buffer.data(), bufferSize, bufferSize, duration) == Core::ERROR_NONE
// TODO: distinghuish return values for partial and non-partial read allowed
                                && bufferSize > 0
                            ) {
                                using measurements_t = Measurements<uint16_t, uint64_t>;
                                using distribution_t = measurements_t::Histogram2D<numberOfBins, bufferMaxSize, upperBoundDuration>;

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

SERVICE_REGISTRATION(SimplePluginCyclicBufferClientImplementation, 1, 0)

} } // namespace Thunder::Plugin
