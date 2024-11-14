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
 
#include "Module.h"
#include "Implementation.h"

#include <interfaces/IScriptEngine.h>
#include <interfaces/IConfiguration.h>

const TCHAR script[] = _T("console.log('hello super duper 4, world!')");

namespace Thunder {
namespace Plugin {

class ScriptEngineImplementation
    : public Exchange::IScriptEngine
    , public Exchange::IConfiguration
    , public Core::Thread {
private:
    class Config: public Core::JSON::Container {
    public:
        Config(Config&&) = delete;
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

        Config()
            : Core::JSON::Container()
            , Url() {
            Add(_T("url"), &Url);
        }
        ~Config() override = default;

    public:
        Core::JSON::String Url;
    };

    using Observers = std::vector<Exchange::IScriptEngine::INotification*>;

public:
    ScriptEngineImplementation(ScriptEngineImplementation&&) = delete;
    ScriptEngineImplementation(const ScriptEngineImplementation&) = delete;
    ScriptEngineImplementation& operator=(const ScriptEngineImplementation&) = delete;

    ScriptEngineImplementation()
        : _url()
        , _adminLock()
        , _clients()
        , _platform(nullptr)
        , _service(nullptr) {
    }
    ~ScriptEngineImplementation() override {
        if (_service) {
            _service->Release();
            _service = nullptr;
        }

        if (_platform != nullptr) {
            Core::Thread::Wait((Core::Thread::BLOCKED | Core::Thread::STOPPED), Core::infinite);

            script_destroy_platform(_platform);
        }
    }

public:
    //
    // IConfiguration members
    // ---------------------------------------------------------------------------------
    uint32_t Configure(PluginHost::IShell* service) override {
        uint32_t result = Core::ERROR_NONE;

        ASSERT((_service == nullptr) && (service != nullptr));
        ASSERT(_platform == nullptr);

        Config config;
        string info(service->ConfigLine());
        config.FromString(info);
        Core::SystemInfo::SetEnvironment(_T("HOME"), service->PersistentPath());

        if (config.Url.IsSet() == true) {
            _url = config.Url.Value();
        }

        std::string converted(Core::ToString(info));
        _platform = script_create_platform(converted.c_str());

        if (_platform != nullptr) {
            _service = service;
            _service->AddRef();

            Run();
        }
 
        return (result);
    }

    //
    // IConfiguration members
    // ---------------------------------------------------------------------------------
    Core::hresult Register(INotification* sink /* @in */) /* override */ {
        ASSERT (sink != nullptr);

        _adminLock.Lock();

        // Make sure a sink is not registered multiple times.
        ASSERT(std::find(_clients.begin(), _clients.end(), sink) == _clients.end());

        _clients.push_back(sink);
        sink->AddRef();

        _adminLock.Unlock();

	return (Core::ERROR_NONE); 
    }
    Core::hresult Unregister(const INotification* sink /* @in */) /* override */ {

        ASSERT (sink != nullptr);

        _adminLock.Lock();

        Observers::iterator index(std::find(_clients.begin(), _clients.end(), sink));

        // Make sure you do not unregister something you did not register !!!
        ASSERT(index != _clients.end());

        if (index != _clients.end()) {
            (*index)->Release();
            _clients.erase(index);
        }

        _adminLock.Unlock();

	return (Core::ERROR_NONE); 
    }

    // @property
    // @brief Script to be loaded into the engine and to be executed.
    // @param url Loaded URL (e.g. https://example.com)
    Core::hresult URL(string& url /* @out */) const /* override */ {
        url = _url;
        return (Core::ERROR_NONE);
    }
    Core::hresult URL(const string& url) /* override */ {
        // Get the contents associated with the URL:
        Core::URL info(url);

        if (info.IsValid() == true) {
            if (info.Type() == Core::URL::SchemeType::SCHEME_FILE) {

            }
            _url = url;
        }

        return (Core::ERROR_NONE);
    }

    BEGIN_INTERFACE_MAP (ScriptEngineImplementation)
        INTERFACE_ENTRY (Exchange::IScriptEngine)
        INTERFACE_ENTRY (Exchange::IConfiguration)
    END_INTERFACE_MAP

private:
    uint32_t Worker() override {
        if (IsRunning() == true) {
            ASSERT(_platform != nullptr);

            uint32_t length = ( (sizeof(script)/sizeof(TCHAR)) - 1);

            if (script_prepare(_platform, length, script) == Core::ERROR_NONE) {
                script_execute(_platform);
            }
        }

        Block();
        // Do plugin de-activation
        return (Core::infinite);
    }

private:
    string _url;
    mutable Core::CriticalSection _adminLock;
    Observers _clients;
    platform* _platform;
    PluginHost::IShell* _service;
};

SERVICE_REGISTRATION(ScriptEngineImplementation, 1, 0);

} // namespace Plugin 
} // namespace
