/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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

#include <example_interfaces/ISimpleAsync.h>
#include <example_interfaces/json/JSimpleAsync.h>

#include <example_interfaces/ISimpleInstanceObjects.h>
#include <example_interfaces/json/JSimpleInstanceObjects.h>

#include <example_interfaces/ISimpleCustomObjects.h>
#include <example_interfaces/json/JSimpleCustomObjects.h>

namespace Thunder {

namespace Plugin {

    class GeneratorShowcase : public PluginHost::IPlugin
                            , public PluginHost::JSONRPCSupportsEventStatus
                            , virtual public PluginHost::JSONRPCSupportsAutoObjectLookup
                            , virtual public PluginHost::JSONRPCSupportsObjectLookup
                            , public Example::JSimpleInstanceObjects::IHandler
                            , public Example::JSimpleCustomObjects::IHandler {
    public:
        GeneratorShowcase()
            : _service(nullptr)
            , _lock()
            , _connectionNotificationSink(*this)
            , _linkNotificationSink(*this)
            , _customNotificationSink(*this)
            , _imaginaryServer(nullptr)
            , _imaginaryHost(nullptr)
            , _imaginaryCustomHost(nullptr)
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
        class ImaginaryServer : public Example::ISimpleAsync {
        public:
            ImaginaryServer(GeneratorShowcase& parent)
                : _parent(parent)
                , _lock()
                , _job()
                , _timer()
                , _callback(nullptr)
                , _address()
                , _linkAddress()
                , _macAddress()
                , _linkMetadata(_T("empty"))
                , _bindType(_T("none"))
                , _observers()
                , _bindingObservers()
                , _state(Example::ISimpleAsync::DISCONNECTED)
            {
                _linkAddress[0] = 0xFF;
            }
            ~ImaginaryServer() override
            {
                _timer.Revoke();
                _job.Revoke();

                _lock.Lock();
                if (_callback != nullptr) {
                    // Closing but still a callback is installed
                    _callback->Complete(_address, Example::ISimpleAsync::CONNECTING_FAILED);
                    _callback->Release();
                }

                ASSERT(_observers.empty() == true);
                ASSERT(_bindingObservers.empty() == true);

                _lock.Unlock();
            }

            ImaginaryServer(const ImaginaryServer&) = delete;
            ImaginaryServer(ImaginaryServer&&) = delete;
            ImaginaryServer& operator=(const ImaginaryServer&) = delete;
            ImaginaryServer& operator=(ImaginaryServer&&) = delete;

        public:
            Core::hresult Tables(const bool fill, std::vector<string>& stringTables) override
            {
                if (fill == true) {
                    stringTables.push_back("garply");
                    stringTables.push_back("waldo");
                }

                return Core::ERROR_NONE;
            }
            Core::hresult Tables2(const bool fill, std::vector<string>& stringTables, std::vector<uint8_t>& intTables) override
            {
                if (fill == true) {
                    stringTables.push_back("garply");
                    stringTables.push_back("waldo");
                    intTables.push_back(1);
                    intTables.push_back(2);
                }

                return Core::ERROR_NONE;
            }
            Core::hresult Tables3(const bool fill, Core::OptionalType<std::vector<string>>& stringTables) override
            {
                if (fill == true) {
                    std::vector<string> t{"garply", "waldo"};
                    stringTables = std::move(t);
                }

                return Core::ERROR_NONE;
            }
            Core::hresult Tables4(const bool fill, RPC::IStringIterator*& stringTables) override
            {
                if (fill == true) {
                    std::vector<string> t{"garply", "waldo"};
                    stringTables = Core::ServiceType<RPC::StringIterator>::Create<RPC::IStringIterator>(std::move(t));
                }

                return Core::ERROR_NONE;
            }
            Core::hresult Tables5(const bool fill, RPC::IStringIterator*& stringTables, RPC::IValueIterator*& intTables) override
            {
                if (fill == true) {
                    std::vector<string> t{"garply", "waldo"};
                    stringTables = Core::ServiceType<RPC::StringIterator>::Create<RPC::IStringIterator>(std::move(t));
                    std::vector<uint32_t> t2{1, 2};
                    intTables = Core::ServiceType<RPC::ValueIterator>::Create<RPC::IValueIterator>(std::move(t2));
                }

                return Core::ERROR_NONE;
            }
            Core::hresult Tables6(const bool fill, Example::ISimpleAsync::Record& pod) override
            {
                if (fill == true) {
                    pod.param5.param0 = "fred";
                    pod.param5.param1 = true;
                    pod.param1 = { "garply", "waldo" };
                    pod.param3 = { "garply", "waldo", "fred" };
                }

                return Core::ERROR_NONE;
            }
            Core::hresult Tables7(const bool fill, Core::OptionalType<Example::ISimpleAsync::Record>& pod) override
            {
                if (fill == true) {
                    Example::ISimpleAsync::Record p;
                    p.param5.param0 = "fred";
                    p.param5.param1 = true;
                    p.param1 = { "garply", "waldo" };
                    p.param3 = { "garply", "waldo", "fred" };
                    pod = std::move(p);
                }

                return Core::ERROR_NONE;
            }
            Core::hresult Tables8(const bool fill, Core::OptionalType<Example::ISimpleAsync::Record2>& pod) override
            {
                if (fill == true) {
                    Example::ISimpleAsync::Record2 p;
                    p.param3 = { "garply", "waldo", "fred" };
                    pod = std::move(p);
                }
                return Core::ERROR_NONE;
            }
            Core::hresult Tables9(const bool fill, Example::ISimpleAsync::Record2& pod) override
            {
                if (fill == true) {
                    Example::ISimpleAsync::Record2 p;
                    p.param3 = { "garply", "waldo", "fred" };
                    pod = std::move(p);
                }

                return Core::ERROR_NONE;
            }
            Core::hresult OptionalResult(const bool fill, Core::OptionalType<string>& result) override
            {
                if (fill == true) {
                    result = "value";
                }

                return Core::ERROR_NONE;
            }

        public:
            Core::hresult Connect(const Core::OptionalType<std::vector<uint8_t>>& address, const Core::OptionalType<uint16_t>& timeout, Example::ISimpleAsync::ICallback* const cb) override
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_state == Example::ISimpleAsync::CONNECTED) {
                    TRACE(Trace::Error, (_T("Already connected")));
                    result = Core::ERROR_ALREADY_CONNECTED;
                }
                else if (_state == Example::ISimpleAsync::CONNECTING) {
                    TRACE(Trace::Error, (_T("Connection in progress")));
                    result = Core::ERROR_INPROGRESS;
                }
                else {
                    _state = Example::ISimpleAsync::CONNECTING;
                    _address = address.Value();
                    TRACE(Trace::Information, (_T("Connecting to %02x:%02x:%02x:%02x:%02x:%02x..."),
                        _address[0], _address[1], _address[2], _address[3], _address[4], _address[5]));

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

                        if (_state == Example::ISimpleAsync::CONNECTING) {

                            _state = Example::ISimpleAsync::DISCONNECTED;

                            // Timer fired, report a TIMED_OUT state
                            if (_callback != nullptr) {
                                _callback->Complete(_address, Example::ISimpleAsync::CONNECTING_TIMED_OUT);
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

                        if (_state == Example::ISimpleAsync::CONNECTING) {

                            _state = Example::ISimpleAsync::CONNECTED;

                            // Report CONNECTED state
                            if (_callback != nullptr) {
                                _callback->Complete(_address, Example::ISimpleAsync::CONNECTED);
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

                if (_state != Example::ISimpleAsync::CONNECTING) {
                    result = Core::ERROR_ILLEGAL_STATE;
                }
                else {
                    _state = Example::ISimpleAsync::DISCONNECTED;

                    if (_callback != nullptr) {
                        _callback->Complete(_address, Example::ISimpleAsync::CONNECTING_ABORTED);
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

                if (_state == Example::ISimpleAsync::CONNECTING) {
                    TRACE(Trace::Error, (_T("Connecting in progress")));
                    result = Core::ERROR_INPROGRESS;
                }
                else if (_state == Example::ISimpleAsync::DISCONNECTED) {
                    TRACE(Trace::Error, (_T("Not connected")));
                    result = Core::ERROR_ALREADY_RELEASED;
                }
                else {
                    _state = Example::ISimpleAsync::DISCONNECTED;
                    TRACE(Trace::Information, (_T("Disconnected")));
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult Connected(const std::vector<uint8_t>& address, bool& value) const override
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (address == _address) {
                    value = (_state == CONNECTED);
                }
                else {
                    result = Core::ERROR_UNKNOWN_KEY;
                }

                _lock.Unlock();

                return (result);
            }

        public:
            Core::hresult Register(INotification* const notification) override
            {
                Core::hresult result = Core::ERROR_NONE;

                ASSERT(notification != nullptr);

                _lock.Lock();

                auto it = std::find(_observers.begin(), _observers.end(), notification);
                ASSERT(it == _observers.end());

                if (it == _observers.end()) {
                    _observers.push_back(notification);
                    notification->AddRef();
                }
                else {
                    result = Core::ERROR_ALREADY_CONNECTED;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult Unregister(const INotification* const notification) override
            {
                Core::hresult result = Core::ERROR_NONE;

                ASSERT(notification != nullptr);

                _lock.Lock();

                auto it = std::find(_observers.begin(), _observers.end(), notification);
                ASSERT(it != _observers.end());

                if (it != _observers.end()) {
                    (*it)->Release();
                    _observers.erase(it);
                }
                else {
                    result = Core::ERROR_ALREADY_CONNECTED;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult Link(const uint8_t address[6]) override
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_linkAddress[0] == 0xFF) {
                    ::memcpy(_linkAddress, address, sizeof(_linkAddress));

                    for (auto observer : _observers) {
                        observer->StatusChanged(_linkAddress, true);
                    }
                }
                else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult Unlink(const uint8_t address[6]) override
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_linkAddress[0] != 0xFF) {
                    if (::memcmp(_linkAddress, address, sizeof(_linkAddress)) == 0) {

                        for (auto observer : _observers) {
                            observer->StatusChanged(_linkAddress, false);
                        }

                        _linkAddress[0] = 0xFF;
                    }
                    else {
                        result = Core::ERROR_UNKNOWN_KEY;
                    }
                }
                else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult LinkedDevice(uint8_t address[6]) const override
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_linkAddress[0] != 0xFF) {
                    ::memcpy(address, _linkAddress, 6);
                }
                else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult Metadata(const uint8_t address[6], const string& metadata) override
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_linkAddress[0] != 0xFF) {
                    if (::memcmp(address, _linkAddress, sizeof(_linkAddress)) == 0) {
                        _linkMetadata = metadata;
                    }
                    else {
                        result = Core::ERROR_UNKNOWN_KEY;
                    }
                }
                else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult Metadata(const uint8_t address[6], string& metadata) const override
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_linkAddress[0] != 0xFF) {
                    if (::memcmp(address, _linkAddress, sizeof(_linkAddress)) == 0) {
                        metadata = _linkMetadata;
                    }
                    else {
                        result = Core::ERROR_UNKNOWN_KEY;
                    }
                }
                else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }

                _lock.Unlock();

                return (result);
            }

        public:
            Core::hresult Bind(const Core::MACAddress& address) override
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_macAddress.IsValid() == false) {
                    _macAddress = address;

                    for (auto observer : _bindingObservers) {
                        observer->BindingChanged(_macAddress, true);
                    }
                }
                else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult Unbind(const Core::MACAddress& address) override
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_macAddress.IsValid() == true) {
                    if (address == _macAddress) {
                        for (auto observer : _bindingObservers) {
                            observer->BindingChanged(_macAddress, false);
                        }

                        _macAddress = {};
                    }
                    else {
                        result = Core::ERROR_UNKNOWN_KEY;
                    }
                }
                else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult BoundDevice(Core::MACAddress& address) const
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_macAddress.IsValid() == true) {
                    address = _macAddress;
                }
                else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult Type(const Core::MACAddress& address, const string& value)
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_macAddress.IsValid() == true) {
                    if (address == _macAddress) {
                        _bindType = value;
                    }
                    else {
                        result = Core::ERROR_UNKNOWN_KEY;
                    }
                }
                else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult Type(const Core::MACAddress& address, string& value) const
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                if (_macAddress.IsValid() == true) {
                    if (address == _macAddress) {
                        value = _bindType;
                    }
                    else {
                        result = Core::ERROR_UNKNOWN_KEY;
                    }
                }
                else {
                    result = Core::ERROR_ILLEGAL_STATE;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult Register(IBindNotification* const notification) override
            {
                Core::hresult result = Core::ERROR_NONE;

                ASSERT(notification != nullptr);

                _lock.Lock();

                auto it = std::find(_bindingObservers.begin(), _bindingObservers.end(), notification);
                ASSERT(it == _bindingObservers.end());

                if (it == _bindingObservers.end()) {
                    _bindingObservers.push_back(notification);
                    notification->AddRef();
                }
                else {
                    result = Core::ERROR_ALREADY_CONNECTED;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult Unregister(const IBindNotification* const notification) override
            {
                Core::hresult result = Core::ERROR_NONE;

                ASSERT(notification != nullptr);

                _lock.Lock();

                auto it = std::find(_bindingObservers.begin(), _bindingObservers.end(), notification);
                ASSERT(it != _bindingObservers.end());

                if (it != _bindingObservers.end()) {
                    (*it)->Release();
                    _bindingObservers.erase(it);
                }
                else {
                    result = Core::ERROR_ALREADY_RELEASED;
                }

                _lock.Unlock();

                return (result);
            }

        public:
            BEGIN_INTERFACE_MAP(ImaginaryServer)
                INTERFACE_ENTRY(Example::ISimpleAsync)
            END_INTERFACE_MAP

        private:
            GeneratorShowcase& _parent;
            mutable Core::CriticalSection _lock;
            DecoupledJob _job;
            DecoupledJob _timer;
            Example::ISimpleAsync::ICallback* _callback;
            std::vector<uint8_t> _address;
            uint8_t _linkAddress[6];
            Core::MACAddress _macAddress;
            string _linkMetadata;
            string _bindType;
            std::vector<Example::ISimpleAsync::INotification*> _observers;
            std::vector<Example::ISimpleAsync::IBindNotification*> _bindingObservers;
            Example::ISimpleAsync::state _state;
        }; // class ImaginaryServer

    private:
        class ImaginaryHost : public Example::ISimpleInstanceObjects {
        public:
            class DeviceImpl : public Example::ISimpleInstanceObjects::IDevice {
            public:
                DeviceImpl(GeneratorShowcase& parent, const string& name)
                    : _parent(parent)
                    , _lock()
                    , _job()
                    , _name(name)
                    , _state(Example::ISimpleInstanceObjects::state::DISABLED)
                    , _pins()
                    , _observers()
                {
                }
                ~DeviceImpl() override
                {
                    _job.Revoke();

                    ASSERT(_observers.empty() == true);
                }

                DeviceImpl() = delete;
                DeviceImpl(const DeviceImpl&) = delete;
                DeviceImpl(DeviceImpl&&) = delete;
                DeviceImpl& operator=(const DeviceImpl&) = delete;
                DeviceImpl& operator=(DeviceImpl&&) = delete;

            public:
                Core::hresult Register(INotification* const notification) override
                {
                    Core::hresult result = Core::ERROR_NONE;

                    ASSERT(notification != nullptr);

                    _lock.Lock();

                    auto it = std::find(_observers.begin(), _observers.end(), notification);
                    ASSERT(it == _observers.end());

                    if (it == _observers.end()) {
                        _observers.push_back(notification);
                        notification->AddRef();

                        notification->StateChanged(_state);

                        IteratePins([notification](const uint8_t pin, const bool high) {
                            notification->PinChanged(pin, high);
                        });
                    }
                    else {
                        result = Core::ERROR_ALREADY_CONNECTED;
                    }

                    _lock.Unlock();

                    return (result);
                }
                Core::hresult Unregister(const INotification* const notification) override
                {
                    Core::hresult result = Core::ERROR_NONE;

                    ASSERT(notification != nullptr);

                    _lock.Lock();

                    auto it = std::find(_observers.begin(), _observers.end(), notification);
                    ASSERT(it != _observers.end());

                    if (it != _observers.end()) {
                        (*it)->Release();
                        _observers.erase(it);
                    }
                    else {
                        result = Core::ERROR_ALREADY_RELEASED;
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
                        SetState(Example::ISimpleInstanceObjects::ENABLED);
                    }, 1000);

                    return (Core::ERROR_NONE);
                }
                Core::hresult Disable() override
                {
                    SetState(Example::ISimpleInstanceObjects::DISABLED);
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
                Example::ISimpleInstanceObjects::state State() const
                {
                    return (_state);
                }
                void IteratePins(const std::function<void(const uint8_t pin, const bool high)>& cb) const
                {
                    _lock.Lock();

                    for (auto const& pin : _pins) {
                        cb(pin.first, pin.second);
                    }

                    _lock.Unlock();
                }
                bool PinStatus(const uint8_t pin) const
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

                        TRACE(Trace::Information, (_T("Name changed from '%s' to '%s'"), _name.c_str(), name.c_str()));

                        _name = name;

                        for (auto const& observer : _observers) {
                            observer->NameChanged(name);
                        }
                    }

                    _lock.Unlock();
                }
                void SetState(const Example::ISimpleInstanceObjects::state state)
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
                BEGIN_INTERFACE_MAP(Devicempl)
                    INTERFACE_ENTRY(Example::ISimpleInstanceObjects::IDevice)
                END_INTERFACE_MAP

            private:
                GeneratorShowcase& _parent;
                mutable Core::CriticalSection _lock;
                DecoupledJob _job;
                string _name;
                Example::ISimpleInstanceObjects::state _state;
                std::map<uint8_t, bool> _pins;
                std::vector<Example::ISimpleInstanceObjects::IDevice::INotification*> _observers;

            }; // class Device

        public:
            ImaginaryHost(GeneratorShowcase& parent)
                : _parent(parent)
                , _lock()
            {
            }
            ~ImaginaryHost() override = default;

            ImaginaryHost(const ImaginaryHost&) = delete;
            ImaginaryHost(ImaginaryHost&&) = delete;
            ImaginaryHost& operator=(const ImaginaryHost&) = delete;
            ImaginaryHost& operator=(ImaginaryHost&&) = delete;

        public:
            Core::hresult Acquire(const string& name, Example::ISimpleInstanceObjects::IDevice*& device) override
            {
                Core::hresult result = Core::ERROR_NONE;

                auto it = _devices.find(name);

                if (it != _devices.end()) {
                    result = Core::ERROR_UNAVAILABLE;
                    TRACE(Trace::Error, (_T("Device unavailable")));
                }
                else {
                    DeviceImpl* const dev = Core::ServiceType<DeviceImpl>::Create<DeviceImpl>(_parent, name);
                    ASSERT(dev != nullptr);

                    device = dev;

                    _devices.emplace(name, dev);
                    dev->AddRef();
                }

                return (result);
            }
            Core::hresult Relinquish(Example::ISimpleInstanceObjects::IDevice* const device) override
            {
                Core::hresult result = Core::ERROR_NONE;

                if (device == nullptr) {
                    result = Core::ERROR_BAD_REQUEST;
                }
                else {
                    string deviceName;
                    static_cast<const Example::ISimpleInstanceObjects::IDevice*>(device)->Name(deviceName);

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
                INTERFACE_ENTRY(Example::ISimpleInstanceObjects)
            END_INTERFACE_MAP

        private:
            GeneratorShowcase& _parent;
            mutable Core::CriticalSection _lock;
            std::map<string, DeviceImpl*> _devices;

        }; // class ImaginaryHost

    private:
        class ImaginaryCustomHost : public Example::ISimpleCustomObjects {
        public:
            class AccessoryImpl : public Example::ISimpleCustomObjects::IAccessory {
            public:
                AccessoryImpl(GeneratorShowcase& parent, const string& id, const string& name)
                    : _parent(parent)
                    , _lock()
                    , _id(id)
                    , _name(name)
                    , _observers()
                {
                }
                ~AccessoryImpl() override
                {
                    ASSERT(_observers.empty() == true);
                }

                AccessoryImpl() = delete;
                AccessoryImpl(const AccessoryImpl&) = delete;
                AccessoryImpl(AccessoryImpl&&) = delete;
                AccessoryImpl& operator=(const AccessoryImpl&) = delete;
                AccessoryImpl& operator=(AccessoryImpl&&) = delete;

            public:
                Core::hresult Register(INotification* const notification) override
                {
                    Core::hresult result = Core::ERROR_NONE;

                    ASSERT(notification != nullptr);

                    _lock.Lock();

                    auto it = std::find(_observers.begin(), _observers.end(), notification);
                    ASSERT(it == _observers.end());

                    if (it == _observers.end()) {
                        _observers.push_back(notification);
                        notification->AddRef();
                        notification->NameChanged(_name);
                    }
                    else {
                        result = Core::ERROR_ALREADY_CONNECTED;
                    }

                    _lock.Unlock();

                    return (result);
                }
                Core::hresult Unregister(const INotification* const notification) override
                {
                    Core::hresult result = Core::ERROR_NONE;

                    ASSERT(notification != nullptr);

                    _lock.Lock();

                    auto it = std::find(_observers.begin(), _observers.end(), notification);
                    ASSERT(it != _observers.end());

                    if (it != _observers.end()) {
                        (*it)->Release();
                        _observers.erase(it);
                    }
                    else {
                        result = Core::ERROR_ALREADY_RELEASED;
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
                const string& Id() const
                {
                    return (_id);
                }

            public:
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

                        TRACE(Trace::Information, (_T("Name changed from '%s' to '%s'"), _name.c_str(), name.c_str()));

                        _name = name;

                        for (auto const& observer : _observers) {
                            observer->NameChanged(name);
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
                        }
                    }
                    else {
                        _pins.emplace(pin, high);
                    }

                    _lock.Unlock();
                }

            public:
                BEGIN_INTERFACE_MAP(AccessoryImpl)
                    INTERFACE_ENTRY(Example::ISimpleCustomObjects::IAccessory)
                END_INTERFACE_MAP

            private:
                GeneratorShowcase& _parent;
                mutable Core::CriticalSection _lock;
                string _id;
                string _name;
                std::map<uint8_t, bool> _pins;
                std::vector<Example::ISimpleCustomObjects::IAccessory::INotification*> _observers;
            }; // class AccessoryImpl

        public:
            ImaginaryCustomHost(GeneratorShowcase& parent)
                : _parent(parent)
                , _lock()
                , _accessories()
                , _observers()
            {
                // populate some accessories...
                _accessories.emplace_back(Core::ServiceType<AccessoryImpl>::Create<AccessoryImpl>(parent, _T("usb0"), _T("mouse")));
                _accessories.emplace_back(Core::ServiceType<AccessoryImpl>::Create<AccessoryImpl>(parent, _T("usb1"), _T("pendrive")));
                _accessories.emplace_back(Core::ServiceType<AccessoryImpl>::Create<AccessoryImpl>(parent, _T("hci0"), _T("headset")));

            }
            ~ImaginaryCustomHost() override
            {
                _lock.Lock();

                for (auto acc : _accessories) {
                    acc->Release();
                }

                _accessories.clear();

                _lock.Unlock();
            }

            ImaginaryCustomHost(const ImaginaryCustomHost&) = delete;
            ImaginaryCustomHost(ImaginaryCustomHost&&) = delete;
            ImaginaryCustomHost& operator=(const ImaginaryCustomHost&) = delete;
            ImaginaryCustomHost& operator=(ImaginaryCustomHost&&) = delete;

        public:
            Core::hresult Register(INotification* const notification) override
            {
                Core::hresult result = Core::ERROR_NONE;

                ASSERT(notification != nullptr);

                _lock.Lock();

                auto it = std::find(_observers.begin(), _observers.end(), notification);
                ASSERT(it == _observers.end());

                if (it == _observers.end()) {
                    _observers.push_back(notification);
                    notification->AddRef();

                    IterateAccessories([notification](AccessoryImpl* const acc){
                        notification->Added(acc);
                    });
                }
                else {
                    result = Core::ERROR_ALREADY_CONNECTED;
                }

                _lock.Unlock();

                return (result);
            }

            Core::hresult Unregister(const INotification* const notification) override
            {
                Core::hresult result = Core::ERROR_NONE;

                ASSERT(notification != nullptr);

                _lock.Lock();

                auto it = std::find(_observers.begin(), _observers.end(), notification);
                ASSERT(it != _observers.end());

                if (it != _observers.end()) {
                    (*it)->Release();
                    _observers.erase(it);
                }
                else {
                    result = Core::ERROR_ALREADY_RELEASED;
                }

                _lock.Unlock();

                return (result);
            }

            // Name of the device is not related to its ID
            Core::hresult Accessory(const string& name, Example::ISimpleCustomObjects::IAccessory*& accessory) const override
            {
                Core::hresult result = Core::ERROR_NONE;

                _lock.Lock();

                auto it = std::find_if(_accessories.begin(), _accessories.end(), [&name](const AccessoryImpl* const acc) {
                    string n;
                    acc->Name(n);
                    return (n == name);
                });

                if (it != _accessories.end()) {
                    AccessoryImpl* acc = (*it);
                    ASSERT(acc != nullptr);

                    acc->AddRef();
                    accessory = acc;
                }
                else {
                    TRACE(Trace::Error, (_T("No accessory with name '%s' found"), name.c_str()));
                    result = Core::ERROR_UNKNOWN_KEY;
                }

                _lock.Unlock();

                return (result);
            }

        public:
            // Custom lookup routines

            AccessoryImpl* LookUp(const string &id, const Core::JSONRPC::Context& context)
            {
                AccessoryImpl* accessory{};

                // Context only for demonstration purposes here!
                (void) context;

                if (id.empty() == false) {
                    _lock.Lock();

                    auto it = std::find_if(_accessories.begin(), _accessories.end(), [&id](const AccessoryImpl* const acc) {
                        return (acc->Id() == id);
                    });

                    if (it != _accessories.end()) {
                        AccessoryImpl* acc = (*it);
                        ASSERT(acc != nullptr);

                        acc->AddRef();
                        accessory = acc;
                    }
                    else {
                        TRACE(Trace::Error, (_T("Failed to look up id '%s'"), id.c_str()));
                    }

                    _lock.Unlock();
                }

                return (accessory);
            }
            string LookUp(const Core::IUnknown* const object, const Core::JSONRPC::Context&)
            {
                string id;

                ASSERT(object != nullptr);

                const Example::ISimpleCustomObjects::IAccessory* const acc = object->QueryInterface<Example::ISimpleCustomObjects::IAccessory>();

                // This would be odd.
                ASSERT(acc != nullptr);

                if (acc != nullptr) {
                    id = static_cast<const ImaginaryCustomHost::AccessoryImpl*>(acc)->Id();
                    acc->Release();
                }

                return (id);
            }

        public:
            void IterateAccessories(const std::function<void(AccessoryImpl*)>& observer)
            {
                _lock.Lock();

                for (auto& acc : _accessories) {
                    observer(acc);
                }

                _lock.Unlock();
            }

        public:
            BEGIN_INTERFACE_MAP(ImaginaryCustomHost)
                INTERFACE_ENTRY(Example::ISimpleCustomObjects)
            END_INTERFACE_MAP

        private:
            GeneratorShowcase& _parent;
            mutable Core::CriticalSection _lock;
            std::vector<AccessoryImpl*> _accessories;
            std::vector<Example::ISimpleCustomObjects::INotification*> _observers;

        }; // class ImaginaryCustomHost

    public:
        const string Initialize(PluginHost::IShell* service) override
        {
            string message{};

            ASSERT(service != nullptr);
            ASSERT(_service == nullptr);

            _service = service;
            _service->AddRef();

            _imaginaryServer = Core::ServiceType<ImaginaryServer>::Create<ImaginaryServer>(*this);
            _imaginaryHost = Core::ServiceType<ImaginaryHost>::Create<ImaginaryHost>(*this);
            _imaginaryCustomHost = Core::ServiceType<ImaginaryCustomHost>::Create<ImaginaryCustomHost>(*this);

            ASSERT(_imaginaryServer != nullptr);
            ASSERT(_imaginaryHost != nullptr);
            ASSERT(_imaginaryCustomHost != nullptr);

            // The example async interface
            Example::JSimpleAsync::Register(*this, _imaginaryServer);

            // The example lookup object interface
            Example::JSimpleInstanceObjects::Register(*this, _imaginaryHost, this);

            // The example custom lookup object interface
            // Before registering with a custom lookup interface, handlers for each looked up interface need to be installed!
            PluginHost::JSONRPCSupportsObjectLookup::template Add<Example::ISimpleCustomObjects::IAccessory>(
                // Handler for obj to id translation.
                // (Context can be omitted from the lambda altogheter if not needed.)
                [this](const Core::JSONRPC::Context& context, const Core::IUnknown* object) -> string {
                    const string id = _imaginaryCustomHost->LookUp(object, context);

                    // It's impossible for the id to be empty.
                    ASSERT(id.empty() == false);

                    return (id);
                },
                // Handler for id to obj translation.
                [this](const Core::JSONRPC::Context& context, const string& id) -> Core::IUnknown* {
                    Core::IUnknown* const object = _imaginaryCustomHost->LookUp(id, context);

                    // It's okay to return nullptr if the id is not known.
                    return (object);
                }
            );

            Example::JSimpleCustomObjects::Register(*this, _imaginaryCustomHost, this);

            // Register for channel closures
            service->Register(&_connectionNotificationSink);

            _imaginaryServer->Register(static_cast<Example::ISimpleAsync::INotification*>(&_linkNotificationSink));
            _imaginaryServer->Register(static_cast<Example::ISimpleAsync::IBindNotification*>(&_linkNotificationSink));

            // If needed, it's possible to install callbacks when devices are acquired/relinquished.
            // The relinquish callback also fires on channel closure.
            // Here it's used to store device notifcation sinks.
            Callback([this](const bool acquired, const uint32_t, Example::ISimpleInstanceObjects::IUnknown* obj) {
                ASSERT(obj != nullptr);

                Example::ISimpleInstanceObjects::IDevice* device = obj->QueryInterface<Example::ISimpleInstanceObjects::IDevice>();
                if (device != nullptr) {
                    if (acquired == true) {
                        // device was acquired
                        string name;
                        static_cast<const Example::ISimpleInstanceObjects::IDevice*>(device)->Name(name);
                        TRACE(Trace::Information, (_T("Device '%s' acquired"), name.c_str()));

                        DeviceNotificationImpl* const impl = Core::ServiceType<DeviceNotificationImpl>::Create<DeviceNotificationImpl>(*this, device);
                        ASSERT(impl != nullptr);

                        device->AddRef();

                        _lock.Lock();

                        ASSERT(_deviceNotificationSinks.find(device) == _deviceNotificationSinks.end());
                        _deviceNotificationSinks.emplace(device, impl);

                        device->Register(impl);
                        _lock.Unlock();
                    }
                    else {
                        // device is about to be relinquished
                        string name;
                        static_cast<const Example::ISimpleInstanceObjects::IDevice*>(device)->Name(name);
                        TRACE(Trace::Information, (_T("Device '%s' released"), name.c_str()));

                        _lock.Lock();

                        auto it = _deviceNotificationSinks.find(device);
                        ASSERT(_deviceNotificationSinks.find(device) != _deviceNotificationSinks.end());

                        if (it != _deviceNotificationSinks.end()) {
                            DeviceNotificationImpl* const impl = (*it).second;
                            ASSERT(impl != nullptr);
                            device->Unregister(impl);
                            impl->Release();
                            device->Release();
                            _deviceNotificationSinks.erase(it);
                        }

                        _lock.Unlock();
                    }

                    device->Release();
                }
            });

            _imaginaryCustomHost->Register(&_customNotificationSink);

            return (message);
        }

        void Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED) override
        {
            ASSERT(_service == service);

            // Tear down in the opposite order...

            _service->Unregister(&_connectionNotificationSink);

            if (_imaginaryServer != nullptr) {
                _imaginaryServer->Unregister(static_cast<Example::ISimpleAsync::INotification*>(&_linkNotificationSink));
                _imaginaryServer->Unregister(static_cast<Example::ISimpleAsync::IBindNotification*>(&_linkNotificationSink));
                Example::JSimpleAsync::Unregister(*this);
                _imaginaryServer->Release();
                _imaginaryServer = nullptr;
            }

            if (_imaginaryHost != nullptr) {
                Callback(nullptr);
                Example::JSimpleInstanceObjects::Unregister(*this);
                _imaginaryHost->Release();
                _imaginaryHost = nullptr;
            }

            // Uninstall the handler first!
            PluginHost::JSONRPCSupportsObjectLookup::Remove<Example::ISimpleCustomObjects::IAccessory>();

            if (_imaginaryCustomHost != nullptr) {
                _imaginaryCustomHost->Unregister(&_customNotificationSink);
                Example::JSimpleCustomObjects::Unregister(*this);
                _imaginaryCustomHost->Release();
                _imaginaryCustomHost = nullptr;
            }

            _service->Release();
            _service = nullptr;
        }

        string Information() const override
        {
            return {};
        }

    public:
        BEGIN_INTERFACE_MAP(GeneratorShowcase)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Example::ISimpleAsync, _imaginaryServer)
            INTERFACE_AGGREGATE(Example::ISimpleInstanceObjects, _imaginaryHost)
            INTERFACE_AGGREGATE(Example::ISimpleCustomObjects, _imaginaryCustomHost)
        END_INTERFACE_MAP

    private:
        class DeviceNotificationImpl : public Example::ISimpleInstanceObjects::IDevice::INotification {
        public:
            DeviceNotificationImpl(GeneratorShowcase& parent, Example::ISimpleInstanceObjects::IDevice* const device)
                : _parent(parent)
                , _device(device)
            {
                ASSERT(device != nullptr);
                _device->AddRef();
            }
            ~DeviceNotificationImpl()
            {
                _device->Release();
            }

            DeviceNotificationImpl() = delete;
            DeviceNotificationImpl(const DeviceNotificationImpl&) = delete;
            DeviceNotificationImpl(DeviceNotificationImpl&&) = delete;
            DeviceNotificationImpl& operator=(const DeviceNotificationImpl&) = delete;
            DeviceNotificationImpl& operator=(DeviceNotificationImpl&&) = delete;

        public:
            void NameChanged(const string& name) override
            {
                Example::JSimpleInstanceObjects::Event::NameChanged(_parent, _device, name);
            }
            void StateChanged(const Example::ISimpleInstanceObjects::state state) override
            {
                Example::JSimpleInstanceObjects::Event::StateChanged(_parent, _device, state);
            }
            void PinChanged(const uint8_t pin, const bool high) override
            {
                Example::JSimpleInstanceObjects::Event::PinChanged(_parent, _device, pin, high);
            }

        public:
            BEGIN_INTERFACE_MAP(DeviceNotificationImpl)
                INTERFACE_ENTRY(Example::ISimpleInstanceObjects::IDevice::INotification)
            END_INTERFACE_MAP

        private:
            GeneratorShowcase& _parent;
            Example::ISimpleInstanceObjects::IDevice* _device;
        }; // class DeviceNotificationImpl

        class AccessoryNotificationImpl : public Example::ISimpleCustomObjects::IAccessory::INotification {
        public:
            AccessoryNotificationImpl(GeneratorShowcase& parent, Example::ISimpleCustomObjects::IAccessory* const accessory)
                : _parent(parent)
                , _accessory(accessory)
            {
                ASSERT(accessory != nullptr);
                _accessory->AddRef();
            }
            ~AccessoryNotificationImpl() override
            {
                _accessory->Release();
            }

            AccessoryNotificationImpl() = delete;
            AccessoryNotificationImpl(const AccessoryNotificationImpl&) = delete;
            AccessoryNotificationImpl(AccessoryNotificationImpl&&) = delete;
            AccessoryNotificationImpl& operator=(const AccessoryNotificationImpl&) = delete;
            AccessoryNotificationImpl& operator=(AccessoryNotificationImpl&&) = delete;

        public:
            void NameChanged(const string& name) override
            {
                Example::JSimpleCustomObjects::Event::NameChanged(_parent, _accessory, name);
            }

        public:
            BEGIN_INTERFACE_MAP(AccessoryNotificationImpl)
                INTERFACE_ENTRY(Example::ISimpleCustomObjects::IAccessory::INotification)
            END_INTERFACE_MAP

        private:
            GeneratorShowcase& _parent;
            Example::ISimpleCustomObjects::IAccessory* _accessory;

        }; // class AccessoryNotificationImpl

        class ImaginaryServerNotificationImpl : public Example::ISimpleAsync::INotification
                                              , public Example::ISimpleAsync::IBindNotification {
        public:
            ImaginaryServerNotificationImpl(GeneratorShowcase& parent)
                : _parent(parent)
            {
            }
            ~ImaginaryServerNotificationImpl() override = default;

            ImaginaryServerNotificationImpl() = delete;
            ImaginaryServerNotificationImpl(const ImaginaryServerNotificationImpl&) = delete;
            ImaginaryServerNotificationImpl(ImaginaryServerNotificationImpl&&) = delete;
            ImaginaryServerNotificationImpl& operator=(const ImaginaryServerNotificationImpl&) = delete;
            ImaginaryServerNotificationImpl& operator=(ImaginaryServerNotificationImpl&&) = delete;

        public:
            void StatusChanged(const uint8_t address[6], const bool linked) override
            {
                Example::JSimpleAsync::Event::StatusChanged(_parent, address, linked);
            }
            void BindingChanged(const Core::MACAddress& address, const bool bound) override
            {
                Example::JSimpleAsync::Event::BindingChanged(_parent, address, bound);
            }

        public:
            BEGIN_INTERFACE_MAP(ImaginaryServerNotificationImpl)
                INTERFACE_ENTRY(Example::ISimpleAsync::INotification)
                INTERFACE_ENTRY(Example::ISimpleAsync::IBindNotification)
            END_INTERFACE_MAP

        private:
            GeneratorShowcase& _parent;
        }; // class ImaginaryServerNotificationImpl

        class ImaginaryCustomHostNotificationImpl : public Example::ISimpleCustomObjects::INotification {
        public:
            ImaginaryCustomHostNotificationImpl(GeneratorShowcase& parent)
                : _parent(parent)
                , _lock()
                , _sinks()
            {
            }
            ~ImaginaryCustomHostNotificationImpl() override
            {
                _lock.Lock();

                for (auto sink : _sinks) {
                    sink.first->Release();
                    sink.second->Release();
                }

                _sinks.clear();

                _lock.Unlock();
            }

            ImaginaryCustomHostNotificationImpl() = delete;
            ImaginaryCustomHostNotificationImpl(const ImaginaryCustomHostNotificationImpl&) = delete;
            ImaginaryCustomHostNotificationImpl(ImaginaryCustomHostNotificationImpl&&) = delete;
            ImaginaryCustomHostNotificationImpl& operator=(const ImaginaryCustomHostNotificationImpl&) = delete;
            ImaginaryCustomHostNotificationImpl& operator=(ImaginaryCustomHostNotificationImpl&&) = delete;

        public:
            void Added(Example::ISimpleCustomObjects::IAccessory* const accessory) override
            {
                ASSERT(accessory != nullptr);

                AccessoryNotificationImpl* impl = Core::ServiceType<AccessoryNotificationImpl>::Create<AccessoryNotificationImpl>(_parent, accessory);
                ASSERT(impl != nullptr);

                _lock.Lock();

                ASSERT(_sinks.find(accessory) == _sinks.end());
                _sinks.emplace(accessory, impl);

                _lock.Unlock();

                accessory->AddRef();
                accessory->Register(impl);

                Example::JSimpleCustomObjects::Event::Added(_parent, accessory);
            }

            void Removed(Example::ISimpleCustomObjects::IAccessory* const accessory) override
            {
                ASSERT(accessory != nullptr);

                _lock.Lock();

                auto it = _sinks.find(accessory);
                ASSERT(it != _sinks.end());

                if (it != _sinks.end()) {
                    AccessoryNotificationImpl* impl = (*it).second;
                    accessory->Unregister(impl);
                    accessory->Release();
                    impl->Release();
                    _sinks.erase(it);
                }

                _lock.Unlock();

                Example::JSimpleCustomObjects::Event::Removed(_parent, accessory);
            }

        public:
            BEGIN_INTERFACE_MAP(ImaginaryCustomHostNotificationImpl)
                INTERFACE_ENTRY(Example::ISimpleCustomObjects::INotification)
            END_INTERFACE_MAP

        private:
            GeneratorShowcase& _parent;
            mutable Core::CriticalSection _lock;
            std::map<Example::ISimpleCustomObjects::IAccessory*, AccessoryNotificationImpl*> _sinks;
        }; // class ImaginaryCustomHostNotificationImpl

    private:
        // ISimpleInstanceObjects statuslistener callbacks
        void OnStateChangedEventRegistration(Example::ISimpleInstanceObjects::IDevice* object, const string& client, const PluginHost::JSONRPCSupportsEventStatus::Status status) override
        {
            string name;
            static_cast<const Example::ISimpleInstanceObjects::IDevice*>(object)->Name(name);

            TRACE(Trace::Information, (_T("Client '%s' %s for device '%s' state change notifications"), client.c_str(),
                status == PluginHost::JSONRPCSupportsEventStatus::Status::registered? "registered" : "unregistered", name.c_str()));

            // A JSON-RPC client registers for "statechanged" notifications, let them know the state of the device already.
            // Only the registering client will recieve this extra notification, via the default sendif method generated.
            if (status == PluginHost::JSONRPCSupportsEventStatus::Status::registered) {
                ImaginaryHost::DeviceImpl* device = static_cast<ImaginaryHost::DeviceImpl*>(object);

                Example::JSimpleInstanceObjects::Event::StateChanged(*this, object, device->State(), client);
            }
        }
        void OnPinChangedEventRegistration(Example::ISimpleInstanceObjects::IDevice* object, const string& client, const string& index, const PluginHost::JSONRPCSupportsEventStatus::Status status) override
        {
            string name;
            static_cast<const Example::ISimpleInstanceObjects::IDevice*>(object)->Name(name);

            TRACE(Trace::Information, (_T("Client '%s' %s for device '%s' pin %s state change notifications"), client.c_str(),
                status == PluginHost::JSONRPCSupportsEventStatus::Status::registered? "registered" : "unregistered", name.c_str(), index.c_str()));

            // A JSON-RPC client registered for "pinchanged" notifications, let them know the state if the pin is lit already.
            // Only the registering client will recieve this extra notification, via the default sendif method generated.
            if (status == PluginHost::JSONRPCSupportsEventStatus::Status::registered) {
                ImaginaryHost::DeviceImpl* device = static_cast<ImaginaryHost::DeviceImpl*>(object);

                const uint8_t pin = ::atoi(index.c_str());
                if (device->PinStatus(pin) == true) {
                    Example::JSimpleInstanceObjects::Event::PinChanged(*this, object, pin, true, client);
                }
            }
        }

    private:
        // ISimpleCustomObjects statuslistener callbacks
        void OnNameChangedEventRegistration(Example::ISimpleCustomObjects::IAccessory* object, const string& client, const PluginHost::JSONRPCSupportsEventStatus::Status status) override
        {
            string name;
            static_cast<const Example::ISimpleCustomObjects::IAccessory*>(object)->Name(name);

            TRACE(Trace::Information, (_T("Client '%s' %s for accessory '%s' name change notifications"), client.c_str(),
                status == PluginHost::JSONRPCSupportsEventStatus::Status::registered? "registered" : "unregistered", name.c_str()));

            if (status == PluginHost::JSONRPCSupportsEventStatus::Status::registered) {
                Example::JSimpleCustomObjects::Event::NameChanged(*this, object, name, client);
            }
        }
        void OnAddedEventRegistration(const string& client, const PluginHost::JSONRPCSupportsEventStatus::Status status) override
        {
            TRACE(Trace::Information, (_T("Client '%s' %s for accessory added notifications"), client.c_str(),
                status == PluginHost::JSONRPCSupportsEventStatus::Status::registered? "registered" : "unregistered"));

            if (status == PluginHost::JSONRPCSupportsEventStatus::Status::registered) {

                ASSERT(_imaginaryCustomHost != nullptr);

                _imaginaryCustomHost->IterateAccessories([this, &client](ImaginaryCustomHost::AccessoryImpl* acc) {

                    ASSERT(acc != nullptr);
                    Example::JSimpleCustomObjects::Event::Added(*this, acc, client);
                });
            }
        }

    private:
        PluginHost::IShell *_service;
        Core::CriticalSection _lock;
        Core::SinkType<Notification> _connectionNotificationSink;
        Core::SinkType<ImaginaryServerNotificationImpl> _linkNotificationSink;
        Core::SinkType<ImaginaryCustomHostNotificationImpl> _customNotificationSink;
        ImaginaryServer* _imaginaryServer;
        ImaginaryHost* _imaginaryHost;
        ImaginaryCustomHost* _imaginaryCustomHost;
        std::map<Example::ISimpleInstanceObjects::IDevice*, DeviceNotificationImpl*> _deviceNotificationSinks;

    }; // class GeneratorShowcase

} // namespace Plugin

}
