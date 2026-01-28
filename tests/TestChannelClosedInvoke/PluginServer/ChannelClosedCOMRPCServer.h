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
#pragma once

#include "Module.h"

#include <qa_interfaces/ITestChannelClosed.h>

namespace Thunder {
namespace Plugin {

    class ChannelClosedCOMRPCServer : public PluginHost::IPlugin {
    private:

        friend class COMServer;

        class TestChannelClosedService : public QualityAssurance::ITestChannelClosed {
        public:
            TestChannelClosedService(const TestChannelClosedService&) = delete;
            TestChannelClosedService& operator=(const TestChannelClosedService&) = delete;
            TestChannelClosedService(TestChannelClosedService&&) = delete;
            TestChannelClosedService& operator=(TestChannelClosedService&&) = delete;

            TestChannelClosedService() {}
            ~TestChannelClosedService() override
            {
            }

        public:
            Core::hresult Test() override 
            { 
                return Core::ERROR_NONE; 
            }
            Core::hresult Test(ICallback* const cb) override
            {
                ASSERT(cb != nullptr);
                cb->Done();
                return Core::ERROR_NONE;
            }
            Core::hresult Block(const uint8_t seconds) override
            {
                SleepMs(seconds * 1000);
                return Core::ERROR_NONE;
            }
            
            BEGIN_INTERFACE_MAP(TestChannelClosedService)
            INTERFACE_ENTRY(QualityAssurance::ITestChannelClosed)
            END_INTERFACE_MAP

        private:
        };

        class COMServer : public RPC::Communicator {
        public:
            COMServer(const COMServer&) = delete;
            COMServer& operator=(const COMServer&) = delete;
            COMServer(COMServer&&) = delete;
            COMServer& operator=(COMServer&&) = delete;

            COMServer(
                const Core::NodeId& source,
                const string& proxyServerPath)
                : RPC::Communicator(
                      source,
                      proxyServerPath,
                      Core::ProxyType<Core::IIPCServer>(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create()), // make sure to only give it one thread...
                      _T("@ChannelClosedCOMRPCServer"))
            {
                Open(Core::infinite);
            }
            ~COMServer() override
            {
                Close(Core::infinite);
            }

        private:
            void* Acquire(const string& className, const uint32_t interfaceId, const uint32_t versionId) override;
        };

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
            #ifdef __WINDOWS__
                , Connector("127.0.0.1:62010")
            #else
                , Connector("/tmp/ChannelCloseInvoke")
            #endif
            {
                Add(_T("connector"), &Connector);
            }

            ~Config() = default;

        public:
            Core::JSON::String Connector;
        };

    public:
        //Constructors
        ChannelClosedCOMRPCServer(const ChannelClosedCOMRPCServer&) = delete;
        ChannelClosedCOMRPCServer& operator=(const ChannelClosedCOMRPCServer&) = delete;
        ChannelClosedCOMRPCServer(ChannelClosedCOMRPCServer&&) = delete;
        ChannelClosedCOMRPCServer& operator=(ChannelClosedCOMRPCServer&&) = delete;

        ChannelClosedCOMRPCServer()
            : _comserver()
        {
        }
        ~ChannelClosedCOMRPCServer() override
        {
        }

    public:

        //Service Registration
        BEGIN_INTERFACE_MAP(ChannelClosedCOMRPCServer)
            INTERFACE_ENTRY(IPlugin)
        END_INTERFACE_MAP

        //  IPlugin methods
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

    private:
        Core::ProxyType<COMServer> _comserver;
    };
}
}
