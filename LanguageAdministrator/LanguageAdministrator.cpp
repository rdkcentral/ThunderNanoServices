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
 
#include "LanguageAdministrator.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(LanguageAdministrator, 1, 0);

    /* virtual */ const string LanguageAdministrator::Initialize(PluginHost::IShell* service)
    {
 
        ASSERT(_service == nullptr);
        ASSERT(service != nullptr);

        _service = service;
        _language = GetCurrentLanguageFromPersistentStore();
        // Setup skip URL for right offset.
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

        _service->Register(static_cast<RPC::IRemoteConnection::INotification*>(&_sink));
        _service->Register(static_cast<PluginHost::IPlugin::INotification*>(&_sink));

        string result;

        _impl = _service->Root<Exchange::ILanguageTag>(_connectionId, 2000, _T("LanguageAdministratorImpl"));
        if (_impl == nullptr) {
            _service->Unregister(static_cast<RPC::IRemoteConnection::INotification*>(&_sink));
            _service->Unregister(static_cast<PluginHost::IPlugin::INotification*>(&_sink));
            _service = nullptr;
            ConnectionTermination(_connectionId);
            
            result = _T("Couldn't create instance of LanguageAdministratorImpl");

        } else {
            _impl->Register(&_LanguageTagNotification);
            Exchange::JLanguageTag::Register(*this, _impl);
            _impl->Language((const std::string)(_language));
        }

        return result;
    }

    /* virtual */ void LanguageAdministrator::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        UpdateLanguageUsed(_language);
        Exchange::JLanguageTag::Unregister(*this);

        service->Unregister(static_cast<RPC::IRemoteConnection::INotification*>(&_sink));
        _service->Unregister(static_cast<PluginHost::IPlugin::INotification*>(&_sink));
        _impl->Unregister(&_LanguageTagNotification);

        _impl->Release();
        ConnectionTermination(_connectionId);

        _service = nullptr;
        _impl = nullptr;
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

    void LanguageAdministrator::StateChange(PluginHost::IShell* plugin, const string& callsign) 
    {
        _lock.Lock();

        switch(plugin->State() ) {
            case PluginHost::IShell::ACTIVATED:
                TRACE(Trace::Information , (_T("LanguageAdministrator::StateChange Activated")));
                if (0 == _appMap.count(callsign)) {
                    Exchange::IApplication * app(plugin->QueryInterface<Exchange::IApplication>());

                    if (app != nullptr) {
                        _appMap.emplace(make_pair(callsign, app));
                    }
                }
                break;	
                
            case PluginHost::IShell::DEACTIVATION:
                TRACE(Trace::Information , (_T("LanguageAdministrator::StateChange Deactivation")));
                if (_appMap.count(callsign)) {
                    _appMap[callsign]->Release();
                    _appMap.erase(callsign);
                    
                }
                break;	

            default:
                break;
        }

        _lock.Unlock();
    }

    void LanguageAdministrator::NotifyLanguageChangesToApps(const std::string& language) 
    {
        _lock.Lock(); 
        _language = language;
        for (const auto& val: _appMap) {
            (val.second)->Language(language);
        }

        _lock.Unlock();   
    }

    void LanguageAdministrator::ConnectionTermination(uint32_t connectionId)
    {
        if (connectionId != 0) {
            RPC::IRemoteConnection* connection(_service->RemoteConnection(connectionId));
            if (connection != nullptr) {
                connection->Terminate();
                connection->Release();
            }
        }
    }

    std::string LanguageAdministrator::GetCurrentLanguageFromPersistentStore()
    {
        std::string FileName;
        //std::string language("en-US");
        Config config;

        if (Core::Directory(_service->PersistentPath().c_str()).CreatePath())
            FileName = _service->PersistentPath() + "LanguageSettings.json";
        else {
            SYSLOG(Logging::Startup, ("Config directory %s doesn't exist and could not be created!\n", _service->PersistentPath().c_str()));
            // TBD: What to do here...throw exception
        }

        //Check if the file exist
        Core::File configFile(FileName);
        if (configFile.Open(true) == true) {
            Core::OptionalType<Core::JSON::Error> error;
            config.IElement::FromFile(configFile, error);
            if (error.IsSet() == true) {
                SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                // Read default language from plugin config file
                config.FromString(_service->ConfigLine());
            }
            configFile.Close();
        }
        else {
            // Create an empty file. Settings will be updated and written to the file on deactivation.
            configFile.Create();
            //configFile.Close();
        }

        return config.LanguageTag.Value();
    }

    void LanguageAdministrator::UpdateLanguageUsed(const string& language) 
    {
        Config config;
        std::string FileName =  _service->PersistentPath() + "LanguageSettings.json";
        Core::File configFile(FileName);
        if (configFile.Open(false) == true) {
            config.LanguageTag=language;
            config.IElement::ToFile(configFile);
            //configFile.Close();
        }
    }
} //namespace Plugin
} // namespace WPEFramework
