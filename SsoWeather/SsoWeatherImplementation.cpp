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
        : _adminLock()
        , _temperature(0)
        , _isRaining(false)
    {
    }

    SsoWeatherImplementation::~SsoWeatherImplementation() = default;

    void SsoWeatherImplementation::Register(Exchange::ISsoWeather::INotification* notification)
    {
        ASSERT(notification);
        _adminLock.Lock();
        notification->AddRef();
        _notifications.push_back(notification);
        _adminLock.Unlock();
    }

    void SsoWeatherImplementation::Unregister(const Exchange::ISsoWeather::INotification* notification)
    {
        ASSERT(notification);
        _adminLock.Lock();
        auto item = std::find(_notifications.begin(), _notifications.end(), notification);
        ASSERT(item != _notifications.end());
        _notifications.erase(item);
        (*item)->Release();
        _adminLock.Unlock();
    }

    uint32_t SsoWeatherImplementation::Temperature(const uint8_t temperature) 
    {
        _adminLock.Lock();
        _temperature = temperature;
        _adminLock.Unlock();

        NotifyTemperatureChange();
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
        _adminLock.Lock();
        _isRaining = raining;
        _adminLock.Unlock();

        NotifyIsRainingChange();
        TRACE(Trace::Information, (_T("Set Temperature: %i"), _isRaining));
        return Core::ERROR_NONE;
    }

    uint32_t SsoWeatherImplementation::IsRaining(bool& raining) const
    {
        raining = _isRaining;
        
        TRACE(Trace::Information, (_T("Get Temperature: %i"), raining));
        return Core::ERROR_NONE;
    }

    void SsoWeatherImplementation::NotifyIsRainingChange()
    {
        _adminLock.Lock();
        for (auto* notification : _notifications) {
            notification->IsRaining(_isRaining);
        }
        _adminLock.Unlock();
    }

    void SsoWeatherImplementation::NotifyTemperatureChange()
    {
        _adminLock.Lock();
        for (auto* notification : _notifications) {
            notification->Temperature(_temperature);
        }
        _adminLock.Unlock();
    }

}  // namespace Plugin
}  // namespace WPEFramework
