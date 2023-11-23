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
 
#include "TestAutomationToolsImplementation.h"


namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(TestAutomationToolsImplementation, 1, 0)

    TestAutomationToolsImplementation::TestAutomationToolsImplementation()
        : _adminLock{}
        // , _notifications{}
      
    {

    }

    TestAutomationToolsImplementation::~TestAutomationToolsImplementation() = default;

    Core::hresult TestAutomationToolsImplementation::IncreaseMemory(uint32_t memorySize)
    {

        uint32_t result = Core::ERROR_NONE;

        _adminLock.Lock();
        if (memorySize > 0 && _memoryAllocationData == nullptr)
        {
            const size_t totalBytes = memorySize * (1024 * 1024);

            _memoryAllocationData = new char[totalBytes];
        

            for (size_t i = 0; i < totalBytes; i++) {
                _memoryAllocationData[i] = 'a';
            }
            
            if (_memoryAllocationData == nullptr) {
                _adminLock.Unlock();
                TRACE(Trace::Information, (_T("Memory Allocation Failed!!!")));
                result = Core::ERROR_GENERAL;

            } else {   
                _adminLock.Unlock();
            }
        }
        else {
            TRACE(Trace::Information, (_T("Memory Already Allocated! Clear First To Prevent Leaking!!!")));
            result = Core::ERROR_GENERAL;
        }
        return result;
    }

    Core::hresult TestAutomationToolsImplementation::FreeMemory()
    {
        if (_memoryAllocationData != nullptr) {
            TRACE(Trace::Information, (_T("Memory Allocation Cleared!!!")));
            delete[] _memoryAllocationData;
            _memoryAllocationData = nullptr;
        } 
        else {
            TRACE(Trace::Information, (_T("No Memory Allocated Yet!!!")));
        }

        return Core::ERROR_NONE;
    }

    Core::hresult TestAutomationToolsImplementation::BigStringTest(const string& testString)
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



   

}  // namespace Plugin
}  // namespace WPEFramework
