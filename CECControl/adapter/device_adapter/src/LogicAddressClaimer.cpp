#include "LogicAddressClaimer.h"

#include <stdint.h>

namespace Thunder {
namespace CEC {
    static constexpr uint16_t CheckInterval = 500;
    static constexpr uint8_t CheckAttemps = 2;

    uint64_t LogicAddressClaimer::Check()
    {
        uint64_t reschedule(0);

        Message msg(0x0F, *(_addresses.begin()));

        ASSERT(_adapter != nullptr);

        if ((_adapter) && (_adapter->Transmit(Convert(msg.Initiator()), msg.Follower(), msg.PayloadLength(), msg.Payload()) == Core::ERROR_NONE)) {
            _addresses.erase(_addresses.begin());
            _attemps = (_addresses.empty() != false) ? 0 : CheckAttemps;
        }

        if (++_attemps < CheckAttemps) {
            reschedule = Core::Time::Now().Add(CheckInterval).Ticks();
        } else {
            _callback.Finished(this);
            TRACE(Trace::Information, ("%s(%p) claim finished", __FUNCTION__, this));
        }

        TRACE(Trace::Information, ("%s(%p) next check: %" PRIu64, __FUNCTION__, this, reschedule));

        return reschedule;
    }

} // namespace CEC
} // namespace Thunder