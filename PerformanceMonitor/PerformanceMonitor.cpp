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
    uint32_t PerformanceMonitor::RetrieveInfo(const uint32_t packageSize, JsonData::PerformanceMonitor::MeasurementData& measurementData) const {
        const PluginHost::PerformanceAdministrator::Statistics& statistics(PluginHost::PerformanceAdministrator::Instance().Retrieve(packageSize));

        Measurement(statistics.Serialization(), measurementData.Serialization);
        Measurement(statistics.Deserialization(), measurementData.Deserialization);
        Measurement(statistics.Execution(),  measurementData.Execution);
        Measurement(statistics.ThreadPool(), measurementData.Threadpool);
        Measurement(statistics.Communication(), measurementData.Communication);
        Measurement(statistics.Total(), measurementData.Total);

        return Core::ERROR_NONE;
    }

    uint32_t PerformanceMonitor::Send(const JsonData::PerformanceMonitor::BufferInfo& data, Core::JSON::DecUInt32& result)
    {
        uint16_t length = static_cast<uint16_t>(((data.Data.Value().length() * 6) + 7) / 8);
        uint8_t* buffer = static_cast<uint8_t*>(ALLOCA(length));
        Core::FromString(data.Data.Value(), buffer, length);
        result = length;

        return Core::ERROR_NONE;
    }

    uint32_t PerformanceMonitor::Receive(const Core::JSON::DecUInt32& maxSize, JsonData::PerformanceMonitor::BufferInfo& data)
    {
        string convertedBuffer;

        uint32_t length = maxSize.Value();
        uint8_t* buffer = static_cast<uint8_t*>(ALLOCA(length));

        static uint8_t pattern[] = { 0x00, 0x66, 0xBB, 0xEE };
        uint8_t patternLength = sizeof(pattern);
        uint32_t index = 0;
        uint8_t patternIndex = 0;

        while (index < length) {

            buffer[index++] = pattern[patternIndex++];

            patternIndex %= (patternLength - 1);
        }

        Core::ToString(buffer, length, false, convertedBuffer);
        data.Data = convertedBuffer;
        data.Length = static_cast<uint16_t>(convertedBuffer.length());
        data.Duration = static_cast<uint16_t>(convertedBuffer.length()) + 1; //Dummy

        return Core::ERROR_NONE;
    }

    uint32_t PerformanceMonitor::Exchange(const JsonData::PerformanceMonitor::BufferInfo& data, JsonData::PerformanceMonitor::BufferInfo& result)
    {
        string convertedBuffer;

        uint16_t length = static_cast<uint16_t>(data.Data.Value().length());
        uint8_t* buffer = static_cast<uint8_t*>(ALLOCA(length));
        Core::FromString(data.Data.Value(), buffer, length);

        static uint8_t pattern[] = { 0x00, 0x77, 0xCC, 0x88 };
        uint8_t patternLength = sizeof(pattern);
        uint16_t index = 0;
        uint8_t patternIndex = 0;

        while (index < data.Length.Value()) {

            buffer[index++] = pattern[patternIndex++];

            patternIndex %= (patternLength - 1);
        }

        Core::ToString(buffer, length, false, convertedBuffer);
        result.Data = convertedBuffer;
        result.Length = static_cast<uint16_t>(convertedBuffer.length());
        result.Duration = static_cast<uint16_t>(convertedBuffer.length()) + 1; //Dummy

        return Core::ERROR_NONE;
    }
} // namespace Plugin
} // namespace WPEFramework
