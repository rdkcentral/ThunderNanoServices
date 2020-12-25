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

#include "Data.h"
#include "Module.h"

#include <websocket/websocket.h>
#include <interfaces/IPerformance.h>

// The next header file is the source of the interface. 
// From this source file, the ProxyStub (Marshalling code for the COMRPC) s created.
// This code is loaded and handled by the Thunder framework. No need to do anything
// within the plugin to make it available. 
#include <interfaces/IMath.h>

// note JMath tests disabled until this interface is generated from the IMath.h interface
#if 0
// From the source, included above (IMath.h) a precompiler will create a method that 
// can connect a JSONRPC class to the interface implementation. This is typically done 
// in the constructor of the plugin!
// The method to connects the JSORPC implementation to the Interface implementation is 
// generated and it is named as:
// <interface name>::Register(PluginHost::JSONRPC& handler, <interface name>* implementation);
// <interface name>::Unregister(PluginHost::JSONRPC& handler);
#include <interfaces/json/JMath.h>
#endif

namespace WPEFramework {

namespace Plugin {

    // This is a server for a JSONRPC communication channel.
    // For a plugin to be capable to handle JSONRPC, inherit from PluginHost::JSONRPC.
    // By inheriting from this class, the plugin realizes the interface PluginHost::IDispatcher.
    // This realization of this interface implements, by default, the following methods on this plugin
    // - exists
    // - register
    // - unregister
    // Any other methood to be handled by this plugin  can be added can be added by using the
    // templated methods Rgister on the PluginHost::JSONRPC class.
    // As the registration/unregistration of notifications is realized by the class PluginHost::JSONRPC,
    // this class exposes a public method called, Notify(), using this methods, all subscribed clients
    // will receive a JSONRPC message as a notification, in case this method is called.
    class JSONRPCPlugin : public PluginHost::IPlugin, public PluginHost::JSONRPC, public Exchange::IPerformance, public Exchange::IMath {
    private:
        // We do not allow this plugin to be copied !!
        JSONRPCPlugin(const JSONRPCPlugin&) = delete;
        JSONRPCPlugin& operator=(const JSONRPCPlugin&) = delete;

        // The next class describes configuration information for this plugin.
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Connector("0.0.0.0:8899")
            {
                Add(_T("connector"), &Connector);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Connector;
        };

        // For measuring differnt performance times, also expose a COMRPC server. The transport protocol is binairy and the invocation of the
        // calls requires less thread switches.
        // The following class exposes a server that exposes the Exchange::IRPCLink interface to be retrieved from another process. The client
        // process can than measure the overhead of the call towards the Exchange::IRPCLink methods.
        // The processing time of these methods will also be acounted for internally and "send" back as well
        class COMServer : public RPC::Communicator {
        public:
            COMServer() = delete;
            COMServer(const COMServer&) = delete;
            COMServer& operator=(const COMServer&) = delete;

        public:
            COMServer(
                const Core::NodeId& source, 
                Exchange::IPerformance* parentInterface, 
                const string& proxyStubPath, 
                const Core::ProxyType<RPC::InvokeServer>& engine)
                : RPC::Communicator(source, proxyStubPath, Core::ProxyType<Core::IIPCServer>(engine))
                , _parentInterface(parentInterface)
            {
                engine->Announcements(RPC::Communicator::Announcement());
                Open(Core::infinite);
            }
            ~COMServer()
            {
                Close(Core::infinite);
            }

        private:
            virtual void* Aquire(const string& className, const uint32_t interfaceId, const uint32_t versionId)
            {
                void* result = nullptr;

                // Currently we only support version 1 of the IRPCLink :-)
                if ((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) {
                    // Reference count our parent
                    result = _parentInterface->QueryInterface(interfaceId);

                    printf("Pointer => %p\n", result);
                }
                return (result);
            }

        private:
            Exchange::IPerformance* _parentInterface;
        };

