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
#include <interfaces/IRDKAdapter.h>

namespace WPEFramework {
namespace Plugin {

    class RDKAdapter : public PluginHost::IPlugin, public Exchange::IRDKAdapter {
    public:
        RDKAdapter(const RDKAdapter&) = delete;
        RDKAdapter(RDKAdapter&&) = delete;
        RDKAdapter& operator=(const RDKAdapter&) = delete;
        RDKAdapter& operator=(RDKAdapter&&) = delete;

        RDKAdapter()
            : PluginHost::IPlugin()
            , Exchange::IRDKAdapter()
            , _adminLock()
            , _listeners()
            , _sink(*this)
            , _subsystems(nullptr)
            , _connected(false)
        {
        }

        ~RDKAdapter() override = default;

        BEGIN_INTERFACE_MAP(RDKAdapter)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(Exchange::IRDKAdapter)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IRDKAdapter methods
        // -------------------------------------------------------------------------------------------------------
        Core::hresult Register(Exchange::IRDKAdapter::INotification* sink) override;
        Core::hresult Unregister(Exchange::IRDKAdapter::INotification* sink) override;
        Core::hresult Connected(bool& connected) const override;
        Core::hresult PublicIP(string& ipaddress /* @out */) const override;


    private:
    class Notification : public PluginHost::ISubSystem::INotification {
    public:
        explicit Notification(RDKAdapter& parent) : PluginHost::ISubSystem::INotification(), _parent(parent) {}
        ~Notification() override = default;

        void Updated() override {
            _parent.SubsystemUpdate();
        }

    public:
        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(PluginHost::ISubSystem::INotification)
        END_INTERFACE_MAP

    private:
        RDKAdapter& _parent;
    };

    private:
        void SubsystemUpdate();

    private:
        using NotificationContainer = std::list<Exchange::IRDKAdapter::IRDKAdapter::INotification*>;

        mutable Core::CriticalSection _adminLock;
        NotificationContainer _listeners;
        Core::Sink<RDKAdapter::Notification> _sink;
        PluginHost::ISubSystem* _subsystems;
        std::atomic<bool> _connected;
    };

} // namespace Plugin
} // namespace WPEFramework
