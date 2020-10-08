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

#include "Module.h"
#include <core/ProcessInfo.h>
#include <interfaces/IHelloWorld.h>

namespace WPEFramework {
namespace Plugin {
        

   class HelloWorldImplementation : public Exchange::IHelloWorld {

       private:
      HelloWorldImplementation(const HelloWorldImplementation&) = delete;
      HelloWorldImplementation& operator=(const HelloWorldImplementation&) = delete;

       class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Connector(_T("/tmp/helloworld"))
            {
                Add(_T("connector"), &Connector);
            }
            ~Config() override
            {
            }

        public:
            Core::JSON::String Connector;
        } config;

      class COMRPCServer : public RPC::Communicator {
        private:
            COMRPCServer() = delete;
            COMRPCServer(const COMRPCServer&) = delete;
            COMRPCServer& operator=(const COMRPCServer&) = delete;

        public:
            COMRPCServer(
                const Core::NodeId& source,
                Exchange::IHelloWorld* parentInterface,
                const string& proxyStubPath,
                const Core::ProxyType<RPC::InvokeServer> & engine)
                : RPC::Communicator(source, proxyStubPath, Core::ProxyType<Core::IIPCServer>(engine))
                , _parentInterface(parentInterface)
            {
                engine->Announcements(Announcement());
                Open(Core::infinite);
            }
            ~COMRPCServer()
            {
                Close(Core::infinite);
            }

        private:
            virtual void* Aquire(const string& className, const uint32_t interfaceId, const uint32_t versionId)
            {
                void* result = nullptr;

                // Currently we only support version 1 of the IRPCLink :-)
                if (((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) && ((interfaceId == Exchange::IHelloWorld::ID) || (interfaceId == Core::IUnknown::ID))) {
                    // Reference count our parent
                    _parentInterface->AddRef();
                    TRACE(Trace::Information, ("HelloWorldImplementation interface aquired => %p", this));
                    // Allright, respond with the interface.
                    result = _parentInterface;
                }
                return (result);
            }

        private:
            Exchange::IHelloWorld* _parentInterface;
        };

  public:
      HelloWorldImplementation()
      {
           
      }

      virtual ~HelloWorldImplementation()
      {
           delete _rpcServer;
      }

    // IHelloWorld methods
	// ---------------------------------------------------------------------------------------------------------
      virtual uint32_t Configure(PluginHost::IShell* service)
      {

         ASSERT(service != nullptr);

         config.FromString(service->ConfigLine());
         Core::NodeId source(config.Connector.Value().c_str());
         _engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance());
         _rpcServer = new COMRPCServer(source, this,  service->ProxyStubPath(), _engine);

         return Core::ERROR_NONE;

      }

      virtual uint32_t Greet(const string& msg, string& out /* @out */) const override
      {
          TRACE(Trace::Information, (_T("Greet %s from someone"),msg.c_str()));
          out = "Message Received, Thanks !!!  : " + msg; 
          return (Core::ERROR_NONE);
      } 

      BEGIN_INTERFACE_MAP(HelloWorldImplementation)
      INTERFACE_ENTRY(Exchange::IHelloWorld)
      END_INTERFACE_MAP

  private:

        Core::ProxyType<RPC::InvokeServer> _engine;
        COMRPCServer* _rpcServer;

   };//HelloWorldImplementation

   SERVICE_REGISTRATION(HelloWorldImplementation, 1, 0);

}//Plugin
}//WPEFramework