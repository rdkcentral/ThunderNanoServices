#pragma once

#include <CECTypes.h>

namespace Thunder {
namespace CEC {
    struct EXTERNAL IDeviceAdapter {
        struct EXTERNAL INotification {
            virtual ~INotification() = default;
            virtual uint8_t Received(const cec_adapter_role_t follower, const logical_address_t initiator, const uint8_t length, const uint8_t data[]) = 0;
        };

        virtual ~IDeviceAdapter() = default;

        virtual uint32_t Register(INotification* notification) = 0;
        virtual uint32_t Unregister(INotification* notification) = 0;

        virtual uint32_t Transmit(const cec_adapter_role_t initiator, const logical_address_t follower, const uint8_t length, const uint8_t data[]) = 0;
    }; // class adapter
} // namespace CEC
} // namespace Thunder