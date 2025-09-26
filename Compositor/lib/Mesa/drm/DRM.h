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
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>

namespace Thunder {

namespace Exchange {
    struct IGraphicsBuffer;
}

namespace Compositor {
    class PixelFormat;

    namespace DRM {
        class Properties;

        using Identifier = uint32_t;
        static constexpr uint32_t InvalidIdentifier = ~0;

        struct ConnectorScanResult {
            bool success = false;
            bool needsModeSet = false;
            bool dimensionsAdjusted = false;
            drmModeModeInfo selectedMode = {};
            std::string gpuNode;

            // DRM object data for initialization
            struct {
                uint32_t crtcId = 0;
                uint32_t planeId = 0;
            } ids;
        };

        extern void GetNodes(const uint32_t type, std::vector<std::string>& list);
        extern Identifier FindConnectorId(const int fd, const std::string& connectorName);
        extern bool HasCapability(const int cardFd, const uint64_t option);

        /*
         * Re-open the DRM node to avoid GEM handle ref'counting issues.
         * See: https://gitlab.freedesktop.org/mesa/drm/-/merge_requests/110
         *
         */
        extern int ReopenNode(const int fd, const bool openRenderNode);
        extern uint32_t CreateFrameBuffer(const int cardFd, Exchange::IGraphicsBuffer* buffer);
        extern void DestroyFrameBuffer(const int cardFd, const uint32_t frameBufferId);
        extern bool HasNode(const drmDevice* device, const char* deviceName);
        extern int OpenGPU(const std::string& gpuNode);
        extern std::string GetNode(const uint32_t type, drmDevice* device);
        extern std::string GetGPUNode(const std::string& connectorName);
        extern bool HasAlpha(const uint32_t drmFormat);
        extern const char* ModifierVendorString(const uint64_t modifier);
        extern const char* FormatToString(const uint32_t format);
        extern bool ModesEqual(const drmModeModeInfo* mode1, const drmModeModeInfo* mode2);
        extern bool SelectBestMode(const drmModeConnector* const connector, const uint32_t requestedWidth, const uint32_t requestedHeight, const uint32_t requestedRefreshRate, bool& dimensionsAdjusted, drmModeModeInfo& selectedMode);
        extern ConnectorScanResult ScanConnector(const int backendFd, Thunder::Compositor::DRM::Identifier targetConnectorId, const uint32_t requestedWidth, const uint32_t requestedHeigh, const uint32_t requestedRefreshRate);
        extern std::vector<PixelFormat> ExtractFormats(const Properties& properties);

        class FormatString {
        public:
            FormatString() = delete;

            FormatString(const uint32_t format)
                : _name(drmGetFormatName(format))
            {
            }

            ~FormatString()
            {
                if (_name != nullptr) {
                    free(_name);
                }
            }

            // Delete copy constructor and assignment
            FormatString(const FormatString&) = delete;
            FormatString& operator=(const FormatString&) = delete;

            // Move semantics
            FormatString(FormatString&& other) noexcept
                : _name(other._name)
            {
                other._name = nullptr;
            }

            FormatString& operator=(FormatString&& other) noexcept
            {
                if (this != &other) {
                    if (_name != nullptr) {
                        free(_name);
                    }
                    _name = other._name;
                    other._name = nullptr;
                }
                return *this;
            }

            // Other operators
            operator const char*() const { return _name; }
            const char* operator*() const { return _name; }
            bool operator!() const { return _name == nullptr; }
            explicit operator bool() const { return _name != nullptr; }

            // Stream insertion operator (friend function)
            friend std::ostream& operator<<(std::ostream& os, const FormatString& fmt)
            {
                if (fmt._name) {
                    os << fmt._name;
                } else {
                    os << "(null)";
                }
                return os;
            }

        private:
            char* _name;
        };

        class ModifierString {
        public:
            ModifierString() = delete;

            ModifierString(const uint64_t modifier)
                : _name(drmGetFormatModifierName(modifier)) // Assuming this function exists
            {
            }

            ~ModifierString()
            {
                if (_name != nullptr) {
                    free(_name);
                }
            }

            // Delete copy constructor and assignment
            ModifierString(const ModifierString&) = delete;
            ModifierString& operator=(const ModifierString&) = delete;

            // Move semantics
            ModifierString(ModifierString&& other) noexcept
                : _name(other._name)
            {
                other._name = nullptr;
            }

            ModifierString& operator=(ModifierString&& other) noexcept
            {
                if (this != &other) {
                    if (_name != nullptr) {
                        free(_name);
                    }
                    _name = other._name;
                    other._name = nullptr;
                }
                return *this;
            }

            // Operators
            operator const char*() const { return _name; } // Implicit conversion
            const char* operator*() const { return _name; } // Dereference like pointer
            bool operator!() const { return _name == nullptr; } // Check if null
            explicit operator bool() const { return _name != nullptr; } // Check if valid

            // Comparison operators
            bool operator==(const char* other) const
            {
                if (!_name || !other)
                    return _name == other;
                return strcmp(_name, other) == 0;
            }

            bool operator!=(const char* other) const
            {
                return !(*this == other);
            }

            // Stream insertion operator
            friend std::ostream& operator<<(std::ostream& os, const ModifierString& mod)
            {
                if (mod._name) {
                    os << mod._name;
                } else {
                    os << "(unknown modifier)";
                }
                return os;
            }

        private:
            char* _name;
        };

    } // namespace DRM
} // namespace Compositor
} // namespace Thunder
