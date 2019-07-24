#pragma once

#include "Module.h"
#include <interfaces/ICapture.h>
#include <interfaces/json/JsonData_Snapshot.h>

namespace WPEFramework {
namespace Plugin {

    class Snapshot : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    private:
        Snapshot(const Snapshot&) = delete;
        Snapshot& operator=(const Snapshot&) = delete;

    public:
        Snapshot()
            : _skipURL(0)
            , _device(nullptr)
            , _fileName()
            , _inProgress(false)
        {
            RegisterAll();
        }

        virtual ~Snapshot()
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(Snapshot)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::ICapture, _device)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service);
        virtual void Deinitialize(PluginHost::IShell* service);
        virtual string Information() const;

        //	IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request);
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

    private:
        // JSON-RPC
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_capture(JsonData::Snapshot::CaptureResultData& response);

    private:
        uint8_t _skipURL;
        Exchange::ICapture* _device;
        string _fileName;
        Core::BinairySemaphore _inProgress;
    };

} // namespace Plugin
}

