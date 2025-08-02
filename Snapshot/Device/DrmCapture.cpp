/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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

#include "../Module.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <interfaces/ICapture.h>

namespace Thunder {
namespace Plugin {

    class DrmCapture : public Exchange::ICapture {
    private:
        DrmCapture(const DrmCapture&) = delete;
        DrmCapture& operator=(const DrmCapture&) = delete;

        struct DisplayInfo {
            uint32_t connectorId;
            uint32_t crtcId;
            uint32_t width;
            uint32_t height;
            int32_t x;
            int32_t y;
            bool active;
            uint32_t framebufferId;
            int drmFd; // Track which DRM device this display belongs to
            std::string devicePath; // Store device path for debugging

            DisplayInfo()
                : connectorId(0)
                , crtcId(0)
                , width(0)
                , height(0)
                , x(0)
                , y(0)
                , active(false)
                , framebufferId(0)
                , drmFd(-1)
            {
            }
        };

        struct FramebufferInfo {
            uint32_t width;
            uint32_t height;
            uint32_t pitch;
            uint32_t bpp;
            uint32_t depth;
            uint32_t handle;
            void* map;
            size_t size;

            FramebufferInfo()
                : width(0)
                , height(0)
                , pitch(0)
                , bpp(0)
                , depth(0)
                , handle(0)
                , map(nullptr)
                , size(0)
            {
            }
        };

        struct DrmDevice {
            int fd;
            std::string path;
            bool hasDisplays;

            DrmDevice(int deviceFd, const std::string& devicePath)
                : fd(deviceFd)
                , path(devicePath)
                , hasDisplays(false)
            {
            }
        };

    public:
        DrmCapture()
            : _totalWidth(0)
            , _totalHeight(0)
            , _minX(0)
            , _minY(0)
        {
            Initialize();
            TRACE(Trace::Information, (_T("DRM Capture: Multi-GPU Initialized: Build: %s"), __TIMESTAMP__));
        }

        ~DrmCapture() override
        {
            Deinitialize();
        }

        BEGIN_INTERFACE_MAP(DrmCapture)
        INTERFACE_ENTRY(Exchange::ICapture)
        END_INTERFACE_MAP

        const TCHAR* Name() const override
        {
            return (_T("DRM"));
        }

        bool Capture(ICapture::IStore& storer) override
        {
            if (_drmDevices.empty()) {
                return false;
            }

            // Refresh display information to handle hot-plug events
            if (!RefreshDisplays()) {
                TRACE(Trace::Error, (_T("No displays found for DRM capture")));
                return false;
            }

            if (_displays.empty()) {
                TRACE(Trace::Error, (_T("No active displays to capture")));
                return false;
            }

            // Calculate the combined composition dimensions
            CalculateCompositionBounds();

            TRACE(Trace::Information, (_T("Total composition size: %dx%d with min offset (%d, %d)"), _totalWidth, _totalHeight, _minX, _minY));

            // Create combined buffer
            std::vector<uint8_t> combinedBuffer(_totalWidth * _totalHeight * 4, 0);

            // Capture each active display and composite into combined buffer
            for (const auto& display : _displays) {
                if (!display.active) {
                    continue;
                }

                if (!CaptureDisplay(display, combinedBuffer)) {
                    TRACE(Trace::Error, (_T("Failed to capture display %d on device %s"), display.connectorId, display.devicePath.c_str()));
                    continue; // Skip this display but continue with others
                }
            }

            TRACE(Trace::Information, (_T("Captured %d displays across %d GPUs with total size %dx%d"), _displays.size(), _drmDevices.size(), _totalWidth, _totalHeight));

            // Store the combined buffer
            return storer.R8_G8_B8_A8(combinedBuffer.data(), _totalWidth, _totalHeight);
        }

