#pragma once

#include "Module.h"

namespace WPEFramework {

namespace Exchange {

    enum example_ids {
        ID_WALLCLOCK = 0x80001000,
        ID_WALLCLOCK_NOTIFICATION = 0x80001001
    };

    struct IWallClock : virtual public Core::IUnknown {
        enum { ID = ID_WALLCLOCK };

        struct ICallback : virtual public Core::IUnknown {
            enum { ID = ID_WALLCLOCK_NOTIFICATION };

            virtual ~ICallback() {}

            virtual void Elapsed (const uint32_t seconds) = 0;
        };

        virtual ~IWallClock() {}

        virtual uint32_t Callback(ICallback*) = 0;

        virtual void Interval(const uint32_t) = 0;
        virtual uint32_t Interval() const = 0;
        virtual uint64_t Now() const = 0;
    };

}
}
