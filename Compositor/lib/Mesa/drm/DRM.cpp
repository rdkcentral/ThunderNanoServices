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
#include <interfaces/IGraphicsBuffer.h>
#include <CompositorTypes.h>

MODULE_NAME_ARCHIVE_DECLARATION

namespace Thunder {

namespace Compositor {

    using Identifier = uintptr_t;

    namespace DRM {

        /* simple stringification operator to make errorcodes human readable */
        #define CASE_TO_STRING(value) case value: return #value;

        const TCHAR* FormatToString(const uint32_t format)
        {
            switch (format) {
                CASE_TO_STRING(DRM_FORMAT_INVALID)
                CASE_TO_STRING(DRM_FORMAT_C8)
                CASE_TO_STRING(DRM_FORMAT_R8)
                CASE_TO_STRING(DRM_FORMAT_R16)
                CASE_TO_STRING(DRM_FORMAT_RG88)
                CASE_TO_STRING(DRM_FORMAT_GR88)
                CASE_TO_STRING(DRM_FORMAT_RG1616)
                CASE_TO_STRING(DRM_FORMAT_GR1616)
                CASE_TO_STRING(DRM_FORMAT_RGB332)
                CASE_TO_STRING(DRM_FORMAT_BGR233)
                CASE_TO_STRING(DRM_FORMAT_XRGB4444)
                CASE_TO_STRING(DRM_FORMAT_XBGR4444)
                CASE_TO_STRING(DRM_FORMAT_RGBX4444)
                CASE_TO_STRING(DRM_FORMAT_BGRX4444)
                CASE_TO_STRING(DRM_FORMAT_ARGB4444)
                CASE_TO_STRING(DRM_FORMAT_ABGR4444)
                CASE_TO_STRING(DRM_FORMAT_RGBA4444)
                CASE_TO_STRING(DRM_FORMAT_BGRA4444)
                CASE_TO_STRING(DRM_FORMAT_XRGB1555)
                CASE_TO_STRING(DRM_FORMAT_XBGR1555)
                CASE_TO_STRING(DRM_FORMAT_RGBX5551)
                CASE_TO_STRING(DRM_FORMAT_BGRX5551)
                CASE_TO_STRING(DRM_FORMAT_ARGB1555)
                CASE_TO_STRING(DRM_FORMAT_ABGR1555)
                CASE_TO_STRING(DRM_FORMAT_RGBA5551)
                CASE_TO_STRING(DRM_FORMAT_BGRA5551)
                CASE_TO_STRING(DRM_FORMAT_RGB565)
                CASE_TO_STRING(DRM_FORMAT_BGR565)
                CASE_TO_STRING(DRM_FORMAT_RGB888)
                CASE_TO_STRING(DRM_FORMAT_BGR888)
                CASE_TO_STRING(DRM_FORMAT_XRGB8888)
                CASE_TO_STRING(DRM_FORMAT_XBGR8888)
                CASE_TO_STRING(DRM_FORMAT_RGBX8888)
                CASE_TO_STRING(DRM_FORMAT_BGRX8888)
                CASE_TO_STRING(DRM_FORMAT_ARGB8888)
                CASE_TO_STRING(DRM_FORMAT_ABGR8888)
                CASE_TO_STRING(DRM_FORMAT_RGBA8888)
                CASE_TO_STRING(DRM_FORMAT_BGRA8888)
                CASE_TO_STRING(DRM_FORMAT_XRGB2101010)
                CASE_TO_STRING(DRM_FORMAT_XBGR2101010)
                CASE_TO_STRING(DRM_FORMAT_RGBX1010102)
                CASE_TO_STRING(DRM_FORMAT_BGRX1010102)
                CASE_TO_STRING(DRM_FORMAT_ARGB2101010)
                CASE_TO_STRING(DRM_FORMAT_ABGR2101010)
                CASE_TO_STRING(DRM_FORMAT_RGBA1010102)
                CASE_TO_STRING(DRM_FORMAT_BGRA1010102)
                CASE_TO_STRING(DRM_FORMAT_XRGB16161616)
                CASE_TO_STRING(DRM_FORMAT_XBGR16161616)
                CASE_TO_STRING(DRM_FORMAT_ARGB16161616)
                CASE_TO_STRING(DRM_FORMAT_ABGR16161616)
                CASE_TO_STRING(DRM_FORMAT_XRGB16161616F)
                CASE_TO_STRING(DRM_FORMAT_XBGR16161616F)
                CASE_TO_STRING(DRM_FORMAT_ARGB16161616F)
                CASE_TO_STRING(DRM_FORMAT_ABGR16161616F)
                CASE_TO_STRING(DRM_FORMAT_AXBXGXRX106106106106)
                CASE_TO_STRING(DRM_FORMAT_YUYV)
                CASE_TO_STRING(DRM_FORMAT_YVYU)
                CASE_TO_STRING(DRM_FORMAT_UYVY)
                CASE_TO_STRING(DRM_FORMAT_VYUY)
                CASE_TO_STRING(DRM_FORMAT_AYUV)
                CASE_TO_STRING(DRM_FORMAT_XYUV8888)
                CASE_TO_STRING(DRM_FORMAT_VUY888)
                CASE_TO_STRING(DRM_FORMAT_VUY101010)
                CASE_TO_STRING(DRM_FORMAT_Y210)
                CASE_TO_STRING(DRM_FORMAT_Y212)
                CASE_TO_STRING(DRM_FORMAT_Y216)
                CASE_TO_STRING(DRM_FORMAT_Y410)
                CASE_TO_STRING(DRM_FORMAT_Y412)
                CASE_TO_STRING(DRM_FORMAT_Y416)
                CASE_TO_STRING(DRM_FORMAT_XVYU2101010)
                CASE_TO_STRING(DRM_FORMAT_XVYU12_16161616)
                CASE_TO_STRING(DRM_FORMAT_XVYU16161616)
                CASE_TO_STRING(DRM_FORMAT_Y0L0)
                CASE_TO_STRING(DRM_FORMAT_X0L0)
                CASE_TO_STRING(DRM_FORMAT_Y0L2)
                CASE_TO_STRING(DRM_FORMAT_X0L2)
                CASE_TO_STRING(DRM_FORMAT_YUV420_8BIT)
                CASE_TO_STRING(DRM_FORMAT_YUV420_10BIT)
                CASE_TO_STRING(DRM_FORMAT_XRGB8888_A8)
                CASE_TO_STRING(DRM_FORMAT_XBGR8888_A8)
                CASE_TO_STRING(DRM_FORMAT_RGBX8888_A8)
                CASE_TO_STRING(DRM_FORMAT_BGRX8888_A8)
                CASE_TO_STRING(DRM_FORMAT_RGB888_A8)
                CASE_TO_STRING(DRM_FORMAT_BGR888_A8)
                CASE_TO_STRING(DRM_FORMAT_RGB565_A8)
                CASE_TO_STRING(DRM_FORMAT_BGR565_A8)
                CASE_TO_STRING(DRM_FORMAT_NV12)
                CASE_TO_STRING(DRM_FORMAT_NV21)
                CASE_TO_STRING(DRM_FORMAT_NV16)
                CASE_TO_STRING(DRM_FORMAT_NV61)
                CASE_TO_STRING(DRM_FORMAT_NV24)
                CASE_TO_STRING(DRM_FORMAT_NV42)
                CASE_TO_STRING(DRM_FORMAT_NV15)
                CASE_TO_STRING(DRM_FORMAT_P210)
                CASE_TO_STRING(DRM_FORMAT_P010)
                CASE_TO_STRING(DRM_FORMAT_P012)
                CASE_TO_STRING(DRM_FORMAT_P016)
#ifdef DRM_FORMAT_P030
                CASE_TO_STRING(DRM_FORMAT_P030)
#endif
                CASE_TO_STRING(DRM_FORMAT_Q410)
                CASE_TO_STRING(DRM_FORMAT_Q401)
                CASE_TO_STRING(DRM_FORMAT_YUV410)
                CASE_TO_STRING(DRM_FORMAT_YVU410)
                CASE_TO_STRING(DRM_FORMAT_YUV411)
                CASE_TO_STRING(DRM_FORMAT_YVU411)
                CASE_TO_STRING(DRM_FORMAT_YUV420)
                CASE_TO_STRING(DRM_FORMAT_YVU420)
                CASE_TO_STRING(DRM_FORMAT_YUV422)
                CASE_TO_STRING(DRM_FORMAT_YVU422)
                CASE_TO_STRING(DRM_FORMAT_YUV444)
                CASE_TO_STRING(DRM_FORMAT_YVU444)

            default:
                return _T("Unknown Format");
            }
        }