    private:
        bool Initialize()
        {
            // Use drmGetDevices2 to enumerate DRM devices
            drmDevice** devices = nullptr;
            int deviceCount = drmGetDevices2(0, nullptr, 0);

            TRACE(Trace::Information, (_T("DRM Capture: Found %d devices"), deviceCount));

            if (deviceCount <= 0) {
                TRACE(Trace::Error, (_T("No DRM devices found")));
                return false;
            }

            devices = new drmDevice*[deviceCount];
            deviceCount = drmGetDevices2(0, devices, deviceCount);

            if (deviceCount <= 0) {
                delete[] devices;
                TRACE(Trace::Error, (_T("No DRM devices found after enumeration")));
                return false;
            }

            TRACE(Trace::Information, (_T("DRM Capture: Enumerated %d devices"), deviceCount));

            // Try to open ALL available DRM devices with primary nodes
            int openedDevices = 0;
            for (int i = 0; i < deviceCount; ++i) {
                if (devices[i]->available_nodes & (1 << DRM_NODE_PRIMARY)) {
                    TRACE(Trace::Information, (_T("DRM Capture: Attempting to open device %s"), devices[i]->nodes[DRM_NODE_PRIMARY]));
                    int drmFd = open(devices[i]->nodes[DRM_NODE_PRIMARY], O_RDONLY);

                    if (drmFd >= 0) {
                        TRACE(Trace::Information, (_T("DRM Capture: Successfully opened device %s"), devices[i]->nodes[DRM_NODE_PRIMARY]));
                        _drmDevices.emplace_back(drmFd, devices[i]->nodes[DRM_NODE_PRIMARY]);
                        openedDevices++;
                    } else {
                        TRACE(Trace::Error, (_T("DRM Capture: Failed to open device %s: %s"), devices[i]->nodes[DRM_NODE_PRIMARY], strerror(errno)));
                    }
                }
            }

            TRACE(Trace::Information, (_T("DRM Capture initialized with %d/%d devices"), openedDevices, deviceCount));

            drmFreeDevices(devices, deviceCount);
            delete[] devices;

            if (_drmDevices.empty()) {
                TRACE(Trace::Error, (_T("Failed to open any DRM device")));
                return false;
            }

            TRACE(Trace::Information, (_T("DRM Capture initialized successfully with %d devices"), _drmDevices.size()));

            return RefreshDisplays();
        }

        void Deinitialize()
        {
            // Close all DRM device file descriptors
            for (auto& device : _drmDevices) {
                if (device.fd >= 0) {
                    close(device.fd);
                    device.fd = -1;
                }
            }
            _drmDevices.clear();
            _displays.clear();
        }

        bool RefreshDisplays()
        {
            if (_drmDevices.empty()) {
                return false;
            }

            _displays.clear();

            // Scan all DRM devices for displays
            for (auto& device : _drmDevices) {
                if (device.fd < 0) {
                    continue;
                }

                bool foundDisplaysOnDevice = ScanDeviceForDisplays(device);
                device.hasDisplays = foundDisplaysOnDevice;

                if (foundDisplaysOnDevice) {
                    TRACE(Trace::Information, (_T("Found displays on device: %s"), device.path.c_str()));
                } else {
                    TRACE(Trace::Error, (_T("No displays found on device: %s"), device.path.c_str()));
                }
            }

            TRACE(Trace::Information, (_T("Total displays found across all devices: %d"), _displays.size()));
            return !_displays.empty();
        }

        bool ScanDeviceForDisplays(const DrmDevice& device)
        {
            drmModeRes* resources = drmModeGetResources(device.fd);
            if (!resources) {
                TRACE(Trace::Error, (_T("Failed to get DRM resources for device: %s"), device.path.c_str()));
                return false;
            }

            bool foundDisplays = false;

            TRACE(Trace::Information, (_T("Scanning device %s with %d connectors"), device.path.c_str(), resources->count_connectors));

            // Enumerate all connectors on this device
            for (int i = 0; i < resources->count_connectors; ++i) {
                drmModeConnector* connector = drmModeGetConnector(device.fd, resources->connectors[i]);
                if (!connector) {
                    continue;
                }

                // Only process connected displays
                if (connector->connection == DRM_MODE_CONNECTED && connector->encoder_id) {
                    TRACE(Trace::Information, (_T("Found connector %d with %d modes on device %s"), connector->connector_id, connector->count_modes, device.path.c_str()));

                    drmModeEncoder* encoder = drmModeGetEncoder(device.fd, connector->encoder_id);
                    if (encoder && encoder->crtc_id) {
                        drmModeCrtc* crtc = drmModeGetCrtc(device.fd, encoder->crtc_id);
                        if (crtc && crtc->mode_valid && crtc->buffer_id) {
                            DisplayInfo display;
                            display.connectorId = connector->connector_id;
                            display.crtcId = encoder->crtc_id;
                            display.width = crtc->width;
                            display.height = crtc->height;
                            display.x = crtc->x;
                            display.y = crtc->y;
                            display.active = true;
                            display.framebufferId = crtc->buffer_id;
                            display.drmFd = device.fd;
                            display.devicePath = device.path;

                            _displays.push_back(display);
                            foundDisplays = true;

                            TRACE(Trace::Information, (_T("Display %d on %s: %dx%d at (%d, %d)"), display.connectorId, device.path.c_str(), display.width, display.height, display.x, display.y));
                        }
                        drmModeFreeCrtc(crtc);
                    }
                    drmModeFreeEncoder(encoder);
                }

                drmModeFreeConnector(connector);
            }

            drmModeFreeResources(resources);
            return foundDisplays;
        }

