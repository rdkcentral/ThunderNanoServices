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
#include "Handler.h"
#include "TimedInput.h"

namespace WPEFramework {

namespace Plugin {

    class Reporter : public IHandler {
    public:
        Reporter() = delete;
        Reporter(const Reporter&) = delete;
        Reporter& operator=(const Reporter&) = delete;

        Reporter(PluginHost::IShell* service, const string& configuration, const uint32_t start, const uint32_t end)
            : _service(service)
            , _message(configuration)
            , _state()
            , _begin(start)
            , _end(end)
        {
            _state.Add(start);
            _state.Add(end);
        }
        ~Reporter() override
        {
        }

    public:
        void Trigger(GPIO::Pin& pin) override
        {
            uint32_t marker;

            ASSERT(_service != nullptr);

            if ( (_state.Reached(pin.Get(), marker) == true) && (_begin == marker) ) {
 
                TRACE(Trace::Information, (_T("Reached Interval [%d] - [%d] seconds."), _begin / 1000, _end / 1000));
                _service->Notify(_message);
            }
        }

    private:
        PluginHost::IShell* _service;
        string _message;
        GPIO::TimedInput _state;
        uint32_t _begin;
        uint32_t _end;
    };

    static HandlerAdministrator::Entry<Reporter> handler;

} // namespace Plugin
} // namespace WPEFramework
