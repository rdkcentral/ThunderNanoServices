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
#include <interfaces/IInputSwitch.h>
#include <interfaces/json/JsonData_InputSwitch.h>

namespace WPEFramework {
namespace Plugin {

    class InputSwitch : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::IInputSwitch, public PluginHost::JSONRPC {
    private:
        typedef std::list<string> ImunityList;

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
            , _handler(nullptr)
            , _imunityList()
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
        INTERFACE_ENTRY(Exchange::IInputSwitch)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        //   IInputSwitch methods
        // -------------------------------------------------------------------------------------------------------
        RPC::IStringIterator* Consumers() const override;
        bool Consumer(const string& name) const override;
        uint32_t Consumer(const string& name, const mode value) override;
        uint32_t Select(const string& name) override;

    private:
        bool ChannelExists(const string& name) const;

        // JsonRpc
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_channel(const JsonData::InputSwitch::ChannelParamsInfo& params);
        uint32_t endpoint_status(const JsonData::InputSwitch::SelectParamsInfo& params, Core::JSON::ArrayType<JsonData::InputSwitch::ChannelParamsInfo>& response);
        uint32_t endpoint_select(const JsonData::InputSwitch::SelectParamsInfo& params);

    private:
        uint8_t _skipURL;
        PluginHost::VirtualInput* _handler;
        ImunityList _imunityList;
    };

} // namespace Plugin
} // namespace WPEFramework
