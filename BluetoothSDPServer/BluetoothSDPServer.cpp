/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#include "BluetoothSDPServer.h"

namespace Thunder {

ENUM_CONVERSION_BEGIN(Plugin::BluetoothSDPServer::Config::ServiceConfig::serviceclass) { Plugin::BluetoothSDPServer::Config::ServiceConfig::UNSPECIFIED, _TXT("unspecified") },
    { Plugin::BluetoothSDPServer::Config::ServiceConfig::A2DP_AUDIO_SINK, _TXT("a2dp-audio-sink") },
    { Plugin::BluetoothSDPServer::Config::ServiceConfig::A2DP_AUDIO_SOURCE, _TXT("a2dp-audio-source") },
    { Plugin::BluetoothSDPServer::Config::ServiceConfig::AVRCP, _TXT("avrcp") },
    ENUM_CONVERSION_END(Plugin::BluetoothSDPServer::Config::ServiceConfig::serviceclass)

ENUM_CONVERSION_BEGIN(Plugin::BluetoothSDPServer::Config::A2DP::sourcetype) { Plugin::BluetoothSDPServer::Config::A2DP::PLAYER, _TXT("player") },
    { Plugin::BluetoothSDPServer::Config::A2DP::MICROPHONE, _TXT("microphone") },
    { Plugin::BluetoothSDPServer::Config::A2DP::MIXER, _TXT("mixer") },
    { Plugin::BluetoothSDPServer::Config::A2DP::TUNER, _TXT("tuner") },
    ENUM_CONVERSION_END(Plugin::BluetoothSDPServer::Config::A2DP::sourcetype)

ENUM_CONVERSION_BEGIN(Plugin::BluetoothSDPServer::Config::A2DP::sinktype) { Plugin::BluetoothSDPServer::Config::A2DP::HEADPHONES, _TXT("headphones") },
    { Plugin::BluetoothSDPServer::Config::A2DP::SPEAKER, _TXT("speaker") },
    { Plugin::BluetoothSDPServer::Config::A2DP::RECORDER, _TXT("recorder") },
    { Plugin::BluetoothSDPServer::Config::A2DP::AMPLIFIER, _TXT("amplifier") },
    ENUM_CONVERSION_END(Plugin::BluetoothSDPServer::Config::A2DP::sinktype)

namespace Plugin {

    namespace {
        static Metadata<BluetoothSDPServer> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { subsystem::BLUETOOTH },
            // Terminations
            { subsystem::NOT_BLUETOOTH },
            // Controls
            {});
    }

    /* virtual */ const string BluetoothSDPServer::Initialize(PluginHost::IShell* service)
    {
        string result;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        _service = service;
        _service->AddRef();

        Config config(_service->ConfigLine());

        Configure(Config(_service->ConfigLine()));

        if (_services.empty() == false) {
            _server.Start(config.Interface, config.PSM, config.InactivityTimeoutMs);
        }
        else {
            TRACE(Trace::Error, (_T("No services configured for SDP Server")));
        }

        service->Register(this);

        return (result);
    }

    Bluetooth::SDP::Service* BluetoothSDPServer::ConfigureA2DPService(const bool sink, const string& configuration)
    {
        Bluetooth::SDP::Service* newService = nullptr;

        _server.WithServiceTree([&](Bluetooth::SDP::Tree& tree) {

            namespace SDP = Bluetooth::SDP;

            SDP::Service& service = tree.Add();

            service.ServiceClassIDList()->Add(sink? SDP::ClassID::AudioSink : SDP::ClassID::AudioSource);
            service.ProfileDescriptorList()->Add(SDP::ClassID::AdvancedAudioDistribution, 0x0103 /* v1.3 */);
            service.ProtocolDescriptorList()->Add(SDP::ClassID::L2CAP, SDP::Service::Protocol::L2CAP(/* PSM */ 25));
            service.ProtocolDescriptorList()->Add(SDP::ClassID::AVDTP, SDP::Service::Protocol::AVDTP(0x0102 /* v1.2 */));

            uint16_t features = 0;

            if (sink == true) {
                Config::A2DP::Config<Config::A2DP::sinktype> config(configuration);
                features = config.GetFeatures();
            }
            else {
                Config::A2DP::Config<Config::A2DP::sourcetype> config(configuration);
                features = config.GetFeatures();
            }

            if (features != 0) {
                service.Add(SDP::Service::AttributeDescriptor::a2dp::SupportedFeatures, SDP::Service::Data::Element<uint16_t>(features));
            }

            newService = &service;
        });

        return (newService);
    }

    void BluetoothSDPServer::Configure(const Config& config)
    {
        _lock.Lock();

        auto service = config.Services.Elements();

        while (service.Next() == true) {
            Bluetooth::SDP::Service* newService = nullptr;

            if (service.Current().Class.Value() == Config::ServiceConfig::serviceclass::A2DP_AUDIO_SINK) {
                newService = ConfigureA2DPService(true, service.Current().Parameters.Value());
            } else if (service.Current().Class.Value() == Config::ServiceConfig::serviceclass::A2DP_AUDIO_SOURCE) {
                newService = ConfigureA2DPService(false, service.Current().Parameters.Value());
            } else {
                TRACE(Trace::Error, (_T("Unknown service class defined in configration")));
            }

            if (newService != nullptr) {
                newService->Description(service.Current().Name.Value());

                if (service.Current().Public.Value() == true) {
                    newService->BrowseGroupList()->Add(Bluetooth::SDP::ClassID::PublicBrowseRoot);
                }

                _services.emplace_back(std::make_pair(newService, service.Current().Callsign.Value()));
            }
        }

        _lock.Unlock();
    }

    /* virtual */ void BluetoothSDPServer::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);

            _service->Unregister(this);

            _server.Stop();

            _service->Release();
            _service = nullptr;
        }
    }

    /* virtual */ void BluetoothSDPServer::Activated(const string& callsign, PluginHost::IShell* /* service */)
    {
        _lock.Lock();

        for (auto& entry : _services) {
            if (entry.second == callsign) {
                Bluetooth::SDP::Service*& service = entry.first;
                ASSERT(service != nullptr);
                service->Enable(true);
                TRACE(Trace::Information, (_T("Enabled service 0x%08x '%s'"), service->Handle(), service->Name().c_str()));
            }
        }

        _lock.Unlock();
    }

    /* virtual */ void BluetoothSDPServer::Deactivated(const string& callsign, PluginHost::IShell* /* service */)
    {
        _lock.Lock();

        for (auto& entry : _services) {
            if (entry.second == callsign) {
                Bluetooth::SDP::Service*& service = entry.first;
                ASSERT(service != nullptr);
                service->Enable(false);
                TRACE(Trace::Information, (_T("Disabled service 0x%08x '%s'"), service->Handle(), service->Name().c_str()));
            }
        }

        _lock.Unlock();
    }

} // namespace Plugin

}
