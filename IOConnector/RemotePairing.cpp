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
 
#include <interfaces/IKeyHandler.h>

#include "Handler.h"
#include "TimedInput.h"

namespace WPEFramework {

namespace Plugin {

    class RemotePairing : public IHandler {
    private:
        RemotePairing();
        RemotePairing(const RemotePairing&);
        RemotePairing& operator=(const RemotePairing&);

        class Config : public Core::JSON::Container {
        private:
            Config(const Config& copy) = delete;
            Config& operator=(const Config& RHS) = delete;

        public:
            Config()
                : Callsign()
                , Producer()
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("producer"), &Producer);
            }
            virtual ~Config()
            {
            }

        public:
            Core::JSON::String Callsign;
            Core::JSON::String Producer;
        };

    public:
        RemotePairing(PluginHost::IShell* service, const string& configuration)
            : _service(service)
        {
            Config config;
            config.FromString(configuration);
            _producer = config.Producer.Value();
            _callsign = config.Callsign.Value();
        }
        virtual ~RemotePairing()
        {
        }

    public:
        void Interval(const uint32_t start, const uint32_t end) override {
            _state.Clear();
            _state.Add(start);
            _state.Add(end);
            _marker = start;
        }
        void Trigger(GPIO::Pin& pin) override
        {

            ASSERT(_service != nullptr);

       }
       void Trigger(GPIO::Pin& pin) override
        {
            uint32_t marker;

            ASSERT(_service != nullptr);

            if ( (_state.Analyse(pin.Set(), marker) == true) && (_marker == marker) ) {
                Exchange::IKeyHandler* handler(_service->QueryInterfaceByCallsign<Exchange::IKeyHandler>(_callsign));

                if (handler != nullptr) {
                    Exchange::IKeyProducer* producer(handler->Producer(_producer));

                    if (producer != nullptr) {
                        producer->Pair();
                        producer->Release();
                    }

                    handler->Release();
                }
            }
        }

    private:
        PluginHost::IShell* _service;
        string _callsign;
        string _producer;
        TimedInput _state;
        uint32_t _marker;
    };

    static HandlerAdministrator::Entry<RemotePairing> handler;

} // namespace Plugin
} // namespace WPEFramework
