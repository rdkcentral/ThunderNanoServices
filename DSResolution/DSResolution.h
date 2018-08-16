#ifndef __DSRESOLUTION_H
#define __DSRESOLUTION_H

#include "Module.h"
#include "DSResolutionHAL.h"

namespace WPEFramework {
namespace Plugin {

    class DSResolution : public PluginHost::IPlugin, public PluginHost::IWeb {
    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()   
                , Resolution()
            {
                Add(_T("resolution"), &Resolution);
            }
            ~Config()
            {
            }
        public:
            Core::JSON::EnumType<DSResolutionHAL::PixelResolution> Resolution;
        };
    private:
        DSResolution(const DSResolution&) = delete;
        DSResolution& operator=(const DSResolution&) = delete;

    public:
        DSResolution()
            : _skipURL(0)
            , _controller()
        {
        }

        virtual ~DSResolution()
        {
        }

        BEGIN_INTERFACE_MAP(DSResolution)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        END_INTERFACE_MAP

    public:
        // IPlugin Methods
        // ------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service);
        virtual void Deinitialize(PluginHost::IShell* service);
        virtual string Information() const;
       
        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);

    private:
        uint8_t _skipURL;
        DSResolutionHAL _controller;
    };

} //Plugin
} //WPEFramework

#endif // __DSRESOLUTION_H
