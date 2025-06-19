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
 

#include "Module.h"
#include <qa_interfaces/ITestAutomation.h>

namespace Thunder {
namespace Plugin {

    class TestAutomationTextOptionsImplementation : public QualityAssurance::ITestTextOptions {
        
    public:
        TestAutomationTextOptionsImplementation(const TestAutomationTextOptionsImplementation&) = delete;
        TestAutomationTextOptionsImplementation& operator=(const TestAutomationTextOptionsImplementation&) = delete;
        TestAutomationTextOptionsImplementation(TestAutomationTextOptionsImplementation&&) = delete;
        TestAutomationTextOptionsImplementation& operator=(TestAutomationTextOptionsImplementation&&) = delete;

        TestAutomationTextOptionsImplementation() = default;
        ~TestAutomationTextOptionsImplementation() override = default;

        BEGIN_INTERFACE_MAP(TestAutomationTextOptionsImplementation)
            INTERFACE_ENTRY(QualityAssurance::ITestTextOptions)
        END_INTERFACE_MAP

        // ITestTextOptions Methods
        Core::hresult TestStandart() const override
        {
            return Core::ERROR_NONE;
        }

    };
    
    SERVICE_REGISTRATION(TestAutomationTextOptionsImplementation, 1, 0)
}  // namespace Plugin
}  // namespace Thunder
