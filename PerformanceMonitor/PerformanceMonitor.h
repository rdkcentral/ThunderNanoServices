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
#include <interfaces/IPerformance.h>
#include <interfaces/json/JPerformance.h>
#include <interfaces/json/JPerformanceStatistics.h>

namespace Thunder {
namespace Plugin {

    class PerformanceMonitor : public Exchange::IPerformance
                             , public Exchange::IPerformanceStatistics
                             , public PluginHost::IPlugin
                             , public PluginHost::JSONRPC {
    public:
        PerformanceMonitor(const PerformanceMonitor&) = delete;
        PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

    public:
        PerformanceMonitor()
            : _skipURL(0) {
        }

        ~PerformanceMonitor() override = default;

        BEGIN_INTERFACE_MAP(PerformanceMonitor)
        INTERFACE_ENTRY(Exchange::IPerformance)
        INTERFACE_ENTRY(Exchange::IPerformanceStatistics)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        // IPerformance methods - a majority of the code for performance measurement is provided by the generators
        // -------------------------------------------------------------------------------------------------------
        Core::hresult Send(const uint16_t sendSize, const uint8_t buffer[]) override;
        Core::hresult Receive(uint16_t& bufferSize, uint8_t buffer[]) const override;
        Core::hresult Exchange(uint16_t& bufferSize, uint8_t buffer[], const uint16_t maxBufferSize) override;

        // IPerformanceStatistics methods
        // -------------------------------------------------------------------------------------------------------
        Core::hresult Measurement(const uint32_t index, Exchange::IPerformanceStatistics::Measurements& measurement) const override;
        Core::hresult Reset() override;

        inline void Measurement(const PluginHost::PerformanceAdministrator::Statistics::Tuple& statistics, Exchange::IPerformanceStatistics::Statistics& statisticsData) const
        {
            statisticsData.minimum = statistics.Minimum();
            statisticsData.maximum = statistics.Maximum();
            statisticsData.average = statistics.Average();
            statisticsData.count   = statistics.Count();
        }

        inline void FillBuffer(const uint16_t& bufferSize, uint8_t buffer[], const uint8_t patternLength, const uint8_t pattern[]) const
        {
            uint32_t index = 0;
            uint8_t patternIndex = 0;

            while (index < bufferSize) {

                buffer[index++] = pattern[patternIndex++];

                patternIndex %= (patternLength - 1);
            }
        }

    private:
        uint8_t _skipURL;
    };

} // namespace Plugin
} // namespace Thunder
