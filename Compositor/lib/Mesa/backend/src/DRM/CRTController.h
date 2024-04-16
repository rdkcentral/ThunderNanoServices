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

#include "../Module.h"

namespace Thunder {
namespace Compositor {
namespace Backend {

    class CrtControllerAdministration {
    public:
        struct IObject {
        public:
            virtual ~IObject() = default;
            virtual DRM::Identifier Identifier() const = 0;
        };

        template<typename TYPE>
        struct IProperty {
        public:
            virtual ~IProperty() = default;
            virtual const IObject* Property(TYPE) const = 0;
        };

        struct IConnector : public IObject {
            enum class PropertyType {
                BroadcastRgb = 0x00,
                Colorspace,
                ContentProtection,
                Dpms,
                Edid,
                HdcpContentType,
                HdrOutputMetadata,
                Tile,
                Audio,
                LinkStatus,
                MaxBpc,
                NonDesktop,
                SubConnector,
                VrrCapable,
            };

            virtual ~IConnector() = default;

            virtual const IObject* Property(PropertyType) const = 0;
        };

        struct IPlane {
            enum class PropertyType {
                ColorEncoding = 0x00,
                ColorRange,
                InFormats,
                ScalingFilter,
                Alpha,
                PixelBlendMode,
                Rotation,
                Type,
                ZPosition
            };

            virtual ~IPlane() = default;

            virtual DRM::Identifier Identifier() const = 0;

            virtual const IProperty* Property(PropertyType) const = 0;
        };

        struct IController {
            enum class PropertyType {
                Ctm = 0x00,
                DegammaLut,
                DegammaLutSize,
                GammaLut,
                GammaLutSize,
                ScalingFilter,
                VrrEnabled
            };

            virtual ~IController() = default;

            virtual DRM::Identifier Identifier() const = 0;

            virtual const IPlane* Plane(const DRM::PlaneType type) const = 0;

            virtual const IProperty* Property(const PropertyType) const = 0;

            virtual uint32_t Value(const DRM::Identifier object, const DRM::Identifier property, uint64_t& value) const = 0;
        };

        ~CrtControllerAdministration() = default;
        CrtControllerAdministration() = delete;

        CrtControllerAdministration(const int cardFd)
            : _cardFd(cardFd)
        {
        }

        IController* Claim(const DRM::Identifier connector);
        void Relinquish(const DRM::Identifier connector);

    private:
        const int _cardFd;
    };
}
};
