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
#include <interfaces/IMath.h>

namespace Thunder {
namespace Plugin {
    
    class TestPriorityQueue : public PluginHost::IPlugin {
    public:
        TestPriorityQueue(const TestPriorityQueue&) = delete;
        TestPriorityQueue& operator=(const TestPriorityQueue&) = delete;
        TestPriorityQueue(TestPriorityQueue&&) = delete;
        TestPriorityQueue& operator=(TestPriorityQueue&&) = delete;
        
        TestPriorityQueue()
            : PluginHost::IPlugin()
            , _service(nullptr)
            , _connectionId(0)
            , _implMath(nullptr)
            , _notification(*this)
            , _stop(false)
        {
        }
        
        ~TestPriorityQueue() override = default;
    private:
        class Notification : public RPC::IRemoteConnection::INotification {
        public:
            Notification(const Notification&) = delete;
            Notification& operator=(const Notification&) = delete;
            Notification(Notification&&) = delete;
            Notification& operator=(Notification&&) = delete;
            Notification() = delete;
            
            explicit Notification(TestPriorityQueue& parent)
                : RPC::IRemoteConnection::INotification()
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
            
            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP
        private:
            TestPriorityQueue& _parent;
        };
    public:
        // IPlugin Methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;
        
        void StartFlood(uint32_t totalCalls, uint32_t parallelism);
        void StopFlood();

        // Plugin Methods
        void Deactivated(RPC::IRemoteConnection* connection);
        
        BEGIN_INTERFACE_MAP(TestPriorityQueue)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_AGGREGATE(Exchange::IMath, _implMath)
        END_INTERFACE_MAP
        
    private:
        // Timeout (2000ms) may be changed if necessary, however, it must not exceed RPC::CommunicationTimeOut
        static constexpr uint32_t timeout = 2000;

        PluginHost::IShell* _service;
        uint32_t _connectionId;
        Exchange::IMath* _implMath;
        Core::SinkType<Notification> _notification;

        std::atomic<bool> _stop;
        std::vector<std::thread> _workers;
    };
} // Plugin
} // Thunder