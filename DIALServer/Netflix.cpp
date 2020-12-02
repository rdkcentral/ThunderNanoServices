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

#include "interfaces/INetflix.h"

namespace WPEFramework {
namespace DIALHandlers {

    static string Query(const string& params, const string& payload)
    {
        string query = params;
#ifdef NETFLIX_VERSION_5_1
            // Set proper launch type, i.e. launched by DIAL
            query += _T("&source_type=12");
#endif
#ifdef NETFLIX_VERSION_5_2
            // Set proper launch type, i.e. launched by DIAL
            // FIXME: Use project specific iid for now
            query += _T("&iid=7637f789");
#endif
       if (payload.empty() == false) {
           // Netflix expects the payload as urlencoded option "dial"
            const uint16_t maxEncodeSize = static_cast<uint16_t>(payload.length() * 3 * sizeof(TCHAR));
            TCHAR* encodedPayload = reinterpret_cast<TCHAR*>(ALLOCA(maxEncodeSize));
            Core::URL::Encode(payload.c_str(), static_cast<uint16_t>(payload.length()), encodedPayload, maxEncodeSize);
            query = query + _T("&dial=") + encodedPayload;
        }
    
        return (query);
    }

    class Netflix : public Plugin::DIALServer::Default {
    public:
        Netflix() = delete;
        Netflix(const Netflix&) = delete;
        Netflix& operator=(const Netflix&) = delete;

        Netflix(PluginHost::IShell* service, const Plugin::DIALServer::Config::App& config, Plugin::DIALServer *parent)
            : Default(service, config, parent)
            , _netflix(nullptr)
            , _service(nullptr)
            , _notification(*this)
            , _hidden(false)
            , _hasHide(config.Hide.Value())
            , _lock()
            , _callsign(config.Callsign.Value())
        {
            ASSERT(service != nullptr);
            ASSERT(parent != nullptr);
            service->Register(&_notification);
        }
        ~Netflix() override
        {
            Detach();
            _service->Unregister(&_notification);
        }

    public:
        uint32_t Start(const string& params, const string& payload) override
        {
            const string query = Query(params, payload);

            _lock.Lock();
            if ((_netflix != nullptr) && (_hidden == true)) {
                _netflix->SetVisible(true);
                _hidden = false;
            } 
            _lock.Unlock();

            // Set custom query paramters
            Core::SystemInfo::SetEnvironment(_T("ONE_TIME_QUERY_STRING_OVERRIDE"), query.c_str());

            return (Default::Start(query, payload));
        }
        void Stop(const string& params, const string& payload) override
        {
            Detach();
            Default::Stop(params, payload);
        }
        bool Connect() override
        {
            Attach();
            return (_netflix != nullptr);
        }
        bool IsConnected() override
        {
            return (_netflix != nullptr);
        }
        bool HasHide() const override
        {
            return ((_netflix != nullptr) && (_hasHide == true));
        }
        void Hide() override
        {
            _lock.Lock();
            if (_netflix != nullptr) {
                _netflix->SetVisible(false);
                // TODO: Poor man's state tracking; would be optimal to fetch the state from Netflix itself.
                _hidden = true;
            }
            _lock.Unlock();
        }
        bool IsHidden() const override
        {
            return (_hidden);
        }
        bool URL(const string& /* url */, const string& /* payload */) override
        {
            return (false);
        }

    private:
        const string& Callsign() const
        {
            return (_callsign);
        }
        void Attach()
        {
            _lock.Lock();
            if (_netflix == nullptr) {
                _netflix = Plugin::DIALServer::Default::QueryInterface<Exchange::INetflix>();
            }
            _lock.Unlock();
        }
        void Detach()
        {
            _lock.Lock();
            _hidden = false;
            if (_netflix != nullptr) {
                _netflix->Release();
                _netflix = nullptr;
            }
            _lock.Unlock();
        }

    private:
        class Notification : public PluginHost::IPlugin::INotification {
        public:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            explicit Notification(Netflix& parent)
                : _parent(parent)
            {
            }
            ~Notification() = default;

        public:
            void StateChange(PluginHost::IShell* shell) override
            {
                ASSERT(shell != nullptr);
                if (shell->Callsign() == _parent.Callsign()) {
                    if (shell->State() == PluginHost::IShell::ACTIVATED) {
                        _parent.Attach();
                    } else if (shell->State() == PluginHost::IShell::DEACTIVATION) {
                        _parent.Detach();
                    }
                }
            }

        public:
            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
            END_INTERFACE_MAP

        private:
            Netflix& _parent;
        }; // class Notification

    private:
        Exchange::INetflix* _netflix;
        PluginHost::IShell* _service;
        Core::Sink<Notification> _notification;
        bool _hidden;
        bool _hasHide;
        mutable Core::CriticalSection _lock;
        string _callsign;
    }; // class Netflix

    static Plugin::DIALServer::ApplicationRegistrationType<Netflix> _netflixHandler;
}
}
