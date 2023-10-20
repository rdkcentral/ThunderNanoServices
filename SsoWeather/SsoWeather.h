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
#include <interfaces/ISsoWeather.h>
#include <interfaces/json/JSsoWeather.h>

namespace WPEFramework {
namespace Plugin {

    class SsoWeather : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    public:
        class Notification : public RPC::IRemoteConnection::INotification {
        public:
            Notification() = delete;

            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;

            ~Notification() override = default;

            explicit Notification(SsoWeather* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            void Activated(RPC::IRemoteConnection* /* connection */) override
            {
            }

            void Deactivated(RPC::IRemoteConnection* connectionId) override
            {
                _parent.Deactivated(connectionId);
            }

            void Terminated(RPC::IRemoteConnection* /* connection */) override
            {
            }

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            SsoWeather& _parent;
        };

        class WeatherNotification : public Exchange::ISsoWeather::INotification {
        public:
            WeatherNotification() = delete;

            WeatherNotification(const WeatherNotification&) = delete;
            WeatherNotification& operator=(const WeatherNotification&) = delete;

            ~WeatherNotification() override = default;

            explicit WeatherNotification(SsoWeather* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            void Temperature(const uint8_t temperature) override
            {
                Exchange::JSsoWeather::Event::Temperature(_parent, temperature);
            }

            void IsRaining(const bool raining) override
            {
                Exchange::JSsoWeather::Event::IsRaining(_parent, raining);
            }

            BEGIN_INTERFACE_MAP(WeatherNotification)
                INTERFACE_ENTRY(Exchange::ISsoWeather::INotification)
            END_INTERFACE_MAP

        private:
            SsoWeather& _parent;
        };

    public:
        SsoWeather()
            : _implementation(nullptr)
            , _service(nullptr)
            , _connectionId(0)
            , _notification(this)
            , _weatherNotification(this)
        {
        }

        SsoWeather(const SsoWeather&) = delete;
        SsoWeather& operator=(const SsoWeather&) = delete;

        ~SsoWeather() override = default;

        BEGIN_INTERFACE_MAP(SsoWeather)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::ISsoWeather, _implementation)
        END_INTERFACE_MAP

        //   IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

        uint32_t _connectionId;

        Exchange::ISsoWeather* _implementation;
        PluginHost::IShell* _service;
        
        Core::Sink<Notification> _notification;
        Core::Sink<WeatherNotification> _weatherNotification;
    };

} // namespace Plugin
} // namespace WPEFramework
