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

// A single server to serve one to N clients
template <uint32_t FILESIZE, Core::File::Mode ACCESSMODE = Core::File::USER_READ | Core::File::USER_WRITE, bool OVERWRITE = false>
class CyclicBufferServer {
public :
    CyclicBufferServer() = delete;
    CyclicBufferServer(const CyclicBufferServer&) = delete;
    CyclicBufferServer(CyclicBufferServer&&) = delete;

    CyclicBufferServer& operator=(const CyclicBufferServer&) = delete;
    CyclicBufferServer& operator=(CyclicBufferServer&&) = delete;

    CyclicBufferServer(const std::string& fileName);

    ~CyclicBufferServer();

    uint32_t Start(uint32_t waitTime);

    uint32_t Stop(uint32_t waitTime);

    uint32_t SendData(const uint8_t buffer[], uint16_t& bufferSize, uint16_t bufferMaxSize, uint64_t& duration) const;

private :

    const std::string _fileName;

    mutable Core::CyclicBuffer _buffer;

    mutable Core::CriticalSection _lock;

    uint32_t Open(uint32_t waitTime);

    uint32_t Close(uint32_t waitTime);
};


class SimplePluginCyclicBufferServerImplementation : public SimplePluginImplementation<SimplePluginCyclicBufferServerImplementation> {
public :
    SimplePluginCyclicBufferServerImplementation(const SimplePluginCyclicBufferServerImplementation&) = delete;
    SimplePluginCyclicBufferServerImplementation(SimplePluginCyclicBufferServerImplementation&&) = delete;

    SimplePluginCyclicBufferServerImplementation& operator=(const SimplePluginCyclicBufferServerImplementation&) = delete;
    SimplePluginCyclicBufferServerImplementation& operator=(SimplePluginCyclicBufferServerImplementation&&) = delete;

    // SERVICE_REGISTRATION requires a constructor that takes no arguments
    SimplePluginCyclicBufferServerImplementation();

    ~SimplePluginCyclicBufferServerImplementation();

    uint32_t Start(uint32_t waitTime);

    uint32_t Stop(uint32_t waitTime);

private :
// TODO: educated guess 1 MiB
    CyclicBufferServer<1024*1024 /* uint8_t, sufficiently large memory mapped region for write buffers */, static_cast<Core::File::Mode>(Core::File::USER_READ | Core::File::USER_WRITE), false /* do not overwrite existing data without reading */> _server;

    friend SimplePluginImplementation;
    uint32_t Task(STATE& state, uint32_t& waitTime);
};

} } // namespace Thunder::Plugin
