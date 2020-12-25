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
#include <interfaces/IWebPA.h>
#include <set>

namespace WPEFramework {
namespace Plugin {

class WebPA : public PluginHost::IPlugin, public PluginHost::IWeb {
public:
        // maximum wait time for a process to be spawned
        static constexpr uint32_t ImplWaitTime = 2000;

private:
    class Notification : public RPC::IRemoteConnection::INotification {
    public:
        Notification() = delete;
        Notification(const Notification&) = delete;

        explicit Notification(WebPA* parent)
            : _parent(*parent)
        {
            ASSERT(nullptr != parent);
        }
        virtual ~Notification()
        {
            TRACE(Trace::Information, (_T("WebPA::Notification destructed. Line: %d"), __LINE__));
        }

    public:
        virtual void Activated(RPC::IRemoteConnection* /* connection */)
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
        WebPA& _parent;
    };

public:
    WebPA(const WebPA&) = delete;
    WebPA& operator=(const WebPA&) = delete;

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
    WebPA()
        : _skipURL(0)
        , _connectionId(0)
        , _service(nullptr)
        , _webpa(nullptr)
        , _notification(this)
    {

    }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
    virtual ~WebPA()
    {
    }

public:
    BEGIN_INTERFACE_MAP(WebPA)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_AGGREGATE(Exchange::IWebPA, _webpa)
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

    // The plugin is unloaded from WPEFramework. This is call allows the module to notify clients
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
    PluginHost::IShell* GetService() { return _service; }

private:
    void Deactivated(RPC::IRemoteConnection* connection);
private:
    uint32_t _skipURL;
    uint32_t _connectionId;
    PluginHost::IShell* _service;

    Exchange::IWebPA* _webpa;
    Core::Sink<Notification> _notification;
};
} //namespace Plugin
} //namespace WPEFramework

