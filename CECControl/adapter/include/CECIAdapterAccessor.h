#pragma once

#include <CECTypes.h>
#include <CECMessage.h>

namespace Thunder {
namespace CEC {
    struct EXTERNAL IAdapterAccesor {
        virtual uint32_t Exchange(const logical_address_t follower, IExchange& exchange, const uint32_t waitTime=1500) = 0;

        virtual ~IAdapterAccesor() = default;
    };
} // namespace CEC
} // namespace Thunder