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

#include <plugins/plugins.h>

// Interface to provide an implementation for
#include <interfaces/ISimplePlugin.h>

namespace Thunder {
namespace Plugin {

// The 'always' in-process part of the plugin
class SimplePlugin : public PluginHost::IPlugin, public PluginHost::JSONRPC {
private:

    // Global (event) notifications related to all (IPlugin) plugins, registration via IShell
    class PluginNotification : public PluginHost::IPlugin::INotification {
    public :
        // Typically deleted as it does not keep track of its parent
        PluginNotification() = delete;
        PluginNotification(const PluginNotification&) = delete;
        PluginNotification(PluginNotification&&) = delete;

        PluginNotification& operator=(const PluginNotification&) = delete;
        PluginNotification& operator=(PluginNotification&&) = delete;

        // Keep track of parent
        PluginNotification(SimplePlugin& parent)
            : PluginHost::IPlugin::INotification{}
            , _parent{ parent }
        {}

        ~PluginNotification() override = default;

        // Implements QueryInterface for this IPlugin::INotification
        BEGIN_INTERFACE_MAP(SimplePluginPluginNotification)
            // The instantiation of this class resides in-process and thus it's this pointer sufficies for QueryInterface
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        END_INTERFACE_MAP

        //  IPlugin::INotification interface methods
        // -----------------------------------------

        // State of (this) plugin changed to Activated
        void Activated(const string& callsign, PluginHost::IShell* plugin) override;
        // State of (this) plugin changed to Deactivated
        void Deactivated(const string& callsign, PluginHost::IShell* plugin) override;
        // State of (this) plugin changed to Unavailable
        void Unavailable(const string& callsign, PluginHost::IShell* plugin) override;

        // IUnknown interface methods, via IPlugins::INotification base class 
        // ------------------------------------------------------------------

        uint32_t AddRef() const override;
        uint32_t Release() const override;

    private :
        SimplePlugin& _parent;
    };

    // Global (event) notifications related to all (IRemoteConnection) s, registration via IShell
    // Supports monitoring the out-of-process via existence of the connection between both sides
    class RemoteConnectionNotification : public RPC::IRemoteConnection::INotification { 
    public :
        // Typically deleted as it does not keep track of its parent
        RemoteConnectionNotification() = delete;
        RemoteConnectionNotification(const RemoteConnectionNotification&) = delete;
        RemoteConnectionNotification(RemoteConnectionNotification&&) = delete;

        RemoteConnectionNotification& operator=(const RemoteConnectionNotification&) = delete;
        RemoteConnectionNotification& operator=(RemoteConnectionNotification&&) = delete;

        // Keep track of parent
        RemoteConnectionNotification(SimplePlugin& parent)
            : RPC::IRemoteConnection::INotification{}
            , _parent{ parent }
        {}

        ~RemoteConnectionNotification() override = default;

        // Implements QueryInterface for this IPlugin::INotification
        BEGIN_INTERFACE_MAP(SimplePluginRemoteConnectionNotification)
            // The instantiation of this class resides in-process and thus its this pointer sufficies for QueryInterface
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
        END_INTERFACE_MAP

        //  RPC::IRemoteConnection::INotification interface methods
        // --------------------------------------------------------

        // A (remote) COM-RPC connection has been set up
        void Activated(RPC::IRemoteConnection* connection) override;
        // A (remote) COM-RPC connection has been disconnected / destroyed
        void Deactivated(RPC::IRemoteConnection* connection) override;
        // The remote side (process) has been terminated
        void Terminated(RPC::IRemoteConnection* connection) override;

        // IUnknown interface methods, via IRemoteConnection::INotification base class 
        // ---------------------------------------------------------------------------

        uint32_t AddRef() const override;
        uint32_t Release() const override;

    private :
        SimplePlugin& _parent;
    };

    // Passthrough for JSON-RPC notifications of out-of-process event notifications
    // JSON formatted notifications require coupling to the module.Notify(...) in JSimplePlugin
    class SimplePluginNotification: public Exchange::ISimplePlugin::INotification {
    public : 
        // Typically deleted as it does not keep track of its parent
        SimplePluginNotification() = delete;
        SimplePluginNotification(const SimplePluginNotification&) = delete;
        SimplePluginNotification(SimplePluginNotification&&) = delete;

