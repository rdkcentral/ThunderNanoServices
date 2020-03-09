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
#include <interfaces/json/JsonData_InputSwitch.h>

namespace WPEFramework {
namespace Plugin {

    class InputSwitch : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    public:
        class Data : public Core::JSON::Container {
        public:
            Data()
                : Core::JSON::Container()
                , Callsign()
                , Enabled()
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("enabled"), &Enabled);
            }
            Data(const Data& copy)
                : Core::JSON::Container()
                , Callsign(copy.Callsign)
                , Enabled(copy.Enabled)
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("enabled"), &Enabled);
            }
            virtual ~Data()
            {
            }

            Data& operator= (const Data& rhs)
            {
                Callsign = rhs.Callsign;
                Enabled = rhs.Enabled;

                return (*this);
            }

        public:
            Core::JSON::String Callsign;
            Core::JSON::Boolean Enabled;
        };

    public:
        InputSwitch(const InputSwitch&) = delete;
        InputSwitch& operator=(const InputSwitch&) = delete;

        InputSwitch()
            : _skipURL(0)
            , _handler()
        {
            RegisterAll();
        }

        virtual ~InputSwitch()
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(InputSwitch)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        bool FindChannel(const string& name);

        // JsonRpc
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_channel(const JsonData::InputSwitch::ChannelParamsInfo& params);
        uint32_t endpoint_status(const JsonData::InputSwitch::StatusParamsData& params, Core::JSON::ArrayType<JsonData::InputSwitch::ChannelParamsInfo>& response);

    private:
        uint8_t _skipURL;
        PluginHost::IPCUserInput::Iterator _handler;
    };

} // namespace Plugin
} // namespace WPEFramework
