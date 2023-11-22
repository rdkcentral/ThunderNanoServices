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
#include <interfaces/ITestAutomationTools.h>

namespace WPEFramework {
namespace Plugin {

    class TestAutomationToolsPlatform;

    class TestAutomationToolsImplementation : public Exchange::ITestAutomationToolsInternal {
    public:
        TestAutomationToolsImplementation(const TestAutomationToolsImplementation&) = delete;
        TestAutomationToolsImplementation& operator=(const TestAutomationToolsImplementation&) = delete;

        TestAutomationToolsImplementation();
        ~TestAutomationToolsImplementation() override;

        BEGIN_INTERFACE_MAP(TestAutomationToolsImplementation)
            INTERFACE_ENTRY(Exchange::ITestAutomationToolsInternal)
        END_INTERFACE_MAP

        // ITestAutomationToolsInternal Methods
        Core::hresult IncreaseMemory(uint32_t memorySize) override;
        Core::hresult FreeMemory() override;
        Core::hresult BigStringTest(const string& testString) override;

       

    private:
        mutable Core::CriticalSection _adminLock;
        char* _memoryAllocationData;

        // std::vector<Exchange::ITestAutomationTools::INotification*> _notifications;

    };

}  // namespace Plugin
}  // namespace WPEFramework
