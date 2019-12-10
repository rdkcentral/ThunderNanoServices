
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
        Register<StartParamsData,void>(_T("start"), &Containers::endpoint_start, this);
        Register<StopParamsData,void>(_T("stop"), &Containers::endpoint_stop, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("containers"), &Containers::get_containers, nullptr, this);
        Property<Core::JSON::ArrayType<NetworksData>>(_T("networks"), &Containers::get_networks, nullptr, this);
        Property<MemoryData>(_T("memory"), &Containers::get_memory, nullptr, this);
        Property<CpuData>(_T("cpu"), &Containers::get_cpu, nullptr, this);
    }

    void Containers::UnregisterAll()
    {
        Unregister(_T("start"));
        Unregister(_T("stop"));
        Unregister(_T("cpu"));
        Unregister(_T("memory"));
        Unregister(_T("networks"));
        Unregister(_T("containers"));
    }

    // API implementation
    //

    // Method: start - Starts a new container
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Container not found
    //  - ERROR_GENERAL: Failed to start container
    uint32_t Containers::endpoint_start(const StartParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const string& name = params.Name.Value();
        const string& command = params.Command.Value();
        
        auto& administrator = ProcessContainers::IContainerAdministrator::Instance();
        auto container = administrator.Find(name); 

        if (container != nullptr) {
            
            std::vector<string> parameters;
            parameters.reserve(params.Parameters.Elements().Count());
            auto iterator = params.Parameters.Elements();

            while (iterator.Next()) {
                parameters.push_back(iterator.Current().Value());
            }

            ProcessContainers::IStringIterator paramsIterator(parameters);

            if (container->Start(command, paramsIterator) != true) {
                result = Core::ERROR_GENERAL;
            }
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }

        administrator.Release();

        return result;
    }

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
            container->Release();
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }

        administrator.Release();

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

        administrator.Release();

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
                const std::vector<string> ips = container->IPs(networkIterator.Current());   

                for (auto ip : ips) {
                    Core::JSON::String ipJSON;
                    ipJSON = ip;
                    networkData.Ips.Add(ipJSON);
                }

                response.Add(networkData);
            } 
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }

        administrator.Release();
        
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

        administrator.Release();
        
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
            
            for (auto core : cpuInfo.cores) {
                Core::JSON::DecUInt64 coreTime;
                coreTime = core;

                response.Cores.Add(coreTime);
            }
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }

        administrator.Release();
        
        return result;
    }
} // namespace Plugin

}

