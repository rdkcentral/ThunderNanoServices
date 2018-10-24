#include "Module.h"
#include <interfaces/IPower.h>

namespace WPEFramework {

class PowerImplementation : public Exchange::IPower {
private:
    PowerImplementation(const PowerImplementation&) = delete;
    PowerImplementation& operator=(const PowerImplementation&) = delete;

public:
    PowerImplementation ()
    {
        TRACE(Trace::Information, (_T("PowerImplementation::Construct()")));
    }

    virtual ~PowerImplementation()
    {
        TRACE(Trace::Information, (_T("PowerImplementation::Destruct()")));
    }

    BEGIN_INTERFACE_MAP(PowerImplementation)
        INTERFACE_ENTRY(Exchange::IPower)
    END_INTERFACE_MAP

    // IPower methods
    virtual PCState GetState() const override {
        TRACE(Trace::Information, (_T("PowerImplementation::GetState() => %d"), _currentState));
        return (_currentState);
    }
    virtual PCStatus SetState(const PCState state, const uint32 waitTime) override {
        
        TRACE(Trace::Information, (_T("PowerImplementation::SetState(%d, %d) => PCSuccess"), state, waitTime));

        _currentState = state;

        return (PCSuccess);
    }
    virtual void PowerKey() override {
        TRACE(Trace::Information, (_T("PowerImplementation::PowerKey()")));
    }
    virtual void Configure(const string& settings) {
        TRACE(Trace::Information, (_T("PowerImplementation::Configure()")));
    }

private:
    PCState _currentState;
};

// The essence of making the IPower interface available. This instantiates 
// an object that can be created from the outside of the library by looking
// for the PowerImplementation class name, that realizes the IPower interface.
SERVICE_REGISTRATION(PowerImplementation, 1, 0);

}
