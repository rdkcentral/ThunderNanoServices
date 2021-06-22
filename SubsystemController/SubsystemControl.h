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

namespace WPEFramework {
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

    public:
        SubsystemControl(const SubsystemControl&) = delete;
        SubsystemControl& operator=(const SubsystemControl&) = delete;

        #ifdef __WINDOWS__
        #pragma warning(disable: 4355)
        #endif
        SubsystemControl()
            : _service(nullptr)
            , _notification(*this) {
        }
        #ifdef __WINDOWS__
        #pragma warning(default: 4355)
        #endif
        ~SubsystemControl() override = default;

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;
    
    private:
        BEGIN_INTERFACE_MAP(Svalbard)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        void RegisterAll();
        void UnregisterAll();

        void event_activity();
        uint32_t activate(const JsonData::SubsystemControl::ActivateParamsData& parameter, Core::JSON::DecUInt32& response);
        uint32_t deactivate(const Core::JSON::EnumType<JsonData::SubsystemControl::SubsystemType>& parameter);

    private:
        PluginHost::ISubSystem* _service;
        Core::Sink<Notification> _notification;
    };

} // Namespace Plugin.
}
