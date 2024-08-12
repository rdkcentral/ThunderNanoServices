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

    class TestAutomationComRpcImplementation : public QualityAssurance::IComRpc::IComRpcInternal {
        
    public:
        TestAutomationComRpcImplementation(const TestAutomationComRpcImplementation&) = delete;
        TestAutomationComRpcImplementation& operator=(const TestAutomationComRpcImplementation&) = delete;
        TestAutomationComRpcImplementation(TestAutomationComRpcImplementation&&) = delete;
        TestAutomationComRpcImplementation& operator=(TestAutomationComRpcImplementation&&) = delete;

        TestAutomationComRpcImplementation() 
        {
        }
        ~TestAutomationComRpcImplementation() override = default;

        BEGIN_INTERFACE_MAP(TestAutomationComRpcImplementation)
            INTERFACE_ENTRY(QualityAssurance::IComRpc::IComRpcInternal)
        END_INTERFACE_MAP

        // IComRpcInternal Methods
        Core::hresult BigStringTest(const string& testString) override
        {
            TRACE(Trace::Information, (_T("OOP: Length Of The String Is " + std::to_string(testString.length()))));
            string text = testString.c_str();
            TRACE(Trace::Information, (_T("OOP: Content Of The String Is " + text)));
            
            uint32_t result = Core::ERROR_NONE;

            if (testString.length() >= 16){
                string firstFourDigit = testString.substr(0, 8);
                string lastFourDigit = testString.substr((testString.length() - 8), 8);

                if (firstFourDigit == "testaaaa" &&  lastFourDigit == "testzzzz") {
                    TRACE(Trace::Information, (_T("Verification Done For " + std::to_string(testString.length()/1024))));
                }
                
                else {
                    TRACE(Trace::Information, (_T("Verification FAILED For " + std::to_string(testString.length()/1024))));
                    result = Core::ERROR_GENERAL;
                }
            } 
            else {

                TRACE(Trace::Information, (_T("String size is lower than 16")));
                result = Core::ERROR_GENERAL;
            }
            
            return result;
        }

    };
    
    SERVICE_REGISTRATION(TestAutomationComRpcImplementation, 1, 0)
}  // namespace Plugin
}  // namespace Thunder
