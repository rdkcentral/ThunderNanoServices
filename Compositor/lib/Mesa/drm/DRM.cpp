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

        uint16_t GetBlobProperty(const int cardFd, const Identifier object, const Identifier property, const uint16_t /* blobSize */, uint8_t blob[])
        {
            uint16_t length(0);
            uint64_t id;

            if (GetProperty(cardFd, object, property, id) == true) {
                drmModePropertyBlobRes* drmBlob = drmModeGetPropertyBlob(cardFd, id);
                ASSERT(drmBlob != nullptr);
                // ASSERT(blobSize >= drmBlob->length);

                memcpy(blob, drmBlob->data, drmBlob->length);
                length = drmBlob->length;

                drmModeFreePropertyBlob(drmBlob);
            }

            return length;
        }

        void GetNodes(const uint32_t type, std::vector<string>& list)
        {
            const int nDrmDevices = drmGetDevices2(0, nullptr, 0);

            drmDevicePtr* devices = static_cast<drmDevicePtr*>(ALLOCA(nDrmDevices * sizeof(drmDevicePtr)));

            int device_count = drmGetDevices2(0 /* flags */, devices, nDrmDevices);

            if (device_count > 0) {
                for (uint8_t i = 0; i < device_count; i++) {
                    TRACE_GLOBAL(Trace::Information, ("DRM node %p 0x%04X: Primary %s,  Render  %s", devices[i], devices[i]->available_nodes, (devices[i]->available_nodes & (1 << DRM_NODE_PRIMARY)) ? devices[i]->nodes[DRM_NODE_PRIMARY] : "Unavailable", (devices[i]->available_nodes & (1 << DRM_NODE_RENDER)) ? devices[i]->nodes[DRM_NODE_RENDER] : "Unavailable"));

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

        bool HasCapability(const int cardFd, const uint64_t option)
        {
            uint64_t cap(0);
            int r = drmGetCap(cardFd, option, &cap);
            return ((r == 0) && (cap > 0));
        }
        /*
         * Re-open the DRM node to avoid GEM handle ref'counting issues.
         * See: https://gitlab.freedesktop.org/mesa/drm/-/merge_requests/110
         *
         */
        int ReopenNode(int fd, bool openRenderNode)
        {
            if (drmGetDeviceNameFromFd2(fd) == nullptr) {
                TRACE_GLOBAL(Trace::Error, ("%d is not a descriptor to a DRM Node... =^..^= ", fd));
                return InvalidFileDescriptor;
            }

            if (drmIsMaster(fd)) {
                // Only recent kernels support empty leases
                uint32_t lessee_id;
                int lease_fd = drmModeCreateLease(fd, nullptr, 0, O_CLOEXEC, &lessee_id);

                if (lease_fd >= 0) {
                    return lease_fd;
                } else if (lease_fd != -EINVAL && lease_fd != -EOPNOTSUPP) {
                    TRACE_GLOBAL(Trace::Error, ("drmModeCreateLease failed %s", strerror(errno)));
                    return InvalidFileDescriptor;
                }
                TRACE_GLOBAL(Trace::Information, ("drmModeCreateLease failed: %s, falling back to plain open", strerror(errno)));
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

                TRACE_GLOBAL(Trace::Information, ("No render node found, using primary node: '%s'", name));
            } else {
                TRACE_GLOBAL(Trace::Information, ("DRM Render Node: %s", name));
            }

            int newFd = open(name, O_RDWR | O_CLOEXEC);

            if (newFd < 0) {
                TRACE_GLOBAL(Trace::Error, ("Failed to open DRM node '%s'", name));
                free(name);
                return InvalidFileDescriptor;
            } 

            free(name);

            // If we're using a DRM primary node (e.g. because we're running under the
            // DRM backend, or because we're on split render/display machine), we need
            // to use the legacy DRM authentication mechanism to have the permission to
            // manipulate buffers.
            if (drmIsMaster(fd) && drmGetNodeTypeFromFd(newFd) == DRM_NODE_PRIMARY) {
                drm_magic_t magic(0);
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

        /**
         * This function closes DRM handles for multiple planes and avoids double-closing if multiple planes
         * share the same handle.
         *
         * @param cardFd An integer representing the file descriptor for the DRM device.
         * @param handles An array of four uint32_t values representing DRM handles for buffer objects.
         */
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
                    } else if (drmCloseBufferHandle(cardFd, handles.at(currentIndex)) != 0) {
                        TRACE_GLOBAL(Trace::Error, ("Failed to close drm handle %u", handles.at(currentIndex)));
                    }
                }
            }

            handles.fill(0);
        }

        /**
         * The function queries a capability of a given card file descriptor and returns its value.
         *
         * @param cardFd The file descriptor for the DRM device representing the graphics card.
         * @param capability The capability to query from the DRM (Direct Rendering Manager) device driver. It
         * is a 64-bit unsigned integer value.
         *
         * @return a uint64_t value, which is either the queried capability value (if the query was successful)
         * or 0 (if the query failed).
         */
        static uint64_t Capability(const int cardFd, const uint64_t capability)
        {
            uint64_t value(0);

            int result(drmGetCap(cardFd, capability, &value));

            if (result != 0) {
                TRACE_GLOBAL(Trace::Error, (("Failed to query capability 0x%016" PRIx64), capability));
            }

            return (result == 0) ? value : 0;
        }

        /**
         * This function creates a DRM framebuffer object for a given composition buffer.
         *
         * @param cardFd A file descriptor for the DRM device.
         * @param buffer A pointer to an object that implements the Exchange::ICompositionBuffer interface,
         * which represents a buffer used for composition.
         *
         * @return a uint32_t value, which is the ID of the allocated DRM framebuffer object.
         */
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
            buffer->Completed(false);

            TRACE_GLOBAL(Trace::Information, ("DRM framebuffer object %u allocated for buffer %p", framebufferId, buffer));

            return framebufferId;
        }

        /**
         * This function destroys a DRM framebuffer object.
         *
         * @param cardFd The file descriptor for the DRM device, which is used to communicate with the graphics
         * hardware.
         * @param frameBufferId The ID of the framebuffer object that needs to be destroyed.
         */
        void DestroyFrameBuffer(const int cardFd, const uint32_t frameBufferId)
        {
            int drmResult(0);

            if ((frameBufferId > 0) && (drmResult = drmModeRmFB(cardFd, frameBufferId) != 0)) {
                TRACE_GLOBAL(Trace::Error, ("Failed to destroy framebuffer %u: %s", frameBufferId, strerror(drmResult)));
            } else {
                TRACE_GLOBAL(Trace::Information, ("DRM framebuffer object %u destroyed %s", frameBufferId, strerror(drmResult)));
            }
        }

        /**
         * The function checks if a given device name exists in the available nodes of a DRM device.
         *
         * @param drmDevice A pointer to a structure representing a DRM device.
         * @param deviceName A pointer to a character array representing the name of a device node.
         *
         * @return a boolean value indicating whether a given device name exists in the available nodes of a
         * given drmDevice structure.
         */
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

        /**
         * The function returns the node associated with a given device and type.
         *
         * @param type an unsigned 32-bit integer representing the type of node to retrieve (e.g.
         * DRM_NODE_PRIMARY, DRM_NODE_CONTROL, etc.)
         * @param device The pointer to a drmDevice structure representing the device for which the node is
         * being requested.
         *
         * @return a string that represents the node associated with the specified device type.
         */
        string GetNode(const uint32_t type, drmDevice* device)
        {
            string node;

            const int nDrmDevices = drmGetDevices2(0, nullptr, 0);

            drmDevicePtr* devices = static_cast<drmDevicePtr*>(ALLOCA(nDrmDevices * sizeof(drmDevicePtr)));

            drmGetDevices2(0 /* flags */, devices, nDrmDevices);

            for (int i = 0; i < nDrmDevices; ++i) {
                drmDevice* dev = devices[i];
                if (device && !drmDevicesEqual(device, dev)) {
                    continue;
                }
                TRACE_GLOBAL(Trace::Information, ("DRM node %p 0x%04X: Primary %s,  Render  %s", dev, dev->available_nodes, (dev->available_nodes & (1 << DRM_NODE_PRIMARY)) ? dev->nodes[DRM_NODE_PRIMARY] : "Unavailable", (dev->available_nodes & (1 << DRM_NODE_RENDER)) ? dev->nodes[DRM_NODE_RENDER] : "Unavailable"));

                if (!(dev->available_nodes & (1 << type)))
                    continue;

                node = dev->nodes[type];
                break;
            }

            drmFreeDevices(devices, nDrmDevices);
            return node;
        }

        /**
         * The function opens a file descriptor for a specified GPU node if it is available.
         *
         * @param gpuNode The name of the GPU node to open.
         *
         * @return an integer value, which is the file descriptor of the opened GPU node. If the GPU node
         * cannot be found, it returns the value of Compositor::DRM::InvalidFileDescriptor.
         */
        int OpenGPU(const string& gpuNode)
        {
            int fd(Compositor::DRM::InvalidFileDescriptor);

            ASSERT(drmAvailable() > 0);

            std::vector<string> nodes;

            GetNodes(DRM_NODE_PRIMARY, nodes);

            const auto& it = std::find(nodes.cbegin(), nodes.cend(), gpuNode);

            if (it != nodes.end()) {
                fd = open(it->c_str(), O_RDWR | O_CLOEXEC);
            }

            return fd;
        }

    } // namespace DRM
} // namespace Compositor
} // namespace WPEFramework
