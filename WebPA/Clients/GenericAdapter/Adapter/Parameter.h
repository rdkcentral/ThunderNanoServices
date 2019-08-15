#pragma once

#include "Handler.h"
#include "Utils.h"
#include "WalDB.h"

namespace WPEFramework {

namespace WebPA {

typedef enum
{
    WEBPA_SET = 0,
    WEBPA_ATOMIC_SET,
    WEBPA_ATOMIC_SET_XPC
} WEBPA_SET_TYPE;

class Parameter {

public:
    Parameter() = delete;
    Parameter(const Parameter&) = delete;
    Parameter& operator= (const Parameter&) = delete;

public:
    Parameter(Handler* handler, WalDB* walDB);
    virtual ~Parameter();

    const void Values(const std::vector<std::string>& parameterNames, std::map<std::vector<Data>, WebPAStatus>& parametersList) const;
    WebPAStatus Values(const std::vector<Data>& parameters, std::vector<WebPAStatus>& status);

private:
    const WebPAStatus Values(const std::string& parameterName, std::vector<Data>& parameters) const;
    WebPAStatus Values(const Data& parameter);

private:
    WalDB* _walDB;
    Handler* _handler;

    mutable Core::CriticalSection _adminLock;
};

} // WebPA
} // WPEFramework
