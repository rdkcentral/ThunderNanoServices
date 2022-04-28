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

#include "PlayerInfo.h"


namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(PlayerInfo, 1, 0);

    static Core::ProxyPoolType<Web::Response> responseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<JsonData::PlayerInfo::CodecsData>> jsonResponseFactory(4);

    /* virtual */ const string PlayerInfo::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        ASSERT(_connectionId == 0);
        ASSERT(_player == nullptr);

        string message;
        Config config;
        config.FromString(service->ConfigLine());
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        _service = service;
        _service->AddRef();
        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_notification);

        _player = service->Root<Exchange::IPlayerProperties>(_connectionId, 2000, _T("PlayerInfoImplementation"));
        if (_player != nullptr) {

            if ((_player->AudioCodecs(_audioCodecs) == Core::ERROR_NONE) && (_audioCodecs != nullptr)) {

                if ((_player->VideoCodecs(_videoCodecs) == Core::ERROR_NONE) && (_videoCodecs != nullptr)) {
                    Exchange::JPlayerProperties::Register(*this, _player);
                    // The code execution should proceed regardless of the _dolbyOut
                    // value, as it is not a essential.
                    // The relevant JSONRPC endpoints will return ERROR_UNAVAILABLE,
                    // if it hasn't been initialized.
#if DOLBY_SUPPORT
                    _dolbyOut = _player->QueryInterface<Exchange::Dolby::IOutput>();
                    if(_dolbyOut == nullptr){
                        SYSLOG(Logging::Startup, (_T("Dolby output switching service is unavailable.")));
                    } else {
                        _dolbyNotification.Initialize(_dolbyOut);
                        Exchange::Dolby::JOutput::Register(*this, _dolbyOut);
                    }
#endif
                } else {
                    message = _T("PlayerInfo Video Codecs not be Loaded.");
                }
            } else {
                message = _T("PlayerInfo Audio Codecs not be Loaded.");
            }
        }
        else {
            message = _T("PlayerInfo could not be instantiated.");
        }

        if(message.length() != 0){
            Deinitialize(service);
        }
        return message;
    }

    /* virtual */ void PlayerInfo::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ASSERT(service == _service);

        _service->Unregister(&_notification);

        if (_player != nullptr) {
            if(_audioCodecs != nullptr && _videoCodecs != nullptr) {
                Exchange::JPlayerProperties::Unregister(*this);
            }
            if (_audioCodecs != nullptr) {
                _audioCodecs->Release();
                _audioCodecs = nullptr;
            }
            if (_videoCodecs != nullptr) {
                _videoCodecs->Release();
                _videoCodecs = nullptr;
            }
            #if DOLBY_SUPPORT
                if (_dolbyOut != nullptr) {
                    _dolbyNotification.Deinitialize();
                    Exchange::Dolby::JOutput::Unregister(*this);
                    _dolbyOut->Release();
                    _dolbyOut = nullptr;
                }
            #endif

            RPC::IRemoteConnection* connection(_service->RemoteConnection(_connectionId));
            VARIABLE_IS_NOT_USED uint32_t result = _player->Release();
            _player = nullptr;
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

    /* virtual */ string PlayerInfo::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void PlayerInfo::Inbound(Web::Request& /* request */)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response> PlayerInfo::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        // By default, we assume everything works..
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";

        // <GET> - currently, only the GET command is supported, returning system info
        if (request.Verb == Web::Request::HTTP_GET) {

            Core::ProxyType<Web::JSONBodyType<JsonData::PlayerInfo::CodecsData>> response(jsonResponseFactory.Element());

            Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');

            // Always skip the first one, it is an empty part because we start with a '/' if there are more parameters.
            index.Next();

            Info(*response);
            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->Body(Core::ProxyType<Web::IBody>(response));
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T("Unsupported request for the [PlayerInfo] service.");
        }

        return result;
    }

    void PlayerInfo::Info(JsonData::PlayerInfo::CodecsData& playerInfo) const
    {
        Core::JSON::EnumType<JsonData::PlayerInfo::CodecsData::AudiocodecsType> audioCodec;
        _audioCodecs->Reset(0);
        Exchange::IPlayerProperties::AudioCodec audio;
        while(_audioCodecs->Next(audio) == true) {
            playerInfo.Audio.Add(audioCodec = static_cast<JsonData::PlayerInfo::CodecsData::AudiocodecsType>(audio));
        }

        Core::JSON::EnumType<JsonData::PlayerInfo::CodecsData::VideocodecsType> videoCodec;
        Exchange::IPlayerProperties::VideoCodec video;
        _videoCodecs->Reset(0);
         while(_videoCodecs->Next(video) == true) {
            playerInfo.Video.Add(videoCodec = static_cast<JsonData::PlayerInfo::CodecsData::VideocodecsType>(video));
        }
    }

    void PlayerInfo::Deactivated(RPC::IRemoteConnection* connection)
    {
        // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
        // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
        if (_connectionId == connection->Id()) {
            ASSERT(_service != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
} // namespace Plugin
} // namespace WPEFramework
