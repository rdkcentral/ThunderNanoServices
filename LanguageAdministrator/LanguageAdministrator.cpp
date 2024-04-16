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

#include "LanguageAdministrator.h"

namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<LanguageAdministrator> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { subsystem::INTERNET },
            // Terminations
            {},
            // Controls
            {}
        );
    }


    /* virtual */ const string LanguageAdministrator::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);
        ASSERT(_connectionId == 0);
        ASSERT(_impl == nullptr);

        string message;

        _service = service;
        _service->AddRef();

        if (Core::Directory(_service->PersistentPath().c_str()).CreatePath()) {
            _langSettingsFileName = _service->PersistentPath() + "LanguageSettings.json";
        }
        _language = GetCurrentLanguageFromPersistentStore();
        // Setup skip URL for right offset.
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

        _service->Register(static_cast<RPC::IRemoteConnection::INotification*>(&_sink));
        _service->Register(static_cast<PluginHost::IPlugin::INotification*>(&_sink));

        _impl = _service->Root<Exchange::ILanguageTag>(_connectionId, 2000, _T("LanguageAdministratorImpl"));
        if (_impl == nullptr) {
            message = _T("Couldn't create instance of LanguageAdministratorImpl");
        } else {
            _impl->Register(&_LanguageTagNotification);
            Exchange::JLanguageTag::Register(*this, _impl);
            _impl->Language((const string)(_language));
        }

        return message;
    }

    /* virtual */ void LanguageAdministrator::Deinitialize(PluginHost::IShell* service)
    {
        if (_service != nullptr) {
            ASSERT(_service == service);
            service->Unregister(static_cast<RPC::IRemoteConnection::INotification*>(&_sink));
            service->Unregister(static_cast<PluginHost::IPlugin::INotification*>(&_sink));

            UpdateLanguageUsed(_language);

            if(_impl != nullptr) {

                Exchange::JLanguageTag::Unregister(*this);
                _impl->Unregister(&_LanguageTagNotification);
                // Stop processing:
                RPC::IRemoteConnection* connection = service->RemoteConnection(_connectionId);

                VARIABLE_IS_NOT_USED uint32_t result = _impl->Release();
                _impl = nullptr;
                // It should have been the last reference we are releasing,
                // so it should endup in a DESTRUCTION_SUCCEEDED, if not we
                // are leaking...
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
                // If this was running in a (container) process...
                if (connection != nullptr) {
                    // Lets trigger the cleanup sequence for
                    // out-of-process code. Which will guard
                    // that unwilling processes, get shot if
                    // not stopped friendly :-)
                    connection->Terminate();
                    connection->Release();
                }
            }
            _service->Release();
            _service = nullptr;
            _connectionId = 0;
        }
    }

    /* virtual */ string LanguageAdministrator::Information() const
    {
        // No additional info to report.
        return {};
    }


    void LanguageAdministrator::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
        }
    }

    void LanguageAdministrator::Activated(const string& callsign, PluginHost::IShell* plugin)
    {
        _lock.Lock();
        TRACE(Trace::Information , (_T("LanguageAdministrator::Activated Called")));
        if (0 == _appMap.count(callsign)) {
            Exchange::IApplication * app(plugin->QueryInterface<Exchange::IApplication>());

            if (app != nullptr) {
                _appMap.emplace(make_pair(callsign, app));
            }
        }
        _lock.Unlock();

    }

    void LanguageAdministrator::Deactivated(const string& callsign, PluginHost::IShell* plugin VARIABLE_IS_NOT_USED)
    {
        _lock.Lock();
        TRACE(Trace::Information , (_T("LanguageAdministrator::Deactivated Called")));
        if (_appMap.count(callsign)) {
            _appMap[callsign]->Release();
            _appMap.erase(callsign);
        }
        _lock.Unlock();
    }

    void LanguageAdministrator::NotifyLanguageChangesToApps(const string& language)
    {
        _lock.Lock();
        _language = language;
        for (const auto& val: _appMap) {
            (val.second)->Language(language);
        }
        _lock.Unlock();
    }

    void LanguageAdministrator::TerminateConnection(uint32_t connectionId)
    {
        if (connectionId != 0) {
            RPC::IRemoteConnection* connection(_service->RemoteConnection(connectionId));
            if (connection != nullptr) {
                connection->Terminate();
                connection->Release();
            }
        }
    }

    string LanguageAdministrator::GetCurrentLanguageFromPersistentStore()
    {
        Config config;

        //Check if the file exist
        Core::File configFile(_langSettingsFileName);
        if (configFile.Open(true) == true) {
            Core::OptionalType<Core::JSON::Error> error;
            config.IElement::FromFile(configFile, error);
            if (error.IsSet() == true) {
                SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                // Read default language from plugin config file
                config.FromString(_service->ConfigLine());
            }
        }
        else {
            // Create an empty file. Settings will be updated and written to the file on deactivation.
            configFile.Create();
        }

        return config.LanguageTag.Value();
    }

    void LanguageAdministrator::UpdateLanguageUsed(const string& language)
    {
        Config config;
        Core::File configFile(_langSettingsFileName);
        if (configFile.Open(false) == true) {
            config.LanguageTag=language;
            config.IElement::ToFile(configFile);
        }
    }
} //namespace Plugin
} // namespace Thunder
