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


// Same as Calculator/CalculatorImplementation.cpp but without any OpenTelemetry usage,
// used as a baseline to measure the memory/time added by OpenTelemetry.

#include "Module.h"

#include <interfaces/IConfiguration.h>
#include <interfaces/IMath.h>

#include <algorithm>
#include <vector>

namespace Thunder {
namespace Plugin {

    class Calculator2Implementation : public Exchange::IMath
        , public Exchange::IMathExtended
        , public Exchange::IConfiguration {
    public:

        PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)

        Calculator2Implementation()
        : _adminLock()
        ,_notifications()
        , _resultjob(*this)
        {
        }

        POP_WARNING()

        Calculator2Implementation(const Calculator2Implementation&) = delete;
        Calculator2Implementation& operator=(const Calculator2Implementation&) = delete;

        ~Calculator2Implementation() override
        {
            _resultjob.Revoke();
        }

        uint32_t Configure(PluginHost::IShell* shell VARIABLE_IS_NOT_USED) override
        {
            fprintf(stderr, "Calculator2 running without OpenTelemetry\n");
            fflush(stderr);

            return Core::ERROR_NONE;
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
            _resultjob.Revoke();
            static_cast<MultiplayResultJob&>(_resultjob).SetContext(result);
            _resultjob.Submit();

            return Core::ERROR_NONE;
        }


        class MultiplayResultJob
        {
        public:
            MultiplayResultJob(Calculator2Implementation& parent)
            : _parent(parent)
            , _result(0)
            {
            }

            void SetContext(const uint32_t result)
            {
                _result = result;
            }

            void Dispatch() const
            {
                Core::SafeSyncType<Core::CriticalSection> lock(_parent._adminLock);
                for (auto* notification : _parent._notifications) {
                    notification->MultiplyResult(_result);
                }
            }

        private:
            Calculator2Implementation& _parent;
            uint32_t _result;
        };

        BEGIN_INTERFACE_MAP(Calculator2Implementation)
            INTERFACE_ENTRY(Exchange::IMath)
            INTERFACE_ENTRY(Exchange::IMathExtended)
            INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

    private:
        Core::CriticalSection _adminLock;
        std::vector<Exchange::IMathExtended::INotification*> _notifications;
        Core::WorkerPool::JobType<MultiplayResultJob> _resultjob;
    };

    SERVICE_REGISTRATION(Calculator2Implementation, 1, 0)

} // namespace Plugin
} // namespace Thunder
