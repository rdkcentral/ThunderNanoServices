/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#include "Module.h"

#include <CECIAdapterAccessor.h>
#include <CECIDeviceAdapter.h>

#include <CECAdapter.h>
#include <CECTypes.h>

#include "CECOperationFrame.h"

namespace Thunder {
namespace CEC {
    uint32_t Adapter::Exchange(const logical_address_t follower, CEC::IExchange& exchange, const uint32_t waitTime)
    {
        uint32_t result(Core::ERROR_UNAVAILABLE);

        if (IsValid()) {
            bool awnsered(true);
            OperationFrame operation;

            _currentExchange = &exchange;
            _currentFollower = follower;

            uint8_t* buffer = operation.LockData();

            // return opcode and parameters
            uint8_t length = exchange.Serialize(operation.MaxLength, buffer);

            operation.UnlockData(length);

            _deviceAdapter->Register(&_sink);

            result = _deviceAdapter->Transmit(_initiator, follower, operation.Size(), operation.Data());

            if (exchange.AwnserExpected()) {
                awnsered = WaitForAwnser(waitTime);
            }

            TRACE(Trace::Information, ("Exchange Transmit(result=0x%04X, awnsered=0x%04X)", result, awnsered));

            _deviceAdapter->Unregister(&_sink);

            Core::SafeSyncType<Core::CriticalSection> scopedLock(_exchangeLock);

            _currentExchange = nullptr;
            _currentFollower = CEC_LOGICAL_ADDRESS_INVALID;

            result = (awnsered == true) ? result : uint32_t(Core::ERROR_TIMEDOUT);
        }

        return result;
    }

    uint8_t Adapter::Received(const logical_address_t initiator, const uint8_t length, const uint8_t data[])
    {
        uint8_t result(0);

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_exchangeLock);

        if ((_currentExchange != nullptr) && ((initiator == _currentFollower) || (initiator == CEC_LOGICAL_ADDRESS_BROADCAST))) {
            OperationFrame msg(length, data);
            _currentExchange->Deserialize(msg.Size(), msg.Data());
            result = length;
        }

        // Set event so WaitForAwnser() can continue.
        _awnserEvent.SetEvent();

        return result;
    }
} // namespace CEC
} // namespace Thunder
