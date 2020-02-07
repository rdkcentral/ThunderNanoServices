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

#include <interfaces/ITestController.h>

#include "Core/TestAdministrator.h"

namespace WPEFramework {
namespace TestCore {

    class TestControllerImp : public Exchange::ITestController {
    public:
        TestControllerImp() = default;
        TestControllerImp(const TestControllerImp&) = delete;
        TestControllerImp& operator=(const TestControllerImp&) = delete;

        virtual ~TestControllerImp() {}

        //  TestControllerImp methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Setup(){
            //This is overall TestController setup
            //ToDo: Do 'Setup' for ITestController if it is needed
        };
        virtual void TearDown(){
            //This is overall TestController tear down
            //ToDo: Do 'Tear Down' for ITestController if it is needed
        };

        virtual ICategory::IIterator* Categories() const override
        {
            return TestCore::TestAdministrator::Instance().Categories();
        }

        virtual ICategory* Category(const string& category) const override
        {
            return TestCore::TestAdministrator::Instance().Category(category);
        }

        BEGIN_INTERFACE_MAP(TestControllerImp)
        INTERFACE_ENTRY(Exchange::ITestController)
        END_INTERFACE_MAP
    private:
    };

    SERVICE_REGISTRATION(TestControllerImp, 1, 0);
} // namespace TestCore
} // namespace WPEFramewor
