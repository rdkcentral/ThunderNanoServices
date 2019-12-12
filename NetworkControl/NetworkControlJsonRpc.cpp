
#include "Module.h"
#include "NetworkControl.h"
#include <interfaces/json/JsonData_NetworkControl.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::NetworkControl;

    // Registration
    //

    void NetworkControl::RegisterAll()
    {
        Register<ReloadParamsInfo,void>(_T("reload"), &NetworkControl::endpoint_reload, this);
        Register<ReloadParamsInfo,void>(_T("request"), &NetworkControl::endpoint_request, this);
        Register<ReloadParamsInfo,void>(_T("assign"), &NetworkControl::endpoint_assign, this);
        Register<ReloadParamsInfo,void>(_T("flush"), &NetworkControl::endpoint_flush, this);
        Property<Core::JSON::ArrayType<NetworkData>>(_T("network"), &NetworkControl::get_network, nullptr, this);
        Property<Core::JSON::Boolean>(_T("up"), &NetworkControl::get_up, &NetworkControl::set_up, this);
    }

    void NetworkControl::UnregisterAll()
    {
        Unregister(_T("flush"));
        Unregister(_T("assign"));
        Unregister(_T("request"));
        Unregister(_T("reload"));
        Unregister(_T("up"));
        Unregister(_T("network"));
    }

    // API implementation
    //

    // Method: reload - Reload static and non-static network interface adapter
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavaliable network interface
    uint32_t NetworkControl::endpoint_reload(const ReloadParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Device.IsSet() == true) {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(params.Device.Value()));
            if (entry != _interfaces.end()) {
                if (entry->second.Mode() == NetworkData::ModeType::STATIC) {
                    result = Reload(entry->first, false);
                } else {
                    result = Reload(entry->first, true);
                }
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Method: request - Reload non-static network interface adapter
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavaliable network interface
    uint32_t NetworkControl::endpoint_request(const ReloadParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Device.IsSet() == true) {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(params.Device.Value()));
            if (entry != _interfaces.end()) {
                result = Reload(entry->first, true);
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Method: assign - Reload static network interface adapter
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavaliable network interface
    uint32_t NetworkControl::endpoint_assign(const ReloadParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Device.IsSet() == true) {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(params.Device.Value()));
            if (entry != _interfaces.end()) {
                result = Reload(entry->first, false);
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Method: flush - Flush network interface adapter
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavaliable network interface
    uint32_t NetworkControl::endpoint_flush(const ReloadParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Device.IsSet() == true) {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(params.Device.Value()));
            if (entry != _interfaces.end()) {
                if (entry->second.Mode() == NetworkData::ModeType::STATIC) {
                    result = Reload(entry->first, false);
                } else {
                    result = Reload(entry->first, true);
                }
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Property: network - The actual network information for targeted network interface, if network interface is not given, all network interfaces are returned
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavaliable network interface
    uint32_t NetworkControl::get_network(const string& index, Core::JSON::ArrayType<NetworkData>& response) const
    {
       uint32_t result = Core::ERROR_NONE;

        if(index != "") {
            auto entry = _interfaces.find(index);
            if (entry != _interfaces.end()) {
                NetworkData data;
                data.Interface = entry->first;
                data.Mode = entry->second.Mode();
                data.Address = entry->second.Address().HostAddress();
                data.Mask = entry->second.Address().Mask();
                data.Gateway = entry->second.Gateway().HostAddress();
                data.Broadcast = entry->second.Broadcast().HostAddress();

                response.Add(data);
            } else {
                result = Core::ERROR_UNAVAILABLE;
            }
        } else {
            auto entry = _interfaces.begin();

            while (entry != _interfaces.end()) {
                NetworkData data;
                data.Interface = entry->first;
                data.Mode = entry->second.Mode();
                data.Address = entry->second.Address().HostAddress();
                data.Mask = entry->second.Address().Mask();
                data.Gateway = entry->second.Gateway().HostAddress();
                data.Broadcast = entry->second.Broadcast().HostAddress();
                response.Add(data);

                entry++;
            }
        }

        return result;
    }

    // Property: up - Determines if interface is up
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavaliable network interface
    uint32_t NetworkControl::get_up(const string& index, Core::JSON::Boolean& response) const
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if(index != "") {
            auto entry = _interfaces.find(index);
            if (entry != _interfaces.end()) {
                Core::AdapterIterator adapter(entry->first);
                if (adapter.IsValid() == true) {
                    response = adapter.IsUp();
                    result = Core::ERROR_NONE;
                }
            }
        }

        return result;
    }

    // Property: up - Determines if interface is up
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavaliable network interface
    uint32_t NetworkControl::set_up(const string& index, const Core::JSON::Boolean& param)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(index != "") {
            std::map<const string, StaticInfo>::iterator entry(_interfaces.find(index));
            if (entry != _interfaces.end()) {
                Core::AdapterIterator adapter(entry->first);
                if (adapter.IsValid() == true) {
                    adapter.Up(param);
                    result = Core::ERROR_NONE;
                }
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Event: connectionchange - Notifies about connection status (update, connected or connectionfailed)
    void NetworkControl::event_connectionchange(const string& name, const string& address, const ConnectionchangeParamsData::StatusType& status)
    {
        ConnectionchangeParamsData params;
        params.Name = name;
        params.Address = address;
        params.Status = status;

        Notify(_T("connectionchange"), params);
    }
} // namespace Plugin

}

