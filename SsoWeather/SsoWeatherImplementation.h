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
 
#pragma oncea

#include <memory>

#include "Module.h"
#include <interfaces/ISsoWeather.h>

namespace WPEFramework {
namespace Plugin {

    class SsoWeatherPlatform;


    class SsoWeatherImplementation : public Exchange::ISsoWeather {
    public:
        SsoWeatherImplementation();
        ~SsoWeatherImplementation() override;

        SsoWeatherImplementation(const SsoWeatherImplementation&);
        SsoWeatherImplementation& operator=(const SsoWeatherImplementation&);

        BEGIN_INTERFACE_MAP(SsoWeatherImplementation)
            INTERFACE_ENTRY(Exchange::ISsoWeather)
        END_INTERFACE_MAP

        uint32_t Temperature(const uint8_t temperature) override;   // Set
        uint32_t Temperature(uint8_t& temperature) const override;  // Get

        uint32_t IsRaining(const bool raining) override;
        uint32_t IsRaining(bool& raining) const override;
    
    private:
    
        uint32_t _temperature;
        bool _isRaining;
    };

}  // namespace Plugin
}  // namespace WPEFramework