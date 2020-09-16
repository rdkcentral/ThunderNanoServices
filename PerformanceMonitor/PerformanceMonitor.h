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

#include <interfaces/json/JsonData_PerformanceMonitor.h>

namespace WPEFramework {
namespace Plugin {

    class PerformanceMonitor : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    public:
        PerformanceMonitor(const PerformanceMonitor&) = delete;
        PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

    public:
        PerformanceMonitor()
            : _skipURL(0)
        {
            RegisterAll();
        }

        virtual ~PerformanceMonitor()
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(PerformanceMonitor)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

    private:
        void RegisterAll();
        void UnregisterAll();
        uint32_t get_measurement(const string& index, JsonData::PerformanceMonitor::MeasurementData& response) const;

        void RetrieveInfo(const uint32_t packageSize, JsonData::PerformanceMonitor::MeasurementData& MeasurementData) const;

    private:
        uint8_t _skipURL;
    };

} // namespace Plugin
} // namespace WPEFramework
