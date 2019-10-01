
#include "Module.h"
#include "Containers.h"
#include <processcontainers/ProcessContainer.h>
#include <interfaces/json/JsonData_Containers.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::Containers;

    // Registration
    //

    void Containers::RegisterAll()
    {
        Register<StopParamsData,void>(_T("stop"), &Containers::endpoint_stop, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("containers"), &Containers::get_containers, nullptr, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("networks"), &Containers::get_networks, nullptr, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("ip"), &Containers::get_ip, nullptr, this);
        Property<Core::JSON::String>(_T("memory"), &Containers::get_memory, nullptr, this);
        Property<Core::JSON::EnumType<StatusType>>(_T("status"), &Containers::get_status, nullptr, this);
        Property<Core::JSON::String>(_T("cpu"), &Containers::get_cpu, nullptr, this);
        Property<Core::JSON::String>(_T("log"), &Containers::get_log, nullptr, this);
        Property<Core::JSON::String>(_T("config"), &Containers::get_config, nullptr, this);

    }

    void Containers::UnregisterAll()
    {
        Unregister(_T("stop"));
        Unregister(_T("config"));
        Unregister(_T("log"));
        Unregister(_T("cpu"));
        Unregister(_T("status"));
        Unregister(_T("memory"));
        Unregister(_T("ip"));
        Unregister(_T("networks"));
        Unregister(_T("containers"));
        Unregister(_T("networks"));
        Unregister(_T("containers"));
    }

    // API implementation
    //

    // Method: stop - Stops a container
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Container not found
    uint32_t Containers::endpoint_stop(const StopParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const string& name = params.Name.Value();

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto found = administrator.Find(name); 

        if (found != nullptr) {
            found->Stop(0);
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }

        return result;
    }

    // Property: containers - List of active containers
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Containers::get_containers(Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto iterator = administrator.Containers();

        while (iterator.Next()) {
            Core::JSON::String containerName;
            containerName = iterator.Current()->Id();

            response.Add(containerName);
        }

        return Core::ERROR_NONE;
    }

    // Property: networks - List of networks in the container
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Container not found
    uint32_t Containers::get_networks(const string& index, Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto found = administrator.Find(index); 

        if (found != nullptr) {
            std::vector<string> networks;
            found->Networks(networks);

            // Quite suboptimal (creating list from list, but should suffice for testing)
            for (auto network : networks) {
                Core::JSON::String networkName;
                networkName = network;
                response.Add(networkName);
            }
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }
        
        return result;
    }

    // Property: ip - List of all ip addresses of the container
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Container not found
    uint32_t Containers::get_ip(const string& index, Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto found = administrator.Find(index); 

        if (found != nullptr) {
            std::vector<Core::NodeId> ips;
            found->IPs(ips);

            // Quite suboptimal (creating list from list, but should suffice for testing)
            for (auto ip : ips) {
                Core::JSON::String ipStr;
                ipStr = ip.HostAddress();
                response.Add(ipStr);
            }
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }
        
        return result;  
    }

    // Property: memory - Operating memory allocated to the container
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Container not found
    uint32_t Containers::get_memory(const string& index, Core::JSON::String& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto found = administrator.Find(index); 

        if (found != nullptr) {
            response.SetQuoted(false);
            response = found->Memory();
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }
        
        return result;
    }

    // Property: status - Operational status of the container
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Container not found
    uint32_t Containers::get_status(const string& index, Core::JSON::EnumType<StatusType>& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto found = administrator.Find(index); 

        if (found != nullptr) {
            switch(found->ContainerState()) {
                case ProcessContainers::IContainerAdministrator::IContainer::ABORTING:
                    response = StatusType::ABORTING;
                    break;
                case ProcessContainers::IContainerAdministrator::IContainer::RUNNING:
                    response = StatusType::RUNNING;
                    break;
                case ProcessContainers::IContainerAdministrator::IContainer::STARTING:
                    response = StatusType::STARTING;
                    break;
                case ProcessContainers::IContainerAdministrator::IContainer::STOPPED:
                    response = StatusType::STOPPED;
                    break;
                case ProcessContainers::IContainerAdministrator::IContainer::STOPPING:
                    response = StatusType::STOPPING;
                    break;                
            }
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }
        
        return result;    
    }

    // Property: cpu - CPU time allocated to running the container
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Container not found
    uint32_t Containers::get_cpu(const string& index, Core::JSON::String& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto found = administrator.Find(index); 

        if (found != nullptr) {
            response.SetQuoted(false);
            response = found->Cpu();
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }
        
        return result;
    }

        // Property: log - Containers log
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Container not found
    uint32_t Containers::get_log(const string& index, Core::JSON::String& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto found = administrator.Find(index); 

        if (found != nullptr) {
            response = found->Log();
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }
        
        return result;
    }

    // Property: config - Container's configuration
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Container not found
    uint32_t Containers::get_config(const string& index, Core::JSON::String& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto found = administrator.Find(index); 

        if (found != nullptr) {
            response = found->Configuration();
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }
        
        return result;    
    }

} // namespace Plugin

}

