#pragma once
#include "Module.h"

class Sink : public WPEFramework::Exchange::IWallClock::ICallback {
public:
    Sink(const Sink&) = delete;
    Sink& operator=(const Sink&) = delete;

    Sink()
    {
        printf("Sink constructed!!\n");
    };
    ~Sink() override
    {
        printf("Sink destructed!!\n");
    }

public:
    BEGIN_INTERFACE_MAP(Sink)
    INTERFACE_ENTRY(WPEFramework::Exchange::IWallClock::ICallback)
    END_INTERFACE_MAP

    uint16_t Elapsed(const uint16_t seconds) override
    {
        printf("The wallclock reports that %d seconds have elapsed since we where armed\n", seconds);
        return seconds;
    }
};
