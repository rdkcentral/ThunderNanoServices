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
 
#include "Module.h"
#include "TestUtility.h"
#include <interfaces/json/JsonData_TestUtility.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::TestUtility;

    // Registration
    //

    void TestUtility::RegisterAll()
    {
        Register<RunmemoryParamsData,RunmemoryResultData>(_T("runmemory"), &TestUtility::endpoint_runmemory, this);
        Register<RuncrashParamsData,void>(_T("runcrash"), &TestUtility::endpoint_runcrash, this);
        Property<Core::JSON::ArrayType<Core::JSON::String>>(_T("commands"), &TestUtility::get_commands, nullptr, this);
        Property<DescriptionData>(_T("description"), &TestUtility::get_description, nullptr, this);
        Property<ParametersData>(_T("parameters"), &TestUtility::get_parameters, nullptr, this);
        Property<Core::JSON::DecUInt32>(_T("shutdowntimeout"), nullptr, &TestUtility::set_shutdowntimeout, this);
    }

    void TestUtility::UnregisterAll()
    {
        Unregister(_T("runcrash"));
        Unregister(_T("runmemory"));
        Unregister(_T("parameters"));
        Unregister(_T("description"));
        Unregister(_T("commands"));
        Unregister(_T("shutdowntimeout"));
    }

    // API implementation
    //

    // Method: runmemory - Runs a memory test command
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unknown category
    //  - ERROR_BAD_REQUEST: Bad JSON param data format
    uint32_t TestUtility::endpoint_runmemory(const RunmemoryParamsData& params, RunmemoryResultData& response)
    {
        uint32_t result = Core::ERROR_BAD_REQUEST;

        TRACE(Trace::Information, (_T("*** Call endpoint_runmemory ***")));

        if (params.Command.IsSet() == true)
        {
            Exchange::ITestUtility::ICommand* command = _testUtilityImp->Command(params.Command.Value());

            if (command) {
                string tmpParams, tmpResponse;

                params.ToString(tmpParams);
                tmpResponse = command->Execute(tmpParams);
                if (response.FromString(tmpResponse) == true) {
                    result = Core::ERROR_NONE;
                }
            } else {
                result = Core::ERROR_UNAVAILABLE;
            }
        }
        return result;
    }

    // Method: runcrash - Runs a crash test command
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unknown category
    //  - ERROR_BAD_REQUEST: Bad JSON param data format
    uint32_t TestUtility::endpoint_runcrash(const RuncrashParamsData& params)
    {
        uint32_t result = Core::ERROR_BAD_REQUEST;

        TRACE(Trace::Information, (_T("*** Call endpoint_runcrash ***")));

        if (params.Command.IsSet() == true)
        {
            Exchange::ITestUtility::ICommand* command = _testUtilityImp->Command(params.Command.Value());

            if (command) {
                if ((params.Delay.IsSet() == true) || (params.Count.IsSet() == true)) {
                    string tmpParams;

                    params.ToString(tmpParams);
                    command->Execute(tmpParams);
                    result = Core::ERROR_NONE;
                }
            } else {
                result = Core::ERROR_UNAVAILABLE;
            }
        }

        return result;
    }

    // Property: commands - Retrieves the list of test commands
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t TestUtility::get_commands(Core::JSON::ArrayType<Core::JSON::String>& response) const
    {
        TRACE(Trace::Information, (_T("*** Call endpoint_commands ***")));

        Exchange::ITestUtility::ICommand::IIterator* supportedCommands = _testUtilityImp->Commands();
        ASSERT(supportedCommands != nullptr);

        while (supportedCommands->Next()) {
            Core::JSON::String name;
            name = supportedCommands->Command()->Name();
            response.Add(name);
        }

        return (Core::ERROR_NONE);
    }

    // Property: description - Retrieves the description of selected test command
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unknown category
    //  - ERROR_BAD_REQUEST: Bad JSON param data format
    uint32_t TestUtility::get_description(const string& index, DescriptionData& response) const
    {
        uint32_t result = Core::ERROR_BAD_REQUEST;

        TRACE(Trace::Information, (_T("*** Call endpoint_description ***")));

        if (index != "")
        {
            Exchange::ITestUtility::ICommand* command = _testUtilityImp->Command(index);

            if (command) {
                if (response.FromString(command->Description()) == true) {
                    result = Core::ERROR_NONE;
                }
            } else {
                result = Core::ERROR_UNAVAILABLE;
            }
        }

        return result;
    }

    // Property: parameters - Retrieves parameters of the selected test command
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNAVAILABLE: Unknown category
    //  - ERROR_BAD_REQUEST: Bad JSON param data format
    uint32_t TestUtility::get_parameters(const string& index, ParametersData& response) const
    {
        uint32_t result = Core::ERROR_BAD_REQUEST;

        if (index != "")
        {
            Exchange::ITestUtility::ICommand* command = _testUtilityImp->Command(index);

            if (command) {
                if (response.FromString(command->Signature()) == true) {
                    result = Core::ERROR_NONE;
                }
            } else {
                result = Core::ERROR_UNAVAILABLE;
            }
        }

        return result;
    }

    // Property: shutdowntimeout - Timeout to be waited before deactivating the plugin
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t TestUtility::set_shutdowntimeout(const Core::JSON::DecUInt32& param)
    {
        uint32_t result = Core::ERROR_NONE;

        if (param.IsSet()) {
            result = _testUtilityImp->ShutdownTimeout(param.Value());
        }

        return result;
    }

} // namespace Plugin

}

