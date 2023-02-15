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

#include <core/core.h>

namespace Compositor {
namespace Backend {
    struct node_t {
        node_t(const std::string& node, const bool boot)
            : Node(node)
            , Boot(boot)
        {
        }

        const std::string Node;
        const bool Boot;
    };

    static struct udev_enumerate* enumerate_drm_cards(struct udev* udev)
    {
        struct udev_enumerate* en = udev_enumerate_new(udev);

        if (!en) {
            TRACE_GLOBAL(Trace::Error, ("udev_enumerate_new failed"));
            return nullptr;
        }

        udev_enumerate_add_match_subsystem(en, "drm");
        udev_enumerate_add_match_sysname(en, DRM_PRIMARY_MINOR_NAME "[0-9]*");

        if (udev_enumerate_scan_devices(en) != 0) {
            TRACE_GLOBAL(Trace::Error, ("udev_enumerate_scan_devices failed"));
            udev_enumerate_unref(en);
            return nullptr;
        }

        return en;
    }

    class DrmMonitor : WPEFramework::Core::IResource {
    public:
        enum class action : uint8_t {
            ADDED = 1,
            CHANGED,
            REMOVED,
            UNKNOWN = 0
        };
        using action_t = action;

        struct ICallback {
            virtual ~ICallback() = default;
            virtual void Reevaluate(const std::string& node) = 0;
        };

        DrmMonitor() = delete;
        DrmMonitor(const DrmMonitor&) = delete;
        DrmMonitor& operator=(const DrmMonitor&) = delete;

        DrmMonitor(ICallback* callback)
            : _adminLock()
            , _udev(udev_new())
            , _monitor(udev_monitor_new_from_netlink(_udev, "udev"))
            , _callback(callback)
        {
            // UpdateCards();

            udev_monitor_filter_add_match_subsystem_devtype(_monitor, "drm", NULL);
            udev_monitor_enable_receiving(_monitor);

            Core::ResourceMonitor::Instance().Register(*this);
        }

        ~DrmMonitor()
        {
            Core::ResourceMonitor::Instance().Unregister(*this);

            udev_monitor_unref(_monitor);
            udev_unref(_udev);
        }

        handle Descriptor() const override
        {
            return (udev_monitor_get_fd(_monitor));
        }
        uint16_t Events() override
        {
            return (POLLIN | POLLPRI);
        }

        void Handle(const uint16_t events) override
        {
            if (((events & POLLIN) != 0) || ((events & POLLPRI) != 0)) {
                struct udev_device* event;

                event = udev_monitor_receive_device(_monitor);

                if (event != nullptr) {
                    const action_t action(Action(event));
                    uint32_t conn_id, prop_id;

                    ASSERT(action != action::UNKNOWN);

                    const char* sysname = udev_device_get_sysname(event);

                    switch (action) {
                    case action::CHANGED: {
                        TRACE_GLOBAL(Trace::Information, ("DRM device %s changed", sysname));
                        if (IsHotplug(event)) {
                            TRACE_GLOBAL(Trace::Information, ("Hotplug detected"));

                            if (IsPropertyChanged(event, conn_id, prop_id))
                                UpdateConnectorProperty(conn_id, prop_id);
                            else
                                UpdateConnectors(event);
                        } else if (IsLease(event)) {
                            TRACE_GLOBAL(Trace::Information, ("DRM device lease detected"));
                        }
                        break;
                    }
                    case action::ADDED:
                    case action::REMOVED: {
                        TRACE_GLOBAL(Trace::Information, ("DRM device %s %s", sysname, action == action::ADDED ? "Added" : "Removed"));
                        break;
                    }
                    default: {
                        TRACE_GLOBAL(Trace::Information, ("DRM unknown action for %s", sysname));
                        break;
                    }
                    }

                    udev_device_unref(event);
                } else {
                    TRACE_GLOBAL(Trace::Error, ("DRM device failed to get event"));
                }
            }
        }

    private:
        void UpdateConnectors(struct udev_device* event)
        {
            const std::string filename(udev_device_get_devnode(event));

            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            if (_callback != nullptr) {
                _callback->Reevaluate(filename);
            }

            const char* sysnum = udev_device_get_sysnum(event);
            const char* sysname = udev_device_get_sysname(event);
            TRACE_GLOBAL(Trace::Information, ("Reevaluate connectors of %s, sysname=%s, sysnum=%s", filename.c_str(), sysname, sysnum));
        }

        void UpdateConnectorProperty(const uint32_t conn_id, const uint32_t prop_id)
        {
            TRACE_GLOBAL(Trace::Information, ("Updated ConnectorId=%d PropertyId=%d", conn_id, prop_id));
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

        bool IsPropertyChanged(struct udev_device* udev_device, uint32_t& connector_id, uint32_t& property_id)
        {
            const char* val;

            struct udev_list_entry* entry;
            size_t i = 0;

            const char *key, *str;

            udev_list_entry_foreach(entry, udev_device_get_properties_list_entry(udev_device))
            {
                key = udev_list_entry_get_name(entry);
                str = udev_list_entry_get_value(entry);

                TRACE_GLOBAL(Trace::Information, ("Updated %s=%s", key, str));
            }

            val = udev_device_get_property_value(udev_device, "CONNECTOR");
            if (!val)
                return false;
            else
                connector_id = atoi(val);

            val = udev_device_get_property_value(udev_device, "PROPERTY");
            if (!val)
                return false;
            else
                property_id = atoi(val);

            return true;
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

        void Cards(std::vector<node_t>& _nodes)
        {
            _nodes.clear();
            struct udev_enumerate* cards = enumerate_drm_cards(_udev);

            if (cards != nullptr) {
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
                        TRACE_GLOBAL(Trace::Information, ("Found %s boot=%s", udev_device_get_devnode(dev), is_boot_vga ? "yes" : "no"));
                        _nodes.emplace_back(udev_device_get_devnode(dev), is_boot_vga);
                    }

                    udev_device_unref(dev);

                    i++;
                }

                udev_enumerate_unref(cards);
            }
        }

    private:
        Core::CriticalSection _adminLock;
        udev* _udev;
        udev_monitor* _monitor;
        ICallback* _callback;
    };

} // namespace Compositor
} // namespace Backend