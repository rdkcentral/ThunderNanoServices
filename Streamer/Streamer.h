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
#include "Geometry.h"
#include <interfaces/json/JsonData_Streamer.h>

namespace WPEFramework {
namespace Plugin {

    class Streamer : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    private:
        Streamer(const Streamer&) = delete;
        Streamer& operator=(const Streamer&) = delete;

        class Notification : public RPC::IRemoteConnection::INotification {
        private:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            explicit Notification(Streamer* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            virtual ~Notification()
            {
            }

        public:
            virtual void Activated(RPC::IRemoteConnection* connection)
            {
            }
            virtual void Deactivated(RPC::IRemoteConnection* connection)
            {
                _parent.Deactivated(connection);
            }

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            Streamer& _parent;
        };

        class StreamProxy {
        private:
            class StreamSink : public Exchange::IStream::ICallback {
            private:
                StreamSink() = delete;
                StreamSink(const StreamSink&) = delete;
                StreamSink& operator=(const StreamSink&) = delete;

            public:
                StreamSink(StreamProxy* parent)
                    : _parent(*parent)
                {
                    ASSERT(parent != nullptr);
                }
                ~StreamSink() override
                {
                }

            public:
                void StateChange(const Exchange::IStream::state state) override
                {
                    _parent.StateChange(state);
                }
                void Event(const uint32_t eventId) override
                {
                    _parent.StreamEvent(eventId);
                }
                void DRM(const uint32_t state) override
                {
                    _parent.DrmEvent(state);
                }

                BEGIN_INTERFACE_MAP(StreamSink)
                INTERFACE_ENTRY(Exchange::IStream::ICallback)
                END_INTERFACE_MAP

            private:
                StreamProxy& _parent;
            };

        public:
            StreamProxy() = delete;
            StreamProxy(const StreamProxy&) = delete;
            StreamProxy& operator= (const StreamProxy&) = delete;

            StreamProxy(Streamer& parent, const uint8_t index, Exchange::IStream* implementation)
                : _parent(parent)
                , _index(index)
                , _implementation(implementation)
                , _streamSink(this)
            {
                ASSERT (_implementation != nullptr);
                _implementation->Callback(&_streamSink);
            }
            ~StreamProxy()
            {
            }
            Exchange::IStream* operator->() {
                return (_implementation);
            }
            const Exchange::IStream* operator->() const {
                return (_implementation);
            }

        private:
            void StateChange(Exchange::IStream::state state)
            {
                _parent.StateChange(_index, state);
            }
            void StreamEvent(const uint32_t eventId)
            {
                _parent.StreamEvent(_index, eventId);
            }
            void DrmEvent(uint32_t state)
            {
                _parent.DrmEvent(_index, state);
            }

        private:
            Streamer& _parent;
            uint8_t _index;
            Exchange::IStream* _implementation;
            Core::Sink<StreamSink> _streamSink;
        };

        class ControlProxy {
        private:
            class ControlSink : public Exchange::IStream::IControl::ICallback {
            private:
                ControlSink() = delete;
                ControlSink(const ControlSink&) = delete;
                ControlSink& operator=(const ControlSink&) = delete;

            public:
                ControlSink(ControlProxy* parent)
                    : _parent(*parent)
                {
                    ASSERT(parent != nullptr);
                }
                ~ControlSink() override
                {
                }

            public:
                void TimeUpdate(const uint64_t position) override
                {
                    _parent.TimeUpdate(position);
                }
                void Event(const uint32_t eventId) override
                {
                    _parent.ControlEvent(eventId);
                }

                BEGIN_INTERFACE_MAP(ControlSink)
                INTERFACE_ENTRY(Exchange::IStream::IControl::ICallback)
                END_INTERFACE_MAP

            private:
                ControlProxy& _parent;
            };
        public:
            ControlProxy() = delete;
            ControlProxy(const ControlProxy&) = delete;
            ControlProxy& operator= (const ControlProxy&) = delete;

            ControlProxy(Streamer& parent, const uint8_t index, Exchange::IStream::IControl* implementation)
                : _parent(parent)
                , _index(index)
                , _implementation(implementation)
                , _controlSink(this) {
                ASSERT (_implementation != nullptr);
                _implementation->Callback(&_controlSink);
            }
            ~ControlProxy() {
            }

