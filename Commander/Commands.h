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
 
#include "Module.h"

namespace WPEFramework {
namespace Plugin {
    namespace Command {

        class PluginControl {
        private:
            PluginControl(const PluginControl&) = delete;
            PluginControl& operator=(const PluginControl&) = delete;

        public:
            class Config : public Core::JSON::Container {
            private:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;

            public:
                enum state {
                    ACTIVATE,
                    DEACTIVATE
                };

            public:
                Config()
                    : Core::JSON::Container()
                    , Callsign()
                    , State()
                    , Success()
                    , Failure()
                {
                    Add(_T("callsign"), &Callsign);
                    Add(_T("state"), &State);
                    Add(_T("success"), &Success);
                    Add(_T("failure"), &Failure);
                }
                ~Config()
                {
                }

            public:
                Core::JSON::String Callsign;
                Core::JSON::EnumType<state> State;
                Core::JSON::String Success;
                Core::JSON::String Failure;
            };

        public:
            PluginControl(const string& configuration)
                : _config()
            {

                _config.FromString(configuration);
            }
            ~PluginControl()
            {
            }

            const string Execute(PluginHost::IShell* service)
            {

                string result = _config.Failure.Value();
                PluginHost::IShell* pluginShell = service->QueryInterfaceByCallsign<PluginHost::IShell>(_config.Callsign.Value());

                if (pluginShell != nullptr) {

                    switch (_config.State) {

                    case Config::state::ACTIVATE:
                        if (pluginShell->State() == PluginHost::IShell::ACTIVATED) {
                            result = _config.Success.Value();
                        } else {
                            // Time to activate it...
                            if (pluginShell->Activate(PluginHost::IShell::reason::REQUESTED) == Core::ERROR_NONE) {
                                result = _config.Success.Value();
                            }
                        }
                        break;
                    case Config::state::DEACTIVATE:
                        if (pluginShell->State() == PluginHost::IShell::DEACTIVATED) {
                            result = _config.Success.Value();
                        } else {
                            // Time to deactivate it...
                            if (pluginShell->Deactivate(PluginHost::IShell::reason::REQUESTED) == Core::ERROR_NONE) {
                                result = _config.Success.Value();
                            }
                        }
                        break;
                    }

                    pluginShell->Release();
                }

                return (result);
            }

        private:
            Config _config;
        };

        class PluginObserver {
        private:
            PluginObserver(const PluginObserver&) = delete;
            PluginObserver& operator=(const PluginObserver&) = delete;

        private:
            class Observer : public PluginHost::IPlugin::INotification {
            private:
                Observer() = delete;
                Observer(const Observer&) = delete;
                Observer& operator=(Observer&) = delete;

            public:
                Observer(PluginObserver* parent)
                    : _parent(*parent)
                {
                }
                ~Observer()
                {
                }

            public:
                virtual void StateChange(PluginHost::IShell* plugin)
                {
                    _parent.StateChange(plugin);
                }

                BEGIN_INTERFACE_MAP(Observer)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
                END_INTERFACE_MAP

            private:
                PluginObserver& _parent;
            };

            class Config : public Core::JSON::Container {
            private:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;

            public:
                Config()
                    : Core::JSON::Container()
                    , Callsign()
                    , Active()
                {
                    Add(_T("callsign"), &Callsign);
                    Add(_T("active"), &Active);
                }
                ~Config()
                {
                }

            public:
                Core::JSON::String Callsign;
                Core::JSON::Boolean Active;
            };

        public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            PluginObserver(const string& configuration)
                : _config()
                , _waitEvent(true, false)
                , _observer(Core::Service<Observer>::Create<Observer>(this))
            {

                _config.FromString(configuration);
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
            ~PluginObserver()
            {
            }

            const string Execute(PluginHost::IShell* service)
            {

                _waitEvent.ResetEvent();

                service->Register(_observer);

                _waitEvent.Lock();

                service->Unregister(_observer);

                return (EMPTY_STRING);
            }

            void Abort()
            {
                _waitEvent.SetEvent();
            }

        private:
            void StateChange(PluginHost::IShell* plugin)
            {

                if (_config.Callsign.Value() == plugin->Callsign()) {

                    if (((_config.Active.Value() == true) && (plugin->State() == PluginHost::IShell::ACTIVATED)) || ((_config.Active.Value() == false) && (plugin->State() == PluginHost::IShell::DEACTIVATED))) {
                        _waitEvent.SetEvent();
                    }
                }
            }

        private:
            Config _config;
            Core::Event _waitEvent;
            Observer* _observer;
        };
    }
}
}