        void CalculateCompositionBounds()
        {
            if (_displays.empty()) {
                _totalWidth = 0;
                _totalHeight = 0;
                _minX = 0;
                _minY = 0;
                return;
            }

            int32_t maxX = _displays[0].x + static_cast<int32_t>(_displays[0].width);
            int32_t maxY = _displays[0].y + static_cast<int32_t>(_displays[0].height);
            _minX = _displays[0].x;
            _minY = _displays[0].y;

            for (const auto& display : _displays) {
                if (!display.active) {
                    continue;
                }

                _minX = std::min(_minX, display.x);
                _minY = std::min(_minY, display.y);
                maxX = std::max(maxX, display.x + static_cast<int32_t>(display.width));
                maxY = std::max(maxY, display.y + static_cast<int32_t>(display.height));
            }

            _totalWidth = static_cast<uint32_t>(maxX - _minX);
            _totalHeight = static_cast<uint32_t>(maxY - _minY);
        }

        bool CaptureDisplay(const DisplayInfo& display, std::vector<uint8_t>& combinedBuffer)
        {
            if (display.framebufferId == 0 || display.drmFd < 0) {
                return false;
            }

            FramebufferInfo fbInfo;
            if (!GetFramebufferInfo(display.drmFd, display.framebufferId, fbInfo)) {
                return false;
            }

            void* fbMap = MapFramebuffer(display.drmFd, fbInfo);
            if (!fbMap) {
                return false;
            }

            // Copy framebuffer data to combined buffer at correct position
            TRACE(Trace::Information, (_T("Capturing display %d on %s at (%d, %d) with size %dx%d"), display.connectorId, display.devicePath.c_str(), display.x, display.y, fbInfo.width, fbInfo.height));

            CopyFramebufferToBuffer(display, fbInfo, fbMap, combinedBuffer);

            UnmapFramebuffer(fbInfo);

            return true;
        }

        bool GetFramebufferInfo(int drmFd, uint32_t framebufferId, FramebufferInfo& fbInfo)
        {
            drmModeFB* fb = drmModeGetFB(drmFd, framebufferId);
            if (!fb) {
                return false;
            }

            fbInfo.width = fb->width;
            fbInfo.height = fb->height;
            fbInfo.pitch = fb->pitch;
            fbInfo.bpp = fb->bpp;
            fbInfo.depth = fb->depth;
            fbInfo.handle = fb->handle;
            fbInfo.size = fb->pitch * fb->height;

            drmModeFreeFB(fb);

            return true;
        }

        void* MapFramebuffer(int drmFd, const FramebufferInfo& fbInfo)
        {
            drm_mode_map_dumb mapReq = {};
            mapReq.handle = fbInfo.handle;

            if (drmIoctl(drmFd, DRM_IOCTL_MODE_MAP_DUMB, &mapReq) != 0) {
                return nullptr;
            }

            void* map = mmap(0, fbInfo.size, PROT_READ, MAP_SHARED, drmFd, mapReq.offset);
            if (map == MAP_FAILED) {
                return nullptr;
            }

            return map;
        }

        void UnmapFramebuffer(const FramebufferInfo& fbInfo)
        {
            if (fbInfo.map) {
                munmap(fbInfo.map, fbInfo.size);
            }
        }

