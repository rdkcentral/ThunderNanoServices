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

#include <cstdint>
#include <string>
#include <cstring>
#include <unordered_map>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "DRM.h"

namespace Thunder {
namespace Compositor {
    namespace DRM {

        enum class property : uint16_t {
            // Connector properties
            BroadcastRgb = 0x0000,
            Colorspace,
            ContentProtection,
            ContentType,
            CrtcId, /* atomic */
            Dpms,
            Edid,
            HdcpContentType,
            HdrOutputMetadata,
            Tile,
            LinkStatus,
            MaxBpc,
            NonDesktop,
            OutputFormat,
            PanelOrientation,
            Path,
            SubConnector,
            VrrCapable,
            // Plane Properties
            Alpha,
            InFormats,
            InFenceFd,
            ZPosition,
            Rotation,
            FbId,
            FbDamageClips,
            Type,
            SrcX,
            SrcY,
            SrcH,
            SrcW,
            CrtcX,
            CrtcY,
            CrtcH,
            CrtcW,
            // CRTC Properties
            Active,
            Ctm,
            DegammaLut,
            DegammaLutSize,
            GammaLut,
            GammaLutSize,
            ModeId,
            ScalingFilter,
            VrrEnabled,
            InvalidProperty = static_cast<uint16_t>(~0)
        };

        enum class property_type : uint32_t {
            Range = DRM_MODE_PROP_RANGE,
            SignedRange = DRM_MODE_PROP_SIGNED_RANGE,
            Enumerate = DRM_MODE_PROP_ENUM, /* Enumerated type with text strings */
            Bitmask = DRM_MODE_PROP_BITMASK, /* Bitmask of enumerated types */
            Object = DRM_MODE_PROP_OBJECT, /* DRM object id */
            Blob = DRM_MODE_PROP_BLOB,
            Invalid = 0
        };

        enum class object_type : uint32_t {
            Crtc = DRM_MODE_OBJECT_CRTC,
            Connector = DRM_MODE_OBJECT_CONNECTOR,
            Encoder = DRM_MODE_OBJECT_ENCODER, /* Deprecated */
            Mode = DRM_MODE_OBJECT_MODE,
            Property = DRM_MODE_OBJECT_PROPERTY,
            Framebuffer = DRM_MODE_OBJECT_FB,
            Blob = DRM_MODE_OBJECT_BLOB,
            Plane = DRM_MODE_OBJECT_PLANE,
            Any = DRM_MODE_OBJECT_ANY
        };

        enum class plane_type : uint8_t {
            Cursor = DRM_PLANE_TYPE_CURSOR,
            Primary = DRM_PLANE_TYPE_PRIMARY,
            Overlay = DRM_PLANE_TYPE_OVERLAY,
        };

        extern const char* PropertyToString(const property value);
        extern const char* PropertyTypeToString(const property_type value);
        extern const char* ObjectTypeToString(const object_type value);
        extern const char* PlaneTypeToString(const plane_type value);

        class Property {
        public:
            Property()
                : _property(property::InvalidProperty)
                , _id(InvalidIdentifier)
                , _flags(0) {
            }
            Property(const property which, const Identifier id, const uint32_t flags)
                : _property(which)
                , _id(id)
                , _flags(flags) {
            }
            Property(Property&& move)
                : _property(std::move(move._property))
                , _id(std::move(move._id))
                , _flags(std::move(move._flags)) {
            }
            Property(const Property& copy)
                : _property(copy._property)
                , _id(copy._id)
                , _flags(copy._flags) {
            }
            ~Property() = default;

            Property& operator= (Property&& rhs) {
                _property = std::move(rhs._property);
                _id = std::move(rhs._id);
                _flags = std::move(rhs._flags);
                return (*this);
            }
            Property& operator= (const Property& rhs) {
                _property = rhs._property;
                _id = rhs._id;
                _flags = rhs._flags;
                return (*this);
            }

        public:
            bool IsValid() const {
                return (_property != property::InvalidProperty);
            }
            Identifier Id() const {
                return _id;
            }
            bool IsReadOnly() const {
                return ((_flags & DRM_MODE_PROP_IMMUTABLE) != 0);
            }
            bool IsAtomic() const {
                return ((_flags & DRM_MODE_PROP_ATOMIC) != 0);
            }
            property_type Type() const {
                return static_cast<property_type>(_flags & (DRM_MODE_PROP_LEGACY_TYPE | DRM_MODE_PROP_EXTENDED_TYPE));
            }
            operator enum property() const {
                return (_property);
            }
            const char* PropertyToString() const;

        private:
            property _property;
            Identifier _id;
            uint32_t _flags;
        };

        class Properties {
        private:
            using Element = std::pair<Identifier, uint32_t>;
            using Elements = std::unordered_map<property, Element >;

        public:
            Properties(Properties&&) = delete;
            Properties(const Properties&) = delete;
            Properties& operator=(Properties&&) = delete;
            Properties& operator=(const Properties&) = delete;

            Properties() : _objectId(InvalidIdentifier) {}
            Properties(const int fd, object_type type, const Identifier objectId) {
                Load(fd, type, objectId);
            }
            ~Properties() = default;

        public:
            Identifier Id(const property which) const
            {
                const auto& entry = _properties.find(which);
                return (entry != _properties.end() ? entry->second.first : InvalidIdentifier);
            }
            Property Get(const property which) const
            {
                const auto& entry = _properties.find(which);
                return (entry != _properties.end()
                    ? Property(which, entry->second.first, entry->second.second) 
                    : Property(which, InvalidIdentifier, 0));
            }
            Identifier Id () const {
                return (_objectId);
            }

            void Load(const int fd, object_type type, const Identifier objectId);

        private:
            Identifier _objectId;
            Elements _properties;
        };

        class CRTCProperties : public Properties {
        public:
            CRTCProperties(CRTCProperties&&) = delete;
            CRTCProperties(const CRTCProperties&) = delete;
            CRTCProperties& operator=(CRTCProperties&&) = delete;
            CRTCProperties& operator=(const CRTCProperties&) = delete;

            CRTCProperties() = default;
            CRTCProperties(const int fd, const drmModeCrtc* crtc) {
                Load(fd, crtc);
            }
            ~CRTCProperties() = default;

        public:
            void Load(const int fd, const drmModeCrtc* crtc) {
                memset(&_drmModeStatus, 0, sizeof(drmModeModeInfo));

                _drmModeStatus = crtc->mode;

                Properties::Load(fd, Compositor::DRM::object_type::Crtc, crtc->crtc_id);
            }
            operator const drmModeModeInfo* () const {
                return (&_drmModeStatus);
            }

        private:
            drmModeModeInfo _drmModeStatus;
        };

    } // namespace DRM
} // namespace Compositor
} // namespace Thunder
