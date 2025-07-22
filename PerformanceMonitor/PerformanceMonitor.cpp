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

#include "PerformanceMonitor.h"

namespace Thunder {
namespace Plugin {

    namespace {

        static Metadata<PerformanceMonitor> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }


    /* virtual */ const string PerformanceMonitor::Initialize(PluginHost::IShell* service)
    {
        Config config;
        config.FromString(service->ConfigLine());

        ASSERT(service != nullptr);
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        Exchange::JPerformance::Register(*this, this);
        Exchange::JPerformanceStatistics::Register(*this, this);

        return string();
    }

    /* virtual */ void PerformanceMonitor::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        Exchange::JPerformance::Unregister(*this);
        Exchange::JPerformanceStatistics::Unregister(*this);
    }

    /* virtual */ string PerformanceMonitor::Information() const
    {
        // No additional info to report.
        return ((_T("The purpose of this plugin is provide ability to collect performance values of JSONRPC communication")));
    }

    Core::hresult PerformanceMonitor::Send(const uint16_t /* sendSize */, const uint8_t* /* buffer */)
    {
        return (Core::ERROR_NONE);
    }

    Core::hresult PerformanceMonitor::Receive(uint16_t& bufferSize, uint8_t buffer[]) const
    {
        uint8_t pattern[] = { 0x00, 0x66, 0xBB, 0xEE };
        uint8_t patternLength = sizeof(pattern);

        FillBuffer(bufferSize, buffer, patternLength, pattern);

        return (Core::ERROR_NONE);
    }

    Core::hresult PerformanceMonitor::Exchange(uint16_t& /* bufferSize */, uint8_t buffer[], const uint16_t maxBufferSize)
    {
        uint8_t pattern[] = { 0x00, 0x77, 0xCC, 0x88 };
        uint8_t patternLength = sizeof(pattern);

        FillBuffer(maxBufferSize, buffer, patternLength, pattern);

        return (Core::ERROR_NONE);
    }

    Core::hresult PerformanceMonitor::Measurement(const uint32_t index, Exchange::IPerformanceStatistics::Measurements& measurement) const
    {
        const PluginHost::PerformanceAdministrator::Statistics& statistics(PluginHost::PerformanceAdministrator::Instance().Retrieve(index));

        Measurement(statistics.Serialization(),   measurement.serialization);
        Measurement(statistics.Deserialization(), measurement.deserialization);
        Measurement(statistics.Execution(),       measurement.execution);
        Measurement(statistics.ThreadPool(),      measurement.threadPool);
        Measurement(statistics.Communication(),   measurement.communication);
        Measurement(statistics.Total(),           measurement.total);

        return (Core::ERROR_NONE);
    }

    Core::hresult PerformanceMonitor::Reset()
    {
        PluginHost::PerformanceAdministrator::Instance().Clear();

        return (Core::ERROR_NONE);
    }

} // namespace Plugin
} // namespace Thunder
