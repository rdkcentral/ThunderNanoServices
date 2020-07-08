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
#include "Power.h"
#include <interfaces/json/JsonData_Power.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::Power;

    // Registration
    //

    void Power::RegisterAll()
    {
        PluginHost::JSONRPC::Register<PowerData,void>(_T("set"), &Power::endpoint_set, this);
        PluginHost::JSONRPC::Property<Core::JSON::EnumType<StateType>>(_T("state"), &Power::get_state, nullptr, this);
    }

    void Power::UnregisterAll()
    {
        PluginHost::JSONRPC::Unregister(_T("set"));
        PluginHost::JSONRPC::Unregister(_T("state"));
    }

    inline Exchange::IPower::PCState Power::TranslateIn(StateType value)
    {
        Exchange::IPower::PCState newState = Exchange::IPower::PCState::On;

        switch(value)
        {
            case StateType::ON:
                newState = Exchange::IPower::PCState::On;
                break;
            case StateType::ACTIVESTANDBY:
                newState = Exchange::IPower::PCState::ActiveStandby;
                break;
            case StateType::PASSIVESTANDBY:
                newState = Exchange::IPower::PCState::PassiveStandby;
                break;
            case StateType::SUSPENDTORAM:
                newState = Exchange::IPower::PCState::SuspendToRAM;
                break;
            case StateType::HIBERNATE:
                newState = Exchange::IPower::PCState::Hibernate;
                break;
            case StateType::POWEROFF:
                newState = Exchange::IPower::PCState::PowerOff;
                break;
            default:
                ASSERT(false); // Invalid state.
                break;
        }

        return newState;
    }

    inline StateType Power::TranslateOut(Exchange::IPower::PCState value) const
        {
            StateType newState = StateType::ON;

            switch(value)
            {
                case Exchange::IPower::PCState::On:
                    newState = StateType::ON;
                    break;
                case Exchange::IPower::PCState::ActiveStandby:
                    newState = StateType::ACTIVESTANDBY;
                    break;
                case Exchange::IPower::PCState::PassiveStandby:
                    newState = StateType::PASSIVESTANDBY;
                    break;
                case Exchange::IPower::PCState::SuspendToRAM:
                    newState = StateType::SUSPENDTORAM;
                    break;
                case Exchange::IPower::PCState::Hibernate:
                    newState = StateType::HIBERNATE;
                    break;
                case Exchange::IPower::PCState::PowerOff:
                    newState = StateType::POWEROFF;
                    break;
                default:
                    ASSERT(false); // Invalid state.
                    break;
            }

            return newState;
        }

    inline bool Power::InRange(Exchange::IPower::PCState value)
    {
        return (((value >= Exchange::IPower::PCState::On) && (value <= Exchange::IPower::PCState::PowerOff)) ? true : false);
    }

    // API implementation
    //

    // Method: set - Sets power state
        // Return codes:
        //  - ERROR_NONE: Success
        //  - ERROR_GENERAL: General failure
        //  - ERROR_DUPLICATE_KEY: Trying to set the same power mode
        //  - ERROR_ILLEGAL_STATE: Power state is not supported
        //  - ERROR_BAD_REQUEST: Invalid Power state or Bad JSON param data format
        uint32_t Power::endpoint_set(const PowerData& params)
        {
            uint32_t result = Core::ERROR_NONE;

            if ((params.Powerstate.IsSet()) && (params.Timeout.IsSet())) {
                Exchange::IPower::PCState state = TranslateIn(params.Powerstate.Value());
                if (InRange(state)) {
                    const uint32_t& timeout = params.Timeout.Value();
                    ControlClients(state);

                    result = SetState(state, timeout);
                } else {
                    result = Core::ERROR_UNKNOWN_KEY;
                }
            }
            else {
                result = Core::ERROR_BAD_REQUEST;
            }

            return result;
        }

        // Property: state - Power state
        // Return codes:
        //  - ERROR_NONE: Success
        uint32_t Power::get_state(Core::JSON::EnumType<StateType>& response) const
        {
            response = TranslateOut(GetState());

            return Core::ERROR_NONE;
        }

} // namespace Plugin

}

