#ifndef TIMESYNC_H
#define TIMESYNC_H

#include "Module.h"
#include <interfaces/ITimeSync.h>

namespace WPEFramework {
namespace Plugin {

    class TimeSync : public PluginHost::IPlugin, public PluginHost::IWeb {
    public:
        class Data : public Core::JSON::Container {
        public:
            Data(Data const& other) = delete;
            Data& operator=(Data const& other) = delete;

            Data()
                : Core::JSON::Container()
                , IsTimeSynced()
                , TimeSource()
                , SyncTime()
            {
                Add(_T("synced"), &IsTimeSynced);
                Add(_T("source"), &TimeSource);
                Add(_T("time"), &SyncTime);
            }

            virtual ~Data()
            {
            }

        public:
            Core::JSON::Boolean IsTimeSynced;
            Core::JSON::String TimeSource;
            Core::JSON::String SyncTime;
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
                , _service(nullptr)
                , _client(nullptr)
            {
                ASSERT(parent != nullptr);
            }
            ~Notification()
            {
            }

        public:
            void Initialize(PluginHost::IShell* service, Exchange::ITimeSync* client)
            {
                ASSERT(_service == nullptr);
                ASSERT(service != nullptr);
                ASSERT(_client == nullptr);
                ASSERT(client != nullptr);

                _client = client;
                _client->AddRef();

                _service = service;
                _service->AddRef();

                _client->Synchronize();
                _client->Register(this);
            }
            void Deinitialize()
            {

                ASSERT(_service != nullptr);
                ASSERT(_client != nullptr);

                if (_client != nullptr) {
                    _client->Unregister(this);
                    _client->Release();
                    _client = nullptr;
                }

                _service->Release();
                _service = nullptr;
            }

            virtual void Completed()
            {
                uint64_t timeTicks = _client->SyncTime();

                if (timeTicks != 0) {
                    _parent.SyncedTime(timeTicks);
                    // On activation subscribe, on deactivation un-subscribe
                    PluginHost::ISubSystem* subSystem = _service->SubSystems();

                    ASSERT(subSystem != nullptr);

                    if (subSystem != nullptr) {
                        if (subSystem->IsActive(PluginHost::ISubSystem::TIME) == false) {
                            subSystem->Set(PluginHost::ISubSystem::TIME, _client);
                        }
                        subSystem->Release();
                    }
                }
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(Exchange::ITimeSync::INotification)
            END_INTERFACE_MAP

        private:
            Core::CriticalSection _adminLock;
            TimeSync& _parent;
            PluginHost::IShell* _service;
            Exchange::ITimeSync* _client;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Interval(30)
                , Retries(8)
                , Sources()
                , Periodicity(0)
            {
                Add(_T("interval"), &Interval);
                Add(_T("retries"), &Retries);
                Add(_T("sources"), &Sources);
                Add(_T("periodicity"), &Periodicity);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 Interval;
            Core::JSON::DecUInt8 Retries;
            Core::JSON::ArrayType<Core::JSON::String> Sources;
            Core::JSON::DecUInt16 Periodicity;
        };

        class PeriodicSync : public Core::IDispatchType<void> {
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

    private:
        uint16_t _skipURL;
        uint32_t _periodicity;
        Exchange::ITimeSync* _client;
        Core::ProxyType<Core::IDispatchType<void> > _activity;
        Core::Sink<Notification> _sink;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // TIMESYNC_H
