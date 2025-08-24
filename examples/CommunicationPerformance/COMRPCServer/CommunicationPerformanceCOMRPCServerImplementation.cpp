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

#include "CommunicationPerformanceCOMRPCServerImplementation.h"

// Pull in 'Common' template definitions
#include "../Common/CommunicationPerformanceImplementation.cpp"

namespace Thunder {
namespace Plugin {

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::COMRPCServer(const std::string& remoteNode, const string& proxyStubPath, const std::string& sourceName)
    : _remoteNode{ static_cast<const TCHAR*>(remoteNode.c_str()) }
    , _proxyStubPath{ proxyStubPath }
    , _sourceName{ sourceName }
    , _messageHandler{}
    , _communicator{}
    , _lock{}
{}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::~COMRPCServer()
{
    /* uint32_t result = */ Stop(Core::infinite);
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
uint32_t COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Start(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (_communicator.IsValid() != true) {
        _messageHandler = Core::ProxyType<RPC::InvokeServerType<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>>::Create();

        if (_messageHandler.IsValid() != false) {
            _communicator = Core::ProxyType<Communicator>::Create(_remoteNode, _proxyStubPath, Core::ProxyType<Core::IIPCServer>(_messageHandler), static_cast<const TCHAR*>(_sourceName.c_str()));
        }
    }

    if (_communicator.IsValid() != false) {
        result = Open(waitTime);
    }

    return result;
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
uint32_t COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Stop(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (   _communicator.IsValid() != false
        && (result = Close(waitTime)) == Core::ERROR_NONE
       ) {
        _communicator->Revoke(static_cast<Core::IUnknown*>(_communicator->Acquire(_T(""), Exchange::IPerformance::ID, ~0)), Exchange::IPerformance::ID);

        result = _communicator.Release();
        ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
    }

    if (   _communicator.IsValid() != true
        && _messageHandler.IsValid() != false
       ) {
        result = _messageHandler.Release();
        ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
    }

    return result;
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
uint32_t COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Exchange(uint8_t buffer[], uint16_t bufferSize, uint16_t bufferMaxSize, uint64_t& duration) const
{
    uint32_t result = Core::ERROR_GENERAL;

    Exchange::IPerformance* interface = nullptr;

    if (   _communicator.IsValid() != false
        // Cast to the interface type and NOT the derived class type!
        && (interface = static_cast<Exchange::IPerformance*>(_communicator->Acquire("", Exchange::IPerformance::ID, ~0))) != nullptr
       ) {
        Core::StopWatch timer;

        /* uint64_t */ timer.Reset();

        result = interface->Exchange(bufferSize, buffer, bufferMaxSize);

        duration = timer.Elapsed();
    } else {
        duration = 0;
    }

    return result;
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Communicator::Communicator(const Core::NodeId& remoteNode, const std::string& proxyStubPath, const Core::ProxyType<Core::IIPCServer>& messageHandler, const TCHAR* sourceName)
    : RPC::Communicator{ remoteNode, proxyStubPath, messageHandler, sourceName }
    , _lock{}
    , _performanceInterface{ nullptr }
{}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Communicator::~Communicator()
{
    /* uint32_t */ Revoke(static_cast<Core::IUnknown*>(_performanceInterface), Exchange::IPerformance::ID);

    Lock();

    ASSERT(_performanceInterface == nullptr);

    Unlock();
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
void COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Communicator::Lock() const
{
    _lock.Lock();
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
void COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Communicator::Unlock() const
{
   _lock.Unlock();
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
void* COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Communicator::Acquire(VARIABLE_IS_NOT_USED const string& className, const uint32_t interfaceId, VARIABLE_IS_NOT_USED const uint32_t version)
{
    void* result = nullptr;

    if (interfaceId == Exchange::IPerformance::ID) {
        result = _performanceInterface;
    }

    return result;
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
void COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Communicator::Offer(Core::IUnknown* object, const uint32_t interfaceId)
{
    if (interfaceId == Exchange::IPerformance::ID) {
        // Obtain the interface from the service object representing the remote side
        if (object != nullptr) {
            // A new interface has been offered; drop the old one
            // The remote could invoke it as well, re-use what is available
            Revoke(object, interfaceId);

            Lock();

            _performanceInterface = object->QueryInterface<Exchange::IPerformance>();

            ASSERT(_performanceInterface != nullptr);

            Unlock();
        }
    }
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
void COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Communicator::Revoke(const Core::IUnknown* object, const uint32_t interfaceId)
{
    // Performance the object representing the remote side before releasing the interface
    if (interfaceId == Exchange::IPerformance::ID) {
        Lock();

        if (   object != nullptr
            // The examples show that not the same object as in Offer is an input parameter, but the result of QueryInterface
            && object == _performanceInterface
        ) {
            VARIABLE_IS_NOT_USED uint32_t result = _performanceInterface->Release();
            ASSERT((result == Core::ERROR_NONE) || (result == Core::ERROR_DESTRUCTION_SUCCEEDED));

            _performanceInterface = nullptr;
        }

        Unlock();
    }
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
void COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Communicator::Dangling(const Core::IUnknown* object, const uint32_t interfaceId)
{
    if (interfaceId == Exchange::IPerformance::ID) {
        Revoke(object, interfaceId);
    }
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
uint32_t COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Open(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_NONE;

    if (_communicator->IsListening() != true) {
        if (!(   (result = _communicator->Open(waitTime)) == Core::ERROR_NONE
              && _communicator->IsListening()
             )
        ) {
            result = Core::ERROR_CONNECTION_CLOSED;
        }
    }

    return result;
}

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
uint32_t COMRPCServer<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>::Close(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_NONE;

    if (_communicator->IsListening() == true) {
        if (!(   (result = _communicator->Close(waitTime)) == Core::ERROR_NONE
              && !_communicator->IsListening()
            )
        ) {
            result = Core::ERROR_CLOSING_FAILED;
        } else {
        }
    }

    return result;
}


SimplePluginCOMRPCServerImplementation::SimplePluginCOMRPCServerImplementation()
    : _server{ _T("127.0.0.1:8080") /* listeningNode */, _T("") /* proxyStubPath */, _T("") /* sourceName */}
{}

SimplePluginCOMRPCServerImplementation::~SimplePluginCOMRPCServerImplementation()
{
    /* uint32_t */ Stop(Core::infinite);
}

uint32_t SimplePluginCOMRPCServerImplementation::Start(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if ((result = _server.Start(waitTime)) != Core::ERROR_NONE) {
        /* uint32_t */ Stop(waitTime);
    }

    static_assert(  std::is_integral<decltype(std::time(nullptr))>::value
                  , "'std::srand' requires an integral type as seed"
    );

    std::srand(std::time(nullptr));

    return result;
}

uint32_t SimplePluginCOMRPCServerImplementation::Stop(uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if ((result = _server.Stop(waitTime)) == Core::ERROR_NONE) {
    }

    return result;
}

uint32_t SimplePluginCOMRPCServerImplementation::Task(STATE& state, VARIABLE_IS_NOT_USED uint32_t& waitTime)
{
    uint32_t result = Core::ERROR_NONE;

PUSH_WARNING(DISABLE_WARNING_IMPLICIT_FALLTHROUGH);
    switch (state) {
    case STATE::IDLE    :   state = STATE::RUN;
    case STATE::RUN     :   state = STATE::EXECUTE;
    case STATE::EXECUTE :   {
                            // Set to a low value for quick builds
                            constexpr uint16_t bufferMaxSize = 899;

                            constexpr size_t numberOfBins = 30;

#ifndef _USE_TESTDATA
                            std::array<uint8_t, bufferMaxSize> buffer = CommunicationPerformanceHelpers::ConstexprArray<uint8_t, bufferMaxSize>::values;

                            // Educated guess, system dependent, required for distribution
                            constexpr uint64_t upperBoundDuration = 1500;

                            // Round trip time
                            uint64_t duration = 0;

                            // Add some randomness

                            using common_t = std::common_type<int, uint16_t>::type;
                            const uint16_t bufferSize = static_cast<uint16_t>(static_cast<common_t>(std::rand()) % static_cast<common_t>(bufferMaxSize));

                            // With no mistakes this always holds 
                            ASSERT(bufferSize <= bufferMaxSize);

                            // This fails if the client is missing so it is not typically an error
                            if (_server.Exchange(buffer.data(), bufferSize, bufferMaxSize, duration) == Core::ERROR_NONE) {
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
#else
                            std::pair<uint16_t, uint64_t> element{ 0, 0 };

                            if (_data.Next(element)!= false) {
                                uint16_t& bufferSize = element.first;
                                uint64_t& duration = element.second;

                                using measurements_t = Measurements<uint16_t, uint64_t>;
                                // Educated guess
                                constexpr uint64_t upperBoundDuration = 500;
                                using distribution_t = measurements_t::Histogram2D<numberOfBins, bufferMaxSize, upperBoundDuration>;

                                distribution_t& measurements = measurements_t::Instance<distribution_t>();

                                if (measurements.Insert(std::pair<uint16_t, uint64_t>(bufferSize, duration)) != true) {
                                    state = STATE::STOP;
                                }
                            }
#endif
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

SERVICE_REGISTRATION(SimplePluginCOMRPCServerImplementation, 1, 0)

} } // namespace Thunder::Plugin