        void CopyFramebufferToBuffer(const DisplayInfo& display, const FramebufferInfo& fbInfo, void* fbMap, std::vector<uint8_t>& combinedBuffer)
        {
            if (!fbMap) {
                return;
            }

            // Calculate position in combined buffer
            int32_t destX = display.x - _minX;
            int32_t destY = display.y - _minY;

            // Ensure we don't go out of bounds
            if (destX < 0 || destY < 0 || destX + static_cast<int32_t>(display.width) > static_cast<int32_t>(_totalWidth) || destY + static_cast<int32_t>(display.height) > static_cast<int32_t>(_totalHeight)) {
                return;
            }

            const uint8_t* src = static_cast<const uint8_t*>(fbMap);
            uint8_t* dest = combinedBuffer.data();

            TRACE(Trace::Information, (_T("Copying display %d from %s to combined buffer at (%d, %d) with %d bpp"), display.connectorId, display.devicePath.c_str(), destX, destY, fbInfo.bpp));

            // Handle different pixel formats
            if (fbInfo.bpp == 32) {
                // ARGB8888 or XRGB8888
                for (uint32_t y = 0; y < display.height; ++y) {
                    for (uint32_t x = 0; x < display.width; ++x) {
                        const uint32_t srcOffset = (y * fbInfo.pitch) + (x * 4);
                        const uint32_t destOffset = ((destY + y) * _totalWidth + (destX + x)) * 4;

                        if (destOffset + 3 < combinedBuffer.size() && srcOffset + 3 < fbInfo.size) {
                            // Convert from BGRA to RGBA
                            dest[destOffset + 0] = src[srcOffset + 2]; // Red
                            dest[destOffset + 1] = src[srcOffset + 1]; // Green
                            dest[destOffset + 2] = src[srcOffset + 0]; // Blue
                            dest[destOffset + 3] = src[srcOffset + 3]; // Alpha
                        }
                    }
                }
            } else if (fbInfo.bpp == 24) {
                // RGB888
                for (uint32_t y = 0; y < display.height; ++y) {
                    for (uint32_t x = 0; x < display.width; ++x) {
                        const uint32_t srcOffset = (y * fbInfo.pitch) + (x * 3);
                        const uint32_t destOffset = ((destY + y) * _totalWidth + (destX + x)) * 4;

                        if (destOffset + 3 < combinedBuffer.size() && srcOffset + 2 < fbInfo.size) {
                            // Convert from BGR to RGBA
                            dest[destOffset + 0] = src[srcOffset + 2]; // Red
                            dest[destOffset + 1] = src[srcOffset + 1]; // Green
                            dest[destOffset + 2] = src[srcOffset + 0]; // Blue
                            dest[destOffset + 3] = 255; // Alpha
                        }
                    }
                }
            } else if (fbInfo.bpp == 16) {
                // RGB565
                for (uint32_t y = 0; y < display.height; ++y) {
                    for (uint32_t x = 0; x < display.width; ++x) {
                        const uint32_t srcOffset = (y * fbInfo.pitch) + (x * 2);
                        const uint32_t destOffset = ((destY + y) * _totalWidth + (destX + x)) * 4;

                        if (destOffset + 3 < combinedBuffer.size() && srcOffset + 1 < fbInfo.size) {
                            const uint16_t pixel = *reinterpret_cast<const uint16_t*>(&src[srcOffset]);

                            // Extract RGB components from 565 format
                            dest[destOffset + 0] = ((pixel >> 11) & 0x1F) << 3; // Red
                            dest[destOffset + 1] = ((pixel >> 5) & 0x3F) << 2; // Green
                            dest[destOffset + 2] = (pixel & 0x1F) << 3; // Blue
                            dest[destOffset + 3] = 255; // Alpha
                        }
                    }
                }
            }
        }

    private:
        std::vector<DrmDevice> _drmDevices;
        std::vector<DisplayInfo> _displays;
        uint32_t _totalWidth;
        uint32_t _totalHeight;
        int32_t _minX;
        int32_t _minY;
    };

} // namespace Plugin

/* static */ Exchange::ICapture* Exchange::ICapture::Instance()
{
    return (Core::ServiceType<Plugin::DrmCapture>::Create<Exchange::ICapture>());
}

} // namespace Thunder