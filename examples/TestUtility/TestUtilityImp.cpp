/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "CommandCore/TestCommandController.h"
#include <interfaces/ITestUtility.h>

namespace WPEFramework {
namespace TestCore {

    class TestUtilityImp : public Exchange::ITestUtility {
    public:
        TestUtilityImp(const TestUtilityImp&) = delete;
        TestUtilityImp& operator=(const TestUtilityImp&) = delete;

    public:
        TestUtilityImp()
        : _shutdownTimeout(0) {
        }

        virtual ~TestUtilityImp() {
            ::SleepMs(_shutdownTimeout);
            TestCore::TestCommandController::Instance().Release();
        };

        uint32_t ShutdownTimeout(const uint32_t timeout) override
        {
            _shutdownTimeout = timeout;
            return Core::ERROR_NONE;
        }

        //  ITestUtility methods
        // -------------------------------------------------------------------------------------------------------
        Exchange::ITestUtility::ICommand::IIterator* Commands() const override
        {
            return TestCore::TestCommandController::Instance().Commands();
        }

        Exchange::ITestUtility::ICommand* Command(const string& name) const override
        {
            return TestCore::TestCommandController::Instance().Command(name);
        }

        BEGIN_INTERFACE_MAP(TestUtilityImp)
        INTERFACE_ENTRY(Exchange::ITestUtility)
        END_INTERFACE_MAP

    private:
        uint32_t _shutdownTimeout;
    };

    SERVICE_REGISTRATION(TestUtilityImp, 1, 0);
} // namespace TestCore
} // namespace WPEFramewor
