/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological B.V.
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

#include "../../Trace.h"

#include <core/core.h>
#include <libudev.h>

namespace Compositor {
namespace Backend {

    class DrmMonitor {
        enum class action : uint8_t {
            ADDED = 1,
            CHANGED,
            REMOVED,
            UNKNOWN = 0
        };
        using action_t = action;

    public:
        struct node_t {
            node_t(const std::string& node, const bool boot)
                : Node(node)
                , Boot(boot)
            {
            }

            const std::string Node;
            const bool Boot;
        };

        struct INotification {
            virtual ~INotification() = default;
            virtual void Hotplug(const std::string& node) = 0;
            virtual void PropertyChanged(const std::string& node, const uint32_t connectorId, const uint32_t propertyId) = 0;
        };

        class Monitor : virtual public WPEFramework::Core::IResource {
        public:
            Monitor() = delete;
            virtual ~Monitor() = default;

            Monitor(DrmMonitor& parent)
                : _parent(parent)
            {
            }

            uint16_t Events() override
            {
                return (POLLIN | POLLPRI);
            }

            handle Descriptor() const override
            {
                return _parent.Descriptor();
            }

            void Handle(const uint16_t events) override
            {
                if (((events & POLLIN) != 0) || ((events & POLLPRI) != 0)) {
                    _parent.Handle();
                }
            }

        private:
            DrmMonitor& _parent;
        };

        DrmMonitor(const DrmMonitor&) = delete;
        DrmMonitor& operator=(const DrmMonitor&) = delete;

    //private:
        friend class WPEFramework::Core::ProxyObject<DrmMonitor>;
        DrmMonitor()
            : _adminLock()
            , _udev(udev_new())
            , _udevMonitor(udev_monitor_new_from_netlink(_udev, "udev"))
            , _monitor(*this)
        {
            udev_monitor_filter_add_match_subsystem_devtype(_udevMonitor, "drm", NULL);
            udev_monitor_enable_receiving(_udevMonitor);

            WPEFramework::Core::ResourceMonitor::Instance().Register(_monitor);

            TRACE(WPEFramework::Trace::Information, ("Constructed DrmMonitor"));
        }

        virtual ~DrmMonitor()
        {
            WPEFramework::Core::ResourceMonitor::Instance().Unregister(_monitor);

            udev_monitor_unref(_udevMonitor);
            udev_unref(_udev);
            TRACE(WPEFramework::Trace::Information, ("Destructed DrmMonitor"));
        }

    public:
        static WPEFramework::Core::ProxyType<DrmMonitor> Instance()
        {
            static WPEFramework::Core::ProxyType<DrmMonitor> instance = WPEFramework::Core::ProxyType<DrmMonitor>::Create();

            return instance;
        }

        uint32_t Register(INotification* notifcation)
        {
            uint32_t result(WPEFramework::Core::ERROR_NONE);

            WPEFramework::Core::SafeSyncType<WPEFramework::Core::CriticalSection> scopedLock(_adminLock);

            NotificationRegister::iterator index(std::find(_notifications.begin(), _notifications.end(), notifcation));

            if (index == _notifications.end()) {
                _notifications.emplace_back(notifcation);
            }

            return result;
        }

        uint32_t Unregister(INotification* notifcation)
        {
            uint32_t result(WPEFramework::Core::ERROR_NONE);

            WPEFramework::Core::SafeSyncType<WPEFramework::Core::CriticalSection> scopedLock(_adminLock);

            NotificationRegister::iterator index(std::find( _notifications.begin(), _notifications.end(), notifcation));

            if (index != _notifications.end()) {
                _notifications.erase(index);
            }

            return result;
        }

        void Cards(std::vector<node_t>& _nodes)
        {
            ASSERT(_udev != nullptr);

            _nodes.clear();

            struct udev_enumerate* cards = udev_enumerate_new(_udev);

            if (cards != nullptr) {
                udev_enumerate_add_match_subsystem(cards, "drm");
                udev_enumerate_add_match_sysname(cards, DRM_PRIMARY_MINOR_NAME "[0-9]*");

                if (udev_enumerate_scan_devices(cards) == 0) {
                    struct udev_list_entry* entry;
                    size_t i = 0;

                    udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(cards))
                    {
                        bool is_boot_vga = false;
                        const char* path = udev_list_entry_get_name(entry);
                        struct udev_device* dev = udev_device_new_from_syspath(_udev, path);

                        if (!dev) {
                            continue;
                        }

                        // This is owned by 'dev', so we don't need to free it
                        struct udev_device* pci = udev_device_get_parent_with_subsystem_devtype(dev, "pci", NULL);

                        if (pci) {
                            const char* id = udev_device_get_sysattr_value(pci, "boot_vga");
                            if (id && strcmp(id, "1") == 0) {
                                is_boot_vga = true;
                            }
                        }

                        if (udev_device_get_devnode(dev)) {
                            TRACE(WPEFramework::Trace::Information, ("Found %s boot=%s", udev_device_get_devnode(dev), is_boot_vga ? "yes" : "no"));
                            _nodes.emplace_back(udev_device_get_devnode(dev), is_boot_vga);
                        }

                        udev_device_unref(dev);

                        i++;
                    }
                } else {
                    TRACE(WPEFramework::Trace::Error, ("udev_enumerate_scan_devices failed"));
                }

                udev_enumerate_unref(cards);
            } else {
                TRACE(WPEFramework::Trace::Error, ("udev_enumerate_new failed"));
            }
        }

