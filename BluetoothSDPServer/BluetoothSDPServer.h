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

#pragma once

#include "Module.h"

#include <interfaces/IBluetooth.h>

#include "SDPServerImplementation.h"

namespace Thunder {

namespace Plugin {

    class BluetoothSDPServer : public PluginHost::IPlugin
                             , public PluginHost::IPlugin::INotification  {

    public:
        class Config : public Core::JSON::Container {
        public:
            class ServiceConfig : public Core::JSON::Container {
            public:
                enum serviceclass : uint8_t {
                    UNSPECIFIED,
                    A2DP_AUDIO_SINK,
                    A2DP_AUDIO_SOURCE,
                    AVRCP
                };

            public:
                ServiceConfig& operator=(const ServiceConfig&) = delete;
                ServiceConfig()
                    : Core::JSON::Container()
                    , Name()
                    , Description()
                    , Class()
                    , Parameters()
                {
                    Init();
                }
                ServiceConfig(const ServiceConfig& other)
                    : Core::JSON::Container()
                    , Name(other.Name)
                    , Description(other.Description)
                    , Callsign(other.Callsign)
                    , Class(other.Class)
                    , Parameters(other.Parameters)
                {
                    Init();
                }
                ~ServiceConfig() = default;

            private:
                void Init()
                {
                    Add(_T("name"), &Name);
                    Add(_T("description"), &Description);
                    Add(_T("callsign"), &Callsign);
                    Add("class", &Class);
                    Add("public", &Public);
                    Add("parameters", &Parameters);
                }

            public:
                Core::JSON::String Name;
                Core::JSON::String Description;
                Core::JSON::String Callsign;
                Core::JSON::EnumType<serviceclass> Class;
                Core::JSON::Boolean Public;
                Core::JSON::String Parameters;
            }; // class ServiceConfig

            struct A2DP {
                enum sourcetype : uint16_t {
                    PLAYER,
                    MICROPHONE,
                    TUNER,
                    MIXER
                };

                enum sinktype : uint16_t {
                    HEADPHONES,
                    SPEAKER,
                    RECORDER,
                    AMPLIFIER
                };

                template<typename TYPE>
                class Config : public Core::JSON::Container {
                public:
                    Config(const Config&) = delete;
                    Config& operator=(const Config&) = delete;

                    Config(const string& configuration)
                        : Core::JSON::Container()
                        , Features()
                    {
                        Add(_T("features"), &Features);
                        FromString(configuration);
                    }
                    ~Config() = default;

                public:
                    uint16_t GetFeatures() const
                    {
                        uint16_t features = 0;

                        auto entry = Features.Elements();

                        while (entry.Next() == true) {
                            features |= (1<<static_cast<uint16_t>(entry.Current().Value()));
                        }

                        return features;
                    }

                public:
                    Core::JSON::ArrayType<Core::JSON::EnumType<TYPE>> Features;
                }; // class Config
            }; //struct A2DP

        public:
            Config() = delete;
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config(const string& text)
                : Core::JSON::Container()
                , Controller(_T("BluetoothControl"))
                , Provider(_T("Thunder"))
                , Interface(0 /* HCI interface 0 */)
                , PSM(Bluetooth::SDP::PSM)
                , BrowseGroups()
            {
                Add(_T("controller"), &Controller);
                Add(_T("provider"), &Provider);
                Add(_T("interface"), &Interface);
                Add(_T("psm"), &PSM);
                Add(_T("inactivitytimeout"), &InactivityTimeoutMs);
                Add(_T("browsegroups"), &BrowseGroups);
                Add(_T("services"), &Services);
                FromString(text);
            }
            ~Config() = default;

        public:
            Core::JSON::String Controller;
            Core::JSON::String Provider;
            Core::JSON::DecUInt8 Interface;
            Core::JSON::DecUInt8 PSM;
            Core::JSON::DecUInt32 InactivityTimeoutMs;
            Core::JSON::ArrayType<Core::JSON::String> BrowseGroups;
            Core::JSON::ArrayType<ServiceConfig> Services;
        }; // class Config

    public:
        BluetoothSDPServer(const BluetoothSDPServer&) = delete;
        BluetoothSDPServer& operator=(const BluetoothSDPServer&) = delete;
        BluetoothSDPServer()
            : _lock()
            , _service(nullptr)
            , _controller()
            , _server()
            , _services()
        {
        }
        ~BluetoothSDPServer() = default;

    public:
        // IPlugin overrides
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override { return {}; }

    private:
        // INotification overrides
        void Activated(const string& callsign, PluginHost::IShell*) override;
        void Deactivated(const string& callsign, PluginHost::IShell*) override;
        void Unavailable(const string&, PluginHost::IShell*) override { }

    public:
        Exchange::IBluetooth* Controller() const
        {
            ASSERT(_service != nullptr);
            return (_service->QueryInterfaceByCallsign<Exchange::IBluetooth>(_controller));
        }

    public:
        BEGIN_INTERFACE_MAP(BluetoothSDPServer)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        END_INTERFACE_MAP

    private:
        void Configure(const Config& configuration);
        Bluetooth::SDP::Service* ConfigureA2DPService(const bool sink, const string& config = {});

    private:
        mutable Core::CriticalSection _lock;
        PluginHost::IShell* _service;
        string _controller;
        SDP::ServiceDiscoveryServer _server;
        std::list<std::pair<Bluetooth::SDP::Service*, string>> _services;
    }; // class BluetoothSDPServer

} // namespace Plugin

}
