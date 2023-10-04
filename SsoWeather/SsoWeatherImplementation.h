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
 
#pragma once

#include <memory>

#include "Module.h"
#include <interfaces/ISsoWeather.h>

namespace WPEFramework {
namespace Plugin {

    class SsoWeatherPlatform;

    class SsoWeatherImplementation : public Exchange:ISsoWeather {
    public:
        SsoWeatherImplementation(const SsoWeatherImplementation&) = delete;
        SsoWeatherImplementation& operator=(const SsoWeatherImplementation&) = delete;

        SsoWeatherImplementation();
        ~SsoWeatherImplementation() override;

        BEGIN_INTERFACE_MAP(SsoWeatherImplementation)
            INTERFACE_ENTRY(Exchange::ISsoWeather)
        END_INTERFACE_MAP

        uint32_t Temperature(const uint8_t temperature) override;
        uint32_t Temperature(uint8_t& temperature /* @out */) const override;
        uint32_t IsRaining(const bool raining) override;
        uint32_t IsRaining(bool& raining /* @out */) const override;
    
    private:
        Core::CriticalSection _adminLock;
        std::unique_ptr<SsoWeatherPlatform> _platform;
    };

}  // namespace Plugin
}  // namespace WPEFramework