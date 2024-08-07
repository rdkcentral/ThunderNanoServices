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
#include <qa_interfaces/json/JTestUtility.h>

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
        explicit SignatureBuilder(string const& name, QualityAssurance::ITestUtility::ParameterInfo::Type type, string const& comment)
            : _jsonSignature()
        {
            JsonData::TestUtility::ParameterInfoInfo param;
            param.Name = name;
            param.Type = type;
            param.Comment = comment;

            _jsonSignature.Output = param;
        }

        explicit SignatureBuilder(const JsonData::TestUtility::ParameterInfoInfo& param)
            : _jsonSignature()
        {
            _jsonSignature.Output = param;
        }

        SignatureBuilder& InputParameter(const string& name, QualityAssurance::ITestUtility::ParameterInfo::Type type, const string& comment)
        {
            JsonData::TestUtility::ParameterInfoInfo param;
            param.Name = name;
            param.Type = type;
            param.Comment = comment;
            _jsonSignature.Input.Add(param);

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

        JsonData::TestUtility::ParametersResultData _jsonSignature;
    };

public:
    explicit TestCommandBase(const string& description, const SignatureBuilder& signature)
        : QualityAssurance::ITestUtility::ICommand()
        , _description(description)
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
