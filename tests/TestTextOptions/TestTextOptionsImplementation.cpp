/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 Metrological
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
#include <mutex>

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
            , _status(QualityAssurance::ITestTextOptions::STATUS_IDLE)
            , _notification(nullptr)
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
        Core::hresult TestStandard(const uint32_t firstTestParam VARIABLE_IS_NOT_USED, const uint32_t secondTestParam VARIABLE_IS_NOT_USED, 
        const QualityAssurance::ITestTextOptions::TestDetails& thirdTestParam VARIABLE_IS_NOT_USED, const QualityAssurance::ITestTextOptions::EnumTextOptions fourthTestParam VARIABLE_IS_NOT_USED) override {
            TRACE(Trace::Information, (_T("firstTestParam: %d, secondTestParam: %d, thirdTestParam: %s %s, fourthTestParam: %s"), 
            firstTestParam, secondTestParam, thirdTestParam.testDetailsFirst.c_str(), thirdTestParam.testDetailsSecond.c_str(), Core::EnumerateType<QualityAssurance::ITestTextOptions::EnumTextOptions>(fourthTestParam).Data()));
            return Core::ERROR_NONE;
        }

        // Per-field @text — echo struct round-trip
        Core::hresult EchoMixedFields(
            const QualityAssurance::ITestTextOptions::MixedFieldNames& input,
            QualityAssurance::ITestTextOptions::MixedFieldNames& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        // Per-enumerator @text — set/get enum
        // Per-enumerator @text — status property
        Core::hresult Status(const QualityAssurance::ITestTextOptions::ConnectionStatus status) override
        {
            _status = status;
            return Core::ERROR_NONE;
        }

        Core::hresult Status(QualityAssurance::ITestTextOptions::ConnectionStatus& status) const override
        {
            status = _status;
            return Core::ERROR_NONE;
        }

        // Per-method @text override — echo
        Core::hresult InternalMethodName(const uint32_t value, uint32_t& result) const override
        {
            result = value;
            return Core::ERROR_NONE;
        }

        // @text + @alt — echo
        Core::hresult TextAndAltMethod(const uint32_t value, uint32_t& result) const override
        {
            result = value;
            return Core::ERROR_NONE;
        }

        // Event registration and trigger
        Core::hresult Register(QualityAssurance::ITestTextOptions::INotification* notification) override
        {
            ASSERT(notification != nullptr);
            std::lock_guard<std::mutex> lock(_mutex);
            if (_notification != nullptr) {
                return Core::ERROR_UNAVAILABLE;
            }
            _notification = notification;
            _notification->AddRef();
            return Core::ERROR_NONE;
        }

        Core::hresult Unregister(QualityAssurance::ITestTextOptions::INotification* notification) override
        {
            ASSERT(notification != nullptr);
            std::lock_guard<std::mutex> lock(_mutex);
            if (_notification != notification) {
                return Core::ERROR_UNAVAILABLE;
            }
            _notification->Release();
            _notification = nullptr;
            return Core::ERROR_NONE;
        }

        Core::hresult TriggerEvent(const uint32_t firstTestParam, const uint32_t secondTestParam,
            const QualityAssurance::ITestTextOptions::TestDetails& thirdTestParam,
            const QualityAssurance::ITestTextOptions::EnumTextOptions fourthTestParam) override
        {
            QualityAssurance::ITestTextOptions::INotification* sink = nullptr;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                sink = _notification;
                if (sink) {
                    sink->AddRef();
                }
            }
            if (sink) {
                sink->TestEvent(firstTestParam, secondTestParam, thirdTestParam, fourthTestParam);
                sink->Release();
            }
            return Core::ERROR_NONE;
        }

        // ITestLegacy methods
        Core::hresult TestLegacy(const uint32_t firstTestParam VARIABLE_IS_NOT_USED, const uint32_t secondTestParam VARIABLE_IS_NOT_USED,
        const QualityAssurance::ITestTextOptions::ITestLegacy::TestDetails& thirdTestParam VARIABLE_IS_NOT_USED, const QualityAssurance::ITestTextOptions::ITestLegacy::EnumTextOptions fourthTestParam VARIABLE_IS_NOT_USED) override {
            TRACE(Trace::Information, (_T("firstTestParam: %d, secondTestParam: %d, thirdTestParam: %s %s, fourthTestParam: %s"), 
            firstTestParam, secondTestParam, thirdTestParam.testDetailsFirst.c_str(), thirdTestParam.testDetailsSecond.c_str(), Core::EnumerateType<QualityAssurance::ITestTextOptions::EnumTextOptions>(fourthTestParam).Data()));
            return Core::ERROR_NONE;
        }
        
        // ITestKeep methods
        Core::hresult TestKeeP(const uint32_t firstTestParam VARIABLE_IS_NOT_USED, const uint32_t secondTestParam VARIABLE_IS_NOT_USED,
        const QualityAssurance::ITestTextOptions::ITestKeep::TestDetails& thirdTestParam VARIABLE_IS_NOT_USED, const QualityAssurance::ITestTextOptions::ITestKeep::EnumTextOptions fourthTestParam VARIABLE_IS_NOT_USED) override {
            TRACE(Trace::Information, (_T("firstTestParam: %d, secondTestParam: %d, thirdTestParam: %s %s, fourthTestParam: %s"), 
            firstTestParam, secondTestParam, thirdTestParam.testDetailsFirst.c_str(), thirdTestParam.testDetailsSecond.c_str(), Core::EnumerateType<QualityAssurance::ITestTextOptions::EnumTextOptions>(fourthTestParam).Data()));
            return Core::ERROR_NONE;
        }        
        // ITestCustom methods
        Core::hresult TestCustom(const uint32_t firstTestParam VARIABLE_IS_NOT_USED, const uint32_t secondTestParam VARIABLE_IS_NOT_USED,
        const QualityAssurance::ITestTextOptions::ITestCustom::TestDetails& thirdTestParam VARIABLE_IS_NOT_USED, const QualityAssurance::ITestTextOptions::ITestCustom::EnumTextOptions fourthTestParam VARIABLE_IS_NOT_USED) override {
            TRACE(Trace::Information, (_T("firstTestParam: %d, secondTestParam: %d, thirdTestParam: %s %s, fourthTestParam: %s"), 
            firstTestParam, secondTestParam, thirdTestParam.testDetailsFirst.c_str(), thirdTestParam.testDetailsSecond.c_str(), Core::EnumerateType<QualityAssurance::ITestTextOptions::EnumTextOptions>(fourthTestParam).Data()));
            return Core::ERROR_NONE;
        }

    private:
        QualityAssurance::ITestTextOptions::ConnectionStatus _status;
        QualityAssurance::ITestTextOptions::INotification* _notification;
        mutable std::mutex _mutex;
    };
    
    SERVICE_REGISTRATION(TestTextOptionsImplementation, 1, 0)
} // Plugin
} // Thunder
