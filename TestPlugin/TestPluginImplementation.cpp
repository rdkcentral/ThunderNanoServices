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
    
    class TestPluginImplementation : public QualityAssurance::ITestTextOptions, 
                                     public QualityAssurance::ITestTextOptions::ITestLegacy, 
                                     public QualityAssurance::ITestTextOptions::ITestKeep, 
                                     public QualityAssurance::ITestTextOptions::ITestCustom {
    public:
        TestPluginImplementation(const TestPluginImplementation&) = delete;
        TestPluginImplementation& operator=(const TestPluginImplementation&) = delete;
        TestPluginImplementation(TestPluginImplementation&&) = delete;
        TestPluginImplementation& operator=(TestPluginImplementation&&) = delete;
        
        TestPluginImplementation()
            : QualityAssurance::ITestTextOptions()
            , QualityAssurance::ITestTextOptions::ITestLegacy()
            , QualityAssurance::ITestTextOptions::ITestKeep()
            , QualityAssurance::ITestTextOptions::ITestCustom()
        {
        }
        ~TestPluginImplementation() override = default;
        
    public:
        
        BEGIN_INTERFACE_MAP(TestPluginImplementation)
            INTERFACE_ENTRY(QualityAssurance::ITestTextOptions)
            INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::ITestLegacy)
            INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::ITestKeep)
            INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::ITestCustom)
        END_INTERFACE_MAP
        
        // ITestTextOptions methods
        Core::hresult TestStandart(const uint32_t firstTestParam, const uint32_t secondTestParam, const ITestTextOptions::TestDetails& thirdTestParam, const ITestTextOptions::EnumTextOptions fourthTestParam) override {
            return Core::ERROR_NONE;
        }
       
        // ITestLegacy methods
        Core::hresult TestLegacy(const uint32_t firstTestParam, const uint32_t secondTestParam, const ITestLegacy::TestDetails& thirdTestParam, const ITestLegacy::EnumTextOptions fourthTestParam) override {
            return Core::ERROR_NONE;
        }
        
        // ITestKeep methods
        Core::hresult TestKeeP(const uint32_t firstTestParaM, const uint32_t secondTestParaM, const ITestKeep::TestDetails& thirdTestParaM, const ITestKeep::EnumTextOptions fourthTestParaM) override {
            return Core::ERROR_NONE;
        }        
        // ITestCustom methods
        Core::hresult TestCustom(const uint32_t firstTestParam, const uint32_t secondTestParam, const ITestCustom::TestDetails& thirdTestParam, const ITestCustom::EnumTextOptions fourthTestParam) override {
            return Core::ERROR_NONE;
        }
    };
    
    SERVICE_REGISTRATION(TestPluginImplementation, 1, 0)
} // Plugin
} // Thunder