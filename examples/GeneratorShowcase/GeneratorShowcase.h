/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Metrological
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

#include <interfaces/ISimpleAsync.h>
#include <interfaces/JSimpleAsync.h>

#include <interfaces/ISimpleInstanceObjects.h>
#include <interfaces/JSimpleInstanceObjects.h>

namespace Thunder {

namespace Plugin {

    class GeneratorShowcase : public PluginHost::IPlugin
                            , public PluginHost::JSONRPCSupportsEventStatus
                            , public Exchange::JSimpleInstanceObjects::IHandler {
    public:
        GeneratorShowcase()
            : _service(nullptr)
            , _notification(*this)
            , _imaginaryServer(nullptr)
            , _imaginaryHost(nullptr)
            , _storage(nullptr)
            , _deviceNotificationSinks()
        {
        }
        ~GeneratorShowcase() override = default;

        GeneratorShowcase(const GeneratorShowcase&) = delete;
        GeneratorShowcase(GeneratorShowcase&&) = delete;
        GeneratorShowcase& operator=(const GeneratorShowcase&) = delete;
        GeneratorShowcase& operator=(GeneratorShowcase&&) = delete;

    private:
        class Notification : public PluginHost::IShell::IConnectionServer::INotification {
        public:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification(Notification&& ) = delete;
            Notification& operator=(const Notification&) = delete;
            Notification& operator=(Notification&&) = delete;

            explicit Notification(GeneratorShowcase& parent)
                : _parent(parent)
            {
            }
            ~Notification() override = default;

        public:
            void Opened(const uint32_t /* channel */) override
            {
            }
            void Closed(const uint32_t channel) override
            {
                _parent.Closed(channel);
            }

        public:
            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(PluginHost::IShell::IConnectionServer::INotification)
            END_INTERFACE_MAP

        private:
            GeneratorShowcase& _parent;
        }; // class Notification

    private:
        class DecoupledJob : private Core::WorkerPool::JobType<DecoupledJob&> {
            friend class Core::ThreadPool::JobType<DecoupledJob&>;

        public:
            using Job = std::function<void()>;

            DecoupledJob(const DecoupledJob&) = delete;
            DecoupledJob(DecoupledJob&&) = delete;
            DecoupledJob& operator=(const DecoupledJob&) = delete;
            DecoupledJob& operator=(DecoupledJob&&) = delete;

            DecoupledJob()
                : Core::WorkerPool::JobType<DecoupledJob&>(*this)
                , _lock()
                , _job(nullptr)
            {
            }

            ~DecoupledJob() = default;

        public:
            void Schedule(const Job& job, const uint32_t defer = 0)
            {
                _lock.Lock();
                _job = job;
                JobType::Reschedule(Core::Time::Now().Add(defer));
                ASSERT(JobType::IsIdle() == false);
                _lock.Unlock();
            }
            void Revoke()
            {
                _lock.Lock();
                JobType::Revoke();
                _job = nullptr;
                _lock.Unlock();
            }

        private:
            void Dispatch()
            {
                _lock.Lock();
                Job DoTheJob = _job;
                _job = nullptr;
                _lock.Unlock();
                DoTheJob();
            }

        private:
            Core::CriticalSection _lock;
            Job _job;
        }; // class DecoupledJob

    private:
        class ImaginaryServer : public Exchange::ISimpleAsync {
        public:
            ImaginaryServer(GeneratorShowcase& parent)
                : _parent(parent)
                , _lock()
                , _job()
                , _timer()
                , _callback(nullptr)
                , _address()
                , _state(Exchange::ISimpleAsync::DISCONNECTED)
            {
            }
            ~ImaginaryServer() override
            {
                _timer.Revoke();
                _job.Revoke();

                _lock.Lock();

                if (_callback != nullptr) {
                    // Closing but still a callback is installed
                    _callback->Complete(_address, Exchange::ISimpleAsync::CONNECTING_FAILED);
                    _callback->Release();
                }

                _lock.Unlock();
            }

            ImaginaryServer(const ImaginaryServer&) = delete;
            ImaginaryServer(ImaginaryServer&&) = delete;
            ImaginaryServer& operator=(const ImaginaryServer&) = delete;
            ImaginaryServer& operator=(ImaginaryServer&&) = delete;

        public:
            Core::hresult Connect(const string& address, const Core::OptionalType<uint16_t>& timeout, Exchange::ISimpleAsync::ICallback* const cb) override
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_state == Exchange::ISimpleAsync::CONNECTED) {
                    TRACE(Trace::Error, (_T("Already connected")));
                    result = Core::ERROR_ALREADY_CONNECTED;
                }
                else if (_state == Exchange::ISimpleAsync::CONNECTING) {
                    TRACE(Trace::Error, (_T("Connection in progress")));
                    result = Core::ERROR_INPROGRESS;
                }
                else {
                    TRACE(Trace::Information, (_T("Connecting to %s..."), address.c_str()));
                    _state = Exchange::ISimpleAsync::CONNECTING;
                    _address = address;

                    if (_callback != nullptr) {
                        _callback->Release();
                    }

                    if (cb != nullptr) {
                        cb->AddRef();
                    }

                    _callback = cb;

                    // Start a timer for connection timeout
                    _timer.Schedule([&]() {
                        _lock.Lock();

                        // Stop the connection job
                        _job.Revoke();

                        if (_state == Exchange::ISimpleAsync::CONNECTING) {

                            _state = Exchange::ISimpleAsync::DISCONNECTED;

                            // Timer fired, report a TIMED_OUT state
                            if (_callback != nullptr) {
                                _callback->Complete(_address, Exchange::ISimpleAsync::CONNECTING_TIMED_OUT);
                                _callback->Release();
                                _callback = nullptr;
                            }
                        }

                        _lock.Unlock();
                    }, timeout);

                    // Mock connection :)
                    _job.Schedule([&]() {

                        // Stop timeout timer
                        _timer.Revoke();

                        _lock.Lock();

                        if (_state == Exchange::ISimpleAsync::CONNECTING) {

                            _state = Exchange::ISimpleAsync::CONNECTED;

                            // Report CONNECTED state
                            if (_callback != nullptr) {
                                _callback->Complete(_address, Exchange::ISimpleAsync::CONNECTED);
                                _callback->Release();
                                _callback = nullptr;
                            }
                        }

                        _lock.Unlock();

                    }, 5000);
                }

                _lock.Unlock();

                return (result);
            }
            Core::hresult Abort() override
            {
                Core::hresult result = Core::ERROR_NONE;

                // Aborting, stop both timers
                _timer.Revoke();
                _job.Revoke();

                _lock.Lock();

                if (_state != Exchange::ISimpleAsync::CONNECTING) {
                    result = Core::ERROR_ILLEGAL_STATE;
                }
                else {
                    _state = Exchange::ISimpleAsync::DISCONNECTED;

                    if (_callback != nullptr) {
                        _callback->Complete(_address, Exchange::ISimpleAsync::CONNECTING_ABORTED);
                        _callback->Release();
                        _callback = nullptr;
                    }
                }

                _lock.Unlock();

                return (result);
            }
            Core::hresult Disconnect() override
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_state == Exchange::ISimpleAsync::CONNECTING) {
                    TRACE(Trace::Error, (_T("Connecting in progress")));
                    result = Core::ERROR_INPROGRESS;
                }
                else if (_state == Exchange::ISimpleAsync::DISCONNECTED) {
                    TRACE(Trace::Error, (_T("Not connected")));
                    result = Core::ERROR_ALREADY_RELEASED;
                }
                else {
                    _address.clear();
                    _state = Exchange::ISimpleAsync::DISCONNECTED;
                    TRACE(Trace::Information, (_T("Disconnected")));
                }

