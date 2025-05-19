/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#pragma once

#include "Module.h"

#include <interfaces/ISubsystemControl.h>
#include <interfaces/json/JSubsystemControl.h>

namespace Thunder {
namespace Plugin {

    class SubsystemControl : public PluginHost::IPlugin
                           , public PluginHost::JSONRPC
                           , public Exchange::ISubsystemControl {
    private:
        class Administrator {
        private:
            struct IFactory {
                virtual ~IFactory() = default;

                virtual Core::IUnknown* Metadata(const string& metadata) = 0;
            };
            template <typename METADATA>
            class FactoryType : public IFactory {
            private:
                using Service = Core::ServiceType<METADATA>;

            public:
                FactoryType(const FactoryType<METADATA>&) = delete;
                FactoryType<METADATA>& operator= (const FactoryType<METADATA>&) = delete;

                FactoryType() = default;
                ~FactoryType() override = default;

            public:
                Core::IUnknown* Metadata(const string& metadata) override
                {
                    METADATA* object = Service::template Create<METADATA>();
                    object->FromString(metadata);
                    return (object);
                }
            };

            using SubsystemMap = std::unordered_map<PluginHost::ISubSystem::subsystem, IFactory*>;

        public:
            Administrator() = default;
            Administrator(const Administrator&) = delete;
            Administrator& operator= (const Administrator&) = delete;

            ~Administrator()
            {
                for (const std::pair<const PluginHost::ISubSystem::subsystem, IFactory*>& entry : _factoryMap) {
                    delete entry.second;
                }
                _factoryMap.clear();
            }

        public:
            template<typename METADATA>
            void Announce(const PluginHost::ISubSystem::subsystem type)
            {
                _factoryMap.emplace(std::piecewise_construct,
                    std::forward_as_tuple(type),
                    std::forward_as_tuple(new FactoryType<METADATA>()));
            }
            Core::IUnknown* Metadata(const PluginHost::ISubSystem::subsystem type, const string& metadata)
            {
                SubsystemMap::iterator index = _factoryMap.find(type);
                return (index != _factoryMap.end() ? index->second->Metadata(metadata) : nullptr);
            }

        private:
            SubsystemMap _factoryMap;
        };

    public:
        SubsystemControl(const SubsystemControl&) = delete;
        SubsystemControl& operator=(const SubsystemControl&) = delete;

        SubsystemControl();
        ~SubsystemControl() override = default;

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;
    
        BEGIN_INTERFACE_MAP(SubsystemControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::ISubsystemControl)
        END_INTERFACE_MAP

    private:
        Core::hresult Activate(const PluginHost::ISubSystem::subsystem subsystem, const Core::OptionalType<string>& configuration) override;

    private:
        Administrator _subsystemFactory;
        PluginHost::ISubSystem* _service;
    };

} // Namespace Plugin.
}