        SimplePluginNotification& operator=(const SimplePluginNotification&) = delete;
        SimplePluginNotification& operator=(SimplePluginNotification&&) = delete;

        // Keep track of parent
        SimplePluginNotification(SimplePlugin& parent)
            : Exchange::ISimplePlugin::INotification{}
            , _parent{ parent }
        {}

        // Trigger JSON event
        // JSON messages should use lower case for method!
        void LifeChangingEvent(const string& description) override;

        // IUnknown interface methods, via ISimplePlugin::INotification base class 
        // -----------------------------------------------------------------------

        uint32_t AddRef() const override;
        uint32_t Release() const override;

        BEGIN_INTERFACE_MAP(SimplePluginNotification)
            INTERFACE_ENTRY(Exchange::ISimplePlugin::INotification)
        END_INTERFACE_MAP

    private :

        SimplePlugin& _parent;
    };

public :
    SimplePlugin(const SimplePlugin&) = delete;
    SimplePlugin(SimplePlugin&&) = delete;

    SimplePlugin& operator=(const SimplePlugin&) = delete;
    SimplePlugin& operator=(SimplePlugin&&) = delete;

    SimplePlugin()
        : PluginHost::IPlugin{}
        , _adminLock{}
        , _service{ nullptr }
        , _pluginNotification{ *this }
        , _remoteConnectionNotification{ *this }
        , _simplePluginNotification{ *this }
        , _simplePluginImplementation{ nullptr }
        , _connectionId{ 0 }
    {}

    ~SimplePlugin() override
    {}

    // Exposed interfaces
    // Implements QueryInterface for this IPlugin
    // INTERFACE_AGGREGATE is used if the this pointer does not apply and another object is implementing a certain interface
    // However, it should be added here to make Thunder's in-process aware; then also add INTERFACE_ENTRY at the true implemntation of the class to support QueryInterface in the out-off-process part
    // AND add SERVICE_REGISTRATION to make Thunder aware that the implemented part (service, eg, classname) is made available through the given MODULE_NAME (.so)
    BEGIN_INTERFACE_MAP(SimplePluginMultiProcess)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        // JSON-RPC support
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::ISimplePlugin, _simplePluginImplementation)
    END_INTERFACE_MAP

    //  IPlugin interface methods
    // --------------------------

    // On plugin activation: return an empty string on success 
    const string Initialize(PluginHost::IShell* service) override;
    // On plugin deactiviation
    void Deinitialize(PluginHost::IShell* service) override;
    // Provides plugin's JSON struct for MetaData plugin
    string Information() const override;

    //  Custom (added) methods
    // -----------------------

    // Required to do some connection ID checking
    void Deactivated(const RPC::IRemoteConnection* connection) const;

protected :

    virtual std::string Classname() const = 0; 

private :
    Core::CriticalSection _adminLock;

    // Accessor to the plugin's runtime, ie instance, information
    PluginHost::IShell* _service;

    // By Sink<> templating implements AddRef and Release, and, balance checking in the AddRef - Release cycle
//        Sink<SimplePluginNotification> _notification;
    //  Here, AddRef and Release need to be implemented
    PluginNotification _pluginNotification;

    // By Sink<> templating implements AddRef and Release, and, balance checking in the AddRef - Release cycle
//    Sink<RemoteConnectionNotification> _remoteConnectionNotification;
    RemoteConnectionNotification _remoteConnectionNotification;

    // Intermediate to trigger JSON events from out-of-process notification events
    SimplePluginNotification _simplePluginNotification;

    // The (out of process) implementation
    // Typically implements an interface from ThunderInterfaces, eg, Exchange namespace

    Exchange::ISimplePlugin* _simplePluginImplementation;   // Handle to identify the 'remote' instance implementing the ISimplePlugin methods 
    uint32_t _connectionId;                                 // Identifier to obtain a handle to identify the connection to the out of process part
};

} } // namespace Thunder::Plugin