        template <typename INTERFACE>
        class JSONObjectFactory : public Core::FactoryType<INTERFACE, char*> {
        public:
            inline JSONObjectFactory()
                : Core::FactoryType<INTERFACE, char*>()
                , _jsonRPCFactory(5)
            {
            }
            JSONObjectFactory(const JSONObjectFactory&);
            JSONObjectFactory& operator=(const JSONObjectFactory&);

            virtual ~JSONObjectFactory()
            {
            }
       public:
            static JSONObjectFactory& Instance()
            {
                static JSONObjectFactory<INTERFACE> _singleton;

                return (_singleton);
            }

            Core::ProxyType<INTERFACE> Element(const string& identifier)
            {
                Core::ProxyType<Web::JSONBodyType<Core::JSONRPC::Message>> message = _jsonRPCFactory.Element();
                return Core::ProxyType<INTERFACE>(message);
            }
        private:
            Core::ProxyPoolType<Web::JSONBodyType<Core::JSONRPC::Message>> _jsonRPCFactory;
        };

        template <typename INTERFACE>
        class JSONRPCChannel;

        template <typename INTERFACE>
        class JSONRPCServer : public Core::StreamJSONType<Web::WebSocketServerType<Core::SocketStream>, JSONObjectFactory<INTERFACE>&, INTERFACE> {
        public:
            JSONRPCServer() = delete;
            JSONRPCServer(const JSONRPCServer& copy) = delete;
            JSONRPCServer& operator=(const JSONRPCServer&) = delete;
        private:
            typedef Core::StreamJSONType<Web::WebSocketServerType<Core::SocketStream>, JSONObjectFactory<INTERFACE>&, INTERFACE> BaseClass;

        public:
            JSONRPCServer(const SOCKET& connector, const Core::NodeId& remoteNode, Core::SocketServerType<JSONRPCServer>* parent)
                : BaseClass(5, JSONObjectFactory<INTERFACE>::Instance(), false, false, false, connector, remoteNode.AnyInterface(), 8096, 8096)
                , _id(0)
                , _parent(static_cast<JSONRPCChannel<INTERFACE>&>(*parent))
            {
            }
            virtual ~JSONRPCServer()
            {
            }

        public:
            virtual void Received(Core::ProxyType<INTERFACE>& jsonObject)
            {
                if (jsonObject.IsValid() == false) {
                    printf("Oops");
                }
                else {
                    Core::ProxyType<Core::JSONRPC::Message> message = Core::ProxyType<Core::JSONRPC::Message>(jsonObject);
                    ProcessMessage(message);
                    // As this is the server, send back the Element we received...
                    this->Submit(jsonObject);
                }
            }
            virtual void Send(Core::ProxyType<INTERFACE>& jsonObject)
            {
                if (jsonObject.IsValid() == false) {
                    printf("Oops");
                } else {
                    ToMessage(jsonObject);
                }
            }
            virtual void StateChange()
            {
                TRACE(Trace::Information, (_T("JSONRPCServer: State change: ")));
                if (this->IsOpen()) {
                    TRACE(Trace::Information, (_T("Open - OK\n")));
                }
                else {
                    TRACE(Trace::Information, (_T("Closed - %s\n"), (this->IsSuspended() ? _T("SUSPENDED") : _T("OK"))));
                }
            }
            virtual bool IsIdle() const
            {
                return (true);
            }
        private:
            friend class Core::SocketServerType<JSONRPCServer<INTERFACE>>;

