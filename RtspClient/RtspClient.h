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

#ifndef __RTSPCLIENT_H
#define __RTSPCLIENT_H

#include "Module.h"
#include <interfaces/IRtspClient.h>

namespace WPEFramework {
namespace Plugin {

    class RtspClient : public PluginHost::IPlugin, public PluginHost::IWeb {
    private:
        RtspClient(const RtspClient&) = delete;
        RtspClient& operator=(const RtspClient&) = delete;

        class Notification : public RPC::IRemoteConnection::INotification {

        private:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            explicit Notification(RtspClient* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification()
            {
            }

        public:
            virtual void Activated(RPC::IRemoteConnection*)
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
            RtspClient& _parent;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , OutOfProcess(true)
            {
                Add(_T("outofconnection"), &OutOfProcess);
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
                , AssetId()
                , Str()
            {
                Add(_T("AssetId"), &AssetId);
                Add(_T("Scale"), &Scale);
                Add(_T("Position"), &Position);
                Add(_T("Str"), &Str);
            }
            ~Data()
            {
            }

        public:
            Core::JSON::String AssetId;
            Core::JSON::DecSInt32 Scale;
            Core::JSON::DecUInt32 Position;
            Core::JSON::String Str;
        };

    public:
        RtspClient()
            : _service(nullptr)
            , _implementation(nullptr)
            , _notification(this)
        {
        }

        virtual ~RtspClient()
        {
        }

    public:
        BEGIN_INTERFACE_MAP(RtspClient)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_AGGREGATE(Exchange::IRtspClient, _implementation)
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
        virtual const string Initialize(PluginHost::IShell* service);

        // The plugin is unloaded from the webbridge. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service);

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        virtual string Information() const;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request);
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        uint8_t _skipURL;
        uint32_t _connectionId;
        PluginHost::IShell* _service;
        Exchange::IRtspClient* _implementation;
        Core::Sink<Notification> _notification;
    };
} //namespace Plugin

} //namespace Solution

#endif // __RTSPCLIENT_H
