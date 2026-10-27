/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

#include "SimpleRichError.h"

namespace Thunder {
namespace Plugin {

    namespace {
        static Metadata<SimpleRichError> metadata(
            1, 0, 0,
            { },
            { },
            { });
    }

    SimpleRichError::SimpleRichError()
        : PluginHost::IPlugin()
        , PluginHost::JSONRPC()
        , Example::ISimpleRichError()
        , _operational(false)
    {
    }

    SimpleRichError::~SimpleRichError() = default;

    const string SimpleRichError::Initialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        Example::JSimpleRichError::Register(*this, this);
        return (string());
    }

    void SimpleRichError::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        Example::JSimpleRichError::Unregister(*this);
        _operational = false;
    }

    string SimpleRichError::Information() const
    {
        return (_T("Demonstrates optional JSON-RPC code, message, and typed error data."));
    }

    Core::hresult SimpleRichError::Restart(Core::OptionalType<Core::ErrorDetails<void>>& errorDetails)
    {
        Core::hresult result = Core::ERROR_NONE;

        if (_operational == false) {
            _operational = true;
        }
        else {
            errorDetails = Core::ErrorDetails<void>();
            errorDetails.Value().message = _T("The coffee machine is already running");
            result = Core::ERROR_ILLEGAL_STATE;
        }

        return (result);
    }

    Core::hresult SimpleRichError::MakeCoffee(const Example::ISimpleRichError::coffeetype& type, uint16_t& duration, Core::OptionalType<Example::ISimpleRichError::Error>& errorDetails)
    {
        Core::hresult result = Core::ERROR_NONE;
        duration = 0;

        if (_operational == false) {
            Example::ISimpleRichError::Error& details(SetDiagnostic(errorDetails, Example::ISimpleRichError::Diagnostic::HARDWARE_FAILURE, 20, 100, 0, _T("Restart the machine before making coffee")));
            details.code = -32050;
            details.message = _T("Coffee machine is not running");
            result = Core::ERROR_ILLEGAL_STATE;
        }
        else if (type == Example::ISimpleRichError::ESPRESSO) {
            duration = 25;
        }
        else if (type == Example::ISimpleRichError::CAPPUCCINO) {
            SetDiagnostic(errorDetails, Example::ISimpleRichError::Diagnostic::NO_WATER, 88, 0, 2, _T("Refill the water tank"));
            result = Core::ERROR_UNAVAILABLE;
        }
        else if (type == Example::ISimpleRichError::LATTE) {
            Example::ISimpleRichError::Error& details(SetDiagnostic(errorDetails, Example::ISimpleRichError::Diagnostic::NO_MILK, 92, 80, 9, _T("Refill the milk reservoir")));
            details.message = _T("Unable to prepare latte");
            result = Core::ERROR_UNAVAILABLE;
        }
        else {
            result = Core::ERROR_BAD_REQUEST;
        }

        return (result);
    }

    Example::ISimpleRichError::Error& SimpleRichError::SetDiagnostic(
        Core::OptionalType<Example::ISimpleRichError::Error>& errorDetails,
        const Example::ISimpleRichError::Diagnostic::failreason reason,
        const uint8_t temperature,
        const uint8_t level,
        const uint8_t pressure,
        const string& info)
    {
        Example::ISimpleRichError::Diagnostic diagnostic;
        diagnostic.reason = reason;
        diagnostic.water_temperature = temperature;
        diagnostic.water_level = level;
        diagnostic.brew_pressure = pressure;
        diagnostic.info = info;

        errorDetails = Example::ISimpleRichError::Error();
        errorDetails.Value().data = diagnostic;

        return (errorDetails.Value());
    }

} // namespace Plugin
} // namespace Thunder
