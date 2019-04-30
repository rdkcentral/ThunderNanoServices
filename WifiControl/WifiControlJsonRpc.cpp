
#include <interfaces/json/JsonData_WifiControl.h>
#include "WifiControl.h"
#include "Module.h"

/*
    // Copy the code below to WifiControl class definition

    private:
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_status(JsonData::WifiControl::StatusResultData& response);
        uint32_t endpoint_networks(Core::JSON::ArrayType<JsonData::WifiControl::NetworkInfo>& response);
        uint32_t endpoint_config(const JsonData::WifiControl::ConfigParamsInfo& params, Core::JSON::ArrayType<JsonData::WifiControl::ConfigInfo>& response);
        uint32_t endpoint_delete(const JsonData::WifiControl::ConfigParamsInfo& params);
        uint32_t endpoint_setconfig(const JsonData::WifiControl::SetconfigParamsData& params);
        uint32_t endpoint_store();
        uint32_t endpoint_scan();
        uint32_t endpoint_connect(const JsonData::WifiControl::ConfigParamsInfo& params);
        uint32_t endpoint_disconnect(const JsonData::WifiControl::ConfigParamsInfo& params);
        uint32_t endpoint_debug(const JsonData::WifiControl::DebugParamsData& params);
        //void event_scanresults(const string& ssid, const JsonData::WifiControl::NetworkInfo& pairing, const string& bssid, const uint32_t& frequency, const uint32_t& signal);
        void event_networkchange();
        void event_connectionchange();
*/

namespace WPEFramework {

/*
    // Copy the code below to json/Enumerations.cpp

    ENUM_CONVERSION_BEGIN(JsonData::WifiControl::TypeType)
        { JsonData::WifiControl::TypeType::UNKNOWN, _TXT("unknown") },
        { JsonData::WifiControl::TypeType::UNSECURE, _TXT("unsecure") },
        { JsonData::WifiControl::TypeType::WPA, _TXT("wpa") },
        { JsonData::WifiControl::TypeType::ENTERPRISE, _TXT("enterprise") },
    ENUM_CONVERSION_END(JsonData::WifiControl::TypeType);
*/

namespace Plugin {

    using namespace JsonData::WifiControl;

    // Registration
    //

    void WifiControl::RegisterAll()
    {
        Register<void,StatusResultData>(_T("status"), &WifiControl::endpoint_status, this);
        Register<void,Core::JSON::ArrayType<NetworkInfo>>(_T("networks"), &WifiControl::endpoint_networks, this);
        Register<ConfigParamsInfo,Core::JSON::ArrayType<ConfigInfo>>(_T("config"), &WifiControl::endpoint_config, this);
        Register<ConfigParamsInfo,void>(_T("delete"), &WifiControl::endpoint_delete, this);
        Register<SetconfigParamsData,void>(_T("setconfig"), &WifiControl::endpoint_setconfig, this);
        Register<void,void>(_T("store"), &WifiControl::endpoint_store, this);
        Register<void,void>(_T("scan"), &WifiControl::endpoint_scan, this);
        Register<ConfigParamsInfo,void>(_T("connect"), &WifiControl::endpoint_connect, this);
        Register<ConfigParamsInfo,void>(_T("disconnect"), &WifiControl::endpoint_disconnect, this);
        Register<DebugParamsData,void>(_T("debug"), &WifiControl::endpoint_debug, this);
    }

    void WifiControl::UnregisterAll()
    {
        Unregister(_T("debug"));
        Unregister(_T("disconnect"));
        Unregister(_T("connect"));
        Unregister(_T("scan"));
        Unregister(_T("store"));
        Unregister(_T("setconfig"));
        Unregister(_T("delete"));
        Unregister(_T("config"));
        Unregister(_T("networks"));
        Unregister(_T("status"));
    }

    // API implementation
    //

    // Returns the current status information.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t WifiControl::endpoint_status(StatusResultData& response)
    {
        response.Connected = _controller->Current();
        response.Scanning = _controller->IsScanning();
        return Core::ERROR_NONE;
    }

