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

#pragma once

#include "Module.h"
#include "RemoteAdministrator.h"
#include <interfaces/json/JsonData_RemoteControl.h>
#include <interfaces/IKeyHandler.h>
#include <interfaces/IRemoteControl.h>

namespace WPEFramework {
namespace Plugin {

    class RemoteControl : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC,
                          public Exchange::IKeyHandler,  public Exchange::IWheelHandler,
                          public Exchange::IPointerHandler,  public Exchange::ITouchHandler,
                          public Exchange::IRemoteControl {
    private:
        class Feedback : public PluginHost::VirtualInput::INotifier {
        public:
            Feedback() = delete;
            Feedback(const Feedback&) = delete;
            Feedback& operator= (const Feedback&) = delete;

            Feedback(RemoteControl& parent) 
                : _parent(parent) 
            {
            }
            ~Feedback() override 
            {
            } 

        public:
            void Dispatch(const IVirtualInput::KeyData::type type, const uint32_t code)  override {
                _parent.Activity(type, code);
            }

        private:
            RemoteControl& _parent;
        };

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            class Device : public Core::JSON::Container {
            private:
                Device& operator=(const Device&) = delete;

            public:
                Device()
                    : Core::JSON::Container()
                    , Name()
                    , MapFile()
                    , PassOn(false)
                    , Settings()
                {
                    Add(_T("name"), &Name);
                    Add(_T("mapfile"), &MapFile);
                    Add(_T("passon"), &PassOn);
                    Add(_T("settings"), &Settings);
                }
                Device(const Device& copy)
                    : Core::JSON::Container()
                    , Name(copy.Name)
                    , MapFile(copy.MapFile)
                    , PassOn(copy.PassOn)
                    , Settings(copy.Settings)
                {
                    Add(_T("name"), &Name);
                    Add(_T("mapfile"), &MapFile);
                    Add(_T("passon"), &PassOn);
                    Add(_T("settings"), &Settings);
                }
                ~Device()
                {
                }

                Core::JSON::String Name;
                Core::JSON::String MapFile;
                Core::JSON::Boolean PassOn;
                Core::JSON::String Settings;
            };
            class Link : public Core::JSON::Container {
            private:
                Link& operator=(const Link&) = delete;

            public:
                Link()
                    : Core::JSON::Container()
                    , Name()
                    , MapFile()
                {
                    Add(_T("name"), &Name);
                    Add(_T("mapfile"), &MapFile);
                }
                Link(const Link&)
                {
                    Add(_T("name"), &Name);
                    Add(_T("mapfile"), &MapFile);
                }
                ~Link()
                {
                }

                Core::JSON::String Name;
                Core::JSON::String MapFile;
            };

        public:
            Config()
                : Core::JSON::Container()
                , MapFile("keymap.json")
                , PostLookupFile()
                , PassOn(false)
                , RepeatStart(500)
                , RepeatInterval(100)
                , ReleaseTimeout(30000)
                , Devices()
                , Virtuals()
                , Links()
            {
                Add(_T("mapfile"), &MapFile);
                Add(_T("postlookupfile"), &PostLookupFile);
                Add(_T("passon"), &PassOn);
                Add(_T("repeatstart"), &RepeatStart);
                Add(_T("repeatinterval"), &RepeatInterval);
                Add(_T("releasetimeout"), &ReleaseTimeout);
                Add(_T("devices"), &Devices);
                Add(_T("virtuals"), &Virtuals);
                Add(_T("links"), &Links);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String MapFile;
            Core::JSON::String PostLookupFile;
            Core::JSON::Boolean PassOn;
            Core::JSON::DecUInt16 RepeatStart;
            Core::JSON::DecUInt16 RepeatInterval;
            Core::JSON::DecUInt16 ReleaseTimeout;
            Core::JSON::ArrayType<Device> Devices;
            Core::JSON::ArrayType<Device> Virtuals;
            Core::JSON::ArrayType<Link> Links;
        };

        class Data : public Core::JSON::Container {

        private:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;

        public:
            Data()
                : Core::JSON::Container()
                , Devices()
            {
                Add(_T("devices"), &Devices);
            }

            virtual ~Data()
            {
            }

        public:
            Core::JSON::ArrayType<Core::JSON::String> Devices;
        };

    public:
        RemoteControl(const RemoteControl&) = delete;
        RemoteControl& operator=(const RemoteControl&) = delete;

        RemoteControl();
        virtual ~RemoteControl();

        BEGIN_INTERFACE_MAP(RemoteControl)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(Exchange::IKeyHandler)
        INTERFACE_ENTRY(Exchange::IWheelHandler)
        INTERFACE_ENTRY(Exchange::IPointerHandler)
        INTERFACE_ENTRY(Exchange::ITouchHandler)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_ENTRY(Exchange::IRemoteControl)
        END_INTERFACE_MAP

    public:
        bool IsVirtualDevice(const string& name) const
        {
            return (std::find(_virtualDevices.begin(), _virtualDevices.end(), name) != _virtualDevices.end());
        }
        bool IsPhysicalDevice(const string& name) const
        {
            Remotes::RemoteAdministrator::Iterator index(Remotes::RemoteAdministrator::Instance().Producers());

            while ((index.Next() == true) && (name != index.Current()->Name())) /* Intentionally empty */
                ;

            return (index.IsValid());
        }

        //	IPlugin methods
        // -------------------------------------------------------------------------------------------------------

        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        virtual const string Initialize(PluginHost::IShell* service) override;

        // The plugin is unloaded from WPEFramework. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service) override;

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        virtual string Information() const override;

        //      IWeb methods
        // -------------------------------------------------------------------------------------------------------
        // Whenever a request is received, it might carry some additional data in the body. This method allows
        // the plugin to attach a deserializable data object (ref counted) to be loaded with any potential found
        // in the body of the request.
        virtual void Inbound(Web::Request& request) override;

        // If everything is received correctly, the request is passed on to us, through a thread from the thread pool, to
        // do our thing and to return the result in the response object. Here the actual specific module work,
        // based on a a request is handled.
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        //      IKeyHandler methods
        // -------------------------------------------------------------------------------------------------------
        // Whenever a key is pressed or release, let this plugin now, it will take the proper arrangements and timings
        // to announce this key event to the linux system. Repeat event is triggered by the watchdog implementation
        // in this plugin. No need to signal this.
        uint32_t KeyEvent(const bool pressed, const uint32_t code, const string& mapName) override;
        // Anounce events from keyproducers to registered clients.
        virtual void ProducerEvent(const string& producerName, const Exchange::ProducerEvents event) override;

        uint32_t AxisEvent(const int16_t x, const int16_t y) override;
        uint32_t PointerButtonEvent(const bool pressed, const uint8_t button) override;
        uint32_t PointerMotionEvent(const int16_t x, const int16_t y) override;
        uint32_t TouchEvent(const uint8_t index, const ITouchHandler::touchstate state, const uint16_t x, const uint16_t y) override;

        // Next to handling keys, we also have a number of devices can produce keys. All these key producers have a name.
        // Using the next interface it is possible to retrieve the KeyProducers implemented by ths plugin.
        virtual Exchange::IKeyProducer* Producer(const string& name) override
        {
            Exchange::IKeyProducer* result = nullptr;

            Remotes::RemoteAdministrator::Iterator index(Remotes::RemoteAdministrator::Instance().Producers());

            while ((index.Next() == true) && (name != index.Current()->Name())) /* Intentionally empty */
                ;

            if (index.IsValid() == true) {

                result = index.Current();

                ASSERT(result != nullptr);

                result->AddRef();
            }

            return (result);
        }

        virtual Exchange::IWheelProducer* WheelProducer(const string& name) override
        {
            Exchange::IWheelProducer* result = nullptr;
            auto index(Remotes::RemoteAdministrator::Instance().WheelProducers());

            while ((index.Next() == true) && (name != index.Current()->Name())) /* Intentionally empty */
                ;

            if (index.IsValid() == true) {
                result = index.Current();
                ASSERT(result != nullptr);
                result->AddRef();
            }

            return (result);
        }

        virtual Exchange::IPointerProducer* PointerProducer(const string& name) override
        {
            Exchange::IPointerProducer* result = nullptr;
            auto index(Remotes::RemoteAdministrator::Instance().PointerProducers());

            while ((index.Next() == true) && (name != index.Current()->Name())) /* Intentionally empty */
                ;

            if (index.IsValid() == true) {
                result = index.Current();
                ASSERT(result != nullptr);
                result->AddRef();
            }

            return (result);
        }

        virtual Exchange::ITouchProducer* TouchProducer(const string& name) override
        {
            Exchange::ITouchProducer* result = nullptr;
            auto index(Remotes::RemoteAdministrator::Instance().TouchProducers());

            while ((index.Next() == true) && (name != index.Current()->Name())) /* Intentionally empty */
                ;

            if (index.IsValid() == true) {
                result = index.Current();
                ASSERT(result != nullptr);
                result->AddRef();
            }

            return (result);
        }

        //      IRemoteControl Methods
        // -------------------------------------------------------------------------------------------------------
        // Register for events from RemoteControl plugin originating from key producers.
        virtual void RegisterEvents(IRemoteControl::INotification* sink) override;
        // Unregister for events from RemoteControl originating from key producers.
        virtual void UnregisterEvents(IRemoteControl::INotification* sink) override;

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index, const Web::Request& request) const;
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        const string FindDevice(Core::TextSegmentIterator& index) const;
        bool ParseRequestBody(const Web::Request& request, uint32_t& code, uint16_t& key, uint32_t& modifiers);
        Core::ProxyType<Web::IBody> CreateResponseBody(uint32_t code, uint32_t key, uint16_t modifiers) const;
        void Activity(const IVirtualInput::KeyData::type type, const uint32_t code);

