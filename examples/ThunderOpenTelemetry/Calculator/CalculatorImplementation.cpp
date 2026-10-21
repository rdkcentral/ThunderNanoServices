/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

#include "Module.h"

#include <interfaces/IMath.h>

#include <algorithm>
#include <vector>

namespace Thunder {
namespace Plugin {

    class CalculatorImplementation : public Exchange::IMath
        , public Exchange::IMathExtended {
    public:
        CalculatorImplementation() = default;
        CalculatorImplementation(const CalculatorImplementation&) = delete;
        CalculatorImplementation& operator=(const CalculatorImplementation&) = delete;

        ~CalculatorImplementation() override
        {
            for (auto* notification : _notifications) {
                notification->Release();
            }
        }

        uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum) const override
        {
            sum = static_cast<uint16_t>(A + B);
            return Core::ERROR_NONE;
        }

        uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum) const override
        {
            sum = static_cast<uint16_t>(A - B);
            return Core::ERROR_NONE;
        }

        Core::hresult Register(Exchange::IMathExtended::INotification* const notification) override
        {
            Core::hresult result = Core::ERROR_NONE;
            if (notification == nullptr) {
                result = Core::ERROR_BAD_REQUEST;
            } else {
                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
                if (std::find(_notifications.begin(), _notifications.end(), notification) != _notifications.end()) {
                    result = Core::ERROR_ALREADY_CONNECTED;
                } else {
                    notification->AddRef();
                    _notifications.push_back(notification);
                }
            }

            return result;
        }

        Core::hresult Unregister(const Exchange::IMathExtended::INotification* const notification) override
        {
            Exchange::IMathExtended::INotification* registered = nullptr;
            Core::hresult result = Core::ERROR_UNKNOWN_KEY;
            {
                Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
                auto index = std::find(_notifications.begin(), _notifications.end(), notification);
                if (index != _notifications.end()) {
                    registered = *index;
                    _notifications.erase(index);
                    result = Core::ERROR_NONE;
                }
            }

            if (registered != nullptr) {
                registered->Release();
            }

            return result;
        }

        Core::hresult Multiply(const uint16_t A, const uint16_t B) override
        {
            const uint32_t result = static_cast<uint32_t>(A) * static_cast<uint32_t>(B);
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            for (auto* notification : _notifications) {
                notification->MultiplyResult(result);
            }

            return Core::ERROR_NONE;
        }

        BEGIN_INTERFACE_MAP(CalculatorImplementation)
            INTERFACE_ENTRY(Exchange::IMath)
            INTERFACE_ENTRY(Exchange::IMathExtended)
        END_INTERFACE_MAP

    private:
        Core::CriticalSection _adminLock;
        std::vector<Exchange::IMathExtended::INotification*> _notifications;
    };

    SERVICE_REGISTRATION(CalculatorImplementation, 1, 0)

} // namespace Plugin
} // namespace Thunder