    // Returns information about available networks.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t WifiControl::endpoint_networks(Core::JSON::ArrayType<NetworkInfo>& response)
    {
        WPASupplicant::Network::Iterator list(_controller->Networks());
        while (list.Next() == true) {
            JsonData::WifiControl::NetworkInfo net;
            WifiControl::FillNetworkInfo(list.Current(), net);
            response.Add(net);
        }

        return Core::ERROR_NONE;
    }

    // Returns information about configuration of the specified network(s) (FIXME!!!).
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t WifiControl::endpoint_config(const ConfigParamsInfo& params, Core::JSON::ArrayType<ConfigInfo>& response)
    {
        const string& ssid = params.Ssid.Value();

        if (ssid.empty() == true) {
            WPASupplicant::Config::Iterator list(_controller->Configs());
            while (list.Next() == true) {
                ConfigInfo conf;
                WifiControl::FillConfig(list.Current(), conf);
                response.Add(conf);
            }
        } else {
            WPASupplicant::Config entry(_controller->Get(ssid));
            if (entry.IsValid() == true) {
                ConfigInfo config;
                WifiControl::FillConfig(entry, config);
                response.Add(config);
            }
        }
        return Core::ERROR_NONE;
    }

    // Forgets configuration of the specified network.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t WifiControl::endpoint_delete(const ConfigParamsInfo& params)
    {
        _controller->Destroy(params.Ssid.Value());
        return Core::ERROR_NONE;
    }

    // Updates or creates a configuration for the specified network.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Config does not exist
    //  - ERROR_INCOMPLETE_CONFIG: Passed in config is invalid
    uint32_t WifiControl::endpoint_setconfig(const SetconfigParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const string& ssid = params.Ssid.Value();
        if (ssid.empty() == true) {
            result = Core::ERROR_UNKNOWN_KEY;
        } else {
            WPASupplicant::Config profile(_controller->Create(ssid));
            UpdateConfig(profile, params.Config);
        }
        return result;
    }

    // Stores configurations in the permanent storage.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_WRITE_ERROR: Returned when the operation failed
    uint32_t WifiControl::endpoint_store()
    {
        uint32_t result = Core::ERROR_WRITE_ERROR;
        Core::File configFile(_configurationStore);

        if (configFile.Create() == true) {
            WifiControl::ConfigList configs;
            WPASupplicant::Config::Iterator list(_controller->Configs());
            configs.Set(list);
            if (configs.ToFile(configFile) == true)
                result = Core::ERROR_NONE;
        }

        return result;
    }

    // Searches for an available networks.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INPROGRESS: Operation in progress
    //  - ERROR_UNAVAILABLE: Returned when scanning is not available for some reason
    uint32_t WifiControl::endpoint_scan()
    {
        return _controller->Scan();
    }

    // Attempt connection to the specified network.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Returned when the network with a the given SSID doesn't exists
    //  - ERROR_ASYNC_ABORTED: Returned when connection attempt fails for other reasons
    uint32_t WifiControl::endpoint_connect(const ConfigParamsInfo& params)
    {
        return _controller->Connect(params.Ssid.Value());
    }

    // Disconnects from the specified network.
    // Return codes:
    //  - ERROR_UNKNOWN_KEY: Returned when the network with a the given SSID doesn't exists
    //  - ERROR_ASYNC_ABORTED: Returned when disconnection attempt fails for other reasons
    uint32_t WifiControl::endpoint_disconnect(const ConfigParamsInfo& params)
    {
        return _controller->Disconnect(params.Ssid.Value());
    }

    // Sets specified debug level.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Returned when the operation is unavailable
    uint32_t WifiControl::endpoint_debug(const DebugParamsData& params)
    {
        return _controller->Debug(params.Level.Value());
    }

    // Signals that the scan operation has finished and carries results of it.
    void WifiControl::event_scanresults(const Core::JSON::ArrayType<JsonData::WifiControl::NetworkInfo>& list)
    {
        Notify(_T("scanresults"), list);
    }

    // Informs that something has changed with the network e.
    void WifiControl::event_networkchange()
    {
        Notify(_T("networkchange"));
    }

    // Notifies about connection state change i.
    void WifiControl::event_connectionchange(const string& ssid)
    {
        Core::JSON::String params;
        params = ssid;
        Notify(_T("connectionchange"), params);
    }

} // namespace Plugin

}

