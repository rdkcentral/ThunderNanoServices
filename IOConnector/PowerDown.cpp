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

#include <interfaces/IPower.h>

namespace WPEFramework {

namespace Plugin {

    class PowerDown : public IHandler {
    private:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config& copy) = delete;
            Config& operator=(const Config& RHS) = delete;

            Config()
                : Callsign()
                , State()
                , LongPress(~0)
                , PowerOff(Exchange::IPower::PCState::PowerOff)
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("longpress"), &LongPress);
                Add(_T("poweroff"), &PowerOff);
            }
            ~Config() override
            {
            }

        public:
            Core::JSON::String Callsign;
            Core::JSON::DecUInt8 LongPress;
            Core::JSON::EnumerateType<Exchange::IPower::PCState> PowerOff;
        };

    public:
        PowerDown() = delete;
        PowerDown(const PowerDown&) = delete;
        PowerDown& operator=(const PowerDown&) = delete;

        PowerDown(PluginHost::IShell* service, const string& configuration, const uint32_t start, const uint32_t end)
            : _service(service)
            , _state()
            , _marker1(0)
        {
            Config config;
            config.FromString(configuration);
            _callsign = config.Callsign.Value();
            _marker2 = config.LongPress.Value();
            _powerOff = config.PowerOff.Value();

            _marker2 = start + (_marker2 - _ marker1);
            _marker1 = start;
            _state.Add(_marker1);
            if (_marker2 < end) {
                _state.Add(_marker2);
            }
            _state.Add(end);
 
        }
        ~PowerDown() override
        {
        }

    public:
        void Trigger(GPIO::Pin& pin) override
        {
            uint32_t marker;

            ASSERT(_service != nullptr);

            if (_state.Reached(pin.Get(), marker) == true) {
                Exchange::IPower* handler(_service->QueryInterfaceByCallsign<Exchange::IPower>(_callsign));

                if (handler != nullptr) {
                    if (marker == marker1) {
                        // Get the current Device Power mode
                        int state = handler->GetState();

                        if (state == Exchange::IPower::ActiveStandby)
                        {
                            // Wake the Device from standby mode
                            TRACE(Trace::Information, (_T("The device is in Active Standby mode, Waking the device")));
                            handler->SetState(Exchange::IPower::On, 0);
                        }
                        else if (state == Exchange::IPower::On)
                        {
                            //Set the Device to ActiveStandby Mode
                            TRACE(Trace::Information, (_T("The device is in normal mode setting the device to Active Standby")));
                            handler->SetState(Exchange::IPower::ActiveStandby, 0);
                        }
                    }
                    else if (marker == marker2) {
                        TRACE(Trace::Information, (_T("The device is requested to go to PowerOff mode")));
                        handler->SetState(_powerOff, 0);
                    }

                    handler->Release();
                }
            }
        }

    private:
        PluginHost::IShell* _service;
        string _callsign;
        GPIO::TimedInput _state;
        uint32_t _marker1;
        uint32_t _marker2;
        Exchange::IPower::PCState _powerOff;
    };

    static HandlerAdministrator::Entry<PowerDown> handler;

} // namespace Plugin
} // namespace WPEFramework