        void RegisterAll();
        void UnregisterAll();
        Core::JSON::ArrayType<Core::JSON::EnumType<JsonData::RemoteControl::ModifiersType>> Modifiers(uint16_t modifiers) const;
        uint16_t Modifiers(const Core::JSON::ArrayType<Core::JSON::EnumType<JsonData::RemoteControl::ModifiersType>>& param) const;
        uint32_t endpoint_key(const JsonData::RemoteControl::KeyobjInfo& params, JsonData::RemoteControl::KeyResultData& response);
        uint32_t endpoint_send(const JsonData::RemoteControl::KeyobjInfo& params);
        uint32_t endpoint_press(const JsonData::RemoteControl::KeyobjInfo& params);
        uint32_t endpoint_release(const JsonData::RemoteControl::KeyobjInfo& params);
        uint32_t endpoint_add(const JsonData::RemoteControl::RcobjInfo& params);
        uint32_t endpoint_modify(const JsonData::RemoteControl::RcobjInfo& params);
        uint32_t endpoint_delete(const JsonData::RemoteControl::KeyobjInfo& params);
        uint32_t endpoint_load(const JsonData::RemoteControl::LoadParamsInfo& params);
        uint32_t endpoint_save(const JsonData::RemoteControl::LoadParamsInfo& params);
        uint32_t endpoint_pair(const JsonData::RemoteControl::LoadParamsInfo& params);
        uint32_t endpoint_unpair(const JsonData::RemoteControl::UnpairParamsData& params);
        uint32_t get_devices(Core::JSON::ArrayType<Core::JSON::String>& response) const;
        uint32_t get_device(const string& index, JsonData::RemoteControl::DeviceData& response) const;
        void event_keypressed(const string& id, const bool& pressed);

    private:
        uint32_t _skipURL;
        std::list<string> _virtualDevices;
        PluginHost::VirtualInput* _inputHandler;
        string _persistentPath;
        Feedback _feedback;
        Core::CriticalSection _eventLock;
        std::list<Exchange::IRemoteControl::INotification*> _notificationClients;
    };
}
}
