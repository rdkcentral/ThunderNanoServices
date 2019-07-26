#pragma once

#include "Module.h"

#include <libdropbear.h>

namespace WPEFramework {
namespace Plugin {

    class DropbearServerImplementation {
    public:
        DropbearServerImplementation() = default;
        ~DropbearServerImplementation() = default;

        uint32_t StartService(const std::string& host_keys, const std::string& port_flag, const std::string& port)
        {
            uint32_t result = Core::ERROR_NONE;

            TRACE(Trace::Information, (_T("Starting Dropbear Service with options as: %s %s %s"), host_keys.c_str(), port_flag.c_str(), port.c_str()));
	    // TODO: Check the return value and based on that change result
	    activate_dropbear(const_cast<char*>(host_keys.c_str()), const_cast<char*>(port_flag.c_str()), const_cast<char*>(port.c_str())); 

            return result;
        }

        uint32_t StopService()
        {
            uint32_t result = Core::ERROR_NONE;

            TRACE(Trace::Information, (_T("Stoping Dropbear Service")));
	    deactivate_dropbear(); //TODO: Check the return value and based on that change result

            return result;
        }

        uint32_t GetTotalSessions()
        {
            uint32_t result = Core::ERROR_NONE;

	    uint32_t count = get_total_sessions_served();
            TRACE(Trace::Information, (_T("Get total number of SSH client sessions handled by Dropbear service: %d"), count));

            return count;
        }

        uint32_t GetSessionsCount()
        {
            uint32_t result = Core::ERROR_NONE;

	    uint32_t count = get_active_sessions_count();
            TRACE(Trace::Information, (_T("Get total number of active SSH client sessions managed by Dropbear service: %d"), count));

            return count;
        }

        uint32_t GetSessionsInfo(Core::JSON::ArrayType<JsonData::DropbearServer::SessioninfoResultData>& response) const
        {
            uint32_t result = Core::ERROR_NONE;

	    
	    int32_t count = get_active_sessions_count();
            TRACE(Trace::Information, (_T("Get details of (%d)active SSH client sessions managed by Dropbear service"), count));

	    struct client_info *info = static_cast<struct client_info*>(::malloc(sizeof(struct client_info) * count));

	    get_active_sessions_info(info, count);

	    for(int32_t i=0; i<count; i++)
            {
	            TRACE(Trace::Information, (_T("Count: %d index: %d pid: %d IP: %s Timestamp: %s"), 
					    count, i, info[i].pid, info[i].ipaddress, info[i].timestamp));

	            JsonData::DropbearServer::SessioninfoResultData newElement;

	            newElement.IpAddress = info[i].ipaddress;
	            newElement.Pid = std::to_string(info[i].pid);
		    newElement.TimeStamp = info[i].timestamp;

        	    JsonData::DropbearServer::SessioninfoResultData& element(response.Add(newElement));
	    }
	    ::free(info);

            return result;
        }

        uint32_t CloseClientSession(const std::string& client_pid)
        {
            uint32_t result = Core::ERROR_NONE;

            TRACE(Trace::Information, (_T("closing client session with PID: %s"), client_pid.c_str()));
	    result = close_client_session(std::stoi(client_pid));

            return result;
        }

    private:
        DropbearServerImplementation(const DropbearServerImplementation&) = delete;
        DropbearServerImplementation& operator=(const DropbearServerImplementation&) = delete;
    };

} // namespace Plugin
} // namespace WPEFramework
