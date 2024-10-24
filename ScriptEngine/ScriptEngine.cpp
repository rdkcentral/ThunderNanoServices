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
 
#include "ScriptEngine.h"
#include <interfaces/IConfiguration.h>
#include <interfaces/json/JScriptEngine.h>

namespace Thunder {
namespace ScriptEngine {

class MemoryObserverImpl: public Exchange::IMemory {
public:
    MemoryObserverImpl();
    MemoryObserverImpl(MemoryObserverImpl&&);
    MemoryObserverImpl(const MemoryObserverImpl&);
    MemoryObserverImpl& operator=(const MemoryObserverImpl&);

    MemoryObserverImpl(const RPC::IRemoteConnection* connection) :
        _main(connection == nullptr ? Core::ProcessInfo().Id() : connection->RemoteId()) {
    }
    ~MemoryObserverImpl() override = default;

public:
    uint64_t Resident() const override {
        return _main.Resident();
    }
    uint64_t Allocated() const override {
        return _main.Allocated();
    }
    uint64_t Shared() const override {
        return _main.Shared();
    }
    uint8_t Processes() const override {
        return (IsOperational() ? 1 : 0);
    }
    bool IsOperational() const override {
        return _main.IsActive();
    }

    BEGIN_INTERFACE_MAP(MemoryObserverImpl)
	INTERFACE_ENTRY (Exchange::IMemory)
    END_INTERFACE_MAP

private:
    Core::ProcessInfo _main;
};

Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection)
{
    ASSERT(connection != nullptr);
    Exchange::IMemory* result = Core::ServiceType<MemoryObserverImpl>::Create<Exchange::IMemory>(connection);
    return (result);
}

}

namespace Plugin {

    namespace {

        static Metadata<ScriptEngine> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }



static Core::ProxyPoolType<Web::TextBody> _textBodies(2);
static Core::ProxyPoolType<Web::JSONBodyType<ScriptEngine::Data>> jsonBodyDataFactory(2);

const string ScriptEngine::Initialize(PluginHost::IShell *service)
{
    Config config;
    string message;

    ASSERT(service != nullptr);
    ASSERT(_service == nullptr);
    ASSERT(_connectionId == 0);
    ASSERT(_scriptEngine == nullptr);
    ASSERT(_memory == nullptr);

    config.FromString(service->ConfigLine());

    _service = service;
    _service->AddRef();

    // Register the Connection::Notification stuff. The Remote process might die
    // before we get a
    // change to "register" the sink for these events !!! So do it ahead of
    // instantiation.
    _service->Register(&_notification);

    Plugin::Config::RootConfig rootConfig(service);
    const uint32_t permission = (Core::File::USER_READ | Core::File::USER_WRITE |
                                 Core::File::GROUP_READ | Core::File::GROUP_WRITE);

    if (_service->EnablePersistentStorage(permission, rootConfig.User.Value(), rootConfig.Group.Value()) != Core::ERROR_NONE) {
        message = _T("Could not setup persistent path: ") + service->PersistentPath();
    } 
    else {
        _scriptEngine = _service->Root<Exchange::IScriptEngine> (_connectionId, 2000, _T("ScriptEngineImplementation"));

        if (_scriptEngine != nullptr) {
            _scriptEngine->Register(&_notification);
            Exchange::IConfiguration* configuration(_scriptEngine->QueryInterface<Exchange::IConfiguration>());

            if (configuration == nullptr) {
                message = _T("ScriptEngine IConfiguration could not be Obtained.");
            } else {
                configuration->Configure(_service);
                configuration->Release();
                Exchange::JScriptEngine::Register(*this, _scriptEngine);

                RPC::IRemoteConnection* remoteConnection = _service->RemoteConnection(_connectionId);
                if (remoteConnection != nullptr) {
                    _memory = Thunder::ScriptEngine::MemoryObserver(remoteConnection);
                    ASSERT(_memory != nullptr);
                    remoteConnection->Release();
                }
            }
        } 
        else {
            message = _T("ScriptEngine could not be instantiated.");
        }
    }

    return message;
}

void ScriptEngine::Deinitialize(PluginHost::IShell *service VARIABLE_IS_NOT_USED)
{
    if (_service != nullptr) {
        ASSERT(_service == service);
        _service->Unregister(&_notification);

        if (_scriptEngine != nullptr) {
            Exchange::JScriptEngine::Unregister(*this);
            _scriptEngine->Unregister(&_notification);

            if (_memory != nullptr) {
                _memory->Release();
                _memory = nullptr;
            }

            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
            VARIABLE_IS_NOT_USED uint32_t result = _scriptEngine->Release();
            _scriptEngine = nullptr;
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

            // The connection can disappear in the meantime...
            if (connection != nullptr) {
                // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                connection->Terminate();
                connection->Release();
            }
        }
        _service->Release();
        _service = nullptr;
        _connectionId = 0;
    }
}

string ScriptEngine::Information() const
{
    // No additional info to report.
    return (string());
}

void ScriptEngine::URLChanged(const string &URL)
{
    Exchange::JScriptEngine::Event::URLChanged(*this, URL);
}

void ScriptEngine::Deactivated(RPC::IRemoteConnection *connection)
{
    if (connection->Id() == _connectionId) {

        ASSERT(_service != nullptr);
        Core::IWorkerPool::Instance().Submit(
                PluginHost::IShell::Job::Create(_service,
                        PluginHost::IShell::DEACTIVATED,
                        PluginHost::IShell::FAILURE));
    }
}
}
} // namespace
