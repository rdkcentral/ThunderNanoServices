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

#include "CommunicationPerformance.h"

// JSONRPC support defintions
#include <interfaces/json/JSimplePlugin.h>

namespace Thunder {
namespace Plugin {

//  IPlugin::INotification interface methods
// -----------------------------------------

SimplePlugin::PluginNotification::PluginNotification(SimplePlugin& parent)
    : PluginHost::IPlugin::INotification{}
    , _parent{ parent }
{}

// State of (this) plugin changed to Activated
void SimplePlugin::PluginNotification::Activated(VARIABLE_IS_NOT_USED const string& callsign, VARIABLE_IS_NOT_USED PluginHost::IShell* plugin) /* override */
{
    ASSERT(plugin != nullptr);
    TRACE(Trace::Information, ("SimplePlugin::PluginNotification::Activated"));
}

// State of (this) plugin changed to Deactivated
void SimplePlugin::PluginNotification::Deactivated(VARIABLE_IS_NOT_USED const string& callsign, VARIABLE_IS_NOT_USED PluginHost::IShell* plugin) /* override */
{
    ASSERT(plugin != nullptr);
    TRACE(Trace::Information, ("SimplePlugin::PluginNotification::Deactivated"));
}

// State of (this) plugin changed to Unavailable
void SimplePlugin::PluginNotification::Unavailable(VARIABLE_IS_NOT_USED const string& callsign, VARIABLE_IS_NOT_USED PluginHost::IShell* plugin) /* override */
{
    ASSERT(plugin != nullptr);
    TRACE(Trace::Information, ("SimplePlugin::PluginNotification::Information"));
}

// IPlugin::INotification base IUnknown interface methods
// ------------------------------------------------------

// Not Sink templated, hence, a defintion here
uint32_t SimplePlugin::PluginNotification::AddRef() const /* override */
{
    // Keep the parent alive for our sake
    return _parent.AddRef();
}

// Not Sink templated, hence, a defintion here
uint32_t SimplePlugin::PluginNotification::Release() const /* override */
{
    return _parent.Release();
}

// IRemoteConenction::INotification interface methods
// --------------------------------------------------

SimplePlugin::RemoteConnectionNotification::RemoteConnectionNotification(SimplePlugin& parent)
    : RPC::IRemoteConnection::INotification{}
    , _parent{ parent }
{}

// A (remote) COM-RPC connection has been set up
void SimplePlugin::RemoteConnectionNotification::Activated(RPC::IRemoteConnection* connection VARIABLE_IS_NOT_USED) /* override */
{
    ASSERT(connection != nullptr);
    TRACE(Trace::Information, ("SimplePlugin::RemoteConnectionNotification::Activated"));
}

// A (remote) COM-RPC connection has been (unexpectedly) disconnected / destroyed / dropped
void SimplePlugin::RemoteConnectionNotification::Deactivated(RPC::IRemoteConnection* connection VARIABLE_IS_NOT_USED) /* override */
{
    ASSERT(connection != nullptr);

    TRACE(Trace::Information, ("SimplePlugin::RemoteConnectionNotification::Deactivated"));

    // Check if the connection to the out-of-process part has unexpectedly dropped
    _parent.Deactivated(connection);
}

// The remote side (process) has been terminated
void SimplePlugin::RemoteConnectionNotification::Terminated(RPC::IRemoteConnection* connection VARIABLE_IS_NOT_USED) /* override */
{
    ASSERT(connection != nullptr);

    TRACE(Trace::Information, ("SimplePlugin::RemoteConnectionNotification::Terminated"));
}

// IRemoteConnection::INotification interface methods
// --------------------------------------------------

// Not Sink templated, hence, a defintion here
uint32_t SimplePlugin::RemoteConnectionNotification::AddRef() const /* override */
{
    return _parent.AddRef();
}

// Not Sink templated, hence, a defintion here
uint32_t SimplePlugin::RemoteConnectionNotification::Release() const /* override */
{
    return _parent.Release();
}

// SimplePluginNotification::INotification interface methods
// ---------------------------------------------------------

SimplePlugin::SimplePluginNotification::SimplePluginNotification(SimplePlugin& parent)
    : Exchange::ISimplePlugin::INotification{}
    , _parent{ parent }
{}

void SimplePlugin::SimplePluginNotification::ProcessEvent(const string& description) /* override */
{
    Exchange::JSimplePlugin::Event::ProcessEvent(_parent, description);
}

// SimplePluginNotification::INotification base IUnknown interface methods
// ------------------------------------------------------------------------

// Not Sink templated, hence, a defintion here
uint32_t SimplePlugin::SimplePluginNotification::AddRef() const /* override */
{
    return _parent.AddRef();
}

// Not Sink templated, hence, a defintion here
uint32_t SimplePlugin::SimplePluginNotification::Release() const /* override */
{
    return _parent.Release();
}

//  IPlugin interface methods
// --------------------------

SimplePlugin::SimplePlugin()
    : PluginHost::IPlugin{}
    , _adminLock{}
    , _service{ nullptr }
    , _pluginNotification{ *this }
    , _remoteConnectionNotification{ *this }
    , _simplePluginNotification{ *this }
    , _simplePluginImplementation{ nullptr }
    , _connectionId{ 0 }
{}

const string SimplePlugin::Initialize(PluginHost::IShell* service) /* override */
{
    string result;

    ASSERT(_service == nullptr);
    ASSERT(_connectionId == 0);
    ASSERT(_simplePluginImplementation == nullptr);

    ASSERT(service != nullptr);

    _service = service;

    if (   _service != nullptr
        && _service->AddRef() == Core::ERROR_NONE
    ) {
        // Register for global (event) notifications related to all (IPlugin) plugins
        /* void */ _service->Register(static_cast<PluginHost::IPlugin::INotification*>(&_pluginNotification));

        // Register for global (event) notifications related to all (IRemoteConnection) s
        /* void */ _service->Register(static_cast<RPC::IRemoteConnection::INotification*>(&_remoteConnectionNotification));

        // Run the out-of-process capable part out-of-process if the configuration has been set alike
        // See SimplePlugin.conf.in

        // Generic check if there exists a provider for the interface
        // Automagically AddRef'ed but not Released

        auto implementation = [this](const std::string& className, uint32_t waitTime, uint32_t version) -> Exchange::ISimplePlugin*
        {
            Exchange::ISimplePlugin* simplePluginImplementation = nullptr;

            if (simplePluginImplementation == nullptr) {
                // None exist so let's become a provider by firing up the out-of-process part 

                // Automagically AddRef'ed but not Released
                simplePluginImplementation = this->_service->Root<Exchange::ISimplePlugin>(this->_connectionId, waitTime, className.c_str(), version);
            } else if (simplePluginImplementation != nullptr) {
                // Round brackets are needed to let the macro work
                TRACE(Trace::Information, ("An Exchange::ISimplePlugin provider already exist"));
            }

            return simplePluginImplementation;
        };

        // IShell allows only one instantiation
//TODO: waitTime
        if ((_simplePluginImplementation = implementation(Classname() + "Implementation", 1000, ~0)) == nullptr) {
            result = "Unable to obtain an Exchange::ISimplePlugin provider";
        } else {
            VARIABLE_IS_NOT_USED uint32_t result = _simplePluginImplementation->Register(&_simplePluginNotification);
            ASSERT(result == Core::ERROR_NONE);

            // ISimplePlugin::INotification becomes accessible via JSONRPC
            /* void */ Exchange::JSimplePlugin::Register(*this, _simplePluginImplementation);

            // Round brackets are needed to let the macro work
            TRACE(Trace::Information, ("SimplePluginImplementation has become an Exchange::ISimplePlugin provider"));
        }
    } else {
        result = "Unable to register for IPlugin::INotification";
    }

    return result;
}

void SimplePlugin::Deinitialize(PluginHost::IShell* service) /* override */
{
    ASSERT(_service == service);

    // Avoid race-condition before the 'implementation / out-of-process' side is released
    RPC::IRemoteConnection* connection{ nullptr };

    if (_service != nullptr) {
        /* void */ _service->Unregister(static_cast<PluginHost::IPlugin::INotification*>(&_pluginNotification));

        // No need to monitor a possible dropped connection as this plugin soon will be deactivated and the out-of-process side as well
        /* void */ _service->Unregister(static_cast<RPC::IRemoteConnection::INotification*>(&_remoteConnectionNotification));

        // Get the handle of the  connection before realisng the out-of-process side
        // A non-null indicates the remote part is still reachable 
        // It is AddRef-ed
        connection = service->RemoteConnection(_connectionId);
    }

    if (_simplePluginImplementation != nullptr) {
        uint32_t result = _simplePluginImplementation->Unregister(&_simplePluginNotification);
        ASSERT(result == Core::ERROR_NONE);

        result = _simplePluginImplementation->Release();

        // The last reference has been released and the object is destructed as it is supposed to
        // Expect the out-of-process side to be destructed
        ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED || result == Core::ERROR_CONNECTION_CLOSED);

        _simplePluginImplementation = nullptr;
    }

