// Copyright (c) 2022 Metrological. All rights reserved.

#pragma once

#include "Module.h"
#include "RequestSender.h"
#include <unordered_set>

namespace WPEFramework {
namespace Plugin {
    class BackOffice : public PluginHost::IPlugin {
    private:
        class StateChangeObserver {
        public:
            enum State {
                ACTIVATED,
                DEACTIVATED,
                UNAVAILABLE,
                RESUMED,
                SUSPENDED,
                UNKNOWN
            };

            static State Convert(PluginHost::IStateControl::state state)
            {
                switch (state) {
                case PluginHost::IStateControl::RESUMED:
                    return State::RESUMED;
                case PluginHost::IStateControl::SUSPENDED:
                    return State::SUSPENDED;
                case PluginHost::IStateControl::UNINITIALIZED:
                default:
                    return State::UNKNOWN;
                }
            }
            static string Convert(State state)
            {
                switch (state) {
                case ACTIVATED:
                    return _T("Activated");
                case DEACTIVATED:
                    return _T("Deactivated");
                case UNAVAILABLE:
                    return _T("Unavailable");
                case RESUMED:
                    return _T("Resumed");
                case SUSPENDED:
                    return _T("Suspended");
                case UNKNOWN:
                    return _T("Unknown");
                default:
                    Trace::Error(_T("Unknown mapping found!"));
                    return _T("Unknown");
                }
            }

            using Callback = std::function<void(State, const string&)>;

            class OperationalObserver : public PluginHost::IPlugin::INotification {
            public:
                OperationalObserver(StateChangeObserver& parent)
                    : _parent(parent)
                {
                }
                ~OperationalObserver() override = default;
                OperationalObserver(const OperationalObserver&) = delete;
                OperationalObserver& operator=(const OperationalObserver&) = delete;

                BEGIN_INTERFACE_MAP(OperationalObserver)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
                END_INTERFACE_MAP

            private:

#if THUNDER_VERSION >= 3
                void Activated(const string& callsign, PluginHost::IShell* service) override
                {
                    ASSERT(service != nullptr);

                    if (_parent._callback != nullptr) {
                        if (_parent._callsigns.count(callsign) != 0) {
                            _parent.CreateObservable(callsign, service);
                            _parent._callback(State::ACTIVATED, callsign);
                        }
                    }
                }
                void Deactivated(const string& callsign, PluginHost::IShell*) override
                {
                    if (_parent._callback != nullptr) {
                        if (_parent._callsigns.count(callsign) != 0) {
                            _parent.DestroyObservable(callsign);
                            _parent._callback(State::DEACTIVATED, callsign);
                        }
                    }
                }
                void Unavailable(const string& callsign, PluginHost::IShell*) override
                {
                    if (_parent._callback != nullptr) {
                        if ((_parent._callsigns.count(callsign) != 0)) {
                            _parent._callback(State::UNAVAILABLE, callsign);
                        }
                    }
                }
#else
                void StateChange(PluginHost::IShell* plugin) override
                {
                    ASSERT(plugin != nullptr);

                    if (_parent._callback != nullptr) {
                        auto callsign = plugin->Callsign();
                        if ((_parent._callsigns.count(callsign) != 0)) {
                            switch (plugin->State()) {
                            case PluginHost::IShell::ACTIVATED:
                                _parent._callback(State::ACTIVATED, callsign);
                                break;
                            case PluginHost::IShell::DEACTIVATED:
                                _parent._callback(State::DEACTIVATED, callsign);
                                break;
                            default:
                                Trace::Information(_T("Change to unknown state, ignoring."));
                                break;
                            }
                        }
                    }
                }
#endif

            private:
                StateChangeObserver& _parent;
            };

            class StandbyObserver {
            private:
                class Notification : public PluginHost::IStateControl::INotification {
                public:
                    explicit Notification(const std::function<void(StateChangeObserver::State)>& callback)
                        : _callback(callback)
                    {
                    }
                    ~Notification() override = default;

                    Notification(const Notification&) = delete;
                    Notification& operator=(const Notification&) = delete;

                    BEGIN_INTERFACE_MAP(Notification)
                    INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
                    END_INTERFACE_MAP

                private:
                    void StateChange(const PluginHost::IStateControl::state state) override
                    {
                        _callback(Convert(state));
                    }

                    std::function<void(StateChangeObserver::State)> _callback;
                };

