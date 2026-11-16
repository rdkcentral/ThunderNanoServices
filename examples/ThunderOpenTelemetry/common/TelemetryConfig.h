/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

#include <open_telemetry/open_telemetry.h>

namespace Thunder {
namespace Telemetry {

    // Plugin "configuration" section as found in /etc/Thunder/plugins/<Callsign>.json
    class PluginTelemetryConfig : public Core::JSON::Container {
    public:
        PluginTelemetryConfig(const PluginTelemetryConfig&) = delete;
        PluginTelemetryConfig& operator=(const PluginTelemetryConfig&) = delete;

        PluginTelemetryConfig()
            : Core::JSON::Container()
            , Processor(OpenTelemetry::ProcessorType::SYNCHRONOUS)
            , Exporter(OpenTelemetry::ExporterType::CONSOLE)
            , Endpoint()
        {
            Add(_T("processor"), &Processor);
            Add(_T("exporter"), &Exporter);
            Add(_T("endpoint"), &Endpoint);
        }
        ~PluginTelemetryConfig() override = default;

        Core::JSON::EnumType<OpenTelemetry::ProcessorType> Processor;
        Core::JSON::EnumType<OpenTelemetry::ExporterType> Exporter;
        // Optional OTLP/HTTP base URL, e.g. "http://127.0.0.1:4318"; "/v1/traces", "/v1/logs"
        // and "/v1/metrics" are appended. When not set, the addon defaults are used.
        Core::JSON::String Endpoint;
    };

    // Whole plugin file: /etc/Thunder/plugins/<Callsign>.json
    class PluginFile : public Core::JSON::Container {
    public:
        PluginFile(const PluginFile&) = delete;
        PluginFile& operator=(const PluginFile&) = delete;

        PluginFile()
            : Core::JSON::Container()
            , Configuration()
        {
            Add(_T("configuration"), &Configuration);
        }
        ~PluginFile() override = default;

        PluginTelemetryConfig Configuration;
    };

    // Copies the settings that are set in the plugin config onto telemetryConfig.
    inline void ApplyPluginTelemetryConfig(OpenTelemetry::Config& telemetryConfig, const PluginTelemetryConfig& pluginConfig)
    {
        if (pluginConfig.Processor.IsSet() == true) {
            telemetryConfig.Processor = pluginConfig.Processor.Value();
        }
        if (pluginConfig.Exporter.IsSet() == true) {
            telemetryConfig.Exporter = pluginConfig.Exporter.Value();
        }
        if ((pluginConfig.Endpoint.IsSet() == true) && (pluginConfig.Endpoint.Value().empty() == false)) {
            string base = pluginConfig.Endpoint.Value();
            while ((base.empty() == false) && (base.back() == '/')) {
                base.pop_back();
            }
            telemetryConfig.TracerEndpoint = base + _T("/v1/traces");
            telemetryConfig.Logger.Endpoint = base + _T("/v1/logs");
            telemetryConfig.Metrics.Endpoint = base + _T("/v1/metrics");
        }
    }

    // Applies processor/exporter/endpoint from a plugin config line (IShell::ConfigLine()).
    inline bool ApplyPluginConfig(OpenTelemetry::Config& telemetryConfig, const string& pluginConfigLine, string& errorMessage)
    {
        PluginTelemetryConfig pluginConfig;
        Core::OptionalType<Core::JSON::Error> error;
        pluginConfig.FromString(pluginConfigLine, error);

        bool result = (error.IsSet() == false);
        if (result == false) {
            errorMessage = error.Value().Message();
        } else {
            ApplyPluginTelemetryConfig(telemetryConfig, pluginConfig);
        }
        return result;
    }

    // Same as ApplyPluginConfig, but reads the "configuration" section of a plugin JSON file.
    inline bool ApplyPluginConfigFromFile(OpenTelemetry::Config& telemetryConfig, const string& pluginFilePath, string& errorMessage)
    {
        bool result = false;
        Core::File file(pluginFilePath);

        if (file.Open(true) == false) {
            errorMessage = _T("Could not open ") + pluginFilePath;
        } else {
            PluginFile pluginFile;
            Core::OptionalType<Core::JSON::Error> error;
            pluginFile.IElement::FromFile(file, error);
            file.Close();

            if (error.IsSet() == true) {
                errorMessage = error.Value().Message();
            } else {
                ApplyPluginTelemetryConfig(telemetryConfig, pluginFile.Configuration);
                result = true;
            }
        }
        return result;
    }

    // Human readable summary of the effective settings, e.g. "processor=Batch exporter=HTTP (http://...)".
    inline string Describe(const OpenTelemetry::Config& telemetryConfig)
    {
        string result = string(_T("processor=")) + telemetryConfig.Processor.Data() + _T(" exporter=") + telemetryConfig.Exporter.Data();
        if ((telemetryConfig.Exporter.Value() == OpenTelemetry::ExporterType::HTTP) && (telemetryConfig.TracerEndpoint.IsSet() == true)) {
            result += _T(" (") + telemetryConfig.TracerEndpoint.Value() + _T(")");
        } else if (telemetryConfig.Exporter.Value() == OpenTelemetry::ExporterType::HTTP) {
            result += _T(" (addon default http://127.0.0.1:4318/v1/traces)");
        }
        return (result);
    }

} // namespace Telemetry
} // namespace Thunder
