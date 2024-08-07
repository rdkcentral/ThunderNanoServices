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
 
#include "Module.h"

#include "CommandCore/TestCommandController.h"
#include <qa_interfaces/json/JTestUtility.h>

namespace Thunder {
namespace TestCore {

    class TestUtilityImp : public QualityAssurance::ITestUtility {
    private:
        using StringList = RPC::IteratorType<QualityAssurance::ITestUtility::IStringIterator>;
        using ParameterInfoList = RPC::IteratorType<QualityAssurance::ITestUtility::IParameterInfoIterator>;

    public:
        TestUtilityImp(const TestUtilityImp&) = delete;
        TestUtilityImp& operator=(const TestUtilityImp&) = delete;

    public:
        TestUtilityImp()
        : _shutdownTimeout(0) {
        }

        ~TestUtilityImp() override
        {
            ::SleepMs(_shutdownTimeout);
            TestCore::TestCommandController::Instance().Release();
        };

        //  ITestUtility methods
        // -------------------------------------------------------------------------------------------------------
        uint32_t Description(const string& commandName, string& description) const override
        {
            uint32_t result = Core::ERROR_BAD_REQUEST;

            TRACE(Trace::Information, (_T("*** Call Description ***")));

            if (commandName.empty() != true) {
                auto const command = TestCore::TestCommandController::Instance().Command(commandName);

                if (command) {
                    description = command->Description();
                    if (description.empty() != true) {
                        result = Core::ERROR_NONE;
                    }
                } else {
                    result = Core::ERROR_UNAVAILABLE;
                }
            }
            return result;
        }

        uint32_t Parameters(const string& commandName, QualityAssurance::ITestUtility::IParameterInfoIterator*& input, QualityAssurance::ITestUtility::ParameterInfo& output) const override
        {
            uint32_t result = Core::ERROR_BAD_REQUEST;

            if (commandName.empty() != true) {
                auto const command = TestCore::TestCommandController::Instance().Command(commandName);

                if (command) {
                    JsonData::TestUtility::ParametersResultData parameterResult;
                    if (parameterResult.FromString(command->Signature()) == true) {
                        output.name = parameterResult.Output.Name.Value();
                        output.type = parameterResult.Output.Type.Value();
                        output.comment = parameterResult.Output.Comment.Value();

                        std::list<QualityAssurance::ITestUtility::ParameterInfo> parameterList;
                        Core::JSON::ArrayType<JsonData::TestUtility::ParameterInfoInfo>::Iterator paramIterator(parameterResult.Input.Elements());
                        while (paramIterator.Next()) {
                            QualityAssurance::ITestUtility::ParameterInfo param;
                            param.name = paramIterator.Current().Name.Value();
                            param.type = paramIterator.Current().Type.Value();
                            param.comment = paramIterator.Current().Comment.Value();
                            parameterList.push_back(param);
                        }
                        if (parameterList.empty() != true) {
                            input = Core::ServiceType<ParameterInfoList>::Create<ParameterInfoList>(parameterList);
                        }

                        result = Core::ERROR_NONE;
                    }
                } else {
                    result = Core::ERROR_UNAVAILABLE;
                }
            }

            return result;
        }

        uint32_t Commands(IStringIterator*& commands) const override
        {
            TRACE(Trace::Information, (_T("*** Call Commands ***")));

            auto const supportedCommands = TestCore::TestCommandController::Instance().Commands();
            ASSERT(supportedCommands != nullptr);

            std::list<string> commandList;
            while (supportedCommands->Next()) {
                commandList.push_back(supportedCommands->Command()->Name());
            }
            if (commandList.empty() != true) {
                commands = Core::ServiceType<StringList>::Create<StringList>(commandList);
            }

            return (Core::ERROR_NONE);
        }

        uint32_t ShutdownTimeout(const uint32_t timeout) override
        {
            _shutdownTimeout = timeout;
            return Core::ERROR_NONE;
        }


        uint32_t RunCrash(const CrashInfo& info) override
        {
            uint32_t result = Core::ERROR_BAD_REQUEST;

            TRACE(Trace::Information, (_T("*** Call RunCrash ***")));

            if (info.command.empty() != true) {
                auto const command = TestCore::TestCommandController::Instance().Command(info.command);

                if (command) {
                    if ((info.delay) || (info.count)) {
                        JsonData::TestUtility::RunCrashParamsData::CrashInfoData crashInfo;
                        crashInfo.Command = info.command;
                        crashInfo.Delay = info.delay;
                        crashInfo.Count = info.count;

                        string crashInfoStr;
                        crashInfo.ToString(crashInfoStr);
                        command->Execute(crashInfoStr);
                        result = Core::ERROR_NONE;
                    }
                } else {
                    result = Core::ERROR_UNAVAILABLE;
                }
            }

            return result;
        }

        uint32_t RunMemory(const MemoryInfo& info, MemoryResult& memoryResult) override
        {
            uint32_t result = Core::ERROR_BAD_REQUEST;

            TRACE(Trace::Information, (_T("*** Call RunMemory ***")));

            if (info.command.empty() != true) {
                auto const command = TestCore::TestCommandController::Instance().Command(info.command);

                if (command) {
                    JsonData::TestUtility::RunMemoryParamsData::MemoryInfoData memoryInfo;
                    memoryInfo.Command = info.command;
                    memoryInfo.Size = info.size;

                    string strMemoryInfo;
                    memoryInfo.ToString(strMemoryInfo);
                    string strMemoryResult = command->Execute(strMemoryInfo);
                    JsonData::TestUtility::MemoryResultData memoryResultData;
                    if (memoryResultData.FromString(strMemoryResult) == true) {
                        memoryResult.allocated = memoryResultData.Allocated.Value(); 
                        memoryResult.size = memoryResultData.Size.Value();
                        memoryResult.resident = memoryResultData.Resident.Value();

                        result = Core::ERROR_NONE;
                    }
                } else {
                    result = Core::ERROR_UNAVAILABLE;
                }
            }
            return result;
        }
        uint32_t Execute(const string& commandName, const string& params, string& result) override
        {
            uint32_t status = Core::ERROR_BAD_REQUEST;

            TRACE(Trace::Information, (_T("*** Call Execute Commands ***")));

            if (commandName.empty() != true) {
                auto const command = TestCore::TestCommandController::Instance().Command(commandName);

                if (command) {
                    result = command->Execute(params);
                    if (result.empty() != true) {
                        status = Core::ERROR_NONE;
                    }
                } else {
                    status = Core::ERROR_UNAVAILABLE;
                }
            }
            return status;
        }

        BEGIN_INTERFACE_MAP(TestUtilityImp)
        INTERFACE_ENTRY(QualityAssurance::ITestUtility)
        END_INTERFACE_MAP

    private:
        uint32_t _shutdownTimeout;
    };

    SERVICE_REGISTRATION(TestUtilityImp, 1, 0)
} // namespace TestCore
} // namespace WPEFramewor
