/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 Metrological
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

namespace {
    class DummyNotification : public Thunder::PluginHost::IPlugin::INotification {
    public:
        DummyNotification() = default;
        DummyNotification(const DummyNotification&) = delete;
        DummyNotification& operator=(const DummyNotification&) = delete;

        void Activated(const Thunder::Core::string&, Thunder::PluginHost::IShell*) override
        {
        }
        void Deactivated(const Thunder::Core::string&, Thunder::PluginHost::IShell*) override
        {
        }
        void Unavailable(const Thunder::Core::string&, Thunder::PluginHost::IShell*) override
        {
        }

        BEGIN_INTERFACE_MAP(DummyNotification)
            INTERFACE_ENTRY(Thunder::PluginHost::IPlugin::INotification)
        END_INTERFACE_MAP
    };
}

namespace Thunder {
namespace Plugin {
    
    class TestPriorityQueueImplementation : public Exchange::IMath, public PluginHost::IPlugin {
    public:
        TestPriorityQueueImplementation(const TestPriorityQueueImplementation&) = delete;
        TestPriorityQueueImplementation& operator=(const TestPriorityQueueImplementation&) = delete;
        TestPriorityQueueImplementation(TestPriorityQueueImplementation&&) = delete;
        TestPriorityQueueImplementation& operator=(TestPriorityQueueImplementation&&) = delete;
        
        TestPriorityQueueImplementation()
            : Exchange::IMath()
            , PluginHost::IPlugin()
            , _service(nullptr)
        {
        }
        ~TestPriorityQueueImplementation() override = default;
        
    public:
        
        BEGIN_INTERFACE_MAP(TestPriorityQueueImplementation)
            INTERFACE_ENTRY(Exchange::IMath)
            INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP
        
        const string Initialize(PluginHost::IShell* service) override
        {
            ASSERT(_service == nullptr);
            _service = service;

            if (_service != nullptr) {
                _service->AddRef();
            }
            return string();
        }
        void Deinitialize(PluginHost::IShell* service) override
        {
            ASSERT(service == _service);

            if (_service != nullptr) {
                _service->Release();
                _service = nullptr;
            }
        }
        string Information() const override
        {
            return _T("TestPriorityQueue test plugin to exercise priority queuing and re-entrancy.");
        }

        // IMath methods
        virtual uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum) const override
        {
            if (_service != nullptr) {
                DummyNotification* sink = Core::ServiceType<DummyNotification>::Create<DummyNotification>();

                SleepMs(50);
                _service->Register(sink);

                SleepMs(5);
                _service->Unregister(sink);
                sink->Release();
            } else {
                TRACE_L1("TestPriorityQueue: _service is null; plugin not initialized?");
            }

            sum = static_cast<uint16_t>(A + B);
            std::cout << "Add: " << A << " + " << B << " = " << sum << std::endl;

            return (Core::ERROR_NONE);
        }
        virtual uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum) const override
        {
            sum = static_cast<uint16_t>(A - B);
            std::cout << "Sub: " << A << " - " << B << " = " << sum << std::endl;

            return (Core::ERROR_NONE);
        }

    private:
        PluginHost::IShell* _service;
    };
    
    SERVICE_REGISTRATION(TestPriorityQueueImplementation, 1, 0)
} // Plugin
} // Thunder