            public:
                StandbyObserver(const std::function<void(StateChangeObserver::State)>& callback, PluginHost::IShell* service)
                    : _notification(callback)
                {
                    ASSERT(service != nullptr);
                    _control = service->QueryInterface<PluginHost::IStateControl>();
                    if (_control != nullptr) {
                        _control->Register(&_notification);
                    }
                }
                ~StandbyObserver()
                {
                    if (_control != nullptr) {
                        _control->Unregister(&_notification);
                        _control->Release();
                    }
                }

                StandbyObserver(const StandbyObserver&) = delete;
                StandbyObserver& operator=(const StandbyObserver&) = delete;

            private:
                PluginHost::IStateControl* _control;
                Core::Sink<Notification> _notification;
            };

        public:
            StateChangeObserver()
                : _callback(nullptr)
                , _operationalObserver(*this)
            {
            }
            ~StateChangeObserver()
            {
                _callsigns.clear();
                _standbyObservers.clear();
            }

            StateChangeObserver(const StateChangeObserver&) = delete;
            StateChangeObserver& operator=(const StateChangeObserver&) = delete;

            void RegisterOperationalNotifications(const Callback& callback, PluginHost::IShell* service)
            {
                ASSERT(service != nullptr);
                _callback = callback;
                service->Register(&_operationalObserver);
            }
            void UnregisterOperationalNotifications(PluginHost::IShell* service)
            {
                ASSERT(service != nullptr);
                _callback = nullptr;
                service->Unregister(&_operationalObserver);
            }

            void RegisterObservable(std::initializer_list<string> observables)
            {
                _callsigns.insert(observables);
            }
            void UnregisterObservable(std::initializer_list<string> observables)
            {
                for (const auto& observable : observables) {
                    _callsigns.erase(observable);
                }
            }

        private:
            void CreateObservable(const string& callsign, PluginHost::IShell* service)
            {
                if (_standbyObservers.count(callsign) == 0) {
                    _standbyObservers.emplace(std::piecewise_construct,
                        std::forward_as_tuple(callsign),
                        std::forward_as_tuple(std::bind(&StateChangeObserver::StateChange, this, callsign, std::placeholders::_1), service));
                }
            }
            void DestroyObservable(const string& callsign)
            {
                if (_standbyObservers.count(callsign) != 0) {
                    _standbyObservers.erase(callsign);
                }
            }

            void StateChange(const string& callsign, State state)
            {
                if (_callback != nullptr) {
                    _callback(state, callsign);
                }
            }

        private:
            Callback _callback;
            std::unordered_set<string> _callsigns;
            std::unordered_map<string, StandbyObserver> _standbyObservers;
            Core::Sink<OperationalObserver> _operationalObserver;
        };

    public:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , ServerAddress()
                , ServerPort()
                , Customer()
                , Platform()
                , Country()
                , Type()
                , Session()
                , CallsignMapping()
                , StateMapping()
            {
                Add(_T("server_address"), &ServerAddress);
                Add(_T("server_port"), &ServerPort);
                Add(_T("customer"), &Customer);
                Add(_T("platform"), &Platform);
                Add(_T("country"), &Country);
                Add(_T("type"), &Type);
                Add(_T("session"), &Session);
                Add(_T("callsign_mapping"), &CallsignMapping);
                Add(_T("state_mapping"), &StateMapping);
            }

            ~Config() override = default;

        public:
            Core::JSON::String ServerAddress;
            Core::JSON::DecUInt16 ServerPort;
            Core::JSON::String Customer;
            Core::JSON::String Platform;
            Core::JSON::String Country;
            Core::JSON::String Type;
            Core::JSON::DecUInt16 Session;
            Core::JSON::ArrayType<Core::JSON::String> CallsignMapping;
            Core::JSON::ArrayType<Core::JSON::String> StateMapping;
        };

    public:
        BackOffice()
            : _requestSender(nullptr)
        {
        }
        ~BackOffice() override = default;

        BackOffice(const BackOffice&) = delete;
        BackOffice& operator=(const BackOffice&) = delete;

        BEGIN_INTERFACE_MAP(BackOffice)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        void Send(StateChangeObserver::State state, const string& callsign);

    private:
        StateChangeObserver _stateChangeObserver;
        std::unique_ptr<RequestSender> _requestSender;
        Config _config;
        PluginHost::ISubSystem* _subSystem;
        std::unordered_map<string, string> _callsignMappings;
        std::unordered_map<string, string> _stateMappings;
    };

} // namespace
} // namespace