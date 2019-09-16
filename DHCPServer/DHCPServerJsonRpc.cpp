
#include "Module.h"
#include "DHCPServer.h"
#include "DHCPServerImplementation.h"
#include <interfaces/json/JsonData_DHCPServer.h>


namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::DHCPServer;

    namespace
    {
        void Fill(ServerData& data, const DHCPServerImplementation& server)
        {
            data.Interface = server.Interface();
            data.Active = server.IsActive();
            if (server.IsActive()) {
                data.Begin = server.BeginPool().HostAddress();
                data.End = server.EndPool().HostAddress();
                data.Router = server.Router().HostAddress();

                auto it(server.Leases());
                while (it.Next() == true) {
                    auto& lease = data.Leases.Add();
                    lease.Name = it.Current().Id().Text();
                    lease.Ip = it.Current().Address().HostAddress();
                    if (it.Current().Expiration() != 0) {
                        lease.Expires = Core::Time(it.Current().Expiration()).ToISO8601(true);
                    }
                }
            }
        }
    }

    // Registration
    //

    void DHCPServer::RegisterAll()
    {
        Register<ActivateParamsInfo,void>(_T("activate"), &DHCPServer::endpoint_activate, this);
        Register<ActivateParamsInfo,void>(_T("deactivate"), &DHCPServer::endpoint_deactivate, this);
        Property<Core::JSON::ArrayType<ServerData>>(_T("status"), &DHCPServer::get_status, nullptr, this);
    }

    void DHCPServer::UnregisterAll()
    {
        Unregister(_T("deactivate"));
        Unregister(_T("activate"));
        Unregister(_T("status"));
    }

    // API implementation
    //

    // Method: activate - Activates a DHCP server
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_GENERAL: Failed to activate server
    //  - ERROR_UNKNOWN_KEY: Invalid interface name given
    //  - ERROR_ILLEGAL_STATE: Server is already activated
    uint32_t DHCPServer::endpoint_activate(const ActivateParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& interface = params.Interface.Value();

        if (interface.empty() == false) {
            auto server(_servers.find(interface));
            if (server != _servers.end()) {
                if (server->second.IsActive() == false) {
                    result = server->second.Open();
                }
                else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }
            }
        }

        return result;
    }

    // Method: deactivate - Deactivates a DHCP server
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_GENERAL: Failed to deactivate server
    //  - ERROR_UNKNOWN_KEY: Invalid interface name given
    //  - ERROR_ILLEGAL_STATE: Server is not activated
    uint32_t DHCPServer::endpoint_deactivate(const ActivateParamsInfo& params)
    {
        uint32_t result = Core::ERROR_UNKNOWN_KEY;
        const string& interface = params.Interface.Value();

        if (interface.empty() == false) {
            auto server(_servers.find(interface));
            if (server != _servers.end()) {
                if (server->second.IsActive() == true) {
                    result = server->second.Close();
                } else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }
            }
        }

        return result;
    }

    // Property: status - Server status
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Invalid server name given
    uint32_t DHCPServer::get_status(const string& index, Core::JSON::ArrayType<ServerData>& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        if (index.empty()) {
            auto it = _servers.begin();
            while (it != _servers.end()) {
                ServerData info;
                Fill(info, it->second);
                response.Add(info);
                it++;
            }
        } else {
            auto it(_servers.find(index));
            if (it != _servers.end()) {
                ServerData info;
                Fill(info, it->second);
                response.Add(info);
            } else {
                result = Core::ERROR_UNKNOWN_KEY;
            }
        }

        return result;
    }

} // namespace Plugin

}

