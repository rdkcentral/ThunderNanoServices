#pragma once

#include "Handler.h"
#include "Notifier.h"

namespace WPEFramework {
namespace WebPA {

class Attribute {
private:
    static constexpr const uint32_t MaxParameterLen = 512;
    static constexpr const uint8_t MaxParameterValueLen = 128;

public:
    Attribute() = delete;
    Attribute(const Attribute&) = delete;
    Attribute& operator= (const Attribute&) = delete;

public:
    Attribute(Notifier* notifier, Handler* handler);
    virtual ~Attribute();

    void Values(const std::vector<std::string>& parameterNames, std::map<Data, WebPAStatus>& attributesList) const;
    WebPAStatus Values(const std::map<std::string, Data>& attributes, std::vector<WebPAStatus>& status);

private:
    const WebPAStatus Values(const std::string& parameterName, Data& attributes) const;
    WebPAStatus Values(const std::string& parameterName, const Data& attribute);

private:
    Notifier* _notifier;
    Handler* _handler;

    mutable Core::CriticalSection _adminLock;
};

} // WebPA
} // WPEFramework
