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

#include "../Module.h"

namespace Thunder {
namespace TestCore {

    class TestResult : public Core::JSON::Container {
    public:
        TestResult()
            : Core::JSON::Container()
            , OverallStatus()
            , Name()
        {
            Add(_T("test"), &Name);
            Add(_T("status"), &OverallStatus);
        }

        TestResult(const TestResult& copy)
        {
            this->Name = copy.Name;
            this->OverallStatus = copy.OverallStatus;

            Add(_T("test"), &Name);
            Add(_T("status"), &OverallStatus);
        }

        TestResult& operator=(const TestResult& rhs)
        {
            this->Name = rhs.Name;
            this->OverallStatus = rhs.OverallStatus;

            return *this;
        }

        ~TestResult() = default;

    public:
        Core::JSON::String OverallStatus;
        Core::JSON::String Name;
    };
} // namespace TestCore
} // namespace Thunder
