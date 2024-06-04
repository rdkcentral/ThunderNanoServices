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
 
#include <interfaces/IBluetooth.h>

#include "Handler.h"
#include "TimedInput.h"

namespace Thunder {

namespace Plugin {

    class BluetoothScan : public IHandler {
    private:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config& copy) = delete;
            Config& operator=(const Config& RHS) = delete;

            Config()
                : Callsign()
                , LongPress()
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("longpress"), &LongPress);
            }
            ~Config() override = default;

        public:
            Core::JSON::String Callsign;
            Core::JSON::DecUInt8 LongPress;
        };

    public:
        BluetoothScan() = delete;
        BluetoothScan(const BluetoothScan&) = delete;
        BluetoothScan& operator=(const BluetoothScan&) = delete;

        BluetoothScan(PluginHost::IShell* service, const string& configuration, const uint32_t start, const uint32_t end)
            : _service(service)
            , _state()
            , _marker(0)
        {
            Config config;
            config.FromString(configuration);
            _callsign = config.Callsign.Value();
            _state.Add(start);
            _state.Add(end);
            _marker = start;
        }
        ~BluetoothScan() override = default;

    public:
        void Trigger(GPIO::Pin& pin) override
        {
            uint32_t marker;

            ASSERT(_service != nullptr);

            if ((_state.Reached(pin.Get(), marker) == true) && (_marker == marker)) {

                Exchange::IBluetooth* handler(_service->QueryInterfaceByCallsign<Exchange::IBluetooth>(_callsign));

                if (handler != nullptr) {

                    handler->Scan(true, 30);
                    handler->Release();
                }
            }
        }

    private:
        PluginHost::IShell* _service;
        string _callsign;
        GPIO::TimedInput _state;
        uint32_t _marker;
    };

    static HandlerAdministrator::Entry<BluetoothScan> handler;

} // namespace Plugin
} // namespace Thunder
