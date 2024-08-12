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

#include <interfaces/json/JsonData_SubsystemControl.h>

namespace Thunder {
namespace Plugin {

    class SubsystemControl : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    private:
        class Notification : public PluginHost::ISubSystem::INotification {
        public:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

            explicit Notification(SubsystemControl& parent)
                : _parent(parent) {
            }
            ~Notification() override = default;

        public:
            // Some change happened with respect to the Network..
            void Updated() override {
                _parent.event_activity();
            }

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(PluginHost::ISubSystem::INotification)
            END_INTERFACE_MAP

        private:
            SubsystemControl& _parent;
        };
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
                Core::IUnknown* Metadata(const string& metadata) override {
                    METADATA* object = Service::template Create<METADATA>();
                    object->FromString(metadata);
                    return (object);
                }
            };

            using SubsystemMap = std::unordered_map<JsonData::SubsystemControl::SubsystemType, IFactory*>;
            using IdentifierMap = std::unordered_map< JsonData::SubsystemControl::SubsystemType, PluginHost::ISubSystem::subsystem>;

        public:
            Administrator() = default;
            Administrator(const Administrator&) = delete;
            Administrator& operator= (const Administrator&) = delete;

            ~Administrator() {
                for (const std::pair< JsonData::SubsystemControl::SubsystemType, IFactory* >& entry : _factoryMap) {
                    delete entry.second;
                }
                _factoryMap.clear();
            }

        public:
            template<typename METADATA>
            void Announce(const JsonData::SubsystemControl::SubsystemType type) {
                ASSERT(_identifierMap.find(type) == _identifierMap.end());

                _factoryMap.emplace(std::piecewise_construct,
                    std::forward_as_tuple(type),
                    std::forward_as_tuple(new FactoryType<METADATA>()));
 
                Announce(type, static_cast<PluginHost::ISubSystem::subsystem>(METADATA::SUBSYSTEM));
            }
            void Announce(const JsonData::SubsystemControl::SubsystemType type, const PluginHost::ISubSystem::subsystem other) {
                _identifierMap.emplace(std::piecewise_construct,
                    std::forward_as_tuple(type),
                    std::forward_as_tuple(other));
            }
            Core::IUnknown* Metadata(const JsonData::SubsystemControl::SubsystemType type, const string& metadata) {
                SubsystemMap::iterator index = (_factoryMap.find(type));
                return (index != _factoryMap.end() ? index->second->Metadata(metadata) : nullptr);
            }
            bool Lookup(const JsonData::SubsystemControl::SubsystemType type, PluginHost::ISubSystem::subsystem& outcome) const {
                IdentifierMap::const_iterator index = (_identifierMap.find(type));
                if (index != _identifierMap.end()) {
                    outcome = index->second;
                    return(true);
                }
                return(false);
            }

        private:
            SubsystemMap _factoryMap;
            IdentifierMap _identifierMap;
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
        END_INTERFACE_MAP

    private:
        void RegisterAll();
        void UnregisterAll();

        void event_activity();
        uint32_t activate(const JsonData::SubsystemControl::ActivateParamsData& parameter, Core::JSON::DecUInt32& response);
        uint32_t deactivate(const Core::JSON::EnumType<JsonData::SubsystemControl::SubsystemType>& parameter);

    private:
        Administrator _subsystemFactory;
        PluginHost::ISubSystem* _service;
        Core::SinkType<Notification> _notification;
    };

} // Namespace Plugin.
}
