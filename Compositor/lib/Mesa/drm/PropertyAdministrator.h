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

#include "DRM.h"
#include "DRMTypes.h"

namespace Thunder {
namespace Compositor {
    namespace DRM {
        template <Compositor::DRM::ObjectType OBJECT_TYPE>
        class PropertyAdministratorType : public IProperty {
        public:
            using RegisterType = std::map<Property, IProperty::Entry>;

            PropertyAdministratorType()
                : _register()
            {
            }

            virtual ~PropertyAdministratorType() = default;

            Identifier Id(const Property property) const override
            {
                const auto& entry = _register.find(property);
                return (entry != _register.end()) ? entry->second.Id() : InvalidIdentifier;
            }

            IProperty::Entry Get(const Property property) const override
            {
                const auto& entry = _register.find(property);
                return (entry != _register.end()) ? entry->second : IProperty::Entry(InvalidIdentifier, 0);
            }
            
            uint32_t Scan(const int descriptor, const Identifier objectId)
            {
                _register.clear();

                drmModeObjectProperties* properties = drmModeObjectGetProperties(descriptor, objectId, static_cast<uint32_t>(OBJECT_TYPE));

                if (properties != nullptr) {
                    for (uint32_t i = 0; i < properties->count_props; ++i) {
                        drmModePropertyPtr drmProperty = drmModeGetProperty(descriptor, properties->props[i]);

                        const auto& property = stringToProperty.find(drmProperty->name);

                        if (property != stringToProperty.end()) {
                            
                            _register.emplace(
                                std::piecewise_construct,
                                std::forward_as_tuple(property->second),
                                std::forward_as_tuple(drmProperty->prop_id, drmProperty->flags));
                        }

                        drmModeFreeProperty(drmProperty);
                    }

                    drmModeFreeObjectProperties(properties);
                }

                return Core::ERROR_NONE;
            }

        private:
            RegisterType _register;
        };
    } // namespace DRM
} // namespace Compositor
} // namespace Thunder
