#pragma once
#include "Module.h"
#include "Sink.h"
#include <functional>

class SmartWallClockClient : protected WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IWallClock> {
private:
    using BaseClass = WPEFramework::RPC::SmartInterfaceType<WPEFramework::Exchange::IWallClock>;
    class Sink : public WPEFramework::Exchange::IWallClock::ICallback {
    public:
        Sink(SmartWallClockClient& parent)
            : _parent(parent)
        {
            printf("Sink constructed!!\n");
        }
        Sink(const Sink&) = delete;
        Sink& operator=(const Sink&) = delete;

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
            _parent.OnElapse(seconds);
            return seconds;
        }

    private:
        SmartWallClockClient& _parent;
    };

public:
    SmartWallClockClient(const uint32_t waitTime, const WPEFramework::Core::NodeId& node, const string& callsign, uint32_t wallClockUpdateTime)
        : BaseClass()
        , _interface(nullptr)
        , _sink(nullptr)
    {
        BaseClass::Open(waitTime, node, callsign);
    }
    ~SmartWallClockClient()
    {
        if (_interface != nullptr) {
            if (_sink != nullptr) {
                _sink->Release();
                _sink = nullptr;
            }
            _interface->Release();
            _interface = nullptr;
        }
        BaseClass::Close(WPEFramework::Core::infinite);
    }

    /**
     *  NOTE: For simplicity only one callback can be registered at a time
     */
    uint32_t Arm(const uint16_t time, std::function<void(uint16_t)> callback)
    {
        uint32_t result = WPEFramework::Core::ERROR_NONE;
        if (_interface != nullptr && _sink != nullptr) {
            result = _interface->Arm(time, _sink);

            if (result == WPEFramework::Core::ERROR_NONE) {
                printf("We set the callback on the wallclock. We will be updated every %d second(s)\n", time);
                _callback = callback;
            } else {
                printf("Something went wrong, the imlementation reports: %d\n", result);
            }
        }
        return result;
    }
    uint32_t Disarm()
    {
        uint32_t result = WPEFramework::Core::ERROR_NONE;

        if (_interface != nullptr && _sink != nullptr) {
            uint32_t result = _interface->Disarm(_sink);

            if (result == WPEFramework::Core::ERROR_NONE) {
                printf("We removed the callback from the wallclock. We will no longer be updated\n");
                _sink = nullptr;
            } else if (result == WPEFramework::Core::ERROR_NOT_EXIST) {
                printf("Looks like it was not Armed, or it fired already!\n");
            } else {
                printf("Something went wrong, the imlementation reports: %d\n", result);
            }
        }
        return result;
    }
    uint64_t Now() const
    {
        uint64_t now = 0;
        if (_interface != nullptr) {
            now = _interface->Now();
            printf("The Ticker is at: %llu\n", now);
        } else {
            printf("Interface is not available now\n");
        }

        return now;
    }

private:
    void OnElapse(uint16_t seconds)
    {
        _callback(seconds);
    }
    void Operational(const bool upAndRunning)
    {
        printf("Operational state of WallClock implementation: %s\n", upAndRunning ? _T("true") : _T("false"));

        if (upAndRunning) {
            _interface = BaseClass::Interface();
            if (_sink == nullptr) {
                _sink = WPEFramework::Core::Service<Sink>::Create<Sink>(*this);
            }
        } else {
            if (_interface != nullptr) {
                if (_sink != nullptr) {
                    _interface->Disarm(_sink);
                    _sink->Release();
                    _sink = nullptr;
                }
                _interface->Release();
                _interface = nullptr;
            }
        }
    }

private:
    WPEFramework::Exchange::IWallClock* _interface;
    WPEFramework::Exchange::IWallClock::ICallback* _sink;
    std::function<void(uint16_t)> _callback;
};
