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
#include <interfaces/IMemory.h>
#include <interfaces/IRtspClient.h>

#include "RtspSession.h"

namespace WPEFramework {
namespace Plugin {

    class RtspClientImplementation : public Exchange::IRtspClient, RtspSession::AnnouncementHandler {
    private:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , TestNum(0)
            {
                Add(_T("hostname"), &Hostname);
                Add(_T("port"), &Port);
                Add(_T("testNum"), &TestNum);
                Add(_T("testStr"), &TestStr);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Hostname;
            Core::JSON::DecUInt16 Port;
            Core::JSON::DecUInt16 TestNum;
            Core::JSON::String TestStr;
        };

    private:
        RtspClientImplementation(const RtspClientImplementation&) = delete;
        RtspClientImplementation& operator=(const RtspClientImplementation&) = delete;

    public:
        RtspClientImplementation()
            : _observers()
            , _rtspSession(*this)
        {
        }

        virtual ~RtspClientImplementation()
        {
        }

        uint32_t Configure(PluginHost::IShell* service)
        {
            ASSERT(service != nullptr);
            uint32_t result = 0;

            config.FromString(service->ConfigLine());

            return (result);
        }

        // Exchange::IRtspClient
        uint32_t Setup(const string& assetId, uint32_t position)
        {
            RtspReturnCode rc = ERR_OK;

            rc = _rtspSession.Initialize(config.Hostname.Value().c_str(), config.Port.Value());

            if (rc == ERR_OK) {
                rc = _rtspSession.Open(assetId, position);
            }
            return rc;
        }

        uint32_t Play(int32_t scale, uint32_t position)
        {
            RtspReturnCode rc = ERR_OK;

            rc = _rtspSession.Play((float_t)scale / 1000, position);

            return rc;
        }

        uint32_t Teardown()
        {
            RtspReturnCode rc = ERR_OK;

            rc = _rtspSession.Close();
            _rtspSession.Terminate();

            return rc;
        }

        void Set(const string& name, const string& value)
        {
            _rtspSession.Set(name, value);
        }

        string Get(const string& name) const
        {
            string value;
            RtspReturnCode rc = _rtspSession.Get(name, value);
            return value;
        }

        // RtspSession::AnnouncementHandler
        void announce(const RtspAnnounce& announcement)
        {
            TRACE(Trace::Information, (_T("%s: Announcement received. code = %d"), __FUNCTION__, announcement.GetCode()));
        }

        BEGIN_INTERFACE_MAP(RtspClientImplementation)
        INTERFACE_ENTRY(Exchange::IRtspClient)
        END_INTERFACE_MAP

    private:
        std::list<PluginHost::IStateControl::INotification*> _observers;
        RtspSession _rtspSession;
        Config config;
    };

    SERVICE_REGISTRATION(RtspClientImplementation, 1, 0);

} // namespace Plugin

} // namespace WPEFramework
