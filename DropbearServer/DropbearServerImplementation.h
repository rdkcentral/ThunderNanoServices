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
	    activate_dropbear(host_keys.c_str(), port_flag.c_str(), port.c_str()); // TODO: Check the return value and based on that change result

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

            TRACE(Trace::Information, (_T("Get details of active SSH client sessions managed by Dropbear service")));
	    
	    int32_t count = get_active_sessions_count();

	    struct child_info *info = static_cast<struct child_info*>(::malloc(sizeof(struct child_info) * count);

	    get_active_sessions_info(info, count);

	    for(int32_t i=0; i<count; i++)
            {
	            TRACE(Trace::Information, (_T("Count: %d index: %d pid: %s IP: %s Timestamp: %s"), count, i, info[i].pid, info[i].ip, info[i].timestamp));

	            JsonData::DropbearServer::SessioninfoResultData newElement;

	            newElement.ip = info[i].ip;
	            newElement.pid = info[i].pid;
		    newElement.timestamp = info[i].timestamp;

        	    JsonData::DropbearServer::SessioninfoResultData& element(response.Add(newElement));
	    }
	    ::free(info);

            return result;
        }

        uint32_t CloseClientSession(uint32_t client_pid)
        {
            uint32_t result = Core::ERROR_NONE;

            TRACE(Trace::Information, (_T("closing client session with PID: %d"), client_pid));
	    close_client_session(client_pid);

            return result;
        }

    private:
        DropbearServerImplementation(const DropbearServerImplementation&) = delete;
        DropbearServerImplementation& operator=(const DropbearServerImplementation&) = delete;
    };

} // namespace Plugin
} // namespace WPEFramework
