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
 
#include "Commander.h"
#include "Commands.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Plugin::Commander::state)

    { Plugin::Commander::state::ABORTING, _TXT("aborting") },
    { Plugin::Commander::state::IDLE, _TXT("idle") },
    { Plugin::Commander::state::LOADED, _TXT("loaded") },
    { Plugin::Commander::state::RUNNING, _TXT("running") },

    ENUM_CONVERSION_END(Plugin::Commander::state);

namespace Plugin {

    SERVICE_REGISTRATION(Commander, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<Core::JSON::String>>> jsonBodyDataFactory(1);
    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<Commander::Data>>> jsonBodyArrayDataFactory(2);
    static Core::ProxyPoolType<Web::JSONBodyType<Commander::Data>> jsonBodySingleDataFactory(2);
    static Core::ProxyPoolType<Web::JSONBodyType<Core::JSON::ArrayType<Commander::Command>>> jsonBodyArrayCommandFactory(2);

    Commander::Commander()
        : _skipURL(0)
        , _service(nullptr)
        , _commandAdministrator()
        , _sequencers()
    {

        Register<Plugin::Command::PluginControl>();
        Register<Plugin::Command::PluginObserver>();
    }

    /* virtual */ Commander::~Commander()
    {
        Unregister<Plugin::Command::PluginControl>();
        Unregister<Plugin::Command::PluginObserver>();
    }

    /* virtual */ const string Commander::Initialize(PluginHost::IShell* service)
    {
        ASSERT(_service == nullptr);
        _service = service;

        // Setup skip URL for right offset.
        _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());
        Config config;
        config.FromString(service->ConfigLine());

        auto index(config.Sequencers.Elements());

        while (index.Next() == true) {

            // Create all sequencers defined in the config
            _sequencers.insert(std::pair<const string, Core::ProxyType<Sequencer>>(
                index.Current().Value(),
                Core::ProxyType<Sequencer>::Create(
                    index.Current().Value(),
                    &_commandAdministrator,
                    _service)));
        }

        // On succes return "".
        return _T("");
    }

