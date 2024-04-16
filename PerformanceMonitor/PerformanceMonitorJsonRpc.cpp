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
#include "Module.h"
#include "PerformanceMonitor.h"
#include <interfaces/json/JsonData_PerformanceMonitor.h>

namespace Thunder {
namespace Plugin {

    using namespace JsonData::PerformanceMonitor;

    // Registration
    //

    void PerformanceMonitor::RegisterAll()
    {
        Register<void,void>(_T("clear"), &PerformanceMonitor::endpoint_clear, this);
        Register<BufferInfo,Core::JSON::DecUInt32>(_T("send"), &PerformanceMonitor::endpoint_send, this);
        Register<Core::JSON::DecUInt32,BufferInfo>(_T("receive"), &PerformanceMonitor::endpoint_receive, this);
        Register<BufferInfo,BufferInfo>(_T("exchange"), &PerformanceMonitor::endpoint_exchange, this);
        Property<MeasurementData>(_T("measurement"), &PerformanceMonitor::get_measurement, nullptr, this);
    }

    void PerformanceMonitor::UnregisterAll()
    {
        Unregister(_T("send"));
        Unregister(_T("receive"));
        Unregister(_T("exchange"));
        Unregister(_T("clear"));
        Unregister(_T("measurement"));
    }

    // API implementation
    //

    // Method: clear - Clear all performance data collected
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t PerformanceMonitor::endpoint_clear()
    {
        PluginHost::PerformanceAdministrator::Instance().Clear();
        return Core::ERROR_NONE;
    }

    // Method: send - Interface to test sending data
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t PerformanceMonitor::endpoint_send(const BufferInfo& params, Core::JSON::DecUInt32& response)
    {
        return Send(params, response);
    }

    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t PerformanceMonitor::endpoint_receive(const Core::JSON::DecUInt32& params, BufferInfo& response)
    {
        return Receive(params, response);
    }

    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t PerformanceMonitor::endpoint_exchange(const BufferInfo& params, BufferInfo& response)
    {
        return Exchange(params, response);
    }

    // Property: measurement - Retrieve the performance measurement against given package size
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t PerformanceMonitor::get_measurement(const string& index, MeasurementData& response) const
    {
        const uint32_t& packageSize = atoi(index.c_str());
        return RetrieveInfo(packageSize, response);
    }

} // namespace Plugin
}

