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
#include <memory>
#include "Module.h"
#include <qa_interfaces/ITestAutomation.h>

namespace Thunder {
namespace Plugin {

    class TestAutomationMemoryImplementation : public QualityAssurance::IMemory {
    public:
        TestAutomationMemoryImplementation(const TestAutomationMemoryImplementation&) = delete;
        TestAutomationMemoryImplementation& operator=(const TestAutomationMemoryImplementation&) = delete;
        TestAutomationMemoryImplementation(TestAutomationMemoryImplementation&&) = delete;
        TestAutomationMemoryImplementation& operator=(TestAutomationMemoryImplementation&&) = delete;

        TestAutomationMemoryImplementation()
        : _adminLock{}
        , _memoryAllocationData{nullptr}
        {
        }
        ~TestAutomationMemoryImplementation() override = default;

        BEGIN_INTERFACE_MAP(TestAutomationMemoryImplementation)
            INTERFACE_ENTRY(QualityAssurance::IMemory)
        END_INTERFACE_MAP

        // IMemory Methods
        Core::hresult AllocateMemory(const uint32_t memorySize) override
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
                    TRACE(Trace::Information, (_T("Memory Allocated Successfully!!!")));
                    _adminLock.Unlock();
                }
            }
            else {
                TRACE(Trace::Information, (_T("Memory Already Allocated! Clear First To Prevent Leaking!!!")));
                result = Core::ERROR_GENERAL;
                _adminLock.Unlock();
            }
            return result;
        }
        
        Core::hresult FreeAllocatedMemory() override
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
        
    private:
        mutable Core::CriticalSection _adminLock;
        char* _memoryAllocationData;

    };

    SERVICE_REGISTRATION(TestAutomationMemoryImplementation, 1, 0)

}  // namespace Plugin
}  // namespace Thunder
