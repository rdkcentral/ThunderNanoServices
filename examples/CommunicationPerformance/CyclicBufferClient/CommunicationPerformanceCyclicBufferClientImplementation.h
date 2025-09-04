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

template <Core::File::Mode ACCESSMODE = Core::File::USER_READ>
class CyclicBufferClient {
public:
    CyclicBufferClient() = delete;
    CyclicBufferClient(const CyclicBufferClient&) = delete;
    CyclicBufferClient(CyclicBufferClient&&) = delete;

    CyclicBufferClient& operator=(const CyclicBufferClient&) = delete;
    CyclicBufferClient& operator=(CyclicBufferClient&&) = delete;

    CyclicBufferClient(const std::string& fileName);

    ~CyclicBufferClient(); // no override !

    uint32_t Start(uint32_t waitTime);

    uint32_t Stop(uint32_t waitTime);

    // If bufferSize != bufferMaxSize then buffer may be partially filled
    uint32_t ReceiveData(uint8_t buffer[], uint16_t& bufferSize, uint16_t bufferMaxSize, uint64_t& duration) const;

private:

    const std::string _fileName;

    mutable Core::CyclicBuffer _buffer;

    mutable Core::CriticalSection _lock;

    uint32_t Open(uint32_t waitTime);

    uint32_t Close(uint32_t waitTime);
};


class SimplePluginCyclicBufferClientImplementation : public SimplePluginImplementation<SimplePluginCyclicBufferClientImplementation> {
public :
    SimplePluginCyclicBufferClientImplementation(const SimplePluginCyclicBufferClientImplementation&) = delete;
    SimplePluginCyclicBufferClientImplementation(SimplePluginCyclicBufferClientImplementation&&) = delete;

    SimplePluginCyclicBufferClientImplementation& operator=(const SimplePluginCyclicBufferClientImplementation&) = delete;
    SimplePluginCyclicBufferClientImplementation& operator=(SimplePluginCyclicBufferClientImplementation&&) = delete;

    // SERVICE_REGISTRATION requires a constructor that takes no arguments
    SimplePluginCyclicBufferClientImplementation();

    ~SimplePluginCyclicBufferClientImplementation();

    uint32_t Start(uint32_t waitTime);

    uint32_t Stop(uint32_t waitTime);

private :
    CyclicBufferClient<Core::File::USER_READ> _client;

    friend SimplePluginImplementation;
    uint32_t Task(STATE& state, uint32_t& waitTime);
};

} } // namespace Thunder::Plugin
