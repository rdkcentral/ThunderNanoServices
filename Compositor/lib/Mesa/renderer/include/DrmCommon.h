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

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace Compositor {
constexpr int InvalidFileDescriptor = -1;
namespace DRM {
    /*
     * Re-open the DRM node to avoid GEM handle ref'counting issues.
     * See: https://gitlab.freedesktop.org/mesa/drm/-/merge_requests/110
     *
     */
    inline static int ReopenNode(int fd, bool openRenderNode)
    {
        if (drmIsMaster(fd)) {
            // Only recent kernels support empty leases
            uint32_t lessee_id;
            int lease_fd = drmModeCreateLease(fd, nullptr, 0, O_CLOEXEC, &lessee_id);

            if (lease_fd >= 0) {
                return lease_fd;
            } else if (lease_fd != -EINVAL && lease_fd != -EOPNOTSUPP) {
                TRACE_GLOBAL(WPEFramework::Trace::Error, ("drmModeCreateLease failed"));
                return InvalidFileDescriptor;
            }
            TRACE_GLOBAL(WPEFramework::Trace::Information, ("drmModeCreateLease failed, falling back to plain open"));
        } else {
            TRACE_GLOBAL(WPEFramework::Trace::Information, ("DRM is not in master mode"));
        }

        char* name = nullptr;

        if (openRenderNode) {
            name = drmGetRenderDeviceNameFromFd(fd);
        }

        if (name == nullptr) {
            // Either the DRM device has no render node, either the caller wants
            // a primary node
            name = drmGetDeviceNameFromFd2(fd);

            if (name == nullptr) {
                TRACE_GLOBAL(WPEFramework::Trace::Error, ("drmGetDeviceNameFromFd2 failed"));
                return InvalidFileDescriptor;
            }
        } else {
            TRACE_GLOBAL(WPEFramework::Trace::Information, ("DRM Render Node: %s", name));
        }

        int newFd = open(name, O_RDWR | O_CLOEXEC);

        if (newFd < 0) {
            TRACE_GLOBAL(WPEFramework::Trace::Error, ("Failed to open DRM node '%s'", name));
            free(name);
            return InvalidFileDescriptor;
        } else {
            TRACE_GLOBAL(WPEFramework::Trace::Information, ("DRM Render Node opened: %s", name));
        }

        free(name);

        // If we're using a DRM primary node (e.g. because we're running under the
        // DRM backend, or because we're on split render/display machine), we need
        // to use the legacy DRM authentication mechanism to have the permission to
        // manipulate buffers.
        if (drmGetNodeTypeFromFd(newFd) == DRM_NODE_PRIMARY) {
            drm_magic_t magic;
            int ret(0);

            if (ret = drmGetMagic(newFd, &magic) < 0) {
                TRACE_GLOBAL(WPEFramework::Trace::Error, ("drmGetMagic failed: %s", strerror(ret)));
                close(newFd);
                return InvalidFileDescriptor;
            }

            if (ret = drmAuthMagic(fd, magic) < 0) {
                TRACE_GLOBAL(WPEFramework::Trace::Error, ("drmAuthMagic failed: %s", strerror(ret)));
                close(newFd);
                return InvalidFileDescriptor;
            }
        }

        return newFd;
    }

    inline static void GetDRMNode(uint32_t const type, std::vector<std::string>& list)
    {
        const int nDrmDevices = drmGetDevices2(0, nullptr, 0);

        drmDevicePtr devices[nDrmDevices];

        int device_count = drmGetDevices2(0 /* flags */, &devices[0], nDrmDevices);

        if (device_count > 0) {
            for (decltype(device_count) i = 0; i < device_count; i++) {
                switch (type) {
                case DRM_NODE_PRIMARY: // card<num>, always created, KMS, privileged
                case DRM_NODE_CONTROL: // ControlD<num>, currently unused
                case DRM_NODE_RENDER: // Solely for render clients, unprivileged
                {
                    if ((1 << type) == (devices[i]->available_nodes & (1 << type))) {
                        list.push_back(std::string(devices[i]->nodes[type]));
                    }
                    break;
                }
                case DRM_NODE_MAX:
                default: // Unknown (new) node type
                    break;
                }
            }

            drmFreeDevices(&devices[0], device_count);
        }
    }

    inline static bool HasNode(const drmDevice* drmDevice, const char* deviceName)
    {
        bool result(false);
        for (uint16_t i = 0; i < DRM_NODE_MAX; i++) {
            if ((drmDevice->available_nodes & (1 << i)) && (strcmp(drmDevice->nodes[i], deviceName) == 0)) {
                result == true;
                break;
            }
        }
        return result;
    }
} // namespace Transformation
} // namespace Compositor