            inline uint32_t Id() const
            {
                return (_id);
            }
            inline void Id(const uint32_t id)
            {
                _id = id;
            }
            void ToMessage(const Core::ProxyType<Core::JSON::IElement>& jsonObject) const
            {
                string jsonMessage;
                jsonObject->ToString(jsonMessage);

                TRACE(Trace::Information, (_T("   Bytes: %d\n"), static_cast<uint32_t>(jsonMessage.size())));
                TRACE(Trace::Information, (_T("Received: %s\n"), jsonMessage.c_str()));

            }
            void ToMessage(const Core::ProxyType<Core::JSON::IMessagePack>& jsonObject) const
            {
                std::vector<uint8_t> message;
                jsonObject->ToBuffer(message);
                string jsonMessage(message.begin(), message.end());

                TRACE(Trace::Information, (_T("   Bytes: %d\n"), static_cast<uint32_t>(jsonMessage.size())));
                TRACE(Trace::Information, (_T("Received: %s\n"), jsonMessage.c_str()));
            }
            void ToMessage(const Core::JSON::IElement* jsonObject, string& jsonMessage) const
            {
                jsonObject->ToString(jsonMessage);

                TRACE(Trace::Information, (_T("   Bytes: %d\n"), static_cast<uint32_t>(jsonMessage.size())));
                TRACE(Trace::Information, (_T("Received: %s\n"), jsonMessage.c_str()));
            }
            void ToMessage(const Core::JSON::IMessagePack* jsonObject, string& jsonMessage) const
            {
                std::vector<uint8_t> message;
                jsonObject->ToBuffer(message);
                string strMessage(message.begin(), message.end());
                jsonMessage = strMessage;

                TRACE(Trace::Information, (_T("   Bytes: %d\n"), static_cast<uint32_t>(jsonMessage.size())));
                TRACE(Trace::Information, (_T("Received: %s\n"), jsonMessage.c_str()));
            }
            void ProcessMessage(Core::ProxyType<Core::JSONRPC::Message>& message)
            {
                ASSERT(message->Designator.IsSet() == true);

                string designator = message->Designator.Value();
                std::size_t found = designator.find_last_of(".");
                string method(designator, found + 1, string::npos);

                if (method == "send") {
                    if (message->Parameters.IsSet() == true) {
                        Data::JSONDataBuffer data;
                        FromMessage((INTERFACE*)&data, message);
                        Core::JSON::DecUInt32 result = 0;
                        _parent.Interface().send(data, result);
                        message->Result = Core::NumberType<uint32_t>(result.Value()).Text();
                    }
                } else if (method == "receive") {
                    string result;
                    if (message->Parameters.IsSet() == true) {
                        Data::JSONDataBuffer response;
                        Core::JSON::DecUInt16 length;
                        FromMessage((INTERFACE*)&length, message);
                        _parent.Interface().receive(length, response);
                        response.ToString(result);
                     }
                     message->Result = result;
                } else if (method == "exchange") {
                    string result;
                    if (message->Parameters.IsSet() == true) {
                        Data::JSONDataBuffer data;
                        FromMessage((INTERFACE*)&data, message);

                        Data::JSONDataBuffer response;
                        _parent.Interface().exchange(data, response);
                        ToMessage((INTERFACE*)&response, result);
                    }
                    message->Result = result;
                } else {
                    TRACE(Trace::Error, (_T("Unknown method")));
                }

                message->Parameters.Clear();
                message->Designator.Clear();
                message->JSONRPC = Core::JSONRPC::Message::DefaultVersion;
            }
            void FromMessage(Core::JSON::IElement* jsonObject, const Core::ProxyType<Core::JSONRPC::Message>& message)
            {
                jsonObject->FromString(message->Parameters.Value());
            }
            void FromMessage(Core::JSON::IMessagePack* jsonObject, const Core::ProxyType<Core::JSONRPC::Message>& message)
            {
                string value = message->Parameters.Value();
                std::vector<uint8_t> parameter(value.begin(), value.end());

                jsonObject->FromBuffer(parameter);
            }
        private:
            uint32_t _id;
            JSONRPCChannel<INTERFACE>& _parent;
        };

        template <typename INTERFACE>
        class JSONRPCChannel : public Core::SocketServerType<JSONRPCServer<INTERFACE>> {
        public:
            JSONRPCChannel() = delete;
            JSONRPCChannel(const JSONRPCChannel& copy) = delete;
            JSONRPCChannel& operator=(const JSONRPCChannel&) = delete;
        public:
            JSONRPCChannel(const WPEFramework::Core::NodeId& remoteNode, JSONRPCPlugin& parent)
                : Core::SocketServerType<JSONRPCServer<INTERFACE>>(remoteNode)
                , _parent(parent)
            {
                Core::SocketServerType<JSONRPCServer<INTERFACE>>::Open(Core::infinite);
            }
            ~JSONRPCChannel()
            {
                Core::SocketServerType<JSONRPCServer<INTERFACE>>::Close(1000);
            }
            JSONRPCPlugin& Interface() {
                return _parent;
            }
        private:
            JSONRPCPlugin& _parent;
        };

