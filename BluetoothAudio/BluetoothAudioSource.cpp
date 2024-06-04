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

#include "BluetoothAudioSource.h"

#include <interfaces/IBluetooth.h>
#include <interfaces/IBluetoothAudio.h>
#include <interfaces/json/JBluetoothAudioSource.h>


namespace Thunder {

namespace Plugin {

    string BluetoothAudioSource::Initialize(PluginHost::IShell* service, const string& controller, const Config& config)
    {
        ASSERT(_source == nullptr);
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);

        _lock.Lock();

        _service = service;
        _service->AddRef();

        _controller = controller;
        ASSERT(controller.empty() == false);

        _source = Core::ServiceType<A2DPSource>::Create<A2DPSource>(*this, config);
        ASSERT(_source != nullptr);

        SignallingServer::Instance().Register(_source);

        SignallingServer::Instance().Add(true, new Bluetooth::A2DP::SBC(53), *_source);

        _lock.Unlock();

        Exchange::BluetoothAudio::JSource::Register(*_jsonRpcModule, this);

        return {};
    }

    void BluetoothAudioSource::Deinitialize(PluginHost::IShell* service)
    {
        _lock.Lock();

        if (_service != nullptr) {
            ASSERT(_service == service);

            Exchange::BluetoothAudio::JSource::Unregister(*_jsonRpcModule);

            SignallingServer::Instance().Unregister(_source);

            if (_source != nullptr) {
                _source->Release();
                _source = nullptr;
            }

            service->Release();
            _service = nullptr;
        }

        _lock.Unlock();
    }

} // namespace Plugin

}
