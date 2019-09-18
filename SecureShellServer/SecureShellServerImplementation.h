#pragma once

#include "Module.h"

#include <libdropbear.h>

namespace WPEFramework {
namespace Plugin {

    class SecureShellServerImplementation {
    public:
        SecureShellServerImplementation() = default;
        ~SecureShellServerImplementation() = default;

        uint32_t StartService(const string& params)
        {
            uint32_t result = Core::ERROR_NONE;

            TRACE(Trace::Information, (_T("Starting Dropbear Service with options as: %s"), params.c_str()));
            fprintf(stderr, "Starting Dropbear Service with options as: %s\n", params.c_str());
	    // TODO: Check the return value and based on that change result
	    activate_dropbear(const_cast<char*>(params.c_str())); 

            return result;
        }

        uint32_t StopService()
        {
            uint32_t result = Core::ERROR_NONE;

            TRACE(Trace::Information, (_T("Stoping Dropbear Service")));
	    deactivate_dropbear(); //TODO: Check the return value and based on that change result

            return result;
        }

        uint32_t GetSessionsCount()
        {
            //uint32_t result = Core::ERROR_NONE;

	    uint32_t count = get_active_sessions_count();
            TRACE(Trace::Information, (_T("Get total number of active SSH client sessions managed by Dropbear service: %d"), count));

            return count;
        }

        uint32_t GetSessionsInfo(Core::JSON::ArrayType<JsonData::SecureShellServer::SessioninfoResultData>& response) const
        {
            uint32_t result = Core::ERROR_NONE;

	    
	     _adminLock.Lock();
	    int32_t count = get_active_sessions_count();
            TRACE(Trace::Information, (_T("Get details of (%d)active SSH client sessions managed by Dropbear service"), count));

	    struct client_info *info = static_cast<struct client_info*>(::malloc(sizeof(struct client_info) * count));

	    get_active_sessions_info(info, count);

	    _clients.clear(); // Clear all elements before we re-populate it with new elements

	    for(int32_t i=0; i<count; i++)
            {
	            TRACE(Trace::Information, (_T("Count: %d index: %d pid: %d IP: %s Timestamp: %s"), 
					    count, i, info[i].pid, info[i].ipaddress, info[i].timestamp));

/*	            JsonData::SecureShellServer::SessioninfoResultData newElement;
	            
		    newElement.IpAddress = info[i].ipaddress;
	            newElement.RemoteId = std::to_string(i);
		    newElement.TimeStamp = info[i].timestamp;

        	    JsonData::SecureShellServer::SessioninfoResultData& element(response.Add(newElement));*/

		    ClientImpl newClient(info[i].ipaddress, info[i].timestamp, info[i].pid);

		    _clients.push_back(newClient);
		    response->Clients.Add().Set(newClient);

	    }
	    ::free(info);
	    _adminLock.Unlock();

            return result;
        }

        uint32_t CloseClientSession(const std::string& remote_id)
        {
            uint32_t result = Core::ERROR_NONE;

            TRACE(Trace::Information, (_T("closing client session with PID: %s"), remote_id.c_str()));

	    _adminLock.Lock();

            for (std::list<DeviceImpl*>::iterator index = _devices.begin(), end = _devices.end(); index != end; ++index) 
	    {
                if(index.Current.RemoteId() == remote_id)
		{
	            result = close_client_session(remote_pid);
	    	    _clients.erase(index.Current());
                }
            }

            _adminLock.Unlock();

            return result;
        }

    private:
        SecureShellServerImplementation(const SecureShellServerImplementation&) = delete;
        SecureShellServerImplementation& operator=(const SecureShellServerImplementation&) = delete;
    };

} // namespace Plugin
} // namespace WPEFramework