        // The next class is a helper class, just to trigger an a-synchronous callback every Period()
        // amount of time.
        class PeriodicSync : public Core::IDispatch {
        private:
            PeriodicSync() = delete;
            PeriodicSync(const PeriodicSync&) = delete;
            PeriodicSync& operator=(const PeriodicSync&) = delete;

        public:
            PeriodicSync(JSONRPCPlugin* parent)
                : _parent(*parent)
            {
            }
            ~PeriodicSync()
            {
            }

        public:
            void Period(const uint8_t time)
            {
                _nextSlot = (time * 1000);
            }
            // This method is called by the main process ThreadPool at the scheduled time.
            // After the parent has been called to send out a-synchronous notifications, it
            // will schedule itself again, to be triggered after the set period.
            virtual void Dispatch() override
            {
                _parent.SendTime();

                if (_nextSlot != 0) {
                    Core::IWorkerPool::Instance().Schedule(Core::Time::Now().Add(_nextSlot), Core::ProxyType<Core::IDispatch>(*this));
                }
            }

        private:
            uint32_t _nextSlot;
            JSONRPCPlugin& _parent;
        };
        class Callback : public Core::IDispatch {
        private:
            Callback() = delete;
            Callback(const Callback&) = delete;
            Callback& operator=(const Callback&) = delete;

        public:
            Callback(JSONRPCPlugin* parent, const Core::JSONRPC::Connection& channel)
                : _parent(*parent)
                , _channel(channel)
            {
            }
            ~Callback()
            {
            }

        public:
            // This method is called by the main process ThreadPool at the scheduled time.
            // After the parent has been called to send out a-synchronous notifications, it
            // will schedule itself again, to be triggered after the set period.
            virtual void Dispatch() override
            {
                _parent.SendTime(_channel);
            }

        private:
            JSONRPCPlugin& _parent;
            Core::JSONRPC::Connection _channel;
        };

        // Define a handler for incoming JSONRPC messages. This method does not take any
        // parameters, it just returns the current time of this server, if it is called.
        uint32_t time(Core::JSON::String& response)
        {
            response.SetQuoted(true);
            response = Core::Time::Now().ToRFC1123();
            return (Core::ERROR_NONE);
        }

        uint32_t postmessage(const Data::MessageParameters& params)
        {
            PostMessage(params.Recipient.Value(), params.Message.Value());
            return (Core::ERROR_NONE);
        }

        uint32_t clueless()
        {
            TRACE(Trace::Information, (_T("A parameter less method that returns nothing was triggered")));
            return (Core::ERROR_NONE);
        }
        uint32_t clueless2(const Core::JSON::String& inbound, Core::JSON::String& response)
        {
            TRACE(Trace::Information, (_T("Versioning, this is only on version 1")));
            response = _T("CLUELESS RESPONSE: ") + inbound.Value();
            return (Core::ERROR_NONE);
        }       

        uint32_t input(const Core::JSON::String& info)
        {
            TRACE(Trace::Information, (_T("Received the text: %s"), info.Value().c_str()));
            return (Core::ERROR_NONE);
        }

