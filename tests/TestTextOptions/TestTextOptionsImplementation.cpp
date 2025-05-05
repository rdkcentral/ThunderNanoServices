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
#include <qa_interfaces/ITestAutomation.h>

namespace Thunder {
namespace Plugin {
    
    class TestTextOptionsImplementation : public QualityAssurance::ITestTextOptions, 
                                     public QualityAssurance::ITestTextOptions::ITestLegacy, 
                                     public QualityAssurance::ITestTextOptions::ITestKeep, 
                                     public QualityAssurance::ITestTextOptions::ITestCustom {
    public:
        TestTextOptionsImplementation(const TestTextOptionsImplementation&) = delete;
        TestTextOptionsImplementation& operator=(const TestTextOptionsImplementation&) = delete;
        TestTextOptionsImplementation(TestTextOptionsImplementation&&) = delete;
        TestTextOptionsImplementation& operator=(TestTextOptionsImplementation&&) = delete;
        
        TestTextOptionsImplementation()
            : QualityAssurance::ITestTextOptions()
            , QualityAssurance::ITestTextOptions::ITestLegacy()
            , QualityAssurance::ITestTextOptions::ITestKeep()
            , QualityAssurance::ITestTextOptions::ITestCustom()
        {
        }
        ~TestTextOptionsImplementation() override = default;
        
    public:
        
        BEGIN_INTERFACE_MAP(TestTextOptionsImplementation)
            INTERFACE_ENTRY(QualityAssurance::ITestTextOptions)
            INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::ITestLegacy)
            INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::ITestKeep)
            INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::ITestCustom)
        END_INTERFACE_MAP
        
        // ITestTextOptions methods
        Core::hresult TestStandart(const uint32_t firstTestParam VARIABLE_IS_NOT_USED, const uint32_t secondTestParam VARIABLE_IS_NOT_USED, 
        const QualityAssurance::ITestTextOptions::TestDetails& thirdTestParam VARIABLE_IS_NOT_USED, const QualityAssurance::ITestTextOptions::EnumTextOptions fourthTestParam VARIABLE_IS_NOT_USED) override {
            return Core::ERROR_NONE;
        }
       
        // ITestLegacy methods
        Core::hresult TestLegacy(const uint32_t firstTestParam VARIABLE_IS_NOT_USED, const uint32_t secondTestParam VARIABLE_IS_NOT_USED,
        const QualityAssurance::ITestTextOptions::ITestLegacy::TestDetails& thirdTestParam VARIABLE_IS_NOT_USED, const QualityAssurance::ITestTextOptions::ITestLegacy::EnumTextOptions fourthTestParam VARIABLE_IS_NOT_USED) override {
            return Core::ERROR_NONE;
        }
        
        // ITestKeep methods
        Core::hresult TestKeeP(const uint32_t firstTestParaM VARIABLE_IS_NOT_USED, const uint32_t secondTestParaM VARIABLE_IS_NOT_USED,
        const QualityAssurance::ITestTextOptions::ITestKeep::TestDetails& thirdTestParaM VARIABLE_IS_NOT_USED, const QualityAssurance::ITestTextOptions::ITestKeep::EnumTextOptions fourthTestParaM VARIABLE_IS_NOT_USED) override {
            return Core::ERROR_NONE;
        }        
        // ITestCustom methods
        Core::hresult TestCustom(const uint32_t firstTestParam VARIABLE_IS_NOT_USED, const uint32_t secondTestParam VARIABLE_IS_NOT_USED,
        const QualityAssurance::ITestTextOptions::ITestCustom::TestDetails& thirdTestParam VARIABLE_IS_NOT_USED, const QualityAssurance::ITestTextOptions::ITestCustom::EnumTextOptions fourthTestParam VARIABLE_IS_NOT_USED) override {
            return Core::ERROR_NONE;
        }
    };
    
    SERVICE_REGISTRATION(TestTextOptionsImplementation, 1, 0)
} // Plugin
} // Thunder