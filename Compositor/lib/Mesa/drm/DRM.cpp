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

#include "DRM.h"

#ifndef MODULE_NAME
#define MODULE_NAME DRMCommon
#endif

#include <core/core.h>
#include <messaging/messaging.h>
#include <interfaces/ICompositionBuffer.h>
#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

MODULE_NAME_ARCHIVE_DECLARATION

namespace WPEFramework {

namespace Compositor {

using Identifier = uintptr_t;

namespace DRM {

    constexpr int InvalidFileDescriptor = -1;

    string PropertyString(const PropertyRegister& properties, const bool pretty)
    {
        std::stringstream s;
        s << '{';

        if (pretty == true) {
            s << std::endl;
        }

        for (auto iter = properties.cbegin(); iter != properties.cend(); ++iter) {

            if (pretty == true) {
                s << string(4, ' ');
            }

            s << iter->first << "[" << iter->second << "]";

            if (std::next(iter) != properties.cend()) {
                s << ", ";
            }

            if (pretty == true) {
                s << std::endl;
            }
        }

        s << '}';

        return s.str();
    }

    string IdentifierString(const std::vector<Identifier>& ids, const bool pretty)
    {
        std::stringstream s;
        s << '{';

        if (pretty == true) {
            s << std::endl;
        }

        for (auto iter = ids.cbegin(); iter != ids.cend(); ++iter) {
            if (pretty == true) {
                s << string(4, ' ');
            }

            s << *iter;

            if (std::next(iter) != ids.cend()) {
                s << ", ";
            }

            if (pretty == true) {
                s << std::endl;
            }
        }

        s << '}';

        return s.str();
    }

    uint32_t GetPropertyId(PropertyRegister& registry, const string& name)
    {
        auto typeId = registry.find(name);

        Identifier id(0);

        if (typeId != registry.end()) {
            id = typeId->second;
        }
        return id;
    }

    uint32_t GetProperty(const int cardFd, const Identifier object, const Identifier property, Compositor::DRM::Value& value)
    {
        uint32_t result(Core::ERROR_NOT_SUPPORTED);

        drmModeObjectProperties* properties = drmModeObjectGetProperties(cardFd, object, DRM_MODE_OBJECT_ANY);

        if (properties != nullptr) {
            for (uint32_t i = 0; i < properties->count_props; ++i) {
                if (properties->props[i] == property) {
                    value = properties->prop_values[i];
                    result = Core::ERROR_NONE;
                    break;
                }
            }

            drmModeFreeObjectProperties(properties);
        }

        return result;
    }

    uint16_t GetBlobProperty(const int cardFd, const Identifier object, const Identifier property, const uint16_t blobSize, uint8_t blob[])
    {
        uint16_t length(0);
        uint64_t id;

        if (GetProperty(cardFd, object, property, id) == true) {
            drmModePropertyBlobRes* drmBlob = drmModeGetPropertyBlob(cardFd, id);
            ASSERT(drmBlob != nullptr);
            ASSERT(blobSize >= drmBlob->length);

            memcpy(blob, drmBlob->data, drmBlob->length);
            length = drmBlob->length;

            drmModeFreePropertyBlob(drmBlob);
        }

        return length;
    }

    void GetNodes(const uint32_t type, std::vector<string>& list)
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
                        list.push_back(string(devices[i]->nodes[type]));
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

    /*
     * Re-open the DRM node to avoid GEM handle ref'counting issues.
     * See: https://gitlab.freedesktop.org/mesa/drm/-/merge_requests/110
     *
     */
    int ReopenNode(int fd, bool openRenderNode)
    {
        if (drmIsMaster(fd)) {
            // Only recent kernels support empty leases
            uint32_t lessee_id;
            int lease_fd = drmModeCreateLease(fd, nullptr, 0, O_CLOEXEC, &lessee_id);

            if (lease_fd >= 0) {
                return lease_fd;
            } else if (lease_fd != -EINVAL && lease_fd != -EOPNOTSUPP) {
                TRACE_GLOBAL(Trace::Error, ("drmModeCreateLease failed"));
                return InvalidFileDescriptor;
            }
            TRACE_GLOBAL(Trace::Information, ("drmModeCreateLease failed, falling back to plain open"));
        } else {
            TRACE_GLOBAL(Trace::Information, ("DRM is not in master mode"));
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
                TRACE_GLOBAL(Trace::Error, ("drmGetDeviceNameFromFd2 failed"));
                return InvalidFileDescriptor;
            }
        } else {
            TRACE_GLOBAL(Trace::Information, ("DRM Render Node: %s", name));
        }