        // If the parameters are more complex (aggregated JSON objects) us JSON container
        // classes.
        uint32_t extended(const Data::Parameters& params, Data::Response& response)
        {
            if (params.UTC.Value() == true) {
                response.Time = Core::Time::Now().Ticks();
            } else {
                response.Time = Core::Time::Now().Add(60 * 60 * 100).Ticks();
            }
            if (params.Location.Value() == _T("BadDay")) {
                response.State = Data::Response::INACTIVE;
            } else {
                response.State = Data::Response::IDLE;
            }
            return (Core::ERROR_NONE);
        }
        uint32_t set_geometry(const Data::Geometry& window)
        {
            _window.X = window.X.IsSet() ? window.X.Value() : _window.X;
            _window.Y = window.Y.IsSet() ? window.Y.Value() : _window.Y;
            _window.Width = window.Width;
            _window.Height = window.Height;
            return (Core::ERROR_NONE);
        }
        uint32_t get_geometry(Data::Geometry& window) const
        {
            window = Data::Geometry(_window.X, _window.Y, _window.Width, _window.Height);
            return (Core::ERROR_NONE);
        }
        uint32_t get_data(Core::JSON::String& data) const
        {
            data.SetQuoted(true);
            data = _data;
            return (Core::ERROR_NONE);
        }
        uint32_t set_data(const Core::JSON::String& data)
        {
            _data = data.Value();
            return (Core::ERROR_NONE);
        }
        uint32_t get_array_value(const string& id, Core::JSON::DecUInt32& data) const
        {
            uint8_t index = Core::NumberType<uint8_t>(id.c_str(), static_cast<uint32_t>(id.length())).Value();
            data = _array[index];
            return (Core::ERROR_NONE);
        }
        uint32_t set_array_value(const string& id, const Core::JSON::DecUInt32& data)
        {
            uint8_t index = Core::NumberType<uint8_t>(id.c_str(), static_cast<uint32_t>(id.length()));
            _array[index] = data.Value();
            return (Core::ERROR_NONE);
        }
        uint32_t get_status(Core::JSON::String& data) const
        {
            data.SetQuoted(true);
            data = "Readonly value retrieved";
            return (Core::ERROR_NONE);
        }
        uint32_t set_value(const Core::JSON::String& data)
        {
            _data = data.Value();
            return (Core::ERROR_NONE);
        }
        uint32_t swap(const JsonObject& parameters, JsonObject& response)
        {
            response = JsonObject({ { "x", 111 }, { "y", 222 }, { "width", _window.Width }, { "height", _window.Height } });

            // Now lets see what we got we can set..
            Core::JSON::Variant value = parameters.Get("x");
            if (value.Content() == Core::JSON::Variant::type::NUMBER) {
                _window.X = static_cast<uint32_t>(value.Number());
            } else if (value.Content() == Core::JSON::Variant::type::EMPTY) {
                TRACE(Trace::Information, ("The <x> is not available"));
            } else {
                TRACE(Trace::Information, ("The <x> is not defined as a number"));
            }
            value = parameters.Get("y");
            if (value.Content() == Core::JSON::Variant::type::NUMBER) {
                _window.X = static_cast<uint32_t>(value.Number());
            }
            value = parameters.Get("width");
            if (value.Content() == Core::JSON::Variant::type::NUMBER) {
                _window.X = static_cast<uint32_t>(value.Number());
            }
            value = parameters.Get("height");
            if (value.Content() == Core::JSON::Variant::type::NUMBER) {
                _window.X = static_cast<uint32_t>(value.Number());
            }
            return (Core::ERROR_NONE);
        }
        uint32_t set_opaque_geometry(const JsonObject& window)
        {
            // Now lets see what we got we can set..
            Core::JSON::Variant value = window.Get("x");
            if (value.Content() == Core::JSON::Variant::type::NUMBER) {
                _window.X = static_cast<uint32_t>(value.Number());
            } else if (value.Content() == Core::JSON::Variant::type::EMPTY) {
                TRACE(Trace::Information, ("The <x> is not available"));
            } else {
                TRACE(Trace::Information, ("The <x> is not defined as a number"));
            }
            value = window.Get("y");
            if (value.Content() == Core::JSON::Variant::type::NUMBER) {
                _window.Y = static_cast<uint32_t>(value.Number());
            }
            value = window.Get("width");
            if (value.Content() == Core::JSON::Variant::type::NUMBER) {
                _window.Width = static_cast<uint32_t>(value.Number());
            }
            value = window.Get("height");
            if (value.Content() == Core::JSON::Variant::type::NUMBER) {
                _window.Height = static_cast<uint32_t>(value.Number());
            }
            return (Core::ERROR_NONE);
        }
        uint32_t get_opaque_geometry(JsonObject& window) const
        {
            window = JsonObject({ { "x", _window.X }, { "y", _window.Y }, { "width", _window.Width }, { "height", _window.Height } });
            return (Core::ERROR_NONE);
        }
        void async_callback(const Core::JSONRPC::Connection& connection, const Core::JSON::DecUInt8& seconds)
        {
            Core::ProxyType<Core::IDispatch> job(Core::ProxyType<Callback>::Create(this, connection));
            Core::IWorkerPool::Instance().Schedule(Core::Time::Now().Add(seconds * 1000), job);
        }
        uint32_t checkvalidation(Core::JSON::String& response)
        {
            response.SetQuoted(true);
            response = "Passed Validation";
            return (Core::ERROR_NONE);
        }

