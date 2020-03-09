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
 
#include "interfaces/ITestController.h"

#include "../Module.h"
#include "TestMetadata.h"

namespace WPEFramework {

class TestBase : public Exchange::ITestController::ITest {
public:
    TestBase(const TestBase&) = delete;
    TestBase& operator=(const TestBase&) = delete;

    class DescriptionBuilder {
    public:
        DescriptionBuilder(const DescriptionBuilder&) = delete;
        DescriptionBuilder& operator=(const DescriptionBuilder&) = delete;

        explicit DescriptionBuilder(const string& description)
            : _jsonDescription()
        {
            _jsonDescription.Description = description;
        }

        virtual ~DescriptionBuilder() = default;

    private:
        friend class TestBase;
        string ToString() const
        {
            string outString;
            _jsonDescription.ToString(outString);

            return outString;
        }

        TestCore::TestDescription _jsonDescription;
    };

    explicit TestBase(const DescriptionBuilder& description)
        : Exchange::ITestController::ITest()
        , _description(description.ToString())
    {
    }

    virtual ~TestBase() = default;

    // ICommand methods
    string Description() const final
    {
        return _description;
    }

    BEGIN_INTERFACE_MAP(TestBase)
    INTERFACE_ENTRY(Exchange::ITestController::ITest)
    END_INTERFACE_MAP

private:
    string _description;
};
} // namespace WPEFramework