            Exchange::IStream::IControl* operator->() {
                return (_implementation);
            }
            const Exchange::IStream::IControl* operator->() const {
                return (_implementation);
            }

        private:
            void TimeUpdate(const uint64_t position)
            {
                _parent.TimeUpdate(_index, position);
            }
            void ControlEvent(const uint32_t eventId)
            {
                _parent.PlayerEvent(_index, eventId);
            }

         private:
            Streamer& _parent;
            uint8_t _index;
            Exchange::IStream::IControl* _implementation;
            Core::Sink<ControlSink> _controlSink;
        };

        typedef std::map<uint8_t, StreamProxy> Streams;
        typedef std::map<uint8_t, ControlProxy> Controls;

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , OutOfProcess(true)
            {
                Add(_T("outofprocess"), &OutOfProcess);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::Boolean OutOfProcess;
        };

    public:
        class Data : public Core::JSON::Container {
        private:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;

        public:
            Data()
                : Core::JSON::Container()
                , X()
                , Y()
                , Z()
                , Width()
                , Height()
                , Speed()
                , Position()
                , Type()
                , DRM()
                , State()
                , Url()
                , Begin()
                , End()
                , AbsoluteTime()
                , Id(~0)
                , Metadata(false)
                , Ids()
            {
                Add(_T("url"), &Url);
                Add(_T("x"), &X);
                Add(_T("y"), &Y);
                Add(_T("z"), &Z);
                Add(_T("width"), &Width);
                Add(_T("height"), &Height);
                Add(_T("speed"), &Speed);
                Add(_T("position"), &Position);
                Add(_T("type"), &Type);
                Add(_T("drm"), &DRM);
                Add(_T("state"), &State);
                Add(_T("begin"), &Begin);
                Add(_T("end"), &End);
                Add(_T("absolutetime"), &AbsoluteTime);
                Add(_T("id"), &Id);
                Add(_T("metadata"), &Metadata);
                Add(_T("ids"), &Ids);
            }
            ~Data()
            {
            }

        public:
            Core::JSON::DecUInt32 X;
            Core::JSON::DecUInt32 Y;
            Core::JSON::DecUInt32 Z;
            Core::JSON::DecUInt32 Width;
            Core::JSON::DecUInt32 Height;

            Core::JSON::DecSInt32 Speed;
            Core::JSON::DecUInt64 Position;
            Core::JSON::DecUInt32 Type;
            Core::JSON::DecUInt32 DRM;
            Core::JSON::DecUInt32 State;

            Core::JSON::String Url;
            Core::JSON::DecUInt64 Begin;
            Core::JSON::DecUInt64 End;
            Core::JSON::DecUInt64 AbsoluteTime;
            Core::JSON::DecUInt8 Id;

            Core::JSON::String Metadata;

            Core::JSON::ArrayType<Core::JSON::DecUInt8> Ids;
        };

    public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
        Streamer()
            : _skipURL(0)
            , _connectionId(0)
            , _service(nullptr)
            , _player(nullptr)
            , _notification(this)
            , _streams()
            , _controls()
        {
            RegisterAll();
        }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
        ~Streamer() override
        {
            UnregisterAll();
        }

    public:
        BEGIN_INTERFACE_MAP(Streamer)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::IPlayer, _player)
        END_INTERFACE_MAP

    public:
        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        const string Initialize(PluginHost::IShell* service) override;

