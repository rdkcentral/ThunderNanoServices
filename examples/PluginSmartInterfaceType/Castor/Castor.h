/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#include <interfaces/IMath.h>

namespace Thunder {

namespace Plugin {

    class Castor : public PluginHost::IPlugin
                 , public PluginHost::JSONRPC
                 , public Exchange::IMath
                 , public Thunder::RPC::PluginSmartInterfaceType<Thunder::Exchange::IMath> {
    private:
        using BaseClass = Thunder::RPC::PluginSmartInterfaceType<Thunder::Exchange::IMath>;

        class Config : Core::JSON::Container {
        public:
            Config(const string& configLine)
                : Core::JSON::Container()
                , PolluxCallsign()
            {
                Add(_T("polluxcallsign"), &PolluxCallsign);
                FromString(configLine);
            }

            ~Config() = default;

            Config() = delete;
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config(Config&&) = delete;
            Config& operator=(Config&&) = delete;

        public:
            Core::JSON::String PolluxCallsign;
        }; // class Config

        void Configure(const Config& config)
        {
            _polluxCallsign = config.PolluxCallsign;
        }

    public:
        Castor()
            : _service(nullptr)
            , _polluxCallsign()
        {
        }
        ~Castor() override = default;

        Castor(const Castor&) = delete;
        Castor &operator=(const Castor&) = delete;
        Castor(Castor&&) = delete;
        Castor &operator=(Castor&&) = delete;

    public:
        // PluginHost::IPlugin overrides
        const string Initialize(PluginHost::IShell *service) override;
        void Deinitialize(PluginHost::IShell *service) override;
        string Information() const override;

        // IMath overrides
        uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const override;
        uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const override;

    public:
        BEGIN_INTERFACE_MAP(Castor)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::IMath)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell *_service;
        string _polluxCallsign;

    }; // class Castor

} // namespace Plugin

}
