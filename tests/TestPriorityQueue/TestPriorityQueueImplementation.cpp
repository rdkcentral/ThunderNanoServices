/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 [PLEASE ADD COPYRIGHT NAME!]
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

namespace Thunder {
namespace Plugin {
    
    class TestPriorityQueueImplementation : public Exchange::IMath {
    public:
        TestPriorityQueueImplementation(const TestPriorityQueueImplementation&) = delete;
        TestPriorityQueueImplementation& operator=(const TestPriorityQueueImplementation&) = delete;
        TestPriorityQueueImplementation(TestPriorityQueueImplementation&&) = delete;
        TestPriorityQueueImplementation& operator=(TestPriorityQueueImplementation&&) = delete;
        
        TestPriorityQueueImplementation()
            : Exchange::IMath()
            , _test(0)
        {
        }
        ~TestPriorityQueueImplementation() override = default;
        
    public:
        
        BEGIN_INTERFACE_MAP(TestPriorityQueueImplementation)
            INTERFACE_ENTRY(Exchange::IMath)
        END_INTERFACE_MAP
        
        // Implement methods from the interface
        // IMath methods
        virtual uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum) const override
        {
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

        // Note: test is just an example...
        uint32_t _test;
    };
    
    SERVICE_REGISTRATION(TestPriorityQueueImplementation, 1, 0)
} // Plugin
} // Thunder