        // Methods for performance measurements
        uint32_t send(const Data::JSONDataBuffer& data, Core::JSON::DecUInt32& result) 
        {
            uint16_t length = static_cast<uint16_t>(((data.Data.Value().length() * 6) + 7) / 8);
            uint8_t* buffer = static_cast<uint8_t*>(ALLOCA(length));
            Core::FromString(data.Data.Value(), buffer, length);
            result = Send(length, buffer); 
            return (Core::ERROR_NONE);
        }
        uint32_t receive(const Core::JSON::DecUInt16& maxSize, Data::JSONDataBuffer& data)
        {
            uint32_t status = Core::ERROR_NONE;
            string convertedBuffer;

            uint16_t length = maxSize.Value();
            uint8_t* buffer = static_cast<uint8_t*>(ALLOCA(length));
            status = Receive(length, buffer);

            Core::ToString(buffer, length, false, convertedBuffer);
            data.Data = convertedBuffer;
            data.Length = static_cast<uint16_t>(convertedBuffer.length());
            data.Duration = static_cast<uint16_t>(convertedBuffer.length()) + 1; //Dummy

            return status;
        }
        uint32_t exchange(const Data::JSONDataBuffer& data, Data::JSONDataBuffer& result)
        {
            uint32_t status = Core::ERROR_NONE;
            string convertedBuffer;

            uint16_t length = static_cast<uint16_t>(data.Data.Value().length());
            uint8_t* buffer = static_cast<uint8_t*>(ALLOCA(length));
            Core::FromString(data.Data.Value(), buffer, length);
            status = Exchange(length, buffer, data.Length.Value());

            Core::ToString(buffer, length, false, convertedBuffer);
            result.Data = convertedBuffer;
            result.Length = static_cast<uint16_t>(convertedBuffer.length());
            result.Duration = static_cast<uint16_t>(convertedBuffer.length()) + 1; //Dummy

            return status;
        }

    public:
        JSONRPCPlugin();
        ~JSONRPCPlugin() override;

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(JSONRPCPlugin)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_ENTRY(Exchange::IPerformance)
        INTERFACE_ENTRY(Exchange::IMath)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   Private methods specific to this class.
        // -------------------------------------------------------------------------------------------------------
        void PostMessage(const string& recipient, const string& message);
        void SendTime();
        void SendTime(Core::JSONRPC::Connection& channel);

        //   Exchange::IPerformance methods
        // -------------------------------------------------------------------------------------------------------
        uint32_t Send(const uint16_t sendSize, const uint8_t buffer[]) override;
        uint32_t Receive(uint16_t& bufferSize, uint8_t buffer[]) const override;
        uint32_t Exchange(uint16_t& bufferSize, uint8_t buffer[], const uint16_t maxBufferSize) override;

        //   Exchange::IMath methods
        // -------------------------------------------------------------------------------------------------------
        uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const override;
        uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const override;

    private:
        bool Validation(const string& token, const string& method, const string& parameters);

    private:
        Core::ProxyType<PeriodicSync> _job;
        Data::Window _window;
        string _data;
        std::vector<uint32_t> _array;
        COMServer* _rpcServer;
        JSONRPCChannel<Core::JSON::IElement>* _jsonServer;
        JSONRPCChannel<Core::JSON::IMessagePack>* _msgServer;
    };

} // namespace Plugin
} // namespace WPEFramework
