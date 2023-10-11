/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "SsoWeatherImplementation.h"

namespace WPEFramework {
namespace Plugin {
    
    SERVICE_REGISTRATION(SsoWeatherImplementation, 1, 0);

    SsoWeatherImplementation::SsoWeatherImplementation()
        : _temperature(0)
        , _isRaining(false)
    {
    }

    SsoWeatherImplementation::~SsoWeatherImplementation() = default;

    uint32_t SsoWeatherImplementation::Temperature(const uint8_t temperature) 
    {
        _temperature = temperature;
        TRACE(Trace::Information, (_T("Set Temperature: %i"), _temperature));
        return Core::ERROR_NONE;
    }

    uint32_t SsoWeatherImplementation::Temperature(uint8_t& temperature) const
    {
        temperature = _temperature;
        TRACE(Trace::Information, (_T("Get Temperature: %i"), _temperature));
        return Core::ERROR_NONE;
    }

    uint32_t SsoWeatherImplementation::IsRaining(const bool raining)
    {
        _isRaining = raining;
        TRACE(Trace::Information, (_T("Set Temperature: %i"), _isRaining));
        return Core::ERROR_NONE;
    }

    uint32_t SsoWeatherImplementation::IsRaining(bool& raining) const
    {
        raining = _isRaining;
        TRACE(Trace::Information, (_T("Get Temperature: %i"), raining));
        return Core::ERROR_NONE;
    }

}  // namespace Plugin
}  // namespace WPEFramework
