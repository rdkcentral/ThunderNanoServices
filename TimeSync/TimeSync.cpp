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

#include "TimeSync.h"
#include "NTPClient.h"

namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<TimeSync> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            { subsystem::TIME }
        );
    }

    static Core::ProxyPoolType<Web::Response> responseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<TimeSync::Data<>>> jsonResponseFactory(4);
    static Core::ProxyPoolType<Web::JSONBodyType<TimeSync::SetData<>>> jsonBodyDataFactory(2);

    static const uint16_t NTPPort = 123;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
    TimeSync::TimeSync()
        : _skipURL(0)
        , _periodicity(0)
        , _client(Core::ServiceType<NTPClient>::Create<Exchange::ITimeSync::ISource>())
        , _sink(this)
        , _subSystem(nullptr)
        , _adminLock()
        , _timeSyncObservers()
        , _job(*this)
    {
    }
POP_WARNING()

    /* virtual */ TimeSync::~TimeSync()
    {
        _client->Release();
    }

    /* virtual */ const string TimeSync::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        string message;
        _subSystem = service->SubSystems();
        if (_subSystem != nullptr) {

            Config config;
            config.FromString(service->ConfigLine());
            _skipURL = static_cast<uint16_t>(service->WebPrefix().length());
            _periodicity = config.Periodicity.Value() * 60 /* minutes */ * 60 /* seconds */ * 1000 /* milliSeconds */;
            bool start = (((config.Deferred.IsSet() == true) && (config.Deferred.Value() == true)) == false);

            NTPClient::SourceIterator index(config.Sources.Elements());

            static_cast<NTPClient*>(_client)->Initialize(index, config.Retries.Value(), config.Interval.Value());

            _sink.Initialize(_client, start);

            Exchange::JTimeSync::Register(*this, this);
        }
        else {
            message = _T("Subsystem could not be obtained, TimeSync init failed");
        }

        return message;
    }

    /* virtual */ void TimeSync::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ASSERT(service != nullptr);

        if (_subSystem != nullptr) {

            Exchange::JTimeSync::Unregister(*this);

            _job.Revoke();
            _sink.Deinitialize();

            _subSystem->Release();
            _subSystem = nullptr;
        }
    }

    /* virtual */ string TimeSync::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void TimeSync::Inbound(Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_PUT) {
            request.Body(jsonBodyDataFactory.Element());
        }
    }

    /* virtual */ Core::ProxyType<Web::Response>
    TimeSync::Process(const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        Core::TextSegmentIterator index(
            Core::TextFragment(request.Path, _skipURL, static_cast<uint16_t>(request.Path.length()) - _skipURL),
            false,
            '/');

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = "Unsupported request for the TimeSync service";

        index.Next();

        if (request.Verb == Web::Request::HTTP_GET) {
            Core::ProxyType<Web::JSONBodyType<Data<>>> response(jsonResponseFactory.Element());
            uint64_t syncTime(_client->SyncTime());

            response->TimeSource = _client->Source();
            response->SyncTime = (syncTime == 0 ? _T("invalid time") : Core::Time(syncTime).ToRFC1123(true));

            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->Body(Core::ProxyType<Web::IBody>(response));
            result->ErrorCode = Web::STATUS_OK;
            result->Message = "OK";
        } else if (request.Verb == Web::Request::HTTP_POST) {
            if (index.IsValid() && index.Next()) {
                if (index.Current() == "Sync") {
                    _client->Synchronize();
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = "OK";
                }
            }
        } else if (request.Verb == Web::Request::HTTP_PUT) {
            if (index.IsValid() && index.Next()) {
                if (index.Current() == "Set") {
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = "OK";
                    Core::Time newTime(0);

                    if (request.HasBody()) {
                        Core::JSON::String time = request.Body<const TimeSync::SetData<>>()->Time;
                        if (time.IsSet()) {
                            newTime.FromISO8601(time.Value());
                            if (!newTime.IsValid()) {
                                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                                result->Message = "Invalid time given.";
                            }
                        }
                    }

                    if (result->ErrorCode == Web::STATUS_OK) {
                        // Stop automatic synchronisation
                        _client->Cancel();
                        _job.Revoke();

                        if (newTime.IsValid()) {
                            Core::SystemInfo::Instance().SetTime(newTime);
                        }

                        EnsureSubsystemIsActive();
                    }
                }
            }
        }

        return result;
    }

    void TimeSync::SyncedTime(const uint64_t time)
    {
        Core::Time newTime(time);

        TRACE(Trace::Information, (_T("Syncing time to %s."), newTime.ToRFC1123(false).c_str()));

        Core::SystemInfo::Instance().SetTime(newTime);

        if (_periodicity != 0) {
            Core::Time newSyncTime(Core::Time::Now());

            newSyncTime.Add(_periodicity);

            // Seems we are synchronised with the time. Schedule the next timesync.
            TRACE(Trace::Information, (_T("Waking up again at %s."), newSyncTime.ToRFC1123(false).c_str()));
            _job.Reschedule(newSyncTime);

            NotifyTimeChanged();
        }
    }

    void TimeSync::EnsureSubsystemIsActive()
    {
        ASSERT(_subSystem != nullptr);

        if (_subSystem->IsActive(PluginHost::ISubSystem::TIME) == false) {
            _subSystem->Set(PluginHost::ISubSystem::TIME, _client);
        }
    }

    void TimeSync::NotifyTimeChanged() const
    {
        _adminLock.Lock();
 
        for (auto observer : _timeSyncObservers) {
            observer->TimeChanged();
        }

        _adminLock.Unlock();

        Exchange::JTimeSync::Event::TimeChanged(*this);
    }

    Core::hresult TimeSync::Synchronize()
    {
        _adminLock.Lock();

        uint32_t result = _client->Synchronize();

        _adminLock.Unlock();

        if ((result != Core::ERROR_NONE) && (result != Core::ERROR_INPROGRESS) && (result != Core::ERROR_INCOMPLETE_CONFIG)) {
            result = Core::ERROR_INCOMPLETE_CONFIG;
        }

        return (result);
    }

    Core::hresult TimeSync::SyncTime(Exchange::ITimeSync::TimeInfo& info) const
    {
        Core::hresult result = Core::ERROR_UNAVAILABLE;

        _adminLock.Lock();

        info.time = Core::Time(_client->SyncTime());

        if (info.time.IsValid() == true) {

            info.source = _client->Source();
            result = Core::ERROR_NONE;
        }

        _adminLock.Unlock();

        return (result);
    }

    Core::hresult TimeSync::Time(Core::Time& time) const
    {
        time = Core::Time::Now();

        return (Core::ERROR_NONE);
    }

    Core::hresult TimeSync::Time(const Core::Time& time)
    {
        uint32_t result = Core::ERROR_NONE;

        if (time.IsValid() == true) {

            _adminLock.Lock();

            // Stop automatic synchronisation
            _client->Cancel();
            _job.Revoke();

            _adminLock.Unlock();

            Core::SystemInfo::Instance().SetTime(time);

            EnsureSubsystemIsActive();
        }
        else {
            result = Core::ERROR_BAD_REQUEST;
        }

        return (result);
    }

    Core::hresult TimeSync::Register(Exchange::ITimeSync::INotification* const notification)
    {
        Core::hresult result = Core::ERROR_ALREADY_CONNECTED;

        ASSERT(notification != nullptr);

        _adminLock.Lock();

        auto it = std::find(_timeSyncObservers.begin(), _timeSyncObservers.end(), notification);
        ASSERT(it == _timeSyncObservers.end());

        if (it == _timeSyncObservers.end()) {
            notification->AddRef();

            _timeSyncObservers.push_back(notification);

            result = Core::ERROR_NONE;
        }
        _adminLock.Unlock();

        return (result);
    }

    Core::hresult TimeSync::Unregister(const Exchange::ITimeSync::INotification* const notification)
    {
        Core::hresult result = Core::ERROR_ALREADY_RELEASED;

        ASSERT(notification != nullptr);

        _adminLock.Lock();

        auto it = std::find(_timeSyncObservers.cbegin(), _timeSyncObservers.cend(), notification);
        ASSERT(it != _timeSyncObservers.cend());

        if (it != _timeSyncObservers.cend()) {
            (*it)->Release();

            _timeSyncObservers.erase(it);

            result = Core::ERROR_NONE;
        }
        _adminLock.Unlock();

        return (result);
    }

} // namespace Plugin
} // namespace Thunder
