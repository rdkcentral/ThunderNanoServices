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

#ifndef TIMESYNC_H
#define TIMESYNC_H

#include "Module.h"
#include <interfaces/ITimeSync.h>
#include <interfaces/json/JsonData_TimeSync.h>

namespace WPEFramework {
namespace Plugin {

    class TimeSync : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    public:
        template <typename TimeRep = Core::JSON::String>
        class Data : public Core::JSON::Container {
        public:
            Data(Data const& other) = delete;
            Data& operator=(Data const& other) = delete;

            Data()
                : Core::JSON::Container()
                , TimeSource()
                , SyncTime()
            {
                Add(_T("source"), &TimeSource);
                Add(_T("time"), &SyncTime);
            }

            virtual ~Data()
            {
            }

        public:
            Core::JSON::String TimeSource;
            TimeRep SyncTime;
        };

        template <typename TimeRep = Core::JSON::String>
        class SetData : public Core::JSON::Container {
        public:
            SetData(SetData const& other) = delete;
            SetData& operator=(SetData const& other) = delete;

            SetData()
                : Core::JSON::Container()
                , Time()
            {
                Add(_T("time"), &Time);
            }

            virtual ~SetData()
            {
            }

        public:
            TimeRep Time;
        };

    private:
        class Notification : protected Exchange::ITimeSync::INotification {
        private:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            explicit Notification(TimeSync* parent)
                : _adminLock()
                , _parent(*parent)
                , _client(nullptr)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification()
            {
            }

        public:
            void Initialize(Exchange::ITimeSync* client, bool start = true)
            {
                ASSERT(_client == nullptr);
                ASSERT(client != nullptr);

                _client = client;
                _client->AddRef();
                _client->Register(this);

                if (start == true) {
                    _client->Synchronize();
                }

            }
            void Deinitialize()
            {

                ASSERT(_client != nullptr);

                if (_client != nullptr) {
                    _client->Unregister(this);
                    _client->Release();
                    _client = nullptr;
                }
            }

            virtual void Completed()
            {
                uint64_t timeTicks = _client->SyncTime();

                if (timeTicks != 0) {
                    _parent.SyncedTime(timeTicks);
                    _parent.EnsureSubsystemIsActive();
                }
                else {
                    SYSLOG(Logging::Error, (_T("TimeSync was unable to set a correct system time!")));
                 }
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(Exchange::ITimeSync::INotification)
            END_INTERFACE_MAP

        private:
            Core::CriticalSection _adminLock;
            TimeSync& _parent;
            Exchange::ITimeSync* _client;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Deferred(false)
                , Interval(30)
                , Retries(8)
                , Sources()
                , Periodicity(0)
            {
                Add(_T("deferred"), &Deferred);
                Add(_T("interval"), &Interval);
                Add(_T("retries"), &Retries);
                Add(_T("sources"), &Sources);
                Add(_T("periodicity"), &Periodicity);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::Boolean Deferred;
            Core::JSON::DecUInt16 Interval;
            Core::JSON::DecUInt8 Retries;
            Core::JSON::ArrayType<Core::JSON::String> Sources;
            Core::JSON::DecUInt16 Periodicity;
        };

        class PeriodicSync : public Core::IDispatch {
        private:
            PeriodicSync() = delete;
            PeriodicSync(const PeriodicSync&) = delete;
            PeriodicSync& operator=(const PeriodicSync&) = delete;

        public:
            PeriodicSync(Exchange::ITimeSync* timeSync)
                : _timeSync(timeSync)
            {
                ASSERT(timeSync != nullptr);

                _timeSync = timeSync;
                _timeSync->AddRef();
            }
            ~PeriodicSync()
            {
                _timeSync->Release();
            }

        public:
            virtual void Dispatch() override
            {
                _timeSync->Synchronize();
            }

        private:
            Exchange::ITimeSync* _timeSync;
        };

    private:
        TimeSync(const TimeSync&) = delete;
        TimeSync& operator=(const TimeSync&) = delete;

    public:
        TimeSync();
        virtual ~TimeSync();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(TimeSync)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::ITimeSync, _client)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        void SyncedTime(const uint64_t timeTicks);
        void EnsureSubsystemIsActive();

        // JSON RPC
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_synchronize();
        uint32_t get_synctime(JsonData::TimeSync::SynctimeData& response) const;
        uint32_t get_time(Core::JSON::String& response) const;
        uint32_t set_time(const Core::JSON::String& param);
        void event_timechange();

    private:
        uint16_t _skipURL;
        uint32_t _periodicity;
        Exchange::ITimeSync* _client;
        Core::ProxyType<Core::IDispatch> _activity;
        Core::Sink<Notification> _sink;
        PluginHost::IShell* _service;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // TIMESYNC_H
