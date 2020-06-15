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

#include "BluetoothAudioSink.h"

namespace WPEFramework {

namespace Plugin {

    SERVICE_REGISTRATION(BluetoothAudioSink, 1, 0);

    /* virtual */ const string BluetoothAudioSink::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);

        string error;

        _service = service;
        _service->AddRef();

        Config config;
        config.FromString(_service->ConfigLine());
        _controller = config.Controller.Value();

        Exchange::JBluetoothAudioSink::Register(*this, this);

        return {};
    }

    /* virtual */ void BluetoothAudioSink::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        Exchange::JBluetoothAudioSink::Unregister(*this);

        _service->Release();
        _service = nullptr;
    }

    /* virtual */ uint32_t BluetoothAudioSink::Assign(const string& address)
    {
        uint32_t result = Core::ERROR_ALREADY_CONNECTED;

        if (_sink == nullptr) {
            Exchange::IBluetooth* bluetoothCtl(Controller());
            if (bluetoothCtl != nullptr) {
                Exchange::IBluetooth::IDevice* device = bluetoothCtl->Device(address);
                if (device != nullptr) {
                    const uint8_t seid = 1; // Revisit this if multiple simultaneous sinks are to be supported.
                    _sink = new A2DPSink(this, device, seid, [this](){
                        Updated();
                    });
                    if (_sink != nullptr) {
                        result = Core::ERROR_NONE;
                    } else {
                        TRACE(Trace::Error, (_T("Failed to create audio sink")));
                        result = Core::ERROR_GENERAL;
                    }
                } else {
                    TRACE(Trace::Error, (_T("Device [%s] is not known"), address.c_str()));
                    result = Core::ERROR_UNKNOWN_KEY;
                }

                bluetoothCtl->Release();
            } else {
                TRACE(Trace::Error, (_T("Bluetooth is not available")));
                result = Core::ERROR_UNAVAILABLE;
            }
        } else {
            TRACE(Trace::Error, (_T("Already assigned, revoke first")));
        }

        return (result);
    }

    /* virtual */ uint32_t BluetoothAudioSink::Revoke(const string& device)
    {
        uint32_t result = Core::ERROR_ALREADY_RELEASED;

        if (_sink != nullptr) {
            delete _sink;
            _sink = nullptr;
            result = Core::ERROR_NONE;
        } else {
            TRACE(Trace::Error, (_T("Sink not assigned, assign first")));
        }
        // TODO
        return (result);
    }

} // namespace Plugin

}
