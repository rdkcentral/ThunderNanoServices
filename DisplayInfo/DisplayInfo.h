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
#include <interfaces/IDisplayInfo.h>
#include <interfaces/json/JsonData_DisplayInfo.h>

namespace WPEFramework {
namespace Plugin {

    class DisplayInfo : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    private:private:
        class Notification : protected Exchange::IConnectionProperties::INotification {
        private:
            Notification() = delete;
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

        public:
            explicit Notification(DisplayInfo* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            virtual ~Notification()
            {
            }

            void Initialize(Exchange::IConnectionProperties* client)
            {
                ASSERT(client != nullptr);
                _client = client;
                _client->AddRef();
                _client->Register(this);
            }
            void Deinitialize()
            {
                ASSERT(_client != nullptr);
                if (_client != nullptr) {
                    _client->Unregister(this);
                    _client->Release();
                    _client = nullptr;
                }
            }
            void Updated() override
            {
                _parent.Updated();
            }
            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(Exchange::IConnectionProperties::INotification)
            END_INTERFACE_MAP

        private:
            DisplayInfo& _parent;
            Exchange::IConnectionProperties* _client;
        };

    public:
        DisplayInfo(const DisplayInfo&) = delete;
        DisplayInfo& operator=(const DisplayInfo&) = delete;

        DisplayInfo()
            : _skipURL(0)
            , _connectionId(0)
            , _graphicsProperties(nullptr)
            , _connectionProperties(nullptr)
            , _notification(this)
        {
            RegisterAll();
        }

        virtual ~DisplayInfo()
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(DisplayInfo)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_AGGREGATE(Exchange::IGraphicsProperties, _graphicsProperties)
        INTERFACE_AGGREGATE(Exchange::IConnectionProperties, _connectionProperties)
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

        void Updated()
        {
            event_updated();
        }

    private:
        // JsonRpc
        void RegisterAll();
        void UnregisterAll();
        uint32_t get_displayinfo(JsonData::DisplayInfo::DisplayinfoData&) const;
        void event_updated();

        void Info(JsonData::DisplayInfo::DisplayinfoData&) const;

    private:
        uint8_t _skipURL;
        uint32_t _connectionId;
        Exchange::IGraphicsProperties* _graphicsProperties;
        Exchange::IConnectionProperties* _connectionProperties;
        Core::Sink<Notification> _notification;
    };

} // namespace Plugin
} // namespace WPEFramework
