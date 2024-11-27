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
#include <interfaces/json/JResourceMonitor.h>
#include <interfaces/IMemory.h>
#include <interfaces/IResourceMonitor.h>

namespace WPEFramework {
    namespace Plugin {
        class ResourceMonitor : public PluginHost::IPlugin, public PluginHost::JSONRPC {
            public:
                ResourceMonitor(const ResourceMonitor &) = delete;
                ResourceMonitor &operator=(const ResourceMonitor &) = delete;
                ResourceMonitor()
                    : _service(nullptr), _monitor(nullptr), _connectionId(0), _notification(*this)
                {
                }

                ~ResourceMonitor() override = default;

                const string Initialize(PluginHost::IShell *service) override;
                void Deinitialize(PluginHost::IShell *service) override;
                string Information() const override;

                BEGIN_INTERFACE_MAP(ResourceMonitor)
                    INTERFACE_ENTRY(PluginHost::IPlugin)
                    INTERFACE_ENTRY(PluginHost::IDispatcher)
                    INTERFACE_AGGREGATE(Exchange::IResourceMonitor, _monitor)
                END_INTERFACE_MAP

            private:
                PluginHost::IShell *_service;
                Exchange::IResourceMonitor *_monitor;
                uint32_t _connectionId;

                class Notification : public Exchange::IResourceMonitor::INotification {
                    public:
                        explicit Notification(ResourceMonitor& parent)
                            : _parent(parent) {
                        }

                        Notification() = delete;
                        Notification(const Notification&) = delete;
                        Notification& operator=(const Notification&) = delete;

                        ~Notification() override = default;

                        void OnResourceMonitorData(const Exchange::IResourceMonitor::EventData& data) override {
                            Exchange::JResourceMonitor::Event::OnResourceMonitorData(_parent, data);
                        }

                        BEGIN_INTERFACE_MAP(Notification)
                            INTERFACE_ENTRY(Exchange::IResourceMonitor::INotification)
                        END_INTERFACE_MAP

                    private:
                        ResourceMonitor& _parent;
                };

                Core::Sink<Notification> _notification;
        };
    } // namespace Plugin
} // namespace WPEFramework
