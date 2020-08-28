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

#include "DIALServer.h"

#include "interfaces/IBrowser.h"
#include "interfaces/ISwitchBoard.h"

namespace WPEFramework {
namespace DIALHandlers {

    struct YouTube {
    private:
        static string Query(const string& params, const string& payload)
        {
            return (params + _T("&") + payload);
        }

    public:
        class Passive : public Plugin::DIALServer::Default {
        private:
            Passive() = delete;
            Passive(const Passive&) = delete;
            Passive& operator=(const Passive&) = delete;

        public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            Passive(PluginHost::IShell* service, const Plugin::DIALServer::Config::App& config, Plugin::DIALServer* parent)
                : Default(service, config, parent)
            {
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
            ~Passive() = default;

        public:
            uint32_t Start(const string& params, const string& payload) override
            {
                return Default::Start(Query(params, payload), {});
            }
            bool URL(const string& url, const string& payload) override
            {
                Default::URL(Query(url, payload), {});
                return (true);
            }
        }; // class Passive

    public:
        class Active : public Passive {
        private:
            Active() = delete;
            Active(const Active&) = delete;
            Active& operator=(const Active&) = delete;

        public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            Active(PluginHost::IShell* service, const Plugin::DIALServer::Config::App& config, Plugin::DIALServer* parent)
                : Passive(service, config, parent)
                , _browser(nullptr)
                , _hidden(false)
                , _hasHideAndShow(config.Hide.Value())
                , _notification(this)
            {
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
            ~Active() override
            {
                Stopped({}, {});
            }

        public:
            bool Connect() override
            {
                _browser = Plugin::DIALServer::Default::QueryInterface<Exchange::IBrowser>();
                if (_browser != nullptr) {
                    _browser->Register(&_notification);
                }

                return _browser != nullptr;
            }
            bool IsConnected() override
            {
                return _browser != nullptr;
            }
            virtual void Stopped(const string& data, const string& payload)
            {
                if (_browser != nullptr) {
                    _browser->Unregister(&_notification);
                    _browser->Release();
                    _browser = nullptr;
                }
            }
            bool HasHideAndShow() const override
            {
                return ((_browser != nullptr) && (_hasHideAndShow == true));
            }
            uint32_t Show() override
            {
                _browser->Hide(false);
                return Core::ERROR_NONE;
            }
            void Hide() override
            {
                _browser->Hide(true);
            }
            bool IsHidden() const override
            {
                return _hidden;
            }

        private:
            struct Notification : public Exchange::IBrowser::INotification {
            public:
                Notification() = delete;
                Notification(const Notification&) = delete;
                Notification& operator=(const Notification&) = delete;

            public:
                explicit Notification(Active* parent)
                    : _parent(parent)
                {
                }
                ~Notification() = default;

            public:
                void Hidden(const bool hidden) override
                {
                    _parent->_hidden = hidden;
                }
                void LoadFinished(const string& URL) override
                {
                }
                void URLChanged(const string& URL) override
                {
                }
                void Closure() override
                {
                }

                BEGIN_INTERFACE_MAP(Notification)
                    INTERFACE_ENTRY(Exchange::IBrowser::INotification)
                END_INTERFACE_MAP

            private:
                Active* _parent;
            };

            Exchange::IBrowser* _browser;
            bool _hidden;
            bool _hasHideAndShow;
            Core::Sink<Notification> _notification;
        }; // class Active

    }; // class YouTube

    static Plugin::DIALServer::ApplicationRegistrationType<YouTube> _youTubeHandler;
}
}
