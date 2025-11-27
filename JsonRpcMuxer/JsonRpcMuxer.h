#pragma once

#include "Module.h"

namespace Thunder {
namespace Plugin {
    class JsonRpcMuxer : public PluginHost::IPlugin,
                         public PluginHost::JSONRPC {
    private:
        class Config : public Core::JSON::Container {
        public:
            static constexpr uint8_t DefaultMaxConcurrentJobs = 10;
            static constexpr uint8_t DefaultMaxBatchSize = 100;
            
            Config(Config&&) = delete;
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , MaxConcurrentJobs(DefaultMaxConcurrentJobs)
                , MaxBatchSize(DefaultMaxBatchSize)
            {
                Add(_T("maxconcurrentjobs"), &MaxConcurrentJobs);
                Add(_T("maxbatchsize"), &MaxBatchSize);
            }
            ~Config() override = default;

        public:
            Core::JSON::DecUInt8 MaxConcurrentJobs;
            Core::JSON::DecUInt8 MaxBatchSize;
        };

        class Processor;

    public:
        JsonRpcMuxer(JsonRpcMuxer&&) = delete;
        JsonRpcMuxer(const JsonRpcMuxer&) = delete;
        JsonRpcMuxer& operator=(const JsonRpcMuxer&) = delete;
        JsonRpcMuxer();
        ~JsonRpcMuxer();

        BEGIN_INTERFACE_MAP(JsonRpcMuxer)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        Core::hresult Invoke(
            const uint32_t channelId, const uint32_t id, const string& token,
            const string& designator, const string& parameters, string& response) override;

    private:
        Processor* _processor;
        uint8_t _maxBatchSize;
    };
}
}