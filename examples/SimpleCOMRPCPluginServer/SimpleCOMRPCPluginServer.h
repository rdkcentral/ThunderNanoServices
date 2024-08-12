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

namespace Thunder {
namespace Plugin {

    class SimpleCOMRPCPluginServer : public PluginHost::IPlugin {
        //Inner private classes
    private:
        class WallClock : public Exchange::IWallClock {
        public:
            WallClock() = delete;
            WallClock(const WallClock&) = delete;
            WallClock& operator= (const WallClock&) = delete;

            WallClock(SimpleCOMRPCPluginServer& parent)
                : _adminLock()
                , _parent(&parent)
                , _reference(Core::Library(&Core::System::MODULE_NAME)) {
                printf("Constructing WallClock\n");
            }
            ~WallClock() override {
                printf("Destructing WallClock\n");
            }

        public:
            //  IWallClock methods
            uint32_t Arm(const uint16_t seconds, Exchange::IWallClock::ICallback*) override;
            uint32_t Disarm(const Exchange::IWallClock::ICallback*) override;
            uint64_t Now() const override;

            void Decouple() {
                _adminLock.Lock();
                _parent = nullptr;
                _adminLock.Unlock();
            }
            
            BEGIN_INTERFACE_MAP(WallClock)
                INTERFACE_ENTRY(Exchange::IWallClock)
            END_INTERFACE_MAP

        private:
            Core::CriticalSection _adminLock;
            SimpleCOMRPCPluginServer* _parent;
            Core::Library _reference;
        };

        class ComNotificationSink : public PluginHost::IShell::ICOMLink::INotification {
        public:
            ComNotificationSink() = delete;
            ComNotificationSink(SimpleCOMRPCPluginServer& parent)
                : _parent(parent)
            {
            }
            ~ComNotificationSink() override
            {
            }
            ComNotificationSink(const ComNotificationSink&) = delete;
            ComNotificationSink& operator=(const ComNotificationSink&) = delete;

            BEGIN_INTERFACE_MAP(ComNotificationSink)
                INTERFACE_ENTRY(PluginHost::IShell::ICOMLink::INotification)
            END_INTERFACE_MAP

            void Dangling(const Core::IUnknown* remote, const uint32_t interfaceId) override
            {
                //not interested in this notification
                TRACE(Trace::Information, (_T("Cleanup an interface: %d [%X] on object: [%s]"), interfaceId, interfaceId, typeid(*remote).name()));
            }
            void Revoked(const Core::IUnknown* remote, const uint32_t interfaceId) override
            {   
                TRACE(Trace::Information, (_T("Revoking an interface: %d [%X] on object: [%s]"), interfaceId, interfaceId, typeid(*remote).name()));

                // Something happened to the other side
                ASSERT(interfaceId != Exchange::ID_WALLCLOCK);

                if (interfaceId == Exchange::ID_WALLCLOCK_CALLBACK) {
                    const Exchange::IWallClock::ICallback* result = remote->QueryInterface<const Exchange::IWallClock::ICallback>();

                    ASSERT(result != nullptr);

                    _parent.OnRevoke(result);

                    // Do not forget to release the QI from a few lines above :-)
                    result->Release();
                }
            }

        private:
            SimpleCOMRPCPluginServer& _parent;
        };
        class WallClockNotifier {
        private:
            class Notifier {
            public:
                Notifier() = delete;
                Notifier(const Notifier&) = delete;

                Notifier(const uint16_t period, Exchange::IWallClock::ICallback* callback)
                    : _period(period)
                    , _ticks(Core::Time::Now().Add(_period * 1000).Ticks())
                    , _callback(callback) {
                    _callback->AddRef();
                }
                ~Notifier() {
                    _callback->Release();
                }

            public:
                bool operator== (const Exchange::IWallClock::ICallback* callback) const {
                    return (callback == _callback);
                }
                bool operator!= (const Exchange::IWallClock::ICallback* callback) const {
                    return (!operator==(callback));
                }
                uint64_t NextSlot(const uint64_t currentSlot) {
                    if (currentSlot >= _ticks) {
                        TRACE(Trace::Information, (_T("Reporting activity, %d seconds have elapsed"), _period));
                        // Seems we need to execute..
                        _period = _callback->Elapsed(_period);

                        _ticks = (_period != 0 ? (_ticks + ((_period * 1000) * Core::Time::TicksPerMillisecond)) : ~0);
                    }
                    return (_ticks);
                }

            private:
                uint16_t _period;
                uint64_t _ticks;
                Exchange::IWallClock::ICallback* _callback;
            };
            using NotifierList = std::list<Notifier>;

        public:
            WallClockNotifier(const WallClockNotifier&) = delete;
            WallClockNotifier& operator=(const WallClockNotifier&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            WallClockNotifier()
                : _job(*this) {
            }
POP_WARNING()
            ~WallClockNotifier() {
                Clear();
            }

        public:
            void Clear() {
                _job.Revoke();
                _notifiers.clear();
            }
            uint32_t Arm(const uint16_t seconds, Exchange::IWallClock::ICallback* callback)
            {
                uint32_t result = Core::ERROR_ALREADY_CONNECTED;

                _adminLock.Lock();

                NotifierList::iterator index = std::find(_notifiers.begin(), _notifiers.end(), callback);

                if (index == _notifiers.end()) {
                    _notifiers.emplace_front(seconds, callback);
                    result = Core::ERROR_NONE;
                }

                _adminLock.Unlock();

                _job.Revoke();
                _job.Submit();

                return (result);
            }
            uint32_t Disarm(const Exchange::IWallClock::ICallback* callback)
            {
                uint32_t result = Core::ERROR_NOT_EXIST;

                _adminLock.Lock();

                NotifierList::iterator index = std::find(_notifiers.begin(), _notifiers.end(), callback);

                if (index == _notifiers.end()) {
                    _adminLock.Unlock();
                }
                else {
                    _notifiers.erase(index);

                    _adminLock.Unlock();

                    _job.Revoke();
                    _job.Submit();

                    result = Core::ERROR_NONE;
                }


                return (result);
            }

        private:
            friend class Core::ThreadPool::JobType<WallClockNotifier&>;
            void Dispatch() {

                _adminLock.Lock();

                uint64_t nextSlot;
                uint64_t timeSlot = Core::Time::Now().Ticks();

                do {
                    NotifierList::iterator index = _notifiers.begin();

                    nextSlot = ~0;

                    while (index != _notifiers.end()) {
                        uint64_t thisSlot = index->NextSlot(timeSlot);

                        if (thisSlot == static_cast<uint64_t>(~0)) {
                            index = _notifiers.erase(index);
                        }
                        else if (thisSlot >= nextSlot) {
                            index++;
                        }
                        else {
                            index++;
                            nextSlot = thisSlot;
                        }
                    }

                    timeSlot = Core::Time::Now().Ticks();

                } while (nextSlot < timeSlot);

                if (nextSlot != static_cast<uint64_t>(~0)) {
                    _job.Reschedule(Core::Time(nextSlot));
                }

                _adminLock.Unlock();
            }

        private:
            Core::CriticalSection _adminLock;
            Core::WorkerPool::JobType<WallClockNotifier&> _job;
            NotifierList _notifiers;
        };

    public:
        //Constructors
        SimpleCOMRPCPluginServer(const SimpleCOMRPCPluginServer&) = delete;
        SimpleCOMRPCPluginServer& operator=(const SimpleCOMRPCPluginServer&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        SimpleCOMRPCPluginServer()
            : _notifier()
            , _wallclock(nullptr)
            , _comNotificationSink(*this)
        {
        }
POP_WARNING()
        ~SimpleCOMRPCPluginServer() override {
            printf("Shutting down\n");
        }

    public:
        //Service Registration
        BEGIN_INTERFACE_MAP(SimpleCOMRPCPluginServer)
            INTERFACE_ENTRY(IPlugin)
            INTERFACE_AGGREGATE(Exchange::IWallClock, _wallclock)
        END_INTERFACE_MAP

        //  IPlugin methods
        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        virtual const string Initialize(PluginHost::IShell* service);

        // The plugin is unloaded from the webbridge. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service);

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        virtual string Information() const;

    private:
        void OnRevoke(const Exchange::IWallClock::ICallback* remote);

    private:
        WallClockNotifier _notifier;
        WallClock* _wallclock;
        Core::SinkType<ComNotificationSink> _comNotificationSink;
    };
}
}
