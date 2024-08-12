#include "CECProcessor.h"

#include <CECOperationFrame.h>
#include "CECMessage.h"

#include <messaging/messaging.h>

namespace Thunder {
namespace CEC {
    void Processor::Process(/*const cec_adapter_role_t role, */ Thunder::CEC::OperationFrame& operation, bool& broadcast)
    {
        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        auto index(_awnsers.find(operation.OpCode()));

        uint8_t responseLength(0);
        uint8_t* payload(operation.LockParameters());

        if (index != _awnsers.end()) {
            responseLength = (index->second)->Handle(OperationFrame::MaxLength, operation.Size() - OperationFrame::OpCodeLength, payload);

            if (responseLength != uint8_t(~0)) {
                // Seems like it's a valid request and we have a response.
                broadcast = (index->second)->IsBroadcast();
                operation.OpCode((index->second)->ResponseOpCode());
            } else {
                // Validation of payload failed...
                broadcast = false;
                operation.OpCode(FEATURE_ABORT);
                payload[0] = ABORT_REASON_INVALID_OPERAND;
                responseLength = 1;
            }
        } else {
            // We cannot awnser this request...
            // operation.Destination(operation.Initiator());
            operation.OpCode(FEATURE_ABORT);
            payload[0] = ABORT_REASON_UNRECOGNIZED_OPCODE;
            responseLength = 1;
        }

        operation.UnlockParameters(responseLength);

        TRACE(Trace::Information, ("Awnser operation: Length: %d, Opcode 0x%02x, %s.", operation.Size(), operation.OpCode(), broadcast ? "broadcast" : "direct"));
    }

    uint32_t Processor::Announce(Service* service)
    {
        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);
        auto index = _awnsers.find(service->RequestOpCode());

        if (index == _awnsers.end()) {
            _awnsers.emplace(std::make_pair(service->RequestOpCode(), service));
            TRACE(Trace::Information, ("Announced opcode 0x%02X, %p", service->RequestOpCode(), service));
        } else {
            TRACE(Trace::Error, ("Skipped awnser id 0x%02x, it was allready annouced", service->RequestOpCode()));
        }

        return 0;
    }

    uint32_t Processor::Revoke(Service* service)
    {
        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);
        auto index = _awnsers.find(service->RequestOpCode());

        if (index != _awnsers.end()) {
            _awnsers.erase(index);
        } else {
            TRACE(Trace::Error, ("Skipped awnser id 0x%02x, it was not found", service->RequestOpCode()));
        }

        return 0;
    }
} // namespace CEC
} // namespace Thunder