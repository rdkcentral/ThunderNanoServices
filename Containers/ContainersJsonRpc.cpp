
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
        Property<Core::JSON::ArrayType<NetworksData>>(_T("networks"), &Containers::get_networks, nullptr, this);
        Property<MemoryData>(_T("memory"), &Containers::get_memory, nullptr, this);
        Property<Core::JSON::EnumType<StatusType>>(_T("status"), &Containers::get_status, nullptr, this);
        Property<CpuData>(_T("cpu"), &Containers::get_cpu, nullptr, this);
        Property<Core::JSON::String>(_T("logpath"), &Containers::get_logpath, nullptr, this);
        Property<Core::JSON::String>(_T("configpath"), &Containers::get_configpath, nullptr, this);
    }

    void Containers::UnregisterAll()
    {
        Unregister(_T("stop"));
        Unregister(_T("configpath"));
        Unregister(_T("logpath"));
        Unregister(_T("cpu"));
        Unregister(_T("status"));
        Unregister(_T("memory"));
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
        auto container = administrator.Find(name); 

        if (container != nullptr) {
            container->Stop(0);
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

    // Property: networks - Networks information
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Container not found
    uint32_t Containers::get_networks(const string& index, Core::JSON::ArrayType<NetworksData>& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto container = administrator.Find(index); 

        if (container != nullptr) {
            auto networkIterator = container->NetworkInterfaces();

            while (networkIterator.Next() == true) {
                NetworksData networkData;
                networkData.Interface = networkIterator.Current(); 
                const std::vector<Core::NodeId> ips = container->IPs(networkIterator.Current());                               

                for (auto ip : ips) {
                    Core::JSON::String ipJSON;
                    ipJSON = ip.HostName();
                    networkData.Ips.Add(ipJSON);
                }
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
    uint32_t Containers::get_memory(const string& index, MemoryData& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto found = administrator.Find(index); 

        if (found != nullptr) {
            auto memoryInfo = found->Memory();
            
            response.Allocated = memoryInfo.allocated;
            response.Resident = memoryInfo.resident;
            response.Shared = memoryInfo.shared;
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
        auto container = administrator.Find(index); 

        if (container != nullptr) {
            switch(container->ContainerState()) {
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
    uint32_t Containers::get_cpu(const string& index, CpuData& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto container = administrator.Find(index); 

        if (container != nullptr) {
            auto cpuInfo = container->Cpu();
            
            response.Total = cpuInfo.total;
            
            for (auto thread : cpuInfo.threads) {
                Core::JSON::DecUInt64 threadTime;
                threadTime = thread;

                response.Threads.Add(threadTime);
            }
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }
        
        return result;
    }

    // Property: logpath - Path to logging configuration
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Container not found
    uint32_t Containers::get_logpath(const string& index, Core::JSON::String& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto container = administrator.Find(index); 

        if (container != nullptr) {
            response = container->LogPath();
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }
        
        return result;
    }

    // Property: configpath - Location of containers configuration
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Container not found
    uint32_t Containers::get_configpath(const string& index, Core::JSON::String& response) const
    {
        uint32_t result = Core::ERROR_NONE;

        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto container = administrator.Find(index); 

        if (container != nullptr) {
            response = container->ConfigPath();
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }
        
        return result;    
    }

} // namespace Plugin

}