    private:
        int Descriptor() const
        {
            return (udev_monitor_get_fd(_udevMonitor));
        }

        void Handle()
        {
            struct udev_device* event;

            event = udev_monitor_receive_device(_udevMonitor);

            if (event != nullptr) {
                const action_t action(Action(event));

                ASSERT(action != action::UNKNOWN);

                struct udev_list_entry* entry;
                size_t i = 0;

                const char *key, *str;
                udev_list_entry_foreach(entry, udev_device_get_properties_list_entry(event))
                {
                    key = udev_list_entry_get_name(entry);
                    str = udev_list_entry_get_value(entry);

                    TRACE_GLOBAL(WPEFramework::Trace::Information, ("Updated %s=%s", key, str));
                }

                const std::string sysName(udev_device_get_sysname(event));
                const std::string devNode(udev_device_get_devnode(event));

                switch (action) {
                case action::CHANGED: {
                    TRACE_GLOBAL(WPEFramework::Trace::Information, ("DRM device %s[%s] changed", sysName, devNode));

                    if (IsHotplug(event)) {
                        TRACE_GLOBAL(WPEFramework::Trace::Information, ("Hotplug detected"));

                        if (PropertyChanged(event) == false) {
                            Hotplug(event);
                        }
                    } else if (IsLease(event)) {
                        TRACE_GLOBAL(WPEFramework::Trace::Information, ("DRM device lease detected"));
                    }
                    break;
                }
                case action::ADDED:
                case action::REMOVED: {
                    TRACE_GLOBAL(WPEFramework::Trace::Information, ("DRM device %s %s", sysName, action == action::ADDED ? "Added" : "Removed"));
                    break;
                }
                default: {
                    TRACE_GLOBAL(WPEFramework::Trace::Information, ("DRM unknown action for %s", sysName));
                    break;
                }
                }

                udev_device_unref(event);
            } else {
                TRACE_GLOBAL(WPEFramework::Trace::Error, ("DRM device failed to get event"));
            }
        }

        bool IsLease(struct udev_device* event) const
        {
            const char* value = udev_device_get_property_value(event, "LEASE");
            return (value != NULL && strcmp(value, "1") == 0);
        }

        bool IsHotplug(struct udev_device* event) const
        {
            const char* value = udev_device_get_property_value(event, "HOTPLUG");
            return (value != NULL && strcmp(value, "1") == 0);
        }

        bool Hotplug(struct udev_device* event) const
        {
            ASSERT(event != nullptr);

            bool result(false);

            const char* value = udev_device_get_property_value(event, "HOTPLUG");

            if (value != NULL && strcmp(value, "1") == 0) {
                const std::string devNode(udev_device_get_devnode(event));

                TRACE(WPEFramework::Trace::Information, ("Hotplug detected on %s", devNode.c_str()));

                _adminLock.Lock();

                for (auto& index : _notifications) {
                    index->Hotplug(devNode);
                }

                _adminLock.Unlock();

                result = true;
            }
            return result;
        }

        bool PropertyChanged(struct udev_device* event) const
        {
            ASSERT(event != nullptr);

            bool result(false);

            const char* connectorStr = udev_device_get_property_value(event, "CONNECTOR");
            const char* propertyStr = udev_device_get_property_value(event, "PROPERTY");

            if ((connectorStr != nullptr) && (propertyStr != nullptr)) {
                const std::string devNode(udev_device_get_devnode(event));
                const uint32_t connectorId(atoi(connectorStr));
                const uint32_t propertyId(atoi(propertyStr));

                TRACE(WPEFramework::Trace::Information, ("Property changed on %s connectorId=%d propertyId=%d", devNode, connectorId, propertyId));

                _adminLock.Lock();

                for (auto& index : _notifications) {
                    index->PropertyChanged(devNode, connectorId, propertyId);
                }

                _adminLock.Unlock();

                result = true;
            }

            return result;
        }

        action_t Action(struct udev_device* event) const
        {
            action_t result(action::UNKNOWN);

            const char* action = udev_device_get_action(event);

            if (strcmp(action, "add") == 0) {
                result = action::ADDED;
            } else if (strcmp(action, "change") == 0) {
                result = action::CHANGED;
            } else if (strcmp(action, "remove") == 0) {
                result = action::REMOVED;
            }

            return result;
        }

        using NotificationRegister = std::list<INotification*>;

    private:
        mutable WPEFramework::Core::CriticalSection _adminLock;
        udev* _udev;
        udev_monitor* _udevMonitor;
        Monitor _monitor;
        NotificationRegister _notifications;
    };

} // namespace Compositor
} // namespace Backend