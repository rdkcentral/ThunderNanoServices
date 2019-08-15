#pragma once

#include "IAdapter.h"

namespace WPEFramework {

class DeviceInfo;

class DeviceControl : public IProfileControl {
private:
    typedef std::map<std::string, bool> NotifierMap;
    typedef std::list<std::string> ParameterPrefixList;

public:
    DeviceControl(const DeviceControl&) = delete;
    DeviceControl& operator=(const DeviceControl&) = delete;

public:
    DeviceControl();
    virtual ~DeviceControl();

    virtual bool Initialize() override;
    virtual bool Deinitialize() override;

    virtual FaultCode Parameter(Data& parameter) const override;
    virtual FaultCode Parameter(const Data& parameter) override;

    virtual FaultCode Attribute(Data& parameter) const override;
    virtual FaultCode Attribute(const Data& parameter) override;

    virtual void SetCallback(IProfileControl::ICallback* cb) override;
    virtual void CheckForUpdates() override;

private:
    NotifierMap _notifier;
    ParameterPrefixList _prefixList;

    mutable Core::CriticalSection _adminLock;
    IProfileControl::ICallback* _callback;
};

}
