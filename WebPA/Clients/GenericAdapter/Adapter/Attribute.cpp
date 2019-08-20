#include "Attribute.h"

namespace WPEFramework {
namespace WebPA {

Attribute::Attribute(Notifier* notifier, Handler* handler)
    : _notifier(notifier)
    , _handler(handler)
    , _adminLock()
{
}

Attribute::~Attribute()
{
}

void Attribute::Values(const std::vector<std::string>& parameterNames, std::map<Data, WebPAStatus>& attributesList) const
{
    for (auto& name: parameterNames) {
        Data attribute(name, static_cast<bool>(0));
        const WebPAStatus ret = Values(name, attribute);
        if (ret == WEBPA_SUCCESS) {
            TRACE(Trace::Information, (_T( "Parameter Name: %s has attribute"), name.c_str()));
        }
        attributesList.insert(std::make_pair(attribute, ret));
    }
}

WebPAStatus Attribute::Values(const std::map<std::string, Data>& attributes, std::vector<WebPAStatus>& status)
{
    TRACE(Trace::Information, (_T("[%s:%s:%d] Inside Values, Param Count = %d"), __FILE__, __FUNCTION__, __LINE__, attributes.size()));
    WebPAStatus ret = WEBPA_FAILURE;

    for (auto& attribute: attributes) {
        ret = Values(attribute.first, attribute.second);
        status.push_back(ret);
        if (ret != WEBPA_SUCCESS) {
            break;
        }
    }
    return ret;
}

const WebPAStatus Attribute::Values(const std::string& parameterName, Data& attribute) const
{
    WebPAStatus ret = WEBPA_FAILURE;

    std::vector<std::string> notifyParameters;

    // Read Notify configuration file
    int status = _notifier->Parameters(notifyParameters);
    if (status == Core::ERROR_NONE) {
        // Check if parameterName is in the list of notification parameters and check if the parameter is one among them
        for (auto& notifyParameter: notifyParameters) {
            if (!parameterName.compare(notifyParameter)) {
                Data param(parameterName);
                _adminLock.Lock();
                ret = Utils::ConvertFaultCodeToWPAStatus((static_cast<const Handler&>(*_handler)).Attribute(param));
                _adminLock.Unlock();
                if (ret == WEBPA_SUCCESS) {
                    attribute = param;
                }
                break;
            }
        }
    }
    return ret;
}

WebPAStatus Attribute::Values(const std::string& parameterName, const Data& attribute)
{
    WebPAStatus ret = WEBPA_FAILURE;

    std::vector<std::string> notifyParameters;

    // Read Notify configuration file
    uint32_t status = _notifier->Parameters(notifyParameters);

    // Enable only for notification parameters in the config file
    if (status == Core::ERROR_NONE) {
        for (auto& notifyParameter: notifyParameters) {
            if (!parameterName.compare(notifyParameter)) {
                TRACE(Trace::Information, (_T("[%s:%s:%d] Inside Values, Param:%s is found"), __FILE__, __FUNCTION__, __LINE__, parameterName.c_str()));
                const Data param(parameterName, attribute.Value());
                _adminLock.Lock();
                ret = Utils::ConvertFaultCodeToWPAStatus(_handler->Attribute(param));
                _adminLock.Unlock();
                break;
            }
        }
    }
    return ret;
}

} // WebPA
} // WPEFramework
