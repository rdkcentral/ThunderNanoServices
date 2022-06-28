// Copyright (c) 2022 Metrological. All rights reserved.

#pragma once

#include "Module.h"
#include "RequestSender.h"

namespace WPEFramework {
namespace Plugin {

    class BackOffice : public PluginHost::IPlugin {
    private:
        class StateChangeObserver : public PluginHost::IPlugin::INotification {
        public:
            using Callback = std::function<void(const string&, const string&)>;

            StateChangeObserver()
                : _callback(nullptr)
            {
            }
            ~StateChangeObserver() override = default;

            StateChangeObserver(const StateChangeObserver&) = delete;
            StateChangeObserver& operator=(const StateChangeObserver&) = delete;

            void RegisterObservable(const string& callsign)
            {
                _callsigns.emplace(callsign);
            }
            void UnregisterObservable(const string& callsign)
            {
                _callsigns.erase(callsign);
            }

            void Register(const Callback& callback)
            {
                _callback = callback;
            }
            void Unregister()
            {
                _callback = nullptr;
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            class StateObserver {
            private:
                class Notification : public PluginHost::IStateControl::INotification {
                public:
                    Notification(const std::function<void(PluginHost::IStateControl::state)>& callback)
                        : _callback(callback)
                    {
                    }
                    ~Notification() override = default;

                    BEGIN_INTERFACE_MAP(Notification)
                    INTERFACE_ENTRY(PluginHost::IStateControl::INotification)
                    END_INTERFACE_MAP

                private:
                    void StateChange(const PluginHost::IStateControl::state state) override
                    {
                        _callback(state);
                    }

                    std::function<void(PluginHost::IStateControl::state)> _callback;
                };

            public:
                StateObserver(const std::function<void(const string&, PluginHost::IStateControl::state)>& callback, PluginHost::IShell* service)
                    : _notification(std::bind(callback, std::placeholders::_1))
                {
                    ASSERT(service != nullptr);
                    _control = service->QueryInterface<PluginHost::IStateControl>();
                    if (_control != nullptr) {
                        _control->Register(&_notification);
                    }
                }
                ~StateObserver()
                {
                    if (_control != nullptr) {
                        _control->Unregister(&_notification);
                        _control->Release();
                    }
                }

            private:
                PluginHost::IStateControl* _control;
                Core::Sink<Notification> _notification;
            };

            std::unordered_map<string, StateObserver> _stateObservers;
            void CreateObservable(const string& callsign, PluginHost::IShell* service)
            {
                if (_stateObservers.count(callsign) == 0) {
                    _stateObservers.emplace(std::piecewise_construct,
                        std::forward_as_tuple(callsign),
                        std::forward_as_tuple(std::bind(&StateChangeObserver::StateChange, this, callsign, std::placeholders::_2), service));
                }
            }
            void DestroyObservable(const string& callsign)
            {
                if (_stateObservers.count(callsign) != 0) {
                    _stateObservers.erase(callsign);
                }
            }

            void StateChange(const string& callsign, const PluginHost::IStateControl::state state)
            {
                if (_callback != nullptr) {
                    if (state == PluginHost::IStateControl::RESUMED) {
                        _callback(_T("Resumed"), callsign);
                    } else if (state == PluginHost::IStateControl::SUSPENDED) {
                        _callback(_T("Suspended"), callsign);
                    }
                }
            }

            void Activated(const string& callsign, PluginHost::IShell* service) override
            {
                ASSERT(service != nullptr);

                if (_callback != nullptr) {
                    if (_callsigns.count(callsign) != 0) {
                        CreateObservable(callsign, service);
                        _callback(_T("Activated"), callsign);
                    }
                }
            }
            void Deactivated(const string& callsign, PluginHost::IShell*) override
            {
                if (_callback != nullptr) {
                    if (_callsigns.count(callsign) != 0) {
                        DestroyObservable(callsign);
                        _callback(_T("Deactivated"), callsign);
                    }
                }
            }
            void Unavailable(const string& callsign, PluginHost::IShell*) override
            {
                if (_callback != nullptr) {
                    if (_callsigns.find(callsign) != _callsigns.end()) {
                        _callback(_T("Unavailable"), callsign);
                    }
                }
            }

        private:
            Callback _callback;
            std::unordered_set<string> _callsigns;
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
                , CallsignMapping()
                , StateMapping()
            {
                Add(_T("server_address"), &ServerAddress);
                Add(_T("server_port"), &ServerPort);
                Add(_T("customer"), &Customer);
                Add(_T("platform"), &Platform);
                Add(_T("country"), &Country);
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
        void Send(const string& state, const string& callsign);

    private:
        Core::Sink<StateChangeObserver> _stateChangeObserver;
        std::unique_ptr<RequestSender> _requestSender;
        Config _config;
        PluginHost::ISubSystem* _subSystem;
        PluginHost::IStateControl* _stateControl;
        std::unordered_map<string, string> _callsignMappings;
        std::unordered_map<string, string> _stateMappings;
    };

} // namespace
} // namespace