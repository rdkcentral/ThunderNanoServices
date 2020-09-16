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

#include "PerformanceMonitor.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(PerformanceMonitor, 1, 0);

    /* virtual */ const string PerformanceMonitor::Initialize(PluginHost::IShell* service)
    {
        Config config;
        config.FromString(service->ConfigLine());

        ASSERT(service != nullptr);
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        return string();
    }

    /* virtual */ void PerformanceMonitor::Deinitialize(PluginHost::IShell* service)
    {
    }

    /* virtual */ string PerformanceMonitor::Information() const
    {
        // No additional info to report.
        return ((_T("The purpose of this plugin is provide ability to collect performance values of JSONRPC communication")));
    }
    void PerformanceMonitor::RetrieveInfo(const uint32_t packageSize, JsonData::PerformanceMonitor::MeasurementData& measurementData) const {
        const PluginHost::PerformanceAdministrator::Statistics& statistics(PluginHost::PerformanceAdministrator::Instance().Retrieve(packageSize));

        Measurement(statistics.Serialization(), measurementData.Serialization);
        Measurement(statistics.Deserialization(), measurementData.Deserialization);
        Measurement(statistics.Execution(),  measurementData.Execution);
        Measurement(statistics.ThreadPool(), measurementData.Threadpool);
        Measurement(statistics.Communication(), measurementData.Communication);
        Measurement(statistics.Total(), measurementData.Total);

        return;
    }

} // namespace Plugin
} // namespace WPEFramework
