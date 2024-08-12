#include "CECMessage.h"
#include "CECProcessor.h"

namespace Thunder {
namespace CEC {
    Service::Service(const opcode_t requestOpCode, const opcode_t responseOpCode, const bool isBroadcast, const role_mask_t allowedRoles)
        : _requestOpCode(requestOpCode)
        , _responseOpCode(responseOpCode)
        , _isBroadcast(isBroadcast)
        , _allowedRoles(allowedRoles)
    {
        Processor::Instance().Announce(this);
    }

    Service::~Service()
    {
        Processor::Instance().Revoke(this);
    }
}
}