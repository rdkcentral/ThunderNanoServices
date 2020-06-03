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
        auto container = administrator.Get(name); 

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

            container->Release();
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }

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
        auto container = administrator.Get(name); 

        if (container != nullptr) {
            container->Stop(0);
            container->Release();
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
        auto containers = administrator.Containers();

        while(containers->Next()) {
            Core::JSON::String containerName;
            containerName = containers->Id();
            response.Add(containerName);
        }
        
        containers->Release();

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
        auto container = administrator.Get(index); 

        if (container != nullptr) {
            auto iterator = container->NetworkInterfaces();

            while(iterator->Next() == true) {
                NetworksData networkData;

                networkData.Interface = iterator->Name();
                
                for (int ip = 0; ip < iterator->NumAddresses(); ip++) {
                    Core::JSON::String ipJSON;
                    ipJSON = iterator->Address(ip);

                    networkData.Ips.Add(ipJSON);
                }                
            }

            iterator->Release();
            container->Release();
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
        auto found = administrator.Get(index); 

        if (found != nullptr) {
            auto memoryInfo = found->Memory();
            
            response.Allocated = memoryInfo->Allocated();
            response.Resident = memoryInfo->Resident();
            response.Shared = memoryInfo->Shared();

            memoryInfo->Release();
            found->Release();
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
        auto container = administrator.Get(index); 

        if (container != nullptr) {
            auto processorInfo = container->ProcessorInfo();
            
            response.Total = processorInfo->TotalUsage();
            
            for (int i = 0; i < processorInfo->NumberOfCores(); i++) {
                Core::JSON::DecUInt64 coreTime;
                coreTime = processorInfo->CoreUsage(i);

                response.Cores.Add(coreTime);
            }

            processorInfo->Release();
            container->Release();
        } else {
            result = Core::ERROR_UNAVAILABLE;
        }
        
        return result;
    }
} // namespace Plugin

}

