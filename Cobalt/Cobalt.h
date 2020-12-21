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
#include <interfaces/IBrowser.h>
#include <interfaces/IApplication.h>
#include <interfaces/IMemory.h>
#include <interfaces/json/JsonData_Browser.h>
#include <interfaces/json/JsonData_StateControl.h>

namespace WPEFramework {
namespace Plugin {

class Cobalt: public PluginHost::IPlugin,
        public PluginHost::IWeb,
        public PluginHost::JSONRPC {
private:
    Cobalt(const Cobalt&);
    Cobalt& operator=(const Cobalt&);

    class Notification: public RPC::IRemoteConnection::INotification,
            public PluginHost::IStateControl::INotification,
            public Exchange::IBrowser::INotification {

    private:
        Notification() = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

    public:
        explicit Notification(Cobalt *parent) :
                _parent(*parent) {
            ASSERT(parent != nullptr);
        }
        ~Notification() {
        }

    public:
        BEGIN_INTERFACE_MAP (Notification)
        INTERFACE_ENTRY (PluginHost::IStateControl::INotification)
        INTERFACE_ENTRY (Exchange::IBrowser::INotification)
        INTERFACE_ENTRY (RPC::IRemoteConnection::INotification)END_INTERFACE_MAP

    private:
        virtual void StateChange(
                const PluginHost::IStateControl::state state) override {
            _parent.StateChange(state);
        }
        // Signal changes on the subscribed namespace..
        virtual void LoadFinished(const string &URL) override {
            _parent.LoadFinished(URL);
        }
        virtual void URLChanged(const string &URL) override {
            _parent.URLChanged(URL);
        }
        virtual void Hidden(const bool hidden) override {
            _parent.Hidden(hidden);
        }
        virtual void Closure() override
        {
        }
        virtual void Activated(RPC::IRemoteConnection*) override
        {
        }
        virtual void Deactivated(RPC::IRemoteConnection *connection) override
        {
            _parent.Deactivated(connection);
        }

    private:
        Cobalt &_parent;
    };

public:
    class Data: public Core::JSON::Container {
    private:
        Data(const Data&) = delete;
        Data& operator=(const Data&) = delete;

    public:
        Data() :
            Core::JSON::Container(), URL(),
                  FPS(), Suspended(false), Hidden(false) {
            Add(_T("url"), &URL);
            Add(_T("fps"), &FPS);
            Add(_T("suspended"), &Suspended);
            Add(_T("hidden"), &Hidden);
        }
        ~Data() {
        }

    public:
        Core::JSON::String URL;
        Core::JSON::DecUInt32 FPS;
        Core::JSON::Boolean Suspended;
        Core::JSON::Boolean Hidden;
    };

public:
    Cobalt() :
            _skipURL(0), _hidden(false), _cobalt(nullptr), _application(nullptr),
            _memory(nullptr), _service(nullptr), _notification(this) {
        RegisterAll();
    }
    virtual ~Cobalt() {
        UnregisterAll();
    }

public:
    BEGIN_INTERFACE_MAP (Cobalt)
    INTERFACE_ENTRY (PluginHost::IPlugin)
    INTERFACE_ENTRY (PluginHost::IWeb)
    INTERFACE_ENTRY (PluginHost::IDispatcher)INTERFACE_AGGREGATE(PluginHost::IStateControl, _cobalt)
    INTERFACE_AGGREGATE(Exchange::IBrowser, _cobalt)
    INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
    END_INTERFACE_MAP

public:
    //  IPlugin methods
    // -------------------------------------------------------------------------------------------------------
    virtual const string Initialize(PluginHost::IShell* service);
    virtual void Deinitialize(PluginHost::IShell *service);
    virtual string Information() const;

    //  IWeb methods
    // -------------------------------------------------------------------------------------------------------
    virtual void Inbound(Web::Request &request);
    virtual Core::ProxyType<Web::Response> Process(const Web::Request &request);

private:
    void Deactivated(RPC::IRemoteConnection *connection);
    void StateChange(const PluginHost::IStateControl::state state);
    void LoadFinished(const string &URL);
    void URLChanged(const string &URL);
    void Hidden(const bool hidden);
    void Closure();

    inline void ConnectionTermination(uint32_t connectionId)
    {
        if (connectionId != 0) {
            RPC::IRemoteConnection* connection(_service->RemoteConnection(connectionId));
            if (connection != nullptr) {
                connection->Terminate();
                connection->Release();
            }
        }
    }

    // JsonRpc
    void RegisterAll();
    void UnregisterAll();
    uint32_t get_url(Core::JSON::String &response) const; // Browser
    uint32_t set_url(const Core::JSON::String &param); // Browser
    uint32_t get_visibility(
            Core::JSON::EnumType<JsonData::Browser::VisibilityType> &response) const; // Browser
    uint32_t set_visibility(
            const Core::JSON::EnumType<JsonData::Browser::VisibilityType> &param); // Browser
    uint32_t get_fps(Core::JSON::DecUInt32 &response) const; // Browser
    uint32_t get_state(
            Core::JSON::EnumType<JsonData::StateControl::StateType> &response) const; // StateControl
    uint32_t set_state(
            const Core::JSON::EnumType<JsonData::StateControl::StateType> &param); // StateControl
    uint32_t endpoint_delete(const JsonData::Browser::DeleteParamsData& params);
    uint32_t delete_dir(const string& path);
    uint32_t set_deeplink(const Core::JSON::String &param); // Application
    void event_urlchange(const string &url, const bool &loaded); // Browser
    void event_visibilitychange(const bool &hidden); // Browser
    void event_statechange(const bool &suspended); // StateControl

private:
    uint8_t _skipURL;
    uint32_t _connectionId;
    bool _hidden;
    Exchange::IBrowser *_cobalt;
    Exchange::IApplication *_application;
    Exchange::IMemory *_memory;
    PluginHost::IShell *_service;
    Core::Sink<Notification> _notification;
    string _persistentStoragePath;
};
}
} // namespace