    SleepMs(100);

    if (_service != nullptr) {
        // Notifications have been disabled which make it safe to terminate the out-of-process part without being informed

        // Inform the framework that it could kill the out-of-process part if it keeps running
        if (connection != nullptr) {
            /* void */ connection->Terminate();

            VARIABLE_IS_NOT_USED uint32_t result = connection->Release();
            ASSERT(result == Core::ERROR_NONE);
        }

        // Do no longer hold a reference to _service
        VARIABLE_IS_NOT_USED uint32_t result = _service->Release();
        ASSERT(result == Core::ERROR_NONE);

        _connectionId = 0;
        _service = nullptr;
    }
}

string SimplePlugin::Information() const /* override */
{
    return string{};
}

//  Custom (added) methods
// -----------------------

// Required to do some connection ID checking
void SimplePlugin::Deactivated(const RPC::IRemoteConnection* connection) const
{
    if (   connection != nullptr
        && connection->Id() == _connectionId
    ) {
        // Inform the framework that this plugin requires deactivation as the connection to the out-of-process part has unexpectedly dropped

        ASSERT(_service != nullptr);

        if (_service != nullptr) {
            // Let the framework decide when to complete the job
            // Internally creates the object and makes it refcounted, see ProxyType
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
}

} } // namespace Thunder::Plugin
