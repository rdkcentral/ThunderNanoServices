/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological Management
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
#include <map>
#include <tuple>
#include <vector>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <interfaces/json/JsonData_LogExport.h>
#include <interfaces/ILogExport.h>
#include <core/FileObserver.h>

namespace WPEFramework {

namespace Plugin {




    class LogExport : public PluginHost::IPluginExtended,
                      public PluginHost::IWeb,
                      public PluginHost::JSONRPC,
                      public PluginHost::IWebSocket {
    public:
        LogExport(const LogExport&) = delete;
        LogExport& operator=(const LogExport&) = delete;
        LogExport():  _connectionId(0)
                    , _sinkManager()
                    , _notification(this)
                    , _implementation(nullptr)
        {
            RegisterAll();
        }
        ~LogExport()
        {
            UnregisterAll();
        }
    public:
        BEGIN_INTERFACE_MAP(LogExport)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_ENTRY(PluginHost::IPluginExtended)
        INTERFACE_ENTRY(PluginHost::IWebSocket)
        INTERFACE_AGGREGATE(Exchange::ILogExport, _implementation);
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
        string Information() const override{return "";};

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        // Whenever a request is received, it might carry some additional data in the body. This method allows
        // the plugin to attach a deserializable data object (ref counted) to be loaded with any potential found
        // in the body of the request.
        void Inbound(Web::Request& request) override;

        // If everything is received correctly, the request is passed on to us, through a thread from the thread pool, to
        // do our thing and to return the result in the response object. Here the actual specific module work,
        // based on a a request is handled.
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;
        

        //	IPluginExtended methods
        // -------------------------------------------------------------------------------------------------------

        // Whenever a Channel (WebSocket connection) is created to the plugin that will be reported via the Attach.
        // Whenever the channel is closed, it is reported via the detach method.
        bool Attach(PluginHost::Channel& channel) override;
        void Detach(PluginHost::Channel& channel) override;
        

        //! @{
        //! ================================== CALLED ON COMMUNICATION THREAD =====================================
        //! Whenever a WebSocket is opened with a locator (URL) pointing to this plugin, it is capable of sending
        //! JSON object to us. This method allows the plugin to return a JSON object that will be used to deserialize
        //! the comming content on the communication channel. In case the content does not start with a { or [, the
        //! first keyword deserialized is passed as the identifier.
        //! @}
        Core::ProxyType<Core::JSON::IElement> Inbound(const string& identifier) override;

        //! @{
        //! ==================================== CALLED ON THREADPOOL THREAD ======================================
        //! Once the passed object from the previous method is filled (completed), this method allows it to be handled
        //! and to form an answer on the incoming JSON message(if needed).
        //! @}
        Core::ProxyType<Core::JSON::IElement> Inbound(const uint32_t ID, const Core::ProxyType<Core::JSON::IElement>& element) override;

        uint32_t OutputMessage(const std::string& filename, Exchange::ILogExport::INotification::ILogIterator* logLines);
    public:

        class Config : public Core::JSON::Container {
        public:
            Config(): Core::JSON::Container()
                    , Console(false)
                    , MaxClientConnection(5) {
                
                Add(_T("console"), &Console);
                Add(_T("maxclientconnection"), &MaxClientConnection);
            }
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            ~Config() = default;
        public:
            Core::JSON::Boolean Console;
            Core::JSON::DecUInt32 MaxClientConnection;
        };

        class LogData : public Core::JSON::Container {
        public:
            LogData()
                : Core::JSON::Container()
                , FileName()
                , Message()
            {
                Add(_T("filename"), &FileName);
                Add(_T("message"), &Message);
            }

            LogData(const LogData&) = delete;
            LogData& operator=(const LogData&) = delete;
            ~LogData() = default;

        public:
            Core::JSON::String FileName; // file name
            Core::JSON::ArrayType<Core::JSON::String> Message; // message
        };

    private:

        class IOutputSink {
        public:
            virtual uint32_t ProcessMessage(Core::ProxyType<LogData>& logData) = 0;
            virtual uint32_t DeliverMessage() = 0;
        };

        class WebsocketSink : public IOutputSink {
            public:
                WebsocketSink() = delete;
                explicit WebsocketSink(uint32_t maxConnections): _maxConnections(maxConnections)
                                                        , _lock()
                                                        , _logData(){

                }
                WebsocketSink(const WebsocketSink&) = delete;
                WebsocketSink& operator=(const WebsocketSink&) = delete;
                static WebsocketSink& getInstance(uint32_t connections = 5)
                {
                    static WebsocketSink _instance(connections);
                    return _instance;
                }
                ~WebsocketSink() = default;
                uint32_t ProcessMessage(Core::ProxyType<LogData>& logData) override;
                uint32_t DeliverMessage() override;
                bool AddChannel(WPEFramework::PluginHost::Channel& channel);
                void RemoveChannel(uint32_t channelId);
                Core::ProxyType<LogData> GetLogData();
            private:
                uint32_t _maxConnections;
                Core::CriticalSection _lock;
                static std::map<uint32_t, WPEFramework::PluginHost::Channel*> _exportChannelMap;
                Core::ProxyType<LogData> _logData;
        };

        class ConsoleSink: public IOutputSink
        {
            public:
                ConsoleSink(): _processedMessage() 
                {
                }
                ConsoleSink(const ConsoleSink&) = delete;
                ConsoleSink& operator=(const ConsoleSink& rhs) = delete;
                ~ConsoleSink() = default;
                uint32_t ProcessMessage(Core::ProxyType<LogData>& logData) override;
                uint32_t DeliverMessage() override;
            private:
                std::string _processedMessage;
        };

        class Notification : public Exchange::ILogExport::INotification{
        public:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;
            uint32_t OutputMessage(const std::string& filename, Exchange::ILogExport::INotification::ILogIterator* logLines) override{
                return _parent->OutputMessage(filename, logLines);
            }
            explicit Notification(LogExport* parent):_parent(parent) {
                ASSERT(parent != nullptr)
            }
            ~Notification() = default;
            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(Exchange::ILogExport::INotification)
            END_INTERFACE_MAP

        private:
            LogExport* _parent;
        };


        class SinkManager : public Core::Thread {

        public:
            SinkManager(): _logList()
                            , _mutex()
                            , _sinkLock()
                            , _condVar()
                            , _sinkList()
            {

            }
            SinkManager(const SinkManager&) = delete;
            SinkManager& operator=(const SinkManager&) = delete;
            uint32_t HandleMessage(const std::string& filename, Exchange::ILogExport::INotification::ILogIterator* logs);
            uint32_t Worker() override;
            uint32_t AddOuputSink(std::unique_ptr<LogExport::IOutputSink> );
            Core::ProxyType<LogData> GetLogData();
            void Start();
            void Stop();
            ~SinkManager() = default;
        private:
            uint32_t ProcessMessageList();
        private:
            std::queue<Core::ProxyType<LogData>> _logList;
            std::mutex _mutex;
            Core::CriticalSection _sinkLock;
            std::condition_variable _condVar;
            std::vector<std::unique_ptr<LogExport::IOutputSink>> _sinkList;

        };


    private:
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_watch(const JsonData::LogExport::WatchParamsData& params);
        uint32_t endpoint_unwatch(const Core::JSON::String& filepath);
        uint32_t endpoint_dump(const Core::JSON::String& filepath);
    private:
        uint32_t _connectionId;
        SinkManager _sinkManager;
        Core::Sink<Notification> _notification;
        Exchange::ILogExport* _implementation;
        Config _config;
    };
}
}
