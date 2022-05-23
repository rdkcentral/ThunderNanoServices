#pragma once

#include <CECTypes.h>
#include <CECMessage.h>

namespace WPEFramework {
namespace CEC {
    struct EXTERNAL IAdapterAccessor {
        virtual uint32_t Exchange(const logical_address_t follower, IExchange& exchange, const uint32_t waitTime=1500) = 0;

        virtual ~IAdapterAccessor() = default;
    };
} // namespace CEC
} // namespace WPEFramework