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

#include <qa_interfaces/ITestStateControl.h>
#include <qa_interfaces/json/JTestStateControl.h>

namespace Thunder {
namespace Plugin {

    class TestStateControl
        : public PluginHost::IPlugin
        , public PluginHost::IStateControl
        , public PluginHost::JSONRPC
        , public QualityAssurance::ITestStateControl {

        friend Core::ThreadPool::JobType<TestStateControl&>;

    public:
        TestStateControl(const TestStateControl&) = delete;
        TestStateControl& operator=(const TestStateControl&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        TestStateControl()
            : _adminLock()
            , _service(nullptr)
            , _state(PluginHost::IStateControl::UNINITIALIZED)
            , _requestedCommand(PluginHost::IStateControl::SUSPEND)
            , _observers()
            , _jsonObservers()
            , _job(*this)
        {
        }
POP_WARNING()

        ~TestStateControl() override = default;

        BEGIN_INTERFACE_MAP(TestStateControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IStateControl)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(QualityAssurance::ITestStateControl)
        END_INTERFACE_MAP

    public:
        // IPlugin
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        // IStateControl
        Core::hresult Configure(PluginHost::IShell* framework) override;
        PluginHost::IStateControl::state State() const override;
        Core::hresult Request(const PluginHost::IStateControl::command command) override;
        void Register(PluginHost::IStateControl::INotification* notification) override;
        void Unregister(PluginHost::IStateControl::INotification* notification) override;

        // ITestStateControl
        Core::hresult State(string& state) const override;
        Core::hresult Request(const string& command) override;
        Core::hresult Register(QualityAssurance::ITestStateControl::INotification* notification) override;
        Core::hresult Unregister(QualityAssurance::ITestStateControl::INotification* notification) override;

    private:
        void Dispatch();
        static string StateToString(PluginHost::IStateControl::state state);

        mutable Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        PluginHost::IStateControl::state _state;
        PluginHost::IStateControl::command _requestedCommand;
        std::list<PluginHost::IStateControl::INotification*> _observers;
        std::list<QualityAssurance::ITestStateControl::INotification*> _jsonObservers;
        Core::WorkerPool::JobType<TestStateControl&> _job;
    };

} // namespace Plugin
} // namespace Thunder
