#include "Module.h"
#include "Monitor.h"
#include <interfaces/json/JsonData_Monitor.h>

/*
    // Copy the code below to Monitor class definition

    private:
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_status(const JsonData::Monitor::StatusParamsInfo& params, Core::JSON::ArrayType<JsonData::Monitor::InfoInfo>& response);
        uint32_t endpoint_resetstats(const JsonData::Monitor::StatusParamsInfo& params, JsonData::Monitor::InfoInfo& response);
        uint32_t endpoint_restartlimits(const JsonData::Monitor::RestartlimitsParamsData& params);
        void event_action(const string& callsign, const string& action, const string& reason);
*/

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::Monitor;

    // Registration
    //

    void Monitor::RegisterAll()
    {
        Register<StatusParamsInfo,Core::JSON::ArrayType<InfoInfo>>(_T("status"), &Monitor::endpoint_status, this);
        Register<StatusParamsInfo,InfoInfo>(_T("resetstats"), &Monitor::endpoint_resetstats, this);
        Register<RestartlimitsParamsData,void>(_T("restartlimits"), &Monitor::endpoint_restartlimits, this);
    }

    void Monitor::UnregisterAll()
    {
        Unregister(_T("restartlimits"));
        Unregister(_T("resetstats"));
        Unregister(_T("status"));
    }

    // API implementation
    //

    // Returns the memory and process statistics either for a single plugin or all plugins watched by the Monitor.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Monitor::endpoint_status(const StatusParamsInfo& params, Core::JSON::ArrayType<InfoInfo>& response)
    {
        const string& callsign = params.Callsign.Value();
        _monitor->Snapshot(callsign, &response);
        return Core::ERROR_NONE;
    }

    // Resets memory and process statistics for a single plugin watched by the Monitor.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Monitor::endpoint_resetstats(const StatusParamsInfo& params, InfoInfo& response)
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

    // Sets new restart limits for a plugin.
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

    // Signals action taken by the monitor.
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

