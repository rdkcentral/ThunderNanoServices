/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 RDK Management
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
        _codecSettings = config.Codecs.Value();

        Exchange::JBluetoothAudioSink::Register(*this, this);

        if (config.SDPService.Enable.Value() == true) {
            // Start the SDP server...
            // Add an A2DP v1.3 AudioSource service, supporting AVDTP v1.2, transported via L2CAP connection using PSM 25.
            namespace SDP = Bluetooth::SDP;
            _sdpServer.Lock();
            SDP::Service& audioSource = _sdpServer.Add();
            audioSource.Description(config.SDPService.Name.Value(), config.SDPService.Description.Value(), config.SDPService.Provider.Value());
            audioSource.BrowseGroupList()->Add(SDP::ClassID::PublicBrowseRoot);
            audioSource.ServiceClassIDList()->Add(SDP::ClassID::AudioSource);
            audioSource.ProfileDescriptorList()->Add(SDP::ClassID::AdvancedAudioDistribution, 0x0103);
            audioSource.ProtocolDescriptorList()->Add(SDP::ClassID::L2CAP, SDP::Service::Protocol::L2CAP(25));
            audioSource.ProtocolDescriptorList()->Add(SDP::ClassID::AVDTP, SDP::Service::Protocol::AVDTP(0x0102));
            _sdpServer.Unlock();
            _sdpServer.Start();
        }

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

                                TRACE(Trace::Information, (_T("Loaded assigned audio sink [%s]"), filename.c_str()));
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
        ASSERT(_service == service);

        delete _sink;
        _sink = nullptr;

        _sdpServer.Stop();

        Exchange::JBluetoothAudioSink::Unregister(*this);

        service->Release();
        _service = nullptr;
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
            Core::File file(_service->PersistentPath() + _sink->Address() + _T(".json"));
            if (file.Destroy() == true) {
                TRACE(Trace::Information, (_T("Revoked [%s] from Bluetooth audio sink"), _sink->Address().c_str()));
                result = Core::ERROR_NONE;
                delete _sink;
                _sink = nullptr;
            } else {
                result = Core::ERROR_GENERAL;
                TRACE(Trace::Error, (_T("Failed to revoke [%s] from Bluetooth audio sink"), _sink->Address().c_str()));
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
            data.IElement::ToFile(settingsFile);
            settingsFile.Close();
        }
    }

} // namespace Plugin

}
