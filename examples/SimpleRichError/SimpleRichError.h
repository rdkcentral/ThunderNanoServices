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

#pragma once

#include "Module.h"

#include <interfaces/ISimpleRichError.h>
#include <interfaces/json/JSimpleRichError.h>

namespace WPEFramework {
namespace Plugin {

    class SimpleRichError : public PluginHost::IPlugin
                          , public PluginHost::JSONRPC
                          , public Exchange::ISimpleRichError {
    public:
        SimpleRichError();
        ~SimpleRichError() override;

        SimpleRichError(const SimpleRichError&) = delete;
        SimpleRichError(SimpleRichError&&) = delete;
        SimpleRichError& operator=(const SimpleRichError&) = delete;
        SimpleRichError& operator=(SimpleRichError&&) = delete;

        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        Core::hresult Restart(Core::OptionalType<Core::ErrorDetails<void>>& errorDetails) override;
        Core::hresult MakeCoffee(const Exchange::ISimpleRichError::coffeetype& type, uint16_t& duration, Core::OptionalType<Exchange::ISimpleRichError::Error>& errorDetails) override;

        BEGIN_INTERFACE_MAP(SimpleRichError)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::ISimpleRichError)
        END_INTERFACE_MAP

    private:
        static Exchange::ISimpleRichError::Error& SetDiagnostic(
            Core::OptionalType<Exchange::ISimpleRichError::Error>& errorDetails,
            const Exchange::ISimpleRichError::Diagnostic::failreason reason,
            const uint8_t temperature,
            const uint8_t level,
            const uint8_t pressure,
            const string& info);

        bool _operational;
        Core::CriticalSection _lock;
    };

} // namespace Plugin
}
