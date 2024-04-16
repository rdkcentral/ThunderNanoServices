/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological
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

#include "BluetoothAudio.h"

namespace Thunder {

namespace Plugin {

    namespace {

        static Metadata<BluetoothAudio> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { subsystem::BLUETOOTH },
            // Terminations
            { subsystem::NOT_BLUETOOTH },
            // Controls
            {}
        );
    }

    /* virtual */ const string BluetoothAudio::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);

        //ASSERT(_sink == nullptr);

        service->Register(&_comNotificationSink);

        _service = service;
        _service->AddRef();

        class GlobalConfig : public Core::JSON::Container {
        public:
            GlobalConfig()
                : Core::JSON::Container()
                , Controller("BluetoothControl")
                , Source()
                //, Sink()
                , Server()
            {
                Add("controller", &Controller);
                Add("source", &Source);
                Add("sink", &Sink);
                Add("server", &Server);
            }

            Core::JSON::String Controller;
            BluetoothAudioSource::Config Source;
            BluetoothAudioSink::Config Sink;
            SignallingServer::Config Server;
        };

        GlobalConfig config;
        config.FromString(_service->ConfigLine());

        _source = Core::ServiceType<BluetoothAudioSource>::Create<BluetoothAudioSource>(this);
        ASSERT(_source != nullptr);

        _sink = Core::ServiceType<BluetoothAudioSink>::Create<BluetoothAudioSink>(this);
        ASSERT(_sink != nullptr);

        _source->Initialize(service, config.Controller.Value(), config.Source);
        _sink->Initialize(service, config.Controller.Value(), config.Sink);

        SignallingServer::Instance().Start(config.Server.Interface.Value(), config.Server.PSM.Value(),
                                            config.Server.InactivityTimeoutMs.Value(), (*_source));

        return {};
    }

    /* virtual */ void BluetoothAudio::Deinitialize(PluginHost::IShell* service)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            if (_source != nullptr) {
                _source->Deinitialize(service);
                _source->Release();
                _source = nullptr;
            }

            if (_sink != nullptr) {
                _sink->Deinitialize(service);
                _sink->Release();
                _sink = nullptr;
            }

            service->Unregister(&_comNotificationSink);

            service->Release();

            SignallingServer::Instance().Clear();

            _service = nullptr;
        }
    }

} // namespace Plugin

}