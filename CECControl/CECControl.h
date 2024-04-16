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

#include <plugins/plugins.h>

namespace Thunder {
namespace Plugin {
    class CECControl : public PluginHost::IPlugin {
    public:
        class Config : public Core::JSON::Container {
        public:
            class Interface : public Core::JSON::Container {
            public:
                Interface(const Interface& copy)
                    : Name(copy.Name)
                    , Config(copy.Config)
                {
                }

                Interface& operator=(const Interface& rhs)
                {
                    Name = rhs.Name;
                    Config = rhs.Config;
                    return (*this);
                }

                Interface()
                    : Name()
                    , Config()
                {
                    Add(_T("name"), &Name);
                    Add(_T("configuration"), &Config);
                }

                ~Interface() = default;

            public:
                Core::JSON::String Name;
                Core::JSON::String Config;
            };
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Interfaces()
            {
                Add(_T("interfaces"), &Interfaces);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::ArrayType<Core::JSON::String> Interfaces;
        };

        CECControl(const CECControl&) = delete;
        CECControl& operator=(const CECControl&) = delete;

        CECControl() = default;
        virtual ~CECControl() override = default;

        BEGIN_INTERFACE_MAP(CECControl)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

        //   IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        Config _config;
    }; // class CECControl
} // namespace Plugin
} // namespace Thunder