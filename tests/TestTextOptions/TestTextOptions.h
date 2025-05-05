/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 Metrological
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
#include <qa_interfaces/ITestAutomation.h>
#include <qa_interfaces/json/JTestTextOptions.h>
#include <qa_interfaces/json/JTestTextOptionsTestCustom.h>
#include <qa_interfaces/json/JTestTextOptionsTestKeep.h>
#include <qa_interfaces/json/JTestTextOptionsTestLegacy.h>

namespace Thunder {
namespace Plugin {
    
    class TestTextOptions : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    public:
        TestTextOptions(const TestTextOptions&) = delete;
        TestTextOptions& operator=(const TestTextOptions&) = delete;
        TestTextOptions(TestTextOptions&&) = delete;
        TestTextOptions& operator=(TestTextOptions&&) = delete;
        
        TestTextOptions()
            : PluginHost::IPlugin()
            , PluginHost::JSONRPC()
            , _service(nullptr)
            , _connectionId(0)
            , _implTestTextOptions(nullptr)
            , _implTestLegacy(nullptr)
            , _implTestKeep(nullptr)
            , _implTestCustom(nullptr)
            , _notification(*this)
        {
        }

        ~TestTextOptions() override = default;
    private:
        class Notification;

        class NotificationTestTextOptions : public QualityAssurance::ITestTextOptions::INotification {
        public:
            explicit NotificationTestTextOptions(Notification& parent)
                : _parent(parent)
                {
                }

            void TestEvent() override {
                QualityAssurance::JTestTextOptions::Event::TestEvent(_parent.Parent());
            }
        private:
            Notification& _parent;
        };

        class NotificationTestLegacy : public QualityAssurance::ITestTextOptions::ITestLegacy::INotification {
        public:
            explicit NotificationTestLegacy(Notification& parent)
                : _parent(parent)
                {
                }

            void TestEvent() override {
                QualityAssurance::TestTextOptions::JTestLegacy::Event::TestEvent(_parent.Parent());
            }
        private:
            Notification& _parent;
        };

        class NotificationTestKeep : public QualityAssurance::ITestTextOptions::ITestKeep::INotification {
        public:
            explicit NotificationTestKeep(Notification& parent)
                : _parent(parent)
                {
                }

            void TestEvent() override {
                QualityAssurance::TestTextOptions::JTestKeep::Event::TestEvent(_parent.Parent());
            }
        private:
            Notification& _parent;
        };

        class NotificationTestCustom : public QualityAssurance::ITestTextOptions::ITestCustom::INotification {
        public:
            explicit NotificationTestCustom(Notification& parent)
                : _parent(parent)
                {
                }

            void TestEvent() override {
                QualityAssurance::TestTextOptions::JTestCustom::Event::TestEvent(_parent.Parent());
            }
        private:
            Notification& _parent;
        };

        class Notification : public RPC::IRemoteConnection::INotification,
                            public NotificationTestTextOptions,
                            public NotificationTestLegacy,
                            public NotificationTestKeep,
                            public NotificationTestCustom {
        public:
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;
            Notification(Notification&&) = delete;
            Notification& operator=(Notification&&) = delete;
            Notification() = delete;

            explicit Notification(TestTextOptions& parent)
                : RPC::IRemoteConnection::INotification()
                , NotificationTestTextOptions(*this)
                , NotificationTestLegacy(*this)
                , NotificationTestKeep(*this)
                , NotificationTestCustom(*this)
                , _parent(parent)
            { 
            }

            ~Notification() override = default;

            void Activated(RPC::IRemoteConnection*) override {}
            void Deactivated(RPC::IRemoteConnection* connection) override {
                _parent.Deactivated(connection);
            }

            TestTextOptions& Parent() {
                return _parent;
            }

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::INotification)
                INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::ITestLegacy::INotification)
                INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::ITestKeep::INotification)
                INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::ITestCustom::INotification)
            END_INTERFACE_MAP

        private:
            TestTextOptions& _parent;
        };
    public:
        // IPlugin Methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;
        
        void Deactivated(RPC::IRemoteConnection* connection);
        
        BEGIN_INTERFACE_MAP(TestTextOptions)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(QualityAssurance::ITestTextOptions, _implTestTextOptions)
            INTERFACE_AGGREGATE(QualityAssurance::ITestTextOptions::ITestLegacy, _implTestLegacy)
            INTERFACE_AGGREGATE(QualityAssurance::ITestTextOptions::ITestKeep, _implTestKeep)
            INTERFACE_AGGREGATE(QualityAssurance::ITestTextOptions::ITestCustom, _implTestCustom)
        END_INTERFACE_MAP
        
    private:
        // Timeout (2000ms) may be changed if necassary, however, it must not exceed RPC::CommunicationTimeOut
        static constexpr uint32_t timeout = 2000;
    
        PluginHost::IShell* _service;
        uint32_t _connectionId;
        QualityAssurance::ITestTextOptions* _implTestTextOptions;
        QualityAssurance::ITestTextOptions::ITestLegacy* _implTestLegacy;
        QualityAssurance::ITestTextOptions::ITestKeep* _implTestKeep;
        QualityAssurance::ITestTextOptions::ITestCustom* _implTestCustom;
        Core::SinkType<Notification> _notification;
    };
} // Plugin
} // Thunder