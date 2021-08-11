#pragma once
#include "Module.h"
#include "Sink.h"

class SmartWallClockClient : protected WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IWallClock> {
private:
    using BaseClass = WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IWallClock>;

public:
    SmartWallClockClient(const uint32_t waitTime, const WPEFramework::Core::NodeId& node, const string& callsign, uint32_t wallClockUpdateTime)
        : BaseClass()
        , _wallClockUpdateTime(wallClockUpdateTime)
        , _sink(nullptr)
    {
        BaseClass::Open(waitTime, node, callsign);
    }
    ~SmartWallClockClient()
    {
        BaseClass::Close(WPEFramework::Core::infinite);
    }

private:
    void Operational(const bool upAndRunning)
    {
        printf("Operational state of WallClock implementation: %s\n", upAndRunning ? _T("true") : _T("false"));

        if (upAndRunning) {
            _interface = BaseClass::Interface();
            if (_interface != nullptr && _sink == nullptr) {

                _sink = WPEFramework::Core::Service<Sink>::Create<Sink>();
                uint32_t result = _interface->Arm(_wallClockUpdateTime, _sink);

                if (result == WPEFramework::Core::ERROR_NONE) {
                    printf("We set the callback on the wallclock. We will be updated every %d second(s)\n", _wallClockUpdateTime);
                } else {
                    printf("Something went wrong, the imlementation reports: %d\n", result);
                }
            }
        } else {
            if (_interface != nullptr && _sink != nullptr) {

                uint32_t result = _interface->Disarm(_sink);
                _sink->Release();
                _sink = nullptr;

                if (result == WPEFramework::Core::ERROR_NONE) {
                    printf("We removed the callback from the wallclock. We will no longer be updated\n");
                } else if (result == WPEFramework::Core::ERROR_NOT_EXIST) {
                    printf("Looks like it was not Armed, or it fired already!\n");
                } else {
                    printf("Something went wrong, the imlementation reports: %d\n", result);
                }
                _interface->Release();
            }
        }
    }

private:
    uint32_t _wallClockUpdateTime;
    WPEFramework::Exchange::IWallClock* _interface;
    Sink* _sink;
};