        // The plugin is unloaded from WPEFramework. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        void Deinitialize(PluginHost::IShell* service) override;

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        string Information() const override;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request);
        PluginHost::IShell* GetService() { return _service; }

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index);
        void Deactivated(RPC::IRemoteConnection* connection);

        void StateChange(const uint8_t index, Exchange::IStream::state state)
        {
            TRACE(Trace::Information, (_T("Stream [%d] moved state: [%s]"), index, Core::EnumerateType<Exchange::IStream::state>(state).Data()));

            _service->Notify(_T("{ \"id\": ") +
                             Core::NumberType<uint8_t>(index).Text() +
                             _T(", \"stream\": \"") +
                             Core::EnumerateType<Exchange::IStream::state>(state).Data() +
                             _T("\" }"));
            event_statechange(std::to_string(index), static_cast<JsonData::Streamer::StateType>(state));
        }
        void TimeUpdate(const uint8_t index, const uint64_t position)
        {
            _service->Notify(_T("{ \"id\": ") +
                             Core::NumberType<uint8_t>(index).Text() +
                            _T(", \"time\": ") +
                            Core::NumberType<uint64_t>(position).Text() +
                            _T(" }"));
            event_timeupdate(std::to_string(index), position);
        }
        void StreamEvent(const uint8_t index, const uint32_t eventId)
        {
            TRACE(Trace::Information, (_T("Stream [%d] custom notification: [%08x]"), index, eventId));

            _service->Notify(_T("{ \"id\": ") +
                             Core::NumberType<uint8_t>(index).Text() +
                            _T(", \"stream_event\": \"") +
                            Core::NumberType<uint32_t>(eventId).Text() +
                            _T("\" }"));
            event_stream(std::to_string(index), eventId);
        }
        void PlayerEvent(const uint8_t index, const uint32_t eventId)
        {
            TRACE(Trace::Information, (_T("Stream [%d] custom player notification: [%08x]"), index, eventId));

            _service->Notify(_T("{ \"id\": ") +
                             Core::NumberType<uint8_t>(index).Text() +
                             _T(", \"player_event\": \"") +
                             Core::NumberType<uint32_t>(eventId).Text() +
                             _T("\" }"));
            event_player(std::to_string(index), eventId);
        }
        void DrmEvent(const uint8_t index, uint32_t state)
        {
            _service->Notify(_T("{ \"id\": ") + Core::NumberType<uint8_t>(index).Text() + _T(", \"drm\": \"") + Core::NumberType<uint8_t>(state).Text() + _T("\" }"));
            event_drm(std::to_string(index), state);
        }

        // JsonRpc
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_create(const JsonData::Streamer::CreateParamsData& params, Core::JSON::DecUInt8& response);
        uint32_t endpoint_destroy(const JsonData::Streamer::IdInfo& params);
        uint32_t endpoint_load(const JsonData::Streamer::LoadParamsData& params);
        uint32_t endpoint_attach(const JsonData::Streamer::IdInfo& params);
        uint32_t endpoint_detach(const JsonData::Streamer::IdInfo& params);
        uint32_t get_speed(const string& index, Core::JSON::DecSInt32& response) const;
        uint32_t set_speed(const string& index, const Core::JSON::DecSInt32& param);
        uint32_t get_position(const string& index, Core::JSON::DecUInt64& response) const;
        uint32_t set_position(const string& index, const Core::JSON::DecUInt64& param);
        uint32_t get_window(const string& index, JsonData::Streamer::WindowData& response) const;
        uint32_t set_window(const string& index, const JsonData::Streamer::WindowData& param);
        uint32_t get_speeds(const string& index, Core::JSON::ArrayType<Core::JSON::DecSInt32>& response) const;
        uint32_t get_streams(Core::JSON::ArrayType<Core::JSON::DecUInt32>& response) const;
        uint32_t get_type(const string& index, Core::JSON::EnumType<JsonData::Streamer::StreamType>& response) const;
        uint32_t get_drm(const string& index, Core::JSON::EnumType<JsonData::Streamer::DrmType>& response) const;
        uint32_t get_state(const string& index, Core::JSON::EnumType<JsonData::Streamer::StateType>& response) const;
        uint32_t get_metadata(const string& index, Core::JSON::String& response) const;
        uint32_t get_error(const string& index, Core::JSON::DecUInt32& response) const;
        uint32_t get_elements(const string& index, Core::JSON::ArrayType<JsonData::Streamer::StreamelementData>& response) const;
        void event_statechange(const string& id, const JsonData::Streamer::StateType& state);
        void event_timeupdate(const string& id, const uint64_t& time);
        void event_stream(const string& id, const uint32_t& code);
        void event_player(const string& id, const uint32_t& code);
        void event_drm(const string& id, const uint32_t& code);

    private:
        uint32_t _skipURL;
        uint32_t _connectionId;
        PluginHost::IShell* _service;

        Exchange::IPlayer* _player;
        Core::Sink<Notification> _notification;

        // Stream and StreamControl holding areas for the RESTFull API.
        Streams _streams;
        Controls _controls;
    };
} //namespace Plugin
} //namespace WPEFramework