    /* virtual */ void Commander::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);

        // Stop all running sequencers..
        std::map<const string, Core::ProxyType<Sequencer>>::iterator index(_sequencers.begin());

        while (index != _sequencers.end()) {

            Core::ProxyType<Core::IDispatchType<void>> job(Core::proxy_cast<Core::IDispatchType<void>>(index->second));

            index->second->Abort();
            Core::IWorkerPool::Instance().Revoke(job);

            index++;
        }

        // Kill all sequencer instances.
        _sequencers.clear();

        // Deinitialize what we initialized..
        _service = nullptr;
    }

    /* virtual */ string Commander::Information() const
    {
        // No additional info to report.
        return (nullptr);
    }

    /* virtual */ void Commander::Inbound(Web::Request& request)
    {
        // If it is a put, we expect a list of commands, receive them !!!!
        if (request.Verb == Web::Request::HTTP_PUT) {
            request.Body(jsonBodyArrayCommandFactory.Element());
        }
    }

    // GET: ../Sequencer/[SequencerName]	; Return [ALL] available sequencers and their current state
    // GET: ../Commands						; Return all possible commmands

    /* virtual */ Core::ProxyType<Web::Response> Commander::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        // Proxy object for response type.
        Core::ProxyType<Web::Response> response(PluginHost::IFactories::Instance().Response());

        // Decode request path.
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint16_t>(request.Path.length()) - _skipURL), false, '/');

        // Get first plugin verb.
        index.Next();

        if (request.Verb == Web::Request::HTTP_GET) {
            // Here we might want to return the list of sequencers and their state.
            if (true != index.Next()) {

                response->ErrorCode = Web::STATUS_BAD_REQUEST;
                response->Message = _T("Missing Sequencer name or not existing sequencer");
            } else if (index.Current() == _T("Sequencer")) {

                if (index.Next() == true) {
                    // Seems like we are looking for a specific sequencer..
                    if (_sequencers.find(index.Current().Text()) == _sequencers.end()) {

                        response->ErrorCode = Web::STATUS_BAD_REQUEST;
                        response->Message = _T("Not existing sequencer");
                    } else {
                        Core::ProxyType<Web::JSONBodyType<Commander::Data>> data(jsonBodySingleDataFactory.Element());

                        (*data) = MetaData(*_sequencers[index.Current().Text()]);

                        response->ErrorCode = Web::STATUS_OK;
                        response->Message = "OK";
                        response->Body(Core::proxy_cast<Web::IBody>(data));
                    }
                } else {
                    Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<Commander::Data>>> data(jsonBodyArrayDataFactory.Element());
                    std::map<const string, Core::ProxyType<Sequencer>>::iterator index(_sequencers.begin());

                    while (index != _sequencers.end()) {

                        data->Add(MetaData(*(index->second)));

                        index++;
                    }

                    response->ErrorCode = Web::STATUS_OK;
                    response->Message = "OK";
                    response->Body(Core::proxy_cast<Web::IBody>(data));
                }
            } else if (index.Current() == _T("Commands")) {

                // Make sure the "elemenyt is ste flag" is active.
                Core::JSON::String newValue;

                Core::ProxyType<Web::JSONBodyType<Core::JSON::ArrayType<Core::JSON::String>>> data(jsonBodyDataFactory.Element());

                auto index(_commandAdministrator.Commands());

                while (index.Next() == true) {

                    newValue = index.Key();
                    data->Add(newValue);
                }

                response->ErrorCode = Web::STATUS_OK;
                response->Message = "OK";
                response->Body(Core::proxy_cast<Web::IBody>(data));
            }
        } else if (request.Verb == Web::Request::HTTP_DELETE) {
            if ((true != index.Next()) || (_sequencers.find(index.Current().Text()) == _sequencers.end())) {

                response->ErrorCode = Web::STATUS_BAD_REQUEST;
                response->Message = _T("Missing Sequencer name or not existing sequencer");
            } else {
                // Current name, is the name of the sequencer
                Core::ProxyType<Sequencer> sequencer(_sequencers[index.Current().Text()]);
                Core::ProxyType<Core::IDispatchType<void>> job(Core::proxy_cast<Core::IDispatchType<void>>(sequencer));

                if (sequencer->Abort() != Core::ERROR_NONE) {
                    response->ErrorCode = Web::STATUS_NO_CONTENT;
                    response->Message = _T("Sequencer was not in a running state");
                } else if (Core::IWorkerPool::Instance().Revoke(job, 2000) == Core::ERROR_NONE) {
                    response->ErrorCode = Web::STATUS_OK;
                    response->Message = _T("Sequencer available for next sequence");
                } else {
                    response->ErrorCode = Web::STATUS_REQUEST_TIME_OUT;
                    response->Message = _T("Sequencer did not stop in time (2S)");
                }
            }
        } else if (request.Verb == Web::Request::HTTP_PUT) {

            if ((true != index.Next()) || (request.HasBody() == false) || (_sequencers.find(index.Current().Text()) == _sequencers.end())) {

                response->ErrorCode = Web::STATUS_BAD_REQUEST;
                response->Message = _T("Missing Sequencer name, body or invalid sequencer name");
            } else {
                // Current name, is the name of the sequencer
                Core::ProxyType<Sequencer> sequencer(_sequencers[index.Current().Text()]);
                Core::ProxyType<Core::IDispatchType<void>> job(Core::proxy_cast<Core::IDispatchType<void>>(sequencer));

                if (sequencer->IsActive() == true) {
                    response->ErrorCode = Web::STATUS_TEMPORARY_REDIRECT;
                    response->Message = _T("Sequencer already running");
                } else {
                    sequencer->Load(*(request.Body<Web::JSONBodyType<Core::JSON::ArrayType<Commander::Command>>>()));
                    sequencer->Execute();

                    Core::IWorkerPool::Instance().Submit(job);

                    // Attach to response.
                    response->Message = _T("Sequence List Imported");
                    response->ErrorCode = Web::STATUS_OK;
                }
            }
        }

        return (response);
    }

    Commander::Data Commander::MetaData(const Commander::Sequencer& sequencer)
    {

        Commander::Data data;

        data.Sequencer = sequencer.Name();
        data.State = sequencer.State();

        if (sequencer.State() == Commander::state::RUNNING) {

            data.Label = sequencer.Label();
            data.Index = sequencer.Index();
        }

        return (data);
    }

} // Namespace Plugin.
}