        int newFd = open(name, O_RDWR | O_CLOEXEC);

        if (newFd < 0) {
            TRACE_GLOBAL(Trace::Error, ("Failed to open DRM node '%s'", name));
            free(name);
            return InvalidFileDescriptor;
        } else {
            TRACE_GLOBAL(Trace::Information, ("DRM Node opened: %s", name));
        }

        free(name);

        // If we're using a DRM primary node (e.g. because we're running under the
        // DRM backend, or because we're on split render/display machine), we need
        // to use the legacy DRM authentication mechanism to have the permission to
        // manipulate buffers.
        if (drmGetNodeTypeFromFd(newFd) == DRM_NODE_PRIMARY) {
            drm_magic_t magic;
            int ret(0);

            if ((ret = drmGetMagic(newFd, &magic)) < 0) {
                TRACE_GLOBAL(Trace::Error, ("drmGetMagic failed: %s", strerror(-ret)));
                close(newFd);
                return InvalidFileDescriptor;
            }

            if ((ret = drmAuthMagic(fd, magic)) < 0) {
                TRACE_GLOBAL(Trace::Error, ("drmAuthMagic failed: %s", strerror(-ret)));
                close(newFd);
                return InvalidFileDescriptor;
            }
        }

        return newFd;
    }

   static void CloseDrmHandles(const int cardFd, std::array<uint32_t, 4>& handles)
    {
        for (uint8_t currentIndex = 0; currentIndex < handles.size(); ++currentIndex) {
            if (handles.at(currentIndex) != 0) {
                // If multiple planes share the same BO handle, avoid double-closing it
                bool alreadyClosed = false;

                for (uint8_t previousIndex = 0; previousIndex < currentIndex; ++previousIndex) {
                    if (handles.at(currentIndex) == handles.at(previousIndex)) {
                        alreadyClosed = true;
                        break;
                    }
                }
                if (alreadyClosed == true) {
                    TRACE_GLOBAL(Trace::Error, ("Skipping DRM handle %u, already closed.", handles.at(currentIndex)));
                }
                else if (drmCloseBufferHandle(cardFd, handles.at(currentIndex)) != 0) {
                    TRACE_GLOBAL(Trace::Error, ("Failed to close drm handle %u", handles.at(currentIndex)));
                }
            }
        }

        handles.fill(0);
    }

    static uint64_t Capability(const int cardFd, const uint64_t capability)
    {
        uint64_t value(0);

        int result(drmGetCap(cardFd, capability, &value));

        if (result != 0) {
            TRACE_GLOBAL(Trace::Error, (("Failed to query capability 0x%016" PRIx64), capability));
        }

        return (result == 0) ? value : 0;
    }

