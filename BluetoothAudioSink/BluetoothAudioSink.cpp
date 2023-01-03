/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

    namespace {

        static Metadata<BluetoothAudioSink> metadata(
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
        _codecSettings = config.Codecs.Value();
        _latency = (config.Latency.Value() <= 10000? config.Latency.Value() : 10000);

        service->Register(&_comNotificationSink);

        Exchange::JBluetoothAudioSink::Register(*this, this);

        if (Core::File(_service->PersistentPath()).IsDirectory() == false) {
            if (Core::Directory(_service->PersistentPath().c_str()).CreatePath() == false) {
                TRACE(Trace::Error, (_T("Failed to create persistent storage folder [%s]"), _service->PersistentPath().c_str()));
            }
        } else {
            Exchange::IBluetooth* bluetoothCtl(Controller());
            if (bluetoothCtl != nullptr) {
                Core::Directory storageDir (_service->PersistentPath().c_str(), _T("*.json"));

                while ((_sink == nullptr) && (storageDir.Next() == true)) {
                    string filename = Core::File::FileName(storageDir.Name());

                    Bluetooth::Address address(filename.c_str());
                    if (address.IsValid() == true) {
                        Exchange::IBluetooth::IDevice* device = bluetoothCtl->Device(filename);
                        if (device != nullptr) {
                            Core::File fileData(storageDir.Current().c_str());
                            if (fileData.Open(true) == true) {
                                A2DPSink::AudioServiceData data;
                                data.IElement::FromFile(fileData);

                                _sink = new A2DPSink(this, _codecSettings, device, _sinkSEID, data);
                                ASSERT(_sink != nullptr);

                                TRACE(Trace::Information, (_T("Loaded previously assigned audio sink [%s]"), filename.c_str()));
                            }

                            device->Release();
                        } else {
                            TRACE(Trace::Error, (_T("Device [%s] is not known"), filename.c_str()));
                        }
                    }
                }

                bluetoothCtl->Release();
            } else {
                TRACE(Trace::Error, (_T("Bluetooth is not available")));
            }
        }

        return {};
    }

    /* virtual */ void BluetoothAudioSink::Deinitialize(PluginHost::IShell* service)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            delete _sink;
            _sink = nullptr;

            Exchange::JBluetoothAudioSink::Unregister(*this);

            service->Unregister(&_comNotificationSink);

            service->Release();
            _service = nullptr;
        }
    }

    /* virtual */ uint32_t BluetoothAudioSink::Callback(Exchange::IBluetoothAudioSink::ICallback* callback)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        _lock.Lock();

        if (callback == nullptr) {
            if (_callback != nullptr) {
                _callback->Release();
                _callback = nullptr;
                result = Core::ERROR_NONE;
            }
        }
        else if (_callback == nullptr) {
            _callback = callback;
            _callback->AddRef();
            result = Core::ERROR_NONE;
        }

        _lock.Unlock();

        return (result);
    }

    /* virtual */ uint32_t BluetoothAudioSink::Assign(const string& address)
    {
        uint32_t result = Core::ERROR_ALREADY_CONNECTED;

        _lock.Lock();

        if (_sink == nullptr) {
            Exchange::IBluetooth* bluetoothCtl(Controller());
            if (bluetoothCtl != nullptr) {
                Exchange::IBluetooth::IDevice* device = bluetoothCtl->Device(address);
                if (device != nullptr) {
                    _sink = new A2DPSink(this, _codecSettings, device, _sinkSEID);
                    if (_sink != nullptr) {
                        TRACE(Trace::Information, (_T("Assigned [%s] to Bluetooth audio sink"), address.c_str()));
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

        _lock.Unlock();

        return (result);
    }

    /* virtual */ uint32_t BluetoothAudioSink::Revoke()
    {
        uint32_t result = Core::ERROR_ALREADY_RELEASED;

        _lock.Lock();

        if (_sink != nullptr) {
            if (_sink->State() == Exchange::IBluetoothAudioSink::DISCONNECTED) {
                Core::File file(_service->PersistentPath() + _sink->Address() + _T(".json"));
                if (file.Destroy() == true) {
                    TRACE(Trace::Information, (_T("Revoked from Bluetooth audio sink assignment from [%s]"), _sink->Address().c_str()));
                    result = Core::ERROR_NONE;
                    delete _sink;
                    _sink = nullptr;
                } else {
                    result = Core::ERROR_GENERAL;
                    TRACE(Trace::Error, (_T("Failed to revoke Bluetooth audio sink assignment from [%s]"), _sink->Address().c_str()));
                }
            } else {
                result = Core::ERROR_ILLEGAL_STATE;
                TRACE(Trace::Error, (_T("Sink in use, disconnect first")));
            }
        } else {
            TRACE(Trace::Error, (_T("Sink not assigned")));
        }

        _lock.Unlock();

        return (result);
    }

    void BluetoothAudioSink::Operational(const A2DPSink::AudioServiceData& data)
    {
        ASSERT(_sink != nullptr);

        // Store the settings, if not already done..
        Core::File settingsFile(_service->PersistentPath() + _sink->Address() + _T(".json"));
        if ( (settingsFile.Exists() == false) && (settingsFile.Create() == true) ) {
            TRACE(Trace::Information, (_T("Storing Bluetooth audio sink information [%s]"), _sink->Address().c_str()));
            data.IElement::ToFile(settingsFile);
            settingsFile.Close();
        }
    }

} // namespace Plugin

}