        const TCHAR* ModifierVendorString(const uint64_t modifier)
        {
            uint32_t vendor = (modifier & 0xff00000000000000ULL) << 56;

            switch (vendor) {
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_NONE)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_INTEL)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_AMD)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_NVIDIA)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_SAMSUNG)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_QCOM)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_VIVANTE)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_BROADCOM)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_ARM)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_ALLWINNER)
                CASE_TO_STRING(DRM_FORMAT_MOD_VENDOR_AMLOGIC)
            default:
                return "Unknown Vendor";
            }
        }

#undef CASE_TO_STRING

        bool HasAlpha(const uint32_t drmFormat)
        {
            return (
                (drmFormat == DRM_FORMAT_ARGB4444)
                || (drmFormat == DRM_FORMAT_ABGR4444)
                || (drmFormat == DRM_FORMAT_RGBA4444)
                || (drmFormat == DRM_FORMAT_BGRA4444)
                || (drmFormat == DRM_FORMAT_ARGB1555)
                || (drmFormat == DRM_FORMAT_ABGR1555)
                || (drmFormat == DRM_FORMAT_RGBA5551)
                || (drmFormat == DRM_FORMAT_BGRA5551)
                || (drmFormat == DRM_FORMAT_ARGB8888)
                || (drmFormat == DRM_FORMAT_ABGR8888)
                || (drmFormat == DRM_FORMAT_RGBA8888)
                || (drmFormat == DRM_FORMAT_BGRA8888)
                || (drmFormat == DRM_FORMAT_ARGB2101010)
                || (drmFormat == DRM_FORMAT_ABGR2101010)
                || (drmFormat == DRM_FORMAT_RGBA1010102)
                || (drmFormat == DRM_FORMAT_BGRA1010102)
                || (drmFormat == DRM_FORMAT_ARGB16161616)
                || (drmFormat == DRM_FORMAT_ABGR16161616)
                || (drmFormat == DRM_FORMAT_ARGB16161616F)
                || (drmFormat == DRM_FORMAT_ABGR16161616F));
        }

        constexpr int InvalidFileDescriptor = -1;

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
        int ReopenNode(const int fd, const bool openRenderNode)
        {
            char* name = drmGetDeviceNameFromFd2(fd);

            if (name == nullptr) {
                TRACE_GLOBAL(Trace::Error, ("%d is not a descriptor to a DRM Node... =^..^= ", fd));
                return InvalidFileDescriptor;
            } else {
                free(name);
                name = nullptr;
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

            if (name != nullptr) {
                free(name);
            }

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
         * @param buffer A pointer to an object that implements the Exchange::IGraphicsBuffer interface,
         * which represents a buffer used for composition.
         *
         * @return a uint32_t value, which is the ID of the allocated DRM framebuffer object.
         */
        uint32_t CreateFrameBuffer(const int cardFd, Exchange::IGraphicsBuffer* buffer)
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

            Exchange::IGraphicsBuffer::IIterator* planes = buffer->Acquire(Compositor::DefaultTimeoutMs);
            ASSERT(planes != nullptr);

            while (planes->Next() == true) {
                ASSERT(planes->IsValid() == true);

                if (drmPrimeFDToHandle(cardFd, planes->Descriptor(), &handles[nPlanes]) != 0) {
                    TRACE_GLOBAL(Trace::Error, ("Failed to acquirer drm handle from plane accessor"));
                    CloseDrmHandles(cardFd, handles);
                    break;
                }

                pitches[nPlanes] = planes->Stride();
                offsets[nPlanes] = planes->Offset();

                ++nPlanes;
            }

            buffer->Relinquish();

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
        int OpenGPU(const std::string& gpuNode)
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

        Identifier FindConnectorId(const int fd, const std::string& connectorName)
        {
            Identifier connectorId(InvalidIdentifier);

            if (fd > 0) {
                drmModeResPtr resources = drmModeGetResources(fd);

                if (resources != nullptr) {
                    for (uint8_t i = 0; i < resources->count_connectors; i++) {
                        drmModeConnectorPtr connector = drmModeGetConnector(fd, resources->connectors[i]);

                        if (nullptr != connector) {
                            char name[59];
                            int nameLength;
                            nameLength = snprintf(name, sizeof(name), "%s-%u", drmModeGetConnectorTypeName(connector->connector_type), connector->connector_type_id);
                            name[nameLength] = '\0';

                            if (connectorName.compare(name) == 0) {
                                connectorId = connector->connector_id;
                                break;
                            }

                            drmModeFreeConnector(connector);
                        }
                    }

                    drmModeFreeResources(resources);
                }
            }

            return connectorId;
        }

        std::string GetGPUNode(const std::string& connectorName)
        {
            std::string file;
            std::vector<std::string> nodes;

            GetNodes(DRM_NODE_PRIMARY, nodes);

            for (const auto& node : nodes) {
                int fd = ::open(node.c_str(), O_RDWR);

                if (FindConnectorId(fd, connectorName) != InvalidIdentifier) {
                    file = node;
                }

                ::close(fd);

                if (file.empty() == false) {
                    break;
                }
            }

            return file;
        }

        inline bool ModesEqual(const drmModeModeInfo* mode1, const drmModeModeInfo* mode2)
        {
            ASSERT(mode1 != nullptr);
            ASSERT(mode2 != nullptr);

            return (mode1->hdisplay == mode2->hdisplay && mode1->vdisplay == mode2->vdisplay && mode1->vrefresh == mode2->vrefresh);
        }

        bool SelectBestMode(const drmModeConnector* const connector,
            const uint32_t requestedWidth, const uint32_t requestedHeight,
            const uint32_t requestedRefreshRate, // in mHz (60000 = 60Hz)
            bool& dimensionsAdjusted, drmModeModeInfo& selectedMode)
        {
            if (connector->count_modes == 0) {
                TRACE_GLOBAL(Thunder::Trace::Error, ("No modes available for connector"));
                return false;
            }

            drmModeModeInfoPtr bestMode = nullptr;
            dimensionsAdjusted = false;
            uint32_t targetRefreshHz = requestedRefreshRate / 1000; // Convert mHz to Hz

            // Case 1: H=0, W=0, rate=0 -> Use monitor preferred setting
            if (requestedWidth == 0 && requestedHeight == 0 && requestedRefreshRate == 0) {
                // Find preferred mode
                for (int i = 0; i < connector->count_modes; i++) {
                    if (connector->modes[i].type & DRM_MODE_TYPE_PREFERRED) {
                        bestMode = &connector->modes[i];
                        break;
                    }
                }
                // Fallback to first mode if no preferred mode found
                if (bestMode == nullptr) {
                    bestMode = &connector->modes[0];
                }
                TRACE_GLOBAL(Thunder::Trace::Information,
                    ("Using preferred mode: %ux%u@%uHz",
                        bestMode->hdisplay, bestMode->vdisplay, bestMode->vrefresh));
            }
            // Case 2: H=x, W=x, rate=0 -> Find exact resolution, best refresh rate
            else if (requestedWidth > 0 && requestedHeight > 0 && requestedRefreshRate == 0) {
                uint32_t bestRefreshRate = 0;

                // Find all modes with exact resolution match
                for (int i = 0; i < connector->count_modes; i++) {
                    drmModeModeInfoPtr mode = &connector->modes[i];

                    if (mode->hdisplay == requestedWidth && mode->vdisplay == requestedHeight) {
                        // Pick the highest refresh rate for this resolution
                        if (mode->vrefresh > bestRefreshRate) {
                            bestRefreshRate = mode->vrefresh;
                            bestMode = mode;
                        }
                    }
                }

                if (bestMode != nullptr) {
                    TRACE_GLOBAL(Thunder::Trace::Information,
                        ("Found exact resolution %ux%u, selected best refresh rate %uHz",
                            requestedWidth, requestedHeight, bestMode->vrefresh));
                } else {
                    TRACE_GLOBAL(Thunder::Trace::Warning,
                        ("Exact resolution %ux%u not available, falling back",
                            requestedWidth, requestedHeight));
                }
            }
            // Case 3: H, W, rate all specified -> Find exact match or best possible
            else if (requestedWidth > 0 && requestedHeight > 0 && requestedRefreshRate > 0) {
                drmModeModeInfoPtr exactMatch = nullptr;
                drmModeModeInfoPtr resolutionMatch = nullptr;
                uint32_t closestRefreshDiff = UINT32_MAX;

                // Look for exact match first
                for (int i = 0; i < connector->count_modes; i++) {
                    drmModeModeInfoPtr mode = &connector->modes[i];

                    // Perfect match
                    if (mode->hdisplay == requestedWidth && mode->vdisplay == requestedHeight && mode->vrefresh == targetRefreshHz) {
                        exactMatch = mode;
                        break;
                    }

                    // Same resolution, different refresh rate
                    if (mode->hdisplay == requestedWidth && mode->vdisplay == requestedHeight) {
                        uint32_t refreshDiff = abs((int)mode->vrefresh - (int)targetRefreshHz);
                        if (refreshDiff < closestRefreshDiff) {
                            closestRefreshDiff = refreshDiff;
                            resolutionMatch = mode;
                        }
                    }
                }

                if (exactMatch != nullptr) {
                    bestMode = exactMatch;
                    TRACE_GLOBAL(Thunder::Trace::Information,
                        ("Found exact match: %ux%u@%uHz",
                            requestedWidth, requestedHeight, targetRefreshHz));
                } else if (resolutionMatch != nullptr) {
                    bestMode = resolutionMatch;
                    TRACE_GLOBAL(Thunder::Trace::Information,
                        ("Found resolution match %ux%u, closest refresh rate %uHz (requested %uHz)",
                            requestedWidth, requestedHeight, bestMode->vrefresh, targetRefreshHz));
                } else {
                    TRACE_GLOBAL(Thunder::Trace::Warning,
                        ("Exact resolution %ux%u not available, falling back",
                            requestedWidth, requestedHeight));
                }
            }

            // Fallback: If no match found, find best possible mode
            if (bestMode == nullptr) {
                dimensionsAdjusted = true;

                // Strategy: Find largest mode that fits, or smallest mode that's larger
                drmModeModeInfoPtr bestFit = nullptr;
                drmModeModeInfoPtr smallestLarger = nullptr;

                for (int i = 0; i < connector->count_modes; i++) {
                    drmModeModeInfoPtr mode = &connector->modes[i];

                    // Mode that can contain requested dimensions
                    if (mode->hdisplay >= requestedWidth && mode->vdisplay >= requestedHeight) {
                        if (smallestLarger == nullptr || (mode->hdisplay * mode->vdisplay) < (smallestLarger->hdisplay * smallestLarger->vdisplay)) {
                            smallestLarger = mode;
                        }
                    }

                    // Track best overall mode (for worst-case fallback)
                    if (bestFit == nullptr || (mode->type & DRM_MODE_TYPE_PREFERRED) || (mode->hdisplay * mode->vdisplay) > (bestFit->hdisplay * bestFit->vdisplay)) {
                        bestFit = mode;
                    }
                }

                // Use smallest mode that can contain request, or best available mode
                bestMode = smallestLarger ? smallestLarger : bestFit;

                if (bestMode == nullptr) {
                    bestMode = &connector->modes[0]; // Last resort
                }

                TRACE_GLOBAL(Thunder::Trace::Warning,
                    ("Using fallback mode: %ux%u@%uHz (requested %ux%u@%uHz)",
                        bestMode->hdisplay, bestMode->vdisplay, bestMode->vrefresh,
                        requestedWidth, requestedHeight, targetRefreshHz));
            }

            TRACE_GLOBAL(Trace::Information,
                ("Final selected mode: %ux%u@%uHz",
                    bestMode->hdisplay, bestMode->vdisplay, bestMode->vrefresh));

            selectedMode = *bestMode;
            return true;
        }

        ConnectorScanResult ScanConnector(const int backendFd, Thunder::Compositor::DRM::Identifier targetConnectorId,
            const uint32_t requestedWidth, const uint32_t requestedHeight, const uint32_t requestedRefreshRate)
        {
            ConnectorScanResult result;
            char* node = nullptr; // Declare at the top to avoid goto crossing initialization

            drmModeResPtr drmModeResources = drmModeGetResources(backendFd);
            drmModePlaneResPtr drmModePlaneResources = drmModeGetPlaneResources(backendFd);

            if (!drmModeResources || !drmModePlaneResources) {
                TRACE_GLOBAL(Thunder::Trace::Error, ("Failed to get DRM resources"));
                goto cleanup;
            }

            TRACE_GLOBAL(Trace::Information, ("Found %d connectors", drmModeResources->count_connectors));
            TRACE_GLOBAL(Trace::Information, ("Found %d encoders", drmModeResources->count_encoders));
            TRACE_GLOBAL(Trace::Information, ("Found %d CRTCs", drmModeResources->count_crtcs));
            TRACE_GLOBAL(Trace::Information, ("Found %d planes", drmModePlaneResources->count_planes));

            for (int c = 0; c < drmModeResources->count_connectors; c++) {
                drmModeConnectorPtr drmModeConnector = drmModeGetConnector(backendFd, drmModeResources->connectors[c]);

                if (!drmModeConnector) {
                    continue;
                }

                // Find our target connector if it's connected
                if ((drmModeConnector->connector_id != targetConnectorId) || (drmModeConnector->connection != DRM_MODE_CONNECTED)) {
                    drmModeFreeConnector(drmModeConnector);
                    continue;
                }

                const char* name = drmModeGetConnectorTypeName(drmModeConnector->connector_type);
                TRACE_GLOBAL(Trace::Information, ("Connector %s-%u connected", name, drmModeConnector->connector_type_id));

                // Select the best mode based on requirements
                if (!SelectBestMode(drmModeConnector, requestedWidth, requestedHeight, requestedRefreshRate, result.dimensionsAdjusted, result.selectedMode)) {
                    TRACE_GLOBAL(Thunder::Trace::Error, ("Failed to select a suitable mode for connector %u", targetConnectorId));
                    drmModeFreeConnector(drmModeConnector);
                    goto cleanup;
                }

                // Find the encoder (a deprecated KMS object) for this connector
                if (drmModeConnector->encoder_id == Thunder::Compositor::InvalidIdentifier) {
                    TRACE_GLOBAL(Thunder::Trace::Error, ("No encoder for connector %u", targetConnectorId));
                    drmModeFreeConnector(drmModeConnector);
                    goto cleanup;
                }

                // Get a bitmask of CRTCs the connector is compatible with
                uint32_t possibleCrtcs = drmModeConnectorGetPossibleCrtcs(backendFd, drmModeConnector);
                if (possibleCrtcs == 0) {
                    TRACE_GLOBAL(Thunder::Trace::Error, ("No CRTC possible for connector %u", drmModeConnector->connector_id));
                    drmModeFreeConnector(drmModeConnector);
                    goto cleanup;
                }

                // Find the encoder for this connector
                drmModeEncoderPtr encoder = nullptr;
                for (uint8_t e = 0; e < drmModeResources->count_encoders; e++) {
                    if (drmModeResources->encoders[e] == drmModeConnector->encoder_id) {
                        encoder = drmModeGetEncoder(backendFd, drmModeResources->encoders[e]);
                        break;
                    }
                }

                if (!encoder) {
                    TRACE_GLOBAL(Thunder::Trace::Error, ("Failed to get encoder for connector %u", drmModeConnector->connector_id));
                    drmModeFreeConnector(drmModeConnector);
                    goto cleanup;
                }

                // Find the CRTC currently used by this connector
                if (encoder->crtc_id == Thunder::Compositor::InvalidIdentifier) {
                    TRACE_GLOBAL(Thunder::Trace::Error, ("Invalid encoder CRTC ID for connector %u", targetConnectorId));
                    drmModeFreeEncoder(encoder);
                    drmModeFreeConnector(drmModeConnector);
                    goto cleanup;
                }

                TRACE_GLOBAL(Thunder::Trace::Information, ("Start looking for crtc_id: %d", encoder->crtc_id));

                drmModeCrtcPtr crtc = nullptr;

                for (uint8_t c = 0; c < drmModeResources->count_crtcs; c++) {
                    if (drmModeResources->crtcs[c] == encoder->crtc_id) {
                        crtc = drmModeGetCrtc(backendFd, drmModeResources->crtcs[c]);
                        break;
                    }
                }

                if (!crtc || crtc->crtc_id == 0) {
                    TRACE_GLOBAL(Thunder::Trace::Error, ("Invalid CRTC for connector %u", drmModeConnector->connector_id));
                    if (crtc)
                        drmModeFreeCrtc(crtc);
                    drmModeFreeEncoder(encoder);
                    drmModeFreeConnector(drmModeConnector);
                    goto cleanup;
                }

                /*
                 * On the banana pi, the following ASSERT fired but, although the documentation
                 * suggests that buffer_id means disconnected, the screen was not disconnected
                 * Hence why the ASSERT is, until further investigation, commented out !!!
                 * ASSERT(crtc->buffer_id != 0);
                 */

                /*
                 * The kernel doesn't directly tell us what it considers to be the
                 * single primary plane for this CRTC (i.e. what would be updated
                 * by drmModeSetCrtc), but if it's already active then we can cheat
                 * by looking for something displaying the same framebuffer ID,
                 * since that information is duplicated.
                 */
                drmModePlanePtr plane = nullptr;
                for (uint8_t p = 0; p < drmModePlaneResources->count_planes; p++) {
                    plane = drmModeGetPlane(backendFd, drmModePlaneResources->planes[p]);

                    TRACE_GLOBAL(Trace::Information, ("[PLANE: %" PRIu32 "] CRTC ID %" PRIu32 ", FB ID %" PRIu32, plane->plane_id, plane->crtc_id, plane->fb_id));

                    if ((plane->crtc_id == crtc->crtc_id) && (plane->fb_id == crtc->buffer_id)) {
                        break;
                    }

                    drmModeFreePlane(plane);
                    plane = nullptr;
                }

                if (!plane) {
                    TRACE_GLOBAL(Thunder::Trace::Error, ("No primary plane found for CRTC %" PRIu32, crtc->crtc_id));
                    drmModeFreeCrtc(crtc);
                    drmModeFreeEncoder(encoder);
                    drmModeFreeConnector(drmModeConnector);
                    goto cleanup;
                }

                // Calculate refresh rate for logging
                uint64_t refresh = ((crtc->mode.clock * 1000000LL / crtc->mode.htotal) + (crtc->mode.vtotal / 2)) / crtc->mode.vtotal;

                TRACE_GLOBAL(Trace::Information, ("[CRTC:%" PRIu32 ", CONN %" PRIu32 ", PLANE %" PRIu32 "]: active at %ux%u, %" PRIu64 " mHz", crtc->crtc_id, drmModeConnector->connector_id, plane->plane_id, crtc->width, crtc->height, refresh));

                // Store the IDs for later initialization
                result.ids.crtcId = crtc->crtc_id;
                result.ids.planeId = plane->plane_id;
                result.success = true;

                // Check if we need to change modes
                result.needsModeSet = !ModesEqual(&result.selectedMode, &crtc->mode);

                if (!result.needsModeSet) {
                    // Use current mode to ensure exact match (handles any minor differences)
                    result.selectedMode = crtc->mode;
                    TRACE_GLOBAL(Trace::Information, ("Using current mode: %ux%u@%uHz", crtc->mode.hdisplay, crtc->mode.vdisplay, crtc->mode.vrefresh));
                } else {
                    TRACE_GLOBAL(Trace::Information, ("Mode change needed: %ux%u@%uHz -> %ux%u@%uHz", crtc->mode.hdisplay, crtc->mode.vdisplay, crtc->mode.vrefresh, result.selectedMode.hdisplay, result.selectedMode.vdisplay, result.selectedMode.vrefresh));
                }

                // Cleanup for success case
                drmModeFreePlane(plane);
                drmModeFreeCrtc(crtc);
                drmModeFreeEncoder(encoder);
                drmModeFreeConnector(drmModeConnector);
                break;
            }

            // Get GPU node name
            node = drmGetDeviceNameFromFd2(backendFd); // Now assignment, not initialization
            if (node) {
                result.gpuNode = node;
                free(node);
            }

        cleanup:
            if (drmModeResources) {
                drmModeFreeResources(drmModeResources);
            }
            if (drmModePlaneResources) {
                drmModeFreePlaneResources(drmModePlaneResources);
            }

            return result;
        }
    } // namespace DRM
} // namespace Compositor
} // namespace Thunder
