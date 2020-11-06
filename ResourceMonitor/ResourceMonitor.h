#pragma once

#include "Module.h"
#include <interfaces/IMemory.h>
#include <interfaces/IResourceMonitor.h>

namespace WPEFramework {
namespace Plugin {
    class ResourceMonitor : public PluginHost::IPlugin, public PluginHost::IWeb {
    private:
        ResourceMonitor(const ResourceMonitor&) = delete;
        ResourceMonitor& operator=(const ResourceMonitor&) = delete;

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , OutOfProcess(true)
            {
                Add(_T("outofprocess"), &OutOfProcess);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::Boolean OutOfProcess;
        };

    public:
        ResourceMonitor()
            : _service(nullptr)
            , _monitor(nullptr)
            , _connectionId(0)
        {

        }

        virtual ~ResourceMonitor()
        {
        }

        void Inbound(Web::Request& request) override
        {
        }

        Core::ProxyType<Web::Response> Process(const Web::Request& request) override
        {
            Core::ProxyType<Web::Response> result = PluginHost::Factories::Instance().Response();
            result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
            result->Message = string(_T("Unknown request path specified."));

            if (request.Verb == Web::Request::HTTP_GET) {
                Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');
                
                // Move to request name (shoudl be "history")
                index.Next();

                if (index.IsValid() == true && index.Next() == true) {
                    const string requestStr = index.Current().Text();
                    if (requestStr == "history") {
                        // Asked for history csv
                        result->ErrorCode = Web::STATUS_OK;
                        result->ContentType = Web::MIMETypes::MIME_TEXT;
                        result->Message = _T("OK");
                        Core::ProxyType<Web::TextBody> body(webBodyFactory.Element());
                        *body = _monitor->CompileMemoryCsv();
                        result->Body(body);
                    }
                }
            }

            return result;
        }


        BEGIN_INTERFACE_MAP(ResourceMonitor)
        INTERFACE_ENTRY(IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_AGGREGATE(Exchange::IResourceMonitor, _monitor)
        END_INTERFACE_MAP

    public:
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        PluginHost::IShell* _service;
        Exchange::IResourceMonitor* _monitor;
        uint32_t _connectionId;
        static Core::ProxyPoolType<Web::TextBody> webBodyFactory;
        uint32_t _skipURL;
    };
}
}

