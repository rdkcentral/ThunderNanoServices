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
 
#include "../Module.h"
#include <qa_interfaces/json/JsonData_TestUtility.h>
#include "qa_interfaces/ITestUtility.h"

namespace Thunder {

class TestCommandBase : public QualityAssurance::ITestUtility::ICommand {
public:
    TestCommandBase() = delete;
    TestCommandBase(const TestCommandBase&) = delete;
    TestCommandBase& operator=(const TestCommandBase&) = delete;

public:
    class SignatureBuilder {
    public:
        SignatureBuilder() = delete;
        SignatureBuilder(const SignatureBuilder&) = delete;
        SignatureBuilder& operator=(const SignatureBuilder&) = delete;

    public:
        explicit SignatureBuilder(string const& name, JsonData::TestUtility::TypeType type, string const& comment)
            : _jsonSignature()
        {
            JsonData::TestUtility::ParameterInfo inputParam;
            inputParam.Name = name;
            inputParam.Type = type;
            inputParam.Comment = comment;

            _jsonSignature.Output = inputParam;
        }

        explicit SignatureBuilder(const JsonData::TestUtility::ParameterInfo& inputParam)
            : _jsonSignature()
        {
            _jsonSignature.Output = inputParam;
        }

        SignatureBuilder& InputParameter(const string& name, JsonData::TestUtility::TypeType type, const string& comment)
        {
            JsonData::TestUtility::ParameterInfo inputParam;
            inputParam.Name = name;
            inputParam.Type = type;
            inputParam.Comment = comment;
            _jsonSignature.Input.Add(inputParam);

            return (*this);
        }

        ~SignatureBuilder() = default;

    private:
        friend class TestCommandBase;
        string ToString() const
        {
            string outString;
            _jsonSignature.ToString(outString);

            return outString;
        }

        JsonData::TestUtility::ParametersData _jsonSignature;
    };

    class DescriptionBuilder {
    public:
        DescriptionBuilder() = delete;
        DescriptionBuilder(const DescriptionBuilder&) = delete;
        DescriptionBuilder& operator=(const DescriptionBuilder&) = delete;

    public:
        explicit DescriptionBuilder(const string& description)
            : _jsonDescription()
        {
            _jsonDescription.Description = description;
        }

        ~DescriptionBuilder() = default;

    private:
        friend class TestCommandBase;
        string ToString() const
        {
            string outString;
            _jsonDescription.ToString(outString);

            return outString;
        }

        JsonData::TestUtility::DescriptionData _jsonDescription;
    };

public:
    explicit TestCommandBase(const DescriptionBuilder& description, const SignatureBuilder& signature)
        : QualityAssurance::ITestUtility::ICommand()
        , _description(description.ToString())
        , _signature(signature.ToString())
    {
    }

    ~TestCommandBase() override = default;

public:
    // ICommand methods
    string Description() const final
    {
        return _description;
    }

    string Signature() const final
    {
        return _signature;
    }

private:
    string _description;
    string _signature;
};
} // namespace Thunder
