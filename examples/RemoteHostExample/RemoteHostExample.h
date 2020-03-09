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
 
#pragma once

#include "Module.h"
#include "interfaces/IRemoteHostExample.h"
#include <interfaces/json/JsonData_RemoteHostExample.h>

namespace WPEFramework {
namespace Plugin {

    class RemoteHostExample : public PluginHost::IPlugin, public PluginHost::JSONRPC, public Exchange::IRemoteHostExample::ITimeListener {
    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , Name()
            {
                Add(_T("name"), &Name);
            }

            ~Config()
            {
            }

        public:
            Core::JSON::String Name;
        };

        RemoteHostExample(const RemoteHostExample&) = delete;
        RemoteHostExample& operator=(const RemoteHostExample&) = delete;

        RemoteHostExample()
            : _shell(nullptr)
        {
            RegisterAll();
        }

        virtual ~RemoteHostExample()
        {
            if (_shell) {
                _shell->Release();
                _shell = nullptr;
            }

            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(RemoteHostExample)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(Exchange::IRemoteHostExample::ITimeListener)
            INTERFACE_ENTRY(PluginHost::JSONRPC)
            INTERFACE_AGGREGATE(Exchange::IRemoteHostExample, _implementation)
            INTERFACE_AGGREGATE(RPC::IRemoteLinker, _implementation)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IRemoteHostExample::ITimeListener methods
        // -------------------------------------------------------------------------------------------------------
        uint32_t TimeUpdate(string time) override;
    private:
        //   JSONRPC methods
        // -------------------------------------------------------------------------------------------------------
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_greet(const JsonData::RemoteHostExample::GreetParamsData& params, JsonData::RemoteHostExample::GreetResultData& response);

    private:
        Exchange::IRemoteHostExample* _implementation;
        PluginHost::IShell* _shell;
    };

} // namespace Plugin
} // namespace WPEFramework
