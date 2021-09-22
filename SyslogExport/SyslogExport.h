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
#include <vector>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <tracing/SyslogMonitor.h>
constexpr int max_client_connection = 5;
namespace WPEFramework {

namespace Plugin {
    class SyslogExport : public PluginHost::IPluginExtended,
                      public PluginHost::IWebSocket {
    public:
        SyslogExport(const SyslogExport&) = delete;
        SyslogExport& operator=(const SyslogExport&) = delete;
        SyslogExport(): _sinkManager()
                    , _notification(this)
        {
        }
        ~SyslogExport() = default;
    public:
        BEGIN_INTERFACE_MAP(SyslogExport)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IPluginExtended)
        INTERFACE_ENTRY(PluginHost::IWebSocket)
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

        void OutputMessage(const std::string& logLine);

    private:

        class WebsocketSink {
        public:
            WebsocketSink() = delete;
            explicit WebsocketSink(uint32_t maxConnections): _maxConnections(maxConnections)
                                                    , _lock(){

            }
            WebsocketSink(const WebsocketSink&) = delete;
            WebsocketSink& operator=(const WebsocketSink&) = delete;
            ~WebsocketSink(){
                if(_exportChannelMap.size() > 0) {
                    _exportChannelMap.clear();
                }
            }
            void DeliverMessage(const string& logLine);
            bool AddChannel(WPEFramework::PluginHost::Channel& channel);
            void RemoveChannel(uint32_t channelId);
        private:
            uint32_t _maxConnections;
            Core::CriticalSection _lock;
            std::map<uint32_t, WPEFramework::PluginHost::Channel*> _exportChannelMap;
        };

        class Notification : public Logging::ISyslogMonitorClient {
        public:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;
            void NotifyLog(const std::string& logLine) override {
                _parent->OutputMessage(logLine);
                return;
            }
            explicit Notification(SyslogExport* parent):_parent(parent) {
                ASSERT(parent != nullptr)
            }
            ~Notification() = default;
        private:
            SyslogExport* _parent;
        };

        class Config : public Core::JSON::Container {
        public:
            Config(): Core::JSON::Container()
                    , MaxClientConnection(max_client_connection) {
                
                Add(_T("maxclientconnection"), &MaxClientConnection);
            }
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            ~Config() = default;
        public:
            Core::JSON::DecUInt32 MaxClientConnection;
        };

        class SinkManager : public Core::Thread {
        public:
            SinkManager(): _logList()
                            , _mutex()
                            , _condVar()
                            , _websocketSink()
            {

            }
            SinkManager(const SinkManager&) = delete;
            SinkManager& operator=(const SinkManager&) = delete;
            void HandleMessage(const std::string& logLine);
            uint32_t Worker() override;
            void AddOuputSink(Core::ProxyType<SyslogExport::WebsocketSink>& );
            bool AddChannel(WPEFramework::PluginHost::Channel& channel);
            void RemoveChannel(uint32_t channelId);
            void RemoveOutputSink();
            void Start();
            void Stop();
            ~SinkManager() = default;
        private:
            uint32_t ProcessMessageList();
        private:
            std::queue<std::string> _logList;
            std::mutex _mutex;
            std::condition_variable _condVar;
            Core::ProxyType<SyslogExport::WebsocketSink> _websocketSink;

        };


    private:
        SinkManager _sinkManager;
        Notification _notification;
    };
}
}
