/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#include "DynamicJSONRPCErrorMessage.h"
#include <interfaces/json/JMath.h>

#include <sstream>

namespace Thunder {

namespace Plugin {

    namespace {
        static Metadata<DynamicJSONRPCErrorMessage> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { },
            // Terminations
            { },
            // Controls
            { }
        );
    }

    const string DynamicJSONRPCErrorMessage::Initialize(PluginHost::IShell*)
    {
        string message{};

        Exchange::JMath::Register(*this, this);

        return (message);
    }

    void DynamicJSONRPCErrorMessage::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        Exchange::JMath::Unregister(*this);
    }

    string DynamicJSONRPCErrorMessage::Information() const
    {
        return {};
    }

    uint32_t DynamicJSONRPCErrorMessage::OnJSONRPCError(const Core::JSONRPC::Context&, const string& designator, const string& parameters, string& errormessage) {
        uint32_t result = Core::ERROR_GENERAL;

        string method(Core::JSONRPC::Message::Method(designator));

        if(method == _T("add")) {
            JsonData::Math::AddParamsInfo addparams;
            addparams.FromString(parameters);
            std::stringstream message;
            message <<_T("Error handling add method for some peculiar reason values: ") << addparams.A << _T(" and ") << addparams.B;
            errormessage = message.str();
            result = Core::ERROR_INVALID_PARAMETER;
        } 
        return result;
    }


} // namespace Plugin

}
