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
#include <interfaces/IScriptEngine.h>
#include <interfaces/IMemory.h>

namespace Thunder {
namespace Plugin {

class ScriptEngine: public PluginHost::IPlugin, public PluginHost::JSONRPC {
private:
    class Notification
        : public RPC::IRemoteConnection::INotification
        , public Exchange::IScriptEngine::INotification {
    public:
        Notification() = delete;
        Notification(Notification&&) = delete;
        Notification(const Notification&) = delete;
        Notification& operator=(const Notification&) = delete;

    public:
        explicit Notification(ScriptEngine& parent)
            : _parent(parent) {
        }
        ~Notification() override = default;

    public:
        BEGIN_INTERFACE_MAP (Notification)
            INTERFACE_ENTRY (Exchange::IScriptEngine::INotification)
            INTERFACE_ENTRY (RPC::IRemoteConnection::INotification)
        END_INTERFACE_MAP

    private:
        //
        // Exchange::IScriptEngine::INotification
        // ------------------------------------------------------------------
        void URLChanged(const string& URL) override {
            _parent.URLChanged(URL);
        }

        //
        // RPC::IRemoteConnection::INotification
        // ------------------------------------------------------------------
        void Activated(RPC::IRemoteConnection*) override {
        }
        void Deactivated(RPC::IRemoteConnection *connection) override {
            _parent.Deactivated(connection);
        }
        void Terminated(RPC::IRemoteConnection*) override {
        }

    private:
        ScriptEngine& _parent;
    };

public:
    class Data: public Core::JSON::Container {
    public:
        Data(Data&&) = delete;
        Data(const Data&) = delete;
        Data& operator=(const Data&) = delete;

        Data()
            : Core::JSON::Container()
            , URL() {
            Add(_T("url"), &URL);
        }
        ~Data() override = default;

    public:
        Core::JSON::String URL;
    };

public:
    ScriptEngine(ScriptEngine&&);
    ScriptEngine(const ScriptEngine&);
    ScriptEngine& operator=(const ScriptEngine&);
    ScriptEngine()
        : _skipURL(0)
        , _connectionId(0)
        , _scriptEngine(nullptr)
        , _memory(nullptr)
        , _service(nullptr)
        , _notification(*this) {
    }
    ~ScriptEngine() override = default;

public:
    BEGIN_INTERFACE_MAP (ScriptEngine)
        INTERFACE_ENTRY (PluginHost::IPlugin)
        INTERFACE_ENTRY (PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::IScriptEngine, _scriptEngine)
        INTERFACE_AGGREGATE(Exchange::IMemory, _memory)
    END_INTERFACE_MAP

public:
    //  IPlugin methods
    // -------------------------------------------------------------------------------------------------------
    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell *service) override;
    string Information() const override;

private:
    void Deactivated(RPC::IRemoteConnection *connection);
    void URLChanged(const string &URL);

private:
    uint8_t _skipURL;
    uint32_t _connectionId;
    Exchange::IScriptEngine* _scriptEngine;
    Exchange::IMemory* _memory;
    PluginHost::IShell* _service;
    Core::SinkType<Notification> _notification;
};
}
} // namespace