    uint32_t CreateFrameBuffer(const int cardFd, Exchange::ICompositionBuffer* buffer)
    {
        ASSERT(cardFd > 0);
        ASSERT(buffer != nullptr);

        uint32_t framebufferId = 0;

        bool modifierSupported = (Capability(cardFd, DRM_CAP_ADDFB2_MODIFIERS) == 1) ? true : false;

        TRACE_GLOBAL(Trace::Information, ("Framebuffers with modifiers %s", modifierSupported ? "supported" : "unsupported"));

        uint16_t nPlanes(0);

        // buffer->Lock();

        std::array<uint32_t, 4> handles = { 0, 0, 0, 0 };
        std::array<uint32_t, 4> pitches = { 0, 0, 0, 0 };
        std::array<uint32_t, 4> offsets = { 0, 0, 0, 0 };
        std::array<uint64_t, 4> modifiers;

        modifiers.fill(buffer->Modifier());

        Exchange::ICompositionBuffer::IIterator* planes = buffer->Planes(10);
        ASSERT(planes != nullptr);

        while ((planes->Next() == true) && (planes->IsValid() == true)) {
            ASSERT(planes->IsValid() == true);

            Exchange::ICompositionBuffer::IPlane* plane = planes->Plane();
            ASSERT(plane != nullptr);

            if (drmPrimeFDToHandle(cardFd, plane->Accessor(), &handles[nPlanes]) != 0) {
                TRACE_GLOBAL(Trace::Error, ("Failed to acquirer drm handle from plane accessor"));
                CloseDrmHandles(cardFd, handles);
                break;
            }

            pitches[nPlanes] = plane->Stride();
            offsets[nPlanes] = plane->Offset();

            ++nPlanes;
        }

        buffer->Completed(false);

        if (modifierSupported && buffer->Modifier() != DRM_FORMAT_MOD_INVALID) {

            if (drmModeAddFB2WithModifiers(cardFd, buffer->Width(), buffer->Height(), buffer->Format(), handles.data(), pitches.data(), offsets.data(), modifiers.data(), &framebufferId, DRM_MODE_FB_MODIFIERS) != 0) {
                TRACE_GLOBAL(Trace::Error, ("Failed to allocate drm framebuffer with modifiers"));
            }
        } else {
            if (buffer->Modifier() != DRM_FORMAT_MOD_INVALID && buffer->Modifier() != DRM_FORMAT_MOD_LINEAR) {
                TRACE_GLOBAL(Trace::Error, ("Cannot import drm framebuffer with explicit modifier 0x%" PRIX64, buffer->Modifier()));
                return 0;
            }

            int ret = drmModeAddFB2(cardFd, buffer->Width(), buffer->Height(), buffer->Format(), handles.data(), pitches.data(), offsets.data(), &framebufferId, 0);

            if (ret != 0 && buffer->Format() == DRM_FORMAT_ARGB8888 /*&& nPlanes == 1*/ && offsets[0] == 0) {
                TRACE_GLOBAL(Trace::Error, ("Failed to allocate drm framebuffer (%s), falling back to old school drmModeAddFB", strerror(-ret)));

                uint32_t depth = 32;
                uint32_t bpp = 32;

                if (drmModeAddFB(cardFd, buffer->Width(), buffer->Height(), depth, bpp, pitches[0], handles[0], &framebufferId) != 0) {
                    TRACE_GLOBAL(Trace::Error, ("Failed to allocate a drm framebuffer the old school way..."));
                }

            } else if (ret != 0) {
                TRACE_GLOBAL(Trace::Error, ("Failed to allocate a drm framebuffer..."));
            }
        }

        CloseDrmHandles(cardFd, handles);

        // just unlock and go, we still need to draw something,.
        // buffer->Completed(false);

        TRACE_GLOBAL(Trace::Information, ("DRM framebuffer object %u allocated for buffer %p", framebufferId, buffer));

        return framebufferId;
    }

    void DestroyFrameBuffer(const int cardFd, const uint32_t frameBufferId)
    {
        int drmResult(0);

        if ((frameBufferId > 0) && (drmResult = drmModeRmFB(cardFd, frameBufferId) != 0)) {
            TRACE_GLOBAL(Trace::Error, ("Failed to destroy framebuffer %u: %s", frameBufferId, strerror(drmResult)));
        } else {
            TRACE_GLOBAL(Trace::Information, ("DRM framebuffer object %u destroyed %s", frameBufferId, strerror(drmResult)));
        }
    }

    bool HasNode(const drmDevice* drmDevice, const char* deviceName)
    {
        bool result(false);
        for (uint16_t i = 0; i < DRM_NODE_MAX; i++) {
            if ((drmDevice->available_nodes & (1 << i)) && (strcmp(drmDevice->nodes[i], deviceName) == 0)) {
                result = true;
                break;
            }
        }
        return result;
    }

    int OpenGPU(const string& gpuNode)
    {
        int fd(Compositor::DRM::InvalidFileDescriptor);

        ASSERT(drmAvailable() > 0);

        if (drmAvailable() > 0) {

            std::vector<string> nodes;

            GetNodes(DRM_NODE_PRIMARY, nodes);

            const auto& it = std::find(nodes.cbegin(), nodes.cend(), gpuNode);

            if (it != nodes.end()) {
                fd = open(it->c_str(), O_RDWR | O_CLOEXEC);
            } else {
                TRACE_GLOBAL(Trace::Error, ("Could not find gpu %s", gpuNode.c_str()));
            }
        }

        return fd;
    }

} // namespace DRM
} // namespace Compositor
} // namespace WPEFramework
