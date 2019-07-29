#include "Module.h"
#include "Monitor.h"
#include <interfaces/json/JsonData_Monitor.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::Monitor;

    // Registration
    //

    void Monitor::RegisterAll()
    {
        Register<RestartlimitsParamsData,void>(_T("restartlimits"), &Monitor::endpoint_restartlimits, this);
        Register<ResetstatsParamsData,InfoInfo>(_T("resetstats"), &Monitor::endpoint_resetstats, this);
        Property<Core::JSON::ArrayType<InfoInfo>>(_T("status"), &Monitor::get_status, nullptr, this);
    }

    void Monitor::UnregisterAll()
    {
        Unregister(_T("resetstats"));
        Unregister(_T("restartlimits"));
        Unregister(_T("status"));
    }

    // API implementation
    //

    // Method: restartlimits - Sets new restart limits for a plugin
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Monitor::endpoint_restartlimits(const RestartlimitsParamsData& params)
    {
        const string& callsign = params.Callsign.Value();
        _monitor->Update(
            callsign,
            params.Operational.Window.Value(), params.Operational.Limit.Value(),
            params.Memory.Window.Value(), params.Memory.Limit.Value());
        return Core::ERROR_NONE;
    }

    // Method: resetstats - Resets memory and process statistics for a single plugin watched by the Monitor
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Monitor::endpoint_resetstats(const ResetstatsParamsData& params, InfoInfo& response)
    {
        const string& callsign = params.Callsign.Value();

        Core::JSON::ArrayType<InfoInfo> info;
        _monitor->Snapshot(callsign, &info);
        if (info.Length() == 1) {
            _monitor->Reset(callsign);
            response = info[0];
        }
        return Core::ERROR_NONE;
    }

    // Property: status - The memory and process statistics either for a single plugin or all plugins watched by the Monitor
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Monitor::get_status(const string& index, Core::JSON::ArrayType<InfoInfo>& response) const
    {
        const string& callsign = index;
        _monitor->Snapshot(callsign, &response);
        return Core::ERROR_NONE;
    }

    // Event: action - Signals action taken by the monitor
    void Monitor::event_action(const string& callsign, const string& action, const string& reason)
    {
        ActionParamsData params;
        params.Callsign = callsign;
        params.Action = action;
        params.Reason = reason;

        Notify(_T("action"), params);
    }
} // namespace Plugin
}

