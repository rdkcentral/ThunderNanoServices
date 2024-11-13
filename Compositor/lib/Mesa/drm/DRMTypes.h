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
#include <map>
#include <string>
#include <unordered_map>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace Thunder {
namespace Compositor {
    namespace DRM {

        using Identifier = uint32_t;
        using Value = uint64_t;

        constexpr Identifier InvalidIdentifier = static_cast<Identifier>(~0);
        constexpr int InvalidFileDescriptor = -1;

        enum class PropertyType : uint32_t {
            Range = DRM_MODE_PROP_RANGE,
            SignedRange = DRM_MODE_PROP_SIGNED_RANGE,
            Enumerate = DRM_MODE_PROP_ENUM, /* Enumerated type with text strings */
            Bitmask = DRM_MODE_PROP_BITMASK, /* Bitmask of enumerated types */
            Object = DRM_MODE_PROP_OBJECT, /* DRM object id */
            Blob = DRM_MODE_PROP_BLOB,
            Invalid = 0
        };

        enum class ObjectType : uint32_t {
            Crtc = DRM_MODE_OBJECT_CRTC,
            Connenctor = DRM_MODE_OBJECT_CONNECTOR,
            Encoder = DRM_MODE_OBJECT_ENCODER, /* Deprecated */
            Mode = DRM_MODE_OBJECT_MODE,
            Property = DRM_MODE_OBJECT_PROPERTY,
            Frambuffer = DRM_MODE_OBJECT_FB,
            Blob = DRM_MODE_OBJECT_BLOB,
            Plane = DRM_MODE_OBJECT_PLANE,
            Any = DRM_MODE_OBJECT_ANY
        };

        enum class PlaneType : uint8_t {
            Cursor = DRM_PLANE_TYPE_CURSOR,
            Primary = DRM_PLANE_TYPE_PRIMARY,
            Overlay = DRM_PLANE_TYPE_OVERLAY,
        };

        constexpr Identifier InvalidProperty = uint16_t(~0);

        enum class Property : uint16_t {
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
            // CRTC Prperties
            Active,
            Ctm,
            DegammaLut,
            DegammaLutSize,
            GammaLut,
            GammaLutSize,
            ModeId,
            ScalingFilter,
            VrrEnabled
        };

        static const std::unordered_map<std::string, Property> stringToProperty = {
            { "Broadcast_RGB", Property::BroadcastRgb },
            { "Colorspace", Property::Colorspace },
            { "Content Protection", Property::ContentProtection },
            { "content type", Property::ContentType },
            { "CRTC_ID", Property::CrtcId }, /* atomic */
            { "DPMS", Property::Dpms },
            { "EDID", Property::Edid },
            { "Output_format", Property::OutputFormat },
            { "HDR_OUTPUT_METADATA", Property::HdrOutputMetadata },
            { "TILE", Property::Tile },
            { "link-status", Property::LinkStatus },
            { "max bpc", Property::MaxBpc },
            { "non-desktop", Property::NonDesktop },
            { "PATH", Property::Path },
            { "panel_orientation", Property::PanelOrientation },
            { "subconnector", Property::SubConnector },
            { "vrr_capable", Property::VrrCapable },
            // Plane Properies
            { "type", Property::Type },
            { "alpha", Property::Alpha },
            { "SRC_X", Property::SrcX },
            { "SRC_Y", Property::SrcY },
            { "SRC_W", Property::SrcW },
            { "SRC_H", Property::SrcH },
            { "CRTC_X", Property::CrtcX },
            { "CRTC_Y", Property::CrtcY },
            { "CRTC_W", Property::CrtcW },
            { "CRTC_H", Property::CrtcH },
            { "FB_DAMAGE_CLIPS", Property::FbDamageClips },
            { "FB_ID", Property::FbId },
            { "IN_FENCE_FD", Property::InFenceFd },
            { "IN_FORMATS", Property::InFormats },
            { "rotation", Property::Rotation },
            { "zpos", Property::ZPosition },
            // CRTC Prpoerties
            { "ACTIVE", Property::Active },
            { "CTM", Property::Ctm },
            { "DEGAMMA_LUT", Property::DegammaLut },
            { "DEGAMMA_LUT_SIZE", Property::DegammaLutSize },
            { "GAMMA_LUT", Property::GammaLut },
            { "GAMMA_LUT_SIZE", Property::GammaLutSize },
            { "MODE_ID", Property::ModeId },
            { "VRR_ENABLED", Property::VrrEnabled },
        };

        struct IProperty {
            class Entry {
            public:
                Entry() = delete;
                // Entry(const Entry&) = delete;
                // Entry& operator=(const Entry&) = delete;

                Entry(const DRM::Identifier id, const uint32_t flags)
                    : _id(id)
                    , _flags(flags)
                {
                }

                bool IsReadOnly() const
                {
                    return ((_flags & DRM_MODE_PROP_IMMUTABLE) != 0);
                }

                bool IsAtomic() const
                {
                    return ((_flags & DRM_MODE_PROP_ATOMIC) != 0);
                }

                PropertyType Type() const
                {
                    return static_cast<PropertyType>(_flags & (DRM_MODE_PROP_LEGACY_TYPE | DRM_MODE_PROP_EXTENDED_TYPE));
                }

                Identifier Id() const
                {
                    return _id;
                }

                bool IsValid() const
                {
                    return (_id != InvalidIdentifier);
                }

            private:
                const Identifier _id;
                const uint32_t _flags;
            };

            virtual ~IProperty() = default;
            virtual IProperty::Entry Get(const DRM::Property property) const = 0;
            virtual DRM::Identifier Id(const DRM::Property property) const = 0;
        };

        struct EXTERNAL IDrmObject {
            virtual ~IDrmObject() = default;

            virtual const IProperty* Properties() const = 0;
            virtual const Identifier Id() const = 0;
        };
    } // namespace DRM
} // namespace Compositor
} // namespace Thunder
