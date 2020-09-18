/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
        Property<Core::JSON::ArrayType<NetworkData>>(_T("network"), &NetworkControl::get_network, &NetworkControl::set_network, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("dns"), &NetworkControl::get_dns, &NetworkControl::set_dns, this);
        Property<Core::JSON::Boolean>(_T("up"), &NetworkControl::get_up, &NetworkControl::set_up, this);
    }

    void NetworkControl::UnregisterAll()
    {
        Unregister(_T("flush"));
        Unregister(_T("assign"));
        Unregister(_T("request"));
        Unregister(_T("reload"));
        Unregister(_T("up"));
        Unregister(_T("dns"));
        Unregister(_T("network"));
    }

    // API implementation
    //

    // Method: reload - Reloads a static or non-static network interface adapter
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavailable network interface
    uint32_t NetworkControl::endpoint_reload(const ReloadParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Interface.IsSet() == true) {
            std::map<const string, DHCPEngine>::iterator entry(_dhcpInterfaces.find(params.Interface.Value()));
            if (entry != _dhcpInterfaces.end()) {
                if (entry->second.Info().Mode() == NetworkData::ModeType::STATIC) {
                    result = Reload(entry->first, false);
                } else {
                    result = Reload(entry->first, true);
                }
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Method: request - Reloads a non-static network interface adapter
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavaliable network interface
    uint32_t NetworkControl::endpoint_request(const ReloadParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Interface.IsSet() == true) {
            std::map<const string, DHCPEngine>::iterator entry(_dhcpInterfaces.find(params.Interface.Value()));
            if (entry != _dhcpInterfaces.end()) {
                result = Reload(entry->first, true);
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Method: assign - Reloads a static network interface adapter
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavaliable network interface
    uint32_t NetworkControl::endpoint_assign(const ReloadParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Interface.IsSet() == true) {
            std::map<const string, DHCPEngine>::iterator entry(_dhcpInterfaces.find(params.Interface.Value()));
            if (entry != _dhcpInterfaces.end()) {
                result = Reload(entry->first, false);
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Method: flush - Flushes a network interface adapter
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavaliable network interface
    uint32_t NetworkControl::endpoint_flush(const ReloadParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(params.Interface.IsSet() == true) {
            std::map<const string, DHCPEngine>::iterator entry(_dhcpInterfaces.find(params.Interface.Value()));
            if (entry != _dhcpInterfaces.end()) {
                if (entry->second.Info().Mode() == NetworkData::ModeType::STATIC) {
                    result = Reload(entry->first, false);
                } else {
                    result = Reload(entry->first, true);
                }
            }
        }

        _adminLock.Unlock();

        return result;
    }

    // Property: network - Network information
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavailable network interface
    uint32_t NetworkControl::get_network(const string& index, Core::JSON::ArrayType<NetworkData>& response) const
    {
        return NetworkInfo(index, response);
    }

    // Property: network - Network information
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavailable network interface
    uint32_t NetworkControl::set_network(const string& index, const Core::JSON::ArrayType<NetworkData>& param)
    {
        return NetworkInfo(index, param);
    }

    // Property: dns - DNS addresses
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t NetworkControl::get_dns(Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        return DNS(response);
    }

    // Property: dns - DNS addresses
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t NetworkControl::set_dns(const Core::JSON::ArrayType<Core::JSON::String>& param)
    {
        return DNS(param);
    }

    // Property: up - Interface up status
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavailable network interface
    uint32_t NetworkControl::get_up(const string& index, Core::JSON::Boolean& response) const
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if(index != "") {
            auto entry = _dhcpInterfaces.find(index);
            if (entry != _dhcpInterfaces.end()) {
                Core::AdapterIterator adapter(entry->first);
                if (adapter.IsValid() == true) {
                    response = adapter.IsUp();
                    result = Core::ERROR_NONE;
                }
            }
        }

        return result;
    }

    // Property: up - Interface up status
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unavailable network interface
    uint32_t NetworkControl::set_up(const string& index, const Core::JSON::Boolean& param)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        if(index != "") {
            std::map<const string, DHCPEngine>::iterator entry(_dhcpInterfaces.find(index));
            if (entry != _dhcpInterfaces.end()) {
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

