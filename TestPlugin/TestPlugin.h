/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 [PLEASE ADD COPYRIGHT NAME!]
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
#include <qa_generated/JTestTextOptions.h>
#include <qa_generated/JTestTextOptionsTestCustom.h>
#include <qa_generated/JTestTextOptionsTestKeep.h>
#include <qa_generated/JTestTextOptionsTestLegacy.h>

namespace Thunder {
namespace Plugin {
    
    class TestPlugin : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    public:
        TestPlugin(const TestPlugin&) = delete;
        TestPlugin& operator=(const TestPlugin&) = delete;
        TestPlugin(TestPlugin&&) = delete;
        TestPlugin& operator=(TestPlugin&&) = delete;
        
        TestPlugin()
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

        ~TestPlugin() override = default;
    private:
        class Notification : public RPC::IRemoteConnection::INotification, 
                             public QualityAssurance::ITestTextOptions::INotification, 
                             public QualityAssurance::ITestTextOptions::ITestLegacy::INotification, 
                             public QualityAssurance::ITestTextOptions::ITestKeep::INotification, 
                             public QualityAssurance::ITestTextOptions::ITestCustom::INotification {
        public:
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;
            Notification(Notification&&) = delete;
            Notification& operator=(Notification&&) = delete;
            Notification() = delete;
            
            explicit Notification(TestPlugin& parent)
                : RPC::IRemoteConnection::INotification()
                , QualityAssurance::ITestTextOptions::INotification()
                , QualityAssurance::ITestTextOptions::ITestLegacy::INotification()
                , QualityAssurance::ITestTextOptions::ITestKeep::INotification()
                , QualityAssurance::ITestTextOptions::ITestCustom::INotification()
                , _parent(parent)
            {
            }
            
            ~Notification() override = default;
            
            void Activated(RPC::IRemoteConnection*) override
            {
            }
            
            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }
            
            // TestEvent for ITestTextOptions::INotification
            void TestEvent() override {
                QualityAssurance::JTestTextOptions::Event::TestEvent(_parent);
            }

            // TestEvent for ITestTextOptions::ITestLegacy::INotification
            void TestEvent() override {
                QualityAssurance::TestTextOptions::JTestLegacy::Event::TestEvent(_parent);
            }

            // TestEvent for ITestTextOptions::ITestKeep::INotification
            void TestEvent() override {
                QualityAssurance::TestTextOptions::JTestKeep::Event::TestEvent(_parent);
            }

            // TestEvent for ITestTextOptions::ITestCustom::INotification
            void TestEvent() override {
                QualityAssurance::TestTextOptions::JTestCustom::Event::TestEvent(_parent);
            }
            
            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::INotification)
                INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::ITestLegacy::INotification)
                INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::ITestKeep::INotification)
                INTERFACE_ENTRY(QualityAssurance::ITestTextOptions::ITestCustom::INotification)
            END_INTERFACE_MAP
        private:
            TestPlugin& _parent;
        };
    public:
        // IPlugin Methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;
        
        void Deactivated(RPC::IRemoteConnection* connection);
        
        BEGIN_INTERFACE_MAP(TestPlugin)
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