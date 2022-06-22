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
            using Callback = std::function<void(PluginHost::IShell::state, const string&)>;

            StateChangeObserver()
                : _callback(nullptr)
            {
                _callsigns.emplace(_T("DisplayInfo"));
            }
            ~StateChangeObserver() override = default;

            StateChangeObserver(const StateChangeObserver&) = delete;
            StateChangeObserver& operator=(const StateChangeObserver&) = delete;

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
            void Activated(const string& callsign, PluginHost::IShell*) override
            {
                if (_callback != nullptr) {
                    if (_callsigns.find(callsign) != _callsigns.end()) {
                        _callback(PluginHost::IShell::ACTIVATED, callsign);
                    }
                }
            }
            void Deactivated(const string& callsign, PluginHost::IShell*) override
            {
                if (_callback != nullptr) {
                    if (_callsigns.find(callsign) != _callsigns.end()) {
                        _callback(PluginHost::IShell::DEACTIVATED, callsign);
                    }
                }
            }
            void Unavailable(const string& callsign, PluginHost::IShell*) override
            {
                if (_callback != nullptr) {
                    if (_callsigns.find(callsign) != _callsigns.end()) {
                        _callback(PluginHost::IShell::UNAVAILABLE, callsign);
                    }
                }
            }

        private:
            Callback _callback;
            std::unordered_set<string> _callsigns;
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
        void Send(PluginHost::IShell::state state, const string& callsign);

    private:
        Core::Sink<StateChangeObserver> _stateChangeObserver;
        std::unique_ptr<RequestSender> _requestSender;
    };

} // namespace
} // namespace