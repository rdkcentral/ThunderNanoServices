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
 
#ifndef __PLUGIN_COMPOSITOR_H
#define __PLUGIN_COMPOSITOR_H

#include "Module.h"
#include <interfaces/IComposition.h>
#include <interfaces/IInputSwitch.h>
#include <interfaces/json/JsonData_Compositor.h>


namespace Thunder {
namespace Plugin {
    class Compositor : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    private:
        class Notification : public Exchange::IComposition::INotification, public RPC::IRemoteConnection::INotification {
        public:
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

            Notification(Compositor* parent)
                : _parent(*parent)
            {
            }
            ~Notification() override = default;

            //   IComposition INotification methods
            // -------------------------------------------------------------------------------------------------------
            void Attached(const string& name, Exchange::IComposition::IClient* client) override
            {
                _parent.Attached(name, client);
            }
            void Detached(const string& name) override
            {
                _parent.Detached(name);
            }

            //RPC::IRemoteConnection::INotification methods
            void Activated(RPC::IRemoteConnection* /* connection */ ) override {
            }

            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }
            void Terminated(RPC::IRemoteConnection* /* connection */ ) override {
            }

            BEGIN_INTERFACE_MAP(PluginSink)
            INTERFACE_ENTRY(Exchange::IComposition::INotification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            Compositor& _parent;
        };

    public:
        typedef std::map<string, Exchange::IComposition::IClient*> Clients;

        class Config : public Core::JSON::Container {
        public:
            Config(Config&&);
            Config(const Config&);
            Config& operator=(Config&&);
            Config& operator=(const Config&);

            Config()
                : Core::JSON::Container()
                , System(_T("Controller"))
                , WorkDir()
                , InputSwitch(_T("InputSwitch"))
                , Connector("connector")
                , BufferConnector(_T("bufferconnector"))
                , DisplayConnector("displayconnector")
                , NewOnTop(true)
            {
                Add(_T("system"), &System);
                Add(_T("workdir"), &WorkDir);
                Add(_T("inputswitch"), &InputSwitch);
                Add(_T("connector"), &Connector);
                Add(_T("bufferconnector"), &BufferConnector);
                Add(_T("displayconnector"), &DisplayConnector);
                Add(_T("newontop"), &NewOnTop);
            }
            ~Config() override = default;

        public:
            Core::JSON::String System;
            Core::JSON::String WorkDir;
            Core::JSON::String InputSwitch;
            Core::JSON::String Connector;
            Core::JSON::String BufferConnector;
            Core::JSON::String DisplayConnector;
            Core::JSON::Boolean NewOnTop;
        };

        class Data : public Core::JSON::Container {
        public:
            Data(Data&&) = delete;
            Data(const Data&) = delete;
            Data& operator=(Data&&) = delete;
            Data& operator=(const Data&) = delete;

            Data()
                : Core::JSON::Container()
                , Clients()
            {
                Add(_T("clients"), &Clients);
                Add(_T("resolution"), &Resolution);
                Add(_T("x"), &X);
                Add(_T("y"), &Y);
                Add(_T("width"), &Width);
                Add(_T("height"), &Height);
            }

            ~Data() override = default;

        public:
            Core::JSON::ArrayType<Core::JSON::String> Clients;
            Core::JSON::EnumType<Exchange::IComposition::ScreenResolution> Resolution;
            Core::JSON::DecUInt32 X;
            Core::JSON::DecUInt32 Y;
            Core::JSON::DecUInt32 Width;
            Core::JSON::DecUInt32 Height;
        };

    public:
        Compositor(Compositor&&) = delete;
        Compositor(const Compositor&) = delete;
        Compositor& operator=(Compositor&&) = delete;
        Compositor& operator=(const Compositor&) = delete;
        
        Compositor();
        ~Compositor() override = default;

    public:
        BEGIN_INTERFACE_MAP(Compositor)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::IComposition, _composition)
        INTERFACE_AGGREGATE(Exchange::IBrightness, _brightness)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //	IWeb methods
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        void Attached(const string& name, Exchange::IComposition::IClient* client);
        void Detached(const string& name);
        void Deactivated(RPC::IRemoteConnection* connection);

        uint32_t Resolution(const Exchange::IComposition::ScreenResolution);
        Exchange::IComposition::ScreenResolution Resolution() const;

        uint32_t Opacity(const string& callsign, const uint32_t value);
        uint32_t Visible(const string& callsign, const bool visible);
        uint32_t Geometry(const string& callsign, const Exchange::IComposition::Rectangle& rectangle);
        Exchange::IComposition::Rectangle Geometry(const string& callsign) const;
        uint32_t ToTop(const string& callsign, Exchange::IComposition::IClient* client);
        uint32_t ToTop(const string& callsign);
        uint32_t Select(const string& callsign);
        uint32_t PutBefore(const string& relative, const string& callsign);
        uint32_t GetBrightness(Exchange::IBrightness::Brightness& brightness) const;
        uint32_t SetBrightness(const Exchange::IBrightness::Brightness& brightness);

        void ZOrder(std::list<string>& zOrderedList, const bool primary) const;
        Exchange::IComposition::IClient* InterfaceByCallsign(const string& callsign) const;

        void ZOrder(Core::JSON::ArrayType<Core::JSON::String>& zOrderedList) const {
            std::list<string> list;
            std::list<string>::const_iterator index;
            ZOrder(list, true);
            index = list.begin();
            while (index != list.end()) {
                Core::JSON::String& element(zOrderedList.Add());
                element = *index;
                index++;
            }
        }

        /* JSON-RPC */
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_putontop(const JsonData::Compositor::PutontopParamsInfo& params);
        uint32_t endpoint_select(const JsonData::Compositor::PutontopParamsInfo& params);
        uint32_t endpoint_putbelow(const JsonData::Compositor::PutbelowParamsData& params);
        uint32_t get_resolution(Core::JSON::EnumType<JsonData::Compositor::ResolutionType>& response) const;
        uint32_t set_resolution(const Core::JSON::EnumType<JsonData::Compositor::ResolutionType>& param);
        uint32_t get_zorder(Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_geometry(const string& index, JsonData::Compositor::GeometryData& response) const;
        uint32_t set_geometry(const string& index, const JsonData::Compositor::GeometryData& param);
        uint32_t set_visiblity(const string& index, const Core::JSON::EnumType<JsonData::Compositor::VisiblityType>& param);
        uint32_t set_opacity(const string& index, const Core::JSON::DecUInt8& param);
        uint32_t get_brightness(Core::JSON::EnumType<JsonData::Compositor::BrightnessType>& response) const;
        uint32_t set_brightness(const Core::JSON::EnumType<JsonData::Compositor::BrightnessType>& param);

    private:
        mutable Core::CriticalSection _adminLock;
        uint8_t _skipURL;
        Core::SinkType<Notification> _notification;
        Exchange::IComposition* _composition;
        Exchange::IBrightness* _brightness;
        PluginHost::IShell* _service;
        uint32_t _connectionId;
        bool _newOnTop;
        Clients _clients;
        Exchange::IInputSwitch* _inputSwitch;
        string _inputSwitchCallsign;
    };
}
}

#endif // __PLUGIN_COMPOSITOR_H