                _lock.Unlock();

                return (result);
            }

        public:
            BEGIN_INTERFACE_MAP(ImaginaryServer)
                INTERFACE_ENTRY(Exchange::ISimpleAsync)
            END_INTERFACE_MAP

        private:
            GeneratorShowcase& _parent;
            Core::CriticalSection _lock;
            DecoupledJob _job;
            DecoupledJob _timer;
            Exchange::ISimpleAsync::ICallback* _callback;
            string _address;
            Exchange::ISimpleAsync::state _state;
        }; // class ImaginaryServer

    private:
        class ImaginaryHost : public Exchange::ISimpleInstanceObjects {
        public:
            class Device : public Exchange::ISimpleInstanceObjects::IDevice {
            public:
                Device(GeneratorShowcase& parent, const string& name)
                    : _parent(parent)
                    , _lock()
                    , _job()
                    , _name(name)
                    , _observers()
                {
                }
                ~Device() override
                {
                    _job.Revoke();
                }

                Device() = delete;
                Device(const Device&) = delete;
                Device(Device&&) = delete;
                Device& operator=(const Device&) = delete;
                Device& operator=(Device&&) = delete;

            public:
                Core::hresult Register(INotification* const notification) override
                {
                    Core::hresult result = Core::ERROR_NONE;

                    ASSERT(notification != nullptr);

                    if (notification != nullptr) {

                        _lock.Lock();

                        auto it = std::find(_observers.begin(), _observers.end(), notification);

                        if (it == _observers.end()) {
                            _observers.push_back(notification);
                        }
                        else {
                            result = Core::ERROR_ALREADY_CONNECTED;
                        }

                        _lock.Unlock();
                    }
                    else {
                        result = Core::ERROR_BAD_REQUEST;
                    }

                    return (result);
                }
                Core::hresult Unregister(const INotification* const notification) override
                {
                    Core::hresult result = Core::ERROR_NONE;

                    ASSERT(notification != nullptr);

                    _lock.Lock();

                    auto it = std::find(_observers.begin(), _observers.end(), notification);
                    if (it != _observers.end()) {
                        (*it)->Release();
                        _observers.erase(it);
                    }
                    else {
                        result = Core::ERROR_UNKNOWN_KEY;
                    }

                    _lock.Unlock();

                    return (result);
                }
                Core::hresult Name(const string& name) override
                {
                    SetName(name);
                    return (Core::ERROR_NONE);
                }
                Core::hresult Name(string& name) const override
                {
                    _lock.Lock();
                    name = _name;
                    _lock.Unlock();

                    return (Core::ERROR_NONE);

                }
                Core::hresult Enable() override
                {
                    // Mock enabling in 1 second :)
                    _job.Schedule([this](){
                        SetState(Exchange::ISimpleInstanceObjects::ENABLED);
                    }, 1000);

                    return (Core::ERROR_NONE);
                }
                Core::hresult Disable() override
                {
                    SetState(Exchange::ISimpleInstanceObjects::DISABLED);
                    return (Core::ERROR_NONE);
                }
                Core::hresult Pin(const uint8_t pin, const bool high) override
                {
                    SetPin(pin, high);
                    return (Core::ERROR_NONE);
                }
                Core::hresult Pin(const uint8_t pin, bool& high) const override
                {
                    Core::hresult result = Core::ERROR_UNAVAILABLE;

                    _lock.Lock();

                    auto it = _pins.find(pin);
                    if (it != _pins.end()) {
                        high = (*it).second;
                        result = Core::ERROR_NONE;
                    }
                    else {
                        TRACE(Trace::Error, (_T("Pin not found")));
                    }

                    _lock.Unlock();

                    return (result);
                }

            public:
                Exchange::ISimpleInstanceObjects::state State() const
                {
                    return (_state);
                }
                void IteratePins(const std::function<void(const uint8_t pin, const bool high)>& cb)
                {
                    _lock.Lock();

                    for (auto const& pin : _pins) {
                        cb(pin.first, pin.second);
                    }

                    _lock.Unlock();
                }
                bool PinStatus(const uint8_t pin)
                {
                    bool value = false;

                    _lock.Lock();

                    auto it = _pins.find(pin);
                    if (it != _pins.end()) {
                        value = (*it).second;
                    }

                    _lock.Unlock();

                    return (value);
                }

            private:
                void SetName(const string& name)
                {
                    _lock.Lock();

                    if (_name != name) {
                        _name = name;

                        for (auto const& observer : _observers) {
                            observer->NameChanged(name);
                        }
                    }

                    _lock.Unlock();
                }
                void SetState(const Exchange::ISimpleInstanceObjects::state state)
                {
                    _lock.Lock();

                    if (_state != state) {

                        _state = state;

                        for (auto const& observer : _observers) {
                            observer->StateChanged(state);
                        }
                    }

                    _lock.Unlock();
                }
                void SetPin(const uint8_t pin, const bool high)
                {
                    _lock.Lock();

                    auto it = _pins.find(pin);

                    if (it != _pins.end()) {
                        if ((*it).second != high) {
                            (*it).second = high;

                            for (auto const& observer : _observers) {
                                observer->PinChanged(pin, high);
                            }
                        }
                    }
                    else {
                        _pins.emplace(pin, high);

                        for (auto const& observer : _observers) {
                            observer->PinChanged(pin, high);
                        }
                    }

                    _lock.Unlock();
                }

            public:
                BEGIN_INTERFACE_MAP(Device)
                    INTERFACE_ENTRY(Exchange::ISimpleInstanceObjects::IDevice)
                END_INTERFACE_MAP

            private:
                GeneratorShowcase& _parent;
                mutable Core::CriticalSection _lock;
                DecoupledJob _job;
                string _name;
                Exchange::ISimpleInstanceObjects::state _state;
                std::map<uint8_t, bool> _pins;
                std::vector<Exchange::ISimpleInstanceObjects::IDevice::INotification*> _observers;

            }; // class Device

        public:
            ImaginaryHost(GeneratorShowcase& parent)
                : _parent(parent)
                , _lock()
            {
            }
            ~ImaginaryHost() override
            {
            }

            ImaginaryHost(const ImaginaryHost&) = delete;
            ImaginaryHost(ImaginaryHost&&) = delete;
            ImaginaryHost& operator=(const ImaginaryHost&) = delete;
            ImaginaryHost& operator=(ImaginaryHost&&) = delete;

        public:
            Core::hresult Acquire(const string& name, Exchange::ISimpleInstanceObjects::IDevice*& device) override
            {
                Core::hresult result = Core::ERROR_NONE;

                auto it = _devices.find(name);

                if (it != _devices.end()) {
                    result = Core::ERROR_UNAVAILABLE;
                    TRACE(Trace::Error, (_T("Device unavailable")));
                }
                else {
                    device = Core::ServiceType<Device>::Create<Exchange::ISimpleInstanceObjects::IDevice>(_parent, name);
                    _devices.emplace(name, device);
                }

                return (result);
            }
            Core::hresult Relinquish(Exchange::ISimpleInstanceObjects::IDevice* const device) override
            {
                Core::hresult result = Core::ERROR_NONE;

                if (device == nullptr) {
                    result = Core::ERROR_BAD_REQUEST;
                }
                else {
                    string deviceName;
                    static_cast<const Exchange::ISimpleInstanceObjects::IDevice*>(device)->Name(deviceName);

                    auto it = _devices.find(deviceName);

                    if (it != _devices.end()) {
                        (*it).second->Release();
                        _devices.erase(it);
                    }
                    else {
                        TRACE(Trace::Error, (_T("Device not found")));
                        result = Core::ERROR_UNAVAILABLE;
                    }
                }

                return (result);
            }

        public:
            BEGIN_INTERFACE_MAP(ImaginaryHost)
                INTERFACE_ENTRY(Exchange::ISimpleInstanceObjects)
            END_INTERFACE_MAP

        private:
            GeneratorShowcase& _parent;
            mutable Core::CriticalSection _lock;
            std::map<string, Exchange::ISimpleInstanceObjects::IDevice*> _devices;

        }; // class ImaginaryHost

    public:
        const string Initialize(PluginHost::IShell *service) override
        {
            string message{};

            ASSERT(service != nullptr);
            ASSERT(_service == nullptr);

            _service = service;
            _service->AddRef();

            _imaginaryServer = Core::ServiceType<ImaginaryServer>::Create<Exchange::ISimpleAsync>(*this);
            _imaginaryHost = Core::ServiceType<ImaginaryHost>::Create<Exchange::ISimpleInstanceObjects>(*this);

            ASSERT(_imaginaryServer != nullptr);
            ASSERT(_imaginaryHost != nullptr);

            Exchange::JSimpleAsync::Register(*this, _imaginaryServer);
            Exchange::JSimpleInstanceObjects::Register(*this, _imaginaryHost, this, _storage);

            // Register for channel closures
            service->Register(&_notification);

            // If needed, it's possible to install callbacks when devices are acquired/relinquished.
            // The relinquish callback also fires on channel closure.
            // Here it's used to store device notifcation sinks.
            Exchange::JSimpleInstanceObjects::Lifetime::Callback<Exchange::ISimpleInstanceObjects::IDevice>(_storage, [this](const bool acquired, Exchange::ISimpleInstanceObjects::IDevice* device) {
                ASSERT(device != nullptr);

                if (acquired == true) {
                    // device was acquired
                    string name;
                    static_cast<const Exchange::ISimpleInstanceObjects::IDevice*>(device)->Name(name);
                    TRACE(Trace::Information, (_T("Device %s acquired"), name.c_str()));

                    ASSERT(_deviceNotificationSinks.find(device) == _deviceNotificationSinks.end());
                    _deviceNotificationSinks.emplace(device, Core::ServiceType<ImaginaryHostNotificationImpl>::Create<Exchange::ISimpleInstanceObjects::IDevice::INotification>(*this, device));
                    device->AddRef();
                }
                else {
                    // device is about to be relinquished
                    string name;
                    static_cast<const Exchange::ISimpleInstanceObjects::IDevice*>(device)->Name(name);
                    TRACE(Trace::Information, (_T("Device %s released"), name.c_str()));

                    auto it = _deviceNotificationSinks.find(device);
                    ASSERT(_deviceNotificationSinks.find(device) != _deviceNotificationSinks.end());

                    if (it != _deviceNotificationSinks.end()) {
                        (*it).second->Release();
                        (*it).first->Release();
                        _deviceNotificationSinks.erase(it);
                    }
                }
            });

            return (message);
        }

        void Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED) override
        {
            if (_service != nullptr) {
                ASSERT(_service == service);

                _service->Unregister(&_notification);

                if (_imaginaryServer != nullptr) {
                    Exchange::JSimpleAsync::Unregister(*this);
                    _imaginaryServer->Release();
                    _imaginaryServer = nullptr;
                }

                if (_imaginaryHost != nullptr) {
                    Exchange::JSimpleInstanceObjects::Unregister(*this, _storage);
                    _imaginaryHost->Release();
                    _imaginaryHost = nullptr;
                }

                _service->Release();
                _service = nullptr;
            }
        }

        string Information() const override
        {
            return {};
        }

    public:
        BEGIN_INTERFACE_MAP(GeneratorShowcase)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::ISimpleAsync, _imaginaryServer)
            INTERFACE_AGGREGATE(Exchange::ISimpleInstanceObjects, _imaginaryHost)
        END_INTERFACE_MAP

    private:
        class ImaginaryHostNotificationImpl : public Exchange::ISimpleInstanceObjects::IDevice::INotification {
        public:
            ImaginaryHostNotificationImpl(GeneratorShowcase& parent, Exchange::ISimpleInstanceObjects::IDevice* const device)
                : _parent(parent)
                , _device(device)
            {
                ASSERT(device != nullptr);
                _device->AddRef();
                _device->Register(this);
            }
            ~ImaginaryHostNotificationImpl()
            {
                _device->Unregister(this);
                _device->Release();
            }

            ImaginaryHostNotificationImpl() = delete;
            ImaginaryHostNotificationImpl(const ImaginaryHostNotificationImpl&) = delete;
            ImaginaryHostNotificationImpl(ImaginaryHostNotificationImpl&&) = delete;
            ImaginaryHostNotificationImpl& operator=(const ImaginaryHostNotificationImpl&) = delete;
            ImaginaryHostNotificationImpl& operator=(ImaginaryHostNotificationImpl&&) = delete;

        public:
            void NameChanged(const string& name) override
            {
                Exchange::JSimpleInstanceObjects::Event::NameChanged(_parent, _parent._storage, _device, name);
            }
            void StateChanged(const Exchange::ISimpleInstanceObjects::state state) override
            {
                Exchange::JSimpleInstanceObjects::Event::StateChanged(_parent, _parent._storage, _device, state);
            }
            void PinChanged(const uint8_t pin, const bool high) override
            {
                Exchange::JSimpleInstanceObjects::Event::PinChanged(_parent, _parent._storage, _device, pin, high);
            }

        public:
            BEGIN_INTERFACE_MAP(ImaginaryHostNotificationImpl)
                INTERFACE_ENTRY(Exchange::ISimpleInstanceObjects::IDevice::INotification)
            END_INTERFACE_MAP

        private:
            GeneratorShowcase& _parent;
            Exchange::ISimpleInstanceObjects::IDevice* _device;

        }; // class ImaginaryHostNotificationImpl

    private:
        void Closed(const uint32_t channel)
        {
            ASSERT(_storage != nullptr);

            // A websocket channel was closed, we may need to release some resources!
            Exchange::JSimpleInstanceObjects::Link::Closed(_storage, channel);
        }

    private:
        void OnStateChangedEventRegistration(Exchange::ISimpleInstanceObjects::IDevice* object, const string& client, const JSONRPCSupportsEventStatus::Status status) override
        {
            string name;
            static_cast<const Exchange::ISimpleInstanceObjects::IDevice*>(object)->Name(name);

            TRACE(Trace::Information, (_T("Client '%s' %s for device '%s' state change notifications"), client.c_str(),
                status == Status::registered? "registered" : "unregistered", name.c_str()));

            // A JSON-RPC client registers for "statechanged" notifications, let them know the state of the device already.
            // Only the registering client will recieve this extra notification, via the default sendif method generated.
            if (status == Status::registered) {
                ImaginaryHost::Device* device = static_cast<ImaginaryHost::Device*>(object);

                Exchange::JSimpleInstanceObjects::Event::StateChanged(*this, _storage, object, device->State(), client);
            }
        }
        void OnPinChangedEventRegistration(Exchange::ISimpleInstanceObjects::IDevice* object, const string& client, const JSONRPCSupportsEventStatus::Status status) override
        {
            string name;
            static_cast<const Exchange::ISimpleInstanceObjects::IDevice*>(object)->Name(name);

            TRACE(Trace::Information, (_T("Client '%s' %s for device '%s' pin state change notifications"), client.c_str(),
                status == Status::registered? "registered" : "unregistered", name.c_str()));

            // A JSON-RPC client registered for "pinchanged" notifications, let them know the state of the high pins already
            // Only the registering client will recieve this extra notification, via the default sendif method generated.
            // The tricky part here is the client designator also carries index of the pin.
            if (status == Status::registered) {
                ImaginaryHost::Device* device = static_cast<ImaginaryHost::Device*>(object);

                device->IteratePins([this, object, client](const uint8_t index, const bool high) {

                    if (high == true) {
                        Exchange::JSimpleInstanceObjects::Event::PinChanged(*this, _storage, object, index, true, client);
                    }
                });
            }
        }

    private:
        PluginHost::IShell *_service;
        Core::SinkType<Notification> _notification;
        Exchange::ISimpleAsync* _imaginaryServer;
        Exchange::ISimpleInstanceObjects* _imaginaryHost;
        Exchange::JSimpleInstanceObjects::LookupStorage* _storage;
        std::map<Exchange::ISimpleInstanceObjects::IDevice*, Exchange::ISimpleInstanceObjects::IDevice::INotification*> _deviceNotificationSinks;

    }; // class GeneratorShowcase

} // namespace Plugin

}
