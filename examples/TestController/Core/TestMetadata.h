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
 
#pragma once

#include "../Module.h"

namespace WPEFramework {
namespace TestCore {

    class TestDescription : public Core::JSON::Container {
    public:
        TestDescription(const TestDescription&) = delete;
        TestDescription& operator=(const TestDescription&) = delete;

        TestDescription()
            : Core::JSON::Container()
            , Description()
        {
            Add(_T("description"), &Description);
        }

        ~TestDescription() = default;

    public:
        Core::JSON::String Description;
    };

    class TestResult : public Core::JSON::Container {
    public:
        class TestStep : public Core::JSON::Container {
        public:
            TestStep()
                : Core::JSON::Container()
                , Description()
                , Status()
            {
                Add(_T("testStep"), &Description);
                Add(_T("status"), &Status);
            }

            TestStep(const TestStep& copy)
                : Core::JSON::Container()
                , Description()
                , Status()
            {
                this->Description = copy.Description;
                this->Status = copy.Status;

                Add(_T("testStep"), &Description);
                Add(_T("status"), &Status);
            }

            TestStep& operator=(const TestStep& rhs)
            {
                this->Description = rhs.Description;
                this->Status = rhs.Status;

                return *this;
            }

            ~TestStep() = default;

        public:
            Core::JSON::String Description;
            Core::JSON::String Status;
        };

    public:
        TestResult()
            : Core::JSON::Container()
            , Steps()
            , OverallStatus()
            , Name()
        {
            Add(_T("test"), &Name);
            Add(_T("status"), &OverallStatus);
            Add(_T("steps"), &Steps);
        }

        TestResult(const TestResult& copy)
        {
            this->Name = copy.Name;
            this->OverallStatus = copy.OverallStatus;
            this->Steps = copy.Steps;

            Add(_T("test"), &Name);
            Add(_T("status"), &OverallStatus);
            Add(_T("steps"), &Steps);
        }

        TestResult& operator=(const TestResult& rhs)
        {
            this->Name = rhs.Name;
            this->OverallStatus = rhs.OverallStatus;
            this->Steps = rhs.Steps;

            return *this;
        }

        ~TestResult() = default;

    public:
        Core::JSON::ArrayType<TestStep> Steps;
        Core::JSON::String OverallStatus;
        Core::JSON::String Name;
    };
} // namespace TestCore
} // namespace WPEFramework
