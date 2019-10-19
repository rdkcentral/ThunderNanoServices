
#include "Module.h"
#include "IOConnector.h"
#include <interfaces/json/JsonData_IOConnector.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::IOConnector;

    // Registration
    //

    void IOConnector::RegisterAll()
    {
        Property<Core::JSON::DecSInt32>(_T("pin"), &IOConnector::get_pin, &IOConnector::set_pin, this);
    }

    void IOConnector::UnregisterAll()
    {
         JSONRPC::Unregister(_T("pin"));
    }

    // API implementation
    //

    // Property: pin - GPIO pin value
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown pin ID given
    uint32_t IOConnector::get_pin(const string& index, Core::JSON::DecSInt32& response) const
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;

        if (index.empty() == false) {
            uint32_t pinId = atoi(index.c_str());

            if (pinId <= 0xFFFF) {
                Pins::const_iterator entry = _pins.cbegin();
                while (entry != _pins.cend()) {
                    if ((entry->first->Identifier() & 0xFFFF) == pinId) {
                        int32_t value = 0;
                        result = entry->first->Get(value);
                        response = value;
                        break;
                    } else {
                        ++entry;
                    }
                }
            }
        }

        return result;
    }

    // Property: pin - GPIO pin value
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Unknown pin ID given
    uint32_t IOConnector::set_pin(const string& index, const Core::JSON::DecSInt32& param)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;

        if (index.empty() == false) {
            uint32_t pinId = atoi(index.c_str());

            if (pinId <= 0xFFFF) {
                Pins::iterator entry = _pins.begin();
                while (entry != _pins.end()) {
                    if ((entry->first->Identifier() & 0xFFFF) == pinId) {
                        result = entry->first->Set(param.Value());
                        break;
                    } else {
                        ++entry;
                    }
                }
            }
        }

        return result;
    }

    // Event: pinactivity - Notifies about GPIO pin activity
    void IOConnector::event_pinactivity(const string& id, const int32_t& value)
    {
        ActivityParamsData params;
        params.Value = value;

        Notify(_T("activity"), params, [&](const string& designator) -> bool {
            return (id == designator);
        });
    }

} // namespace Plugin

}

