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


#include "HelloWorld.h"

#include <interfaces/json/JsonData_HelloWorld.h>


namespace WPEFramework {
namespace Plugin {

using namespace JsonData::HelloWorld;

    SERVICE_REGISTRATION(HelloWorld, 1, 0);



  /* virtual */ const string HelloWorld::Initialize(PluginHost::IShell* service)
    {
        string message;
        Config config;
        _connectionId = 0;

        ASSERT(_service == nullptr);
	    ASSERT(service != nullptr);
	    TRACE(Trace::Information, (_T("HelloWorld::Initialize")));

        // Setup skip URL for right offset.
        _service = service;
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
         
        config.FromString(service->ConfigLine());
        
        _hello = service->Root<Exchange::IHelloWorld>(_connectionId, 2000, _T("HelloWorldImplementation"));
         if (_hello != nullptr)
         {
        
           _hello->Configure(service);
           TRACE(Trace::Information, (_T("Successfully instantiated HelloWorld implementation")));
         }
         else
         {
            message = _T("HelloWorld plugin could not be initialised");
            _service = nullptr;
         }
        return message;
    }



   /* virtual */ void HelloWorld::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

       
	   _service = nullptr;
    }

   /* virtual */ string HelloWorld::Information() const
    {
        // No additional info to report.
        return (string());
    }

#if 0
   /* virtual */ uint32_t HelloWorld::Greet(const string& msg, string& out /* @out */) const
   {
       TRACE(Trace::Information, (_T("Greet %s from someone"),msg.c_str()));
       out = "Message Received, Thanks !!!  : " + msg; 
       return (Core::ERROR_NONE);
   }
#endif

   void HelloWorld::RegisterAll()
   {
       TRACE(Trace::Information, (_T("HelloWorld::RegisterAll")));
       PluginHost::JSONRPC::Register<Core::JSON::String, Core::JSON::String>(_T("greet"), &HelloWorld::JGreet, this);
   }


   uint32_t HelloWorld::JGreet(const Core::JSON::String& inbound, Core::JSON::String& response)
   {
       string out = response.Value();
       _hello->Greet(inbound.Value(),out);
       response = out;
       //TRACE(Trace::Information, (_T("Greet %s from someone"),inbound.Value().c_str()));
       //response = "Message Received, Thanks !!!  : " + inbound.Value();    
       return (Core::ERROR_NONE);
   }

   void HelloWorld::UnregisterAll()
   {
       PluginHost::JSONRPC::Unregister(_T("greet"));
   }

} //namespace Plugin
} // namespace WPEFramework


