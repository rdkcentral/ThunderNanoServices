
#include "Module.h"
#include "WifiControl.h"
#include <interfaces/json/JsonData_WifiControl.h>

/*
    // Copy the code below to WifiControl class definition
    // Note: The WifiControl class must inherit from PluginHost::JSONRPC

    private:
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_delete(const JsonData::WifiControl::DeleteParamsInfo& params);
        uint32_t endpoint_store();
        uint32_t endpoint_scan();
        uint32_t endpoint_connect(const JsonData::WifiControl::DeleteParamsInfo& params);
        uint32_t endpoint_disconnect(const JsonData::WifiControl::DeleteParamsInfo& params);
        uint32_t get_status(JsonData::WifiControl::StatusData& response) const;
        uint32_t get_networks(Core::JSON::ArrayType<JsonData::WifiControl::NetworkInfo>& response) const;
        uint32_t get_config(const string& index, JsonData::WifiControl::ConfigData& response) const;
        uint32_t set_config(const string& index, const JsonData::WifiControl::ConfigData& param);
        uint32_t get_debug(Core::JSON::DecUInt32& response) const;
        uint32_t set_debug(const Core::JSON::DecUInt32& param);
        void event_scanresults(const Core::JSON::ArrayType<JsonData::WifiControl::NetworkInfo>& list);
        void event_networkchange();
        void event_connectionchange(const string& ssid);
*/

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::WifiControl;

    // Registration
    //

    void WifiControl::RegisterAll()
    {
        Register<DeleteParamsInfo,void>(_T("delete"), &WifiControl::endpoint_delete, this);
        Register<void,void>(_T("store"), &WifiControl::endpoint_store, this);
        Register<void,void>(_T("scan"), &WifiControl::endpoint_scan, this);
        Register<DeleteParamsInfo,void>(_T("connect"), &WifiControl::endpoint_connect, this);
        Register<DeleteParamsInfo,void>(_T("disconnect"), &WifiControl::endpoint_disconnect, this);
        Property<StatusData>(_T("status"), &WifiControl::get_status, nullptr, this);
        Property<Core::JSON::ArrayType<NetworkInfo>>(_T("networks"), &WifiControl::get_networks, nullptr, this);
        Property<Core::JSON::ArrayType<ConfigInfo>>(_T("configs"), &WifiControl::get_configs, nullptr, this);
        Property<ConfigInfo>(_T("config"), &WifiControl::get_config, &WifiControl::set_config, this);
        Property<Core::JSON::DecUInt32>(_T("debug"), nullptr, &WifiControl::set_debug, this);
    }

    void WifiControl::UnregisterAll()
    {
        Unregister(_T("disconnect"));
        Unregister(_T("connect"));
        Unregister(_T("scan"));
        Unregister(_T("store"));
        Unregister(_T("delete"));
        Unregister(_T("debug"));
        Unregister(_T("config"));
        Unregister(_T("configs"));
        Unregister(_T("networks"));
        Unregister(_T("status"));
    }


    // API implementation
    //

    // Method: delete - Forgets configuration of the specified network
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t WifiControl::endpoint_delete(const DeleteParamsInfo& params)
    {
        _controller->Destroy(params.Ssid.Value());
        return Core::ERROR_NONE;
    }

    // Method: store - Stores configurations in the permanent storage
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
            if (configs.IElement::ToFile(configFile) == true)
                result = Core::ERROR_NONE;
        }

        return result;
    }

    // Method: scan - Searches for an available networks
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INPROGRESS: Returned when scan is already in progress
    //  - ERROR_UNAVAILABLE: Returned when scanning is not available for some reason
    uint32_t WifiControl::endpoint_scan()
    {
        return _controller->Scan();
    }

    // Method: connect - Attempt connection to the specified network
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Returned when the network with a the given SSID doesn't exists
    //  - ERROR_ASYNC_ABORTED: Returned when connection attempt fails for other reasons
    uint32_t WifiControl::endpoint_connect(const DeleteParamsInfo& params)
    {
        const string& ssid = params.Ssid.Value();

        return _controller->Connect(ssid);
    }

    // Method: disconnect - Disconnects from the specified network
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Returned when the network with a the given SSID doesn't exists
    //  - ERROR_ASYNC_ABORTED: Returned when disconnection attempt fails for other reasons
    uint32_t WifiControl::endpoint_disconnect(const DeleteParamsInfo& params)
    {
        const string& ssid = params.Ssid.Value();

        return _controller->Disconnect(ssid);
    }

    // Property: status - Returns the current status information
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t WifiControl::get_status(StatusData& response) const
    {
        response.Connected = _controller->Current();
        response.Scanning = _controller->IsScanning();
        
        return Core::ERROR_NONE;
    }

    // Property: networks - Returns information about available networks
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t WifiControl::get_networks(Core::JSON::ArrayType<NetworkInfo>& response) const
    {
        WPASupplicant::Network::Iterator list(_controller->Networks());
        while (list.Next() == true) {
            JsonData::WifiControl::NetworkInfo net;
            WifiControl::FillNetworkInfo(list.Current(), net);
            response.Add(net);
        }

        return Core::ERROR_NONE;
    }

    // Property: config - Returns information about configuration of the specified network(s) (FIXME!!!)
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Config does not exist
    //  - ERROR_INCOMPLETE_CONFIG: Passed in config is invalid
    uint32_t WifiControl::get_config(const string& ssid, ConfigInfo& response) const
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;

        if (ssid.empty() == false) {
            WPASupplicant::Config entry(_controller->Get(ssid));
            if (entry.IsValid() == true) {
                WifiControl::FillConfig(entry, response);
                result = Core::ERROR_NONE;
            }
        }
        return result;
    }

    // Property: configs - Network configuration (FIXME!!!)
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Config does not exist
    uint32_t WifiControl::get_configs(Core::JSON::ArrayType<ConfigInfo>& response) const
    {
        WPASupplicant::Config::Iterator list(_controller->Configs());
        while (list.Next() == true) {
            ConfigInfo conf;
            WifiControl::FillConfig(list.Current(), conf);
            response.Add(conf);
        }

        return Core::ERROR_NONE;
    }

    // Property: config - Returns information about configuration of the specified network(s) (FIXME!!!)
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Config does not exist
    //  - ERROR_INCOMPLETE_CONFIG: Passed in config is invalid
    uint32_t WifiControl::set_config(const string& ssid, const ConfigInfo& param)
    {
        uint32_t result = Core::ERROR_NONE;

        if (ssid.empty() == true) {
            result = Core::ERROR_UNKNOWN_KEY;
        } else {
            WPASupplicant::Config profile(_controller->Create(ssid));
            UpdateConfig(profile, param);
        }
        return result;
    }

    // Property: debug - Sets specified debug level
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Returned when the operation is unavailable
    uint32_t WifiControl::set_debug(const Core::JSON::DecUInt32& param)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (param.IsSet()) {
            result = _controller->Debug(param.Value());
        }

        return result;
    }

    // Event: scanresults - Signals that the scan operation has finished and carries results of it
    void WifiControl::event_scanresults(const Core::JSON::ArrayType<JsonData::WifiControl::NetworkInfo>& list)
    {
        Notify(_T("scanresults"), list);
    }

    // Event: networkchange - Informs that something has changed with the network e
    void WifiControl::event_networkchange()
    {
        Notify(_T("networkchange"));
    }

    // Event: connectionchange - Notifies about connection state change i
    void WifiControl::event_connectionchange(const string& ssid)
    {
        Core::JSON::String params;
        params = ssid;

        Notify(_T("connectionchange"), params);
    }

} // namespace Plugin

}

