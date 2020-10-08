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
#include "IHelloWorld.h"
#include <interfaces/json/JsonData_HelloWorld.h>

namespace WPEFramework {
namespace Plugin {

    class HelloWorld
        : public PluginHost::IPlugin,
          public PluginHost::JSONRPC	{

    private:
        HelloWorld(const HelloWorld&) = delete;
        HelloWorld& operator=(const HelloWorld&) = delete;       
 
       class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , OutOfProcess(false)
            {
                Add(_T("outofprocess"), &OutOfProcess);
            }
            ~Config() override
            {
            }

        public:
            Core::JSON::Boolean OutOfProcess;
        } config;

   
     public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
        HelloWorld()
            : _service(nullptr),
              _connectionId(0),
              _hello(nullptr)
        {
	        RegisterAll();
        }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
        virtual ~HelloWorld()
        {
	        UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(HelloWorld)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::IHelloWorld, _hello)
        END_INTERFACE_MAP

    /*
    The above macro definitions translate to the below code which is an override implementation of the 
    QueryInterface method in IUnknown interface.


    void* QueryInterface(const uint32_t interfaceNumber) override            
    {                                                                        
        if (interfaceNumber == WPEFramework::Core::IUnknown::ID) {                         
            AddRef();                                                        
            return (static_cast<void*>(static_cast<WPEFramework::Core::IUnknown*>(this))); 
        }
        else if (interfaceNumber == PluginHost::IPlugin)                      
        {                                                          
            AddRef();                                              
            return (static_cast<void*>(static_cast<PluginHost::IPlugin*>(this))); 
        }
        //If this class inherits IHelloWorld
        else if (interfaceNumber == Exchange::IHelloWorld)                      
        {                                                          
            AddRef();                                              
            return (static_cast<void*>(static_cast<Exchange::IHelloWorld*>(this))); 
        }
        //else if this class aggregates the HelloWorld Implementation then
        else if (interfaceNumber == TYPE::ID)                 
        {                                                      
            if (_hello != nullptr)
            {                       
                return (_hello->QueryInterface(TYPE::ID)); 
            }                                                 
           return (nullptr);                                 
        }
        else if (interfaceNumber == PluginHost::IDispatcher)                      
        {                                                          
            AddRef();                                              
            return (static_cast<void*>(static_cast<PluginHost::IDispatcher*>(this))); 
        }
        
        return (nullptr);                                         
    }


    */


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

        // The plugin is unloaded from WPEFramework. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service);

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        virtual string Information() const;

    
    private:
      // JsonRpc
        void RegisterAll();
        void UnregisterAll();
        uint32_t JGreet(const Core::JSON::String& inbound, Core::JSON::String& response);

        uint32_t _skipURL;
        PluginHost::IShell* _service;
        uint32_t _connectionId;
        Exchange::IHelloWorld* _hello;

    }; //class HelloWorld


} //namespace Plugin
} //namespace WPEFramework
