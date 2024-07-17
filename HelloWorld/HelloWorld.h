/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
#include "interfaces/IHelloWorld.h"
#include "interfaces/json/JHelloWorld.h"

namespace Thunder{
namespace Plugin{
    class HelloWorld : public PluginHost::IPlugin, public PluginHost::JSONRPC, public Exchange::IHelloWorld{
    public:
        HelloWorld(const HelloWorld&) = delete;
        HelloWorld& operator=(const HelloWorld&) = delete;

        HelloWorld()
        : _service(nullptr),
          _impl(nullptr),
          _connectionId(0)
        {
        }

        ~HelloWorld() override
        {
        }

        BEGIN_INTERFACE_MAP(HelloWorld)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_ENTRY(Exchange::IHelloWorld);
        END_INTERFACE_MAP

        void SendMessage();

        //  IPlugin Methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        // Exchange::IHelloWorld Methods

        uint32_t PrintStuff(const string &randomWord) const override;
        
        private:
        
            PluginHost::IShell* _service;
            Exchange::IHelloWorld* _impl;
            uint32_t _connectionId;
                    
    };

} // namespace Plugin
} // namespace ThunderS