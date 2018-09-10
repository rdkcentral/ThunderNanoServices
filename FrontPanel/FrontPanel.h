#ifndef __FRONTPANEL_H
#define __FRONTPANEL_H

#include "Module.h"
#include "FrontPanelHAL.h"

namespace WPEFramework {
namespace Plugin {

    class FrontPanel : public PluginHost::IPlugin, public PluginHost::IWeb {
    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Indicator()
                , BlinkDuration()
                , BlinkIterations()
                , Brightness()
                , State()
                , Color()
                , TimeFormat()
                , Hour()
                , Minutes()
                , Text()
                , TextDisplay()
                , Enable()
                , ScrollHoldOnDur()
                , HorzScrollIterations()
                , VertScrollIterations()
                , ToPersist()
            {
                Add(_T("indicator"), &Indicator);
                Add(_T("blinkduration"), &BlinkDuration);
                Add(_T("blinkiterations"), &BlinkIterations);
                Add(_T("brightness"), &Brightness);
                Add(_T("state"), &State);
                Add(_T("color"), &Color);
                Add(_T("timeformat"), &TimeFormat);
                Add(_T("hour"), &Hour);
                Add(_T("minutes"), &Minutes);
                Add(_T("text"), &Text);
                Add(_T("textdisplay"), &TextDisplay);
                Add(_T("enable"), &Enable);
                Add(_T("scrollholdondur"), &ScrollHoldOnDur);
                Add(_T("horzscrolliterations"), &HorzScrollIterations);
                Add(_T("vertscrolliterations"), &VertScrollIterations);
                Add(_T("topersist"), &ToPersist);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::EnumType<FrontPanelHAL::Indicator> Indicator;
            Core::JSON::DecUInt32 BlinkDuration;
            Core::JSON::DecUInt32 BlinkIterations;
            Core::JSON::DecUInt32 Brightness;
            Core::JSON::EnumType<FrontPanelHAL::State> State;
            Core::JSON::EnumType<FrontPanelHAL::Color> Color;
            Core::JSON::EnumType<FrontPanelHAL::TimeFormat> TimeFormat;
            Core::JSON::DecUInt32 Hour;
            Core::JSON::DecUInt32 Minutes;
            Core::JSON::String Text;
            Core::JSON::EnumType<FrontPanelHAL::TextDisplay> TextDisplay;
            Core::JSON::Boolean Enable;
            Core::JSON::DecUInt32 ScrollHoldOnDur;
            Core::JSON::DecUInt32 HorzScrollIterations;
            Core::JSON::DecUInt32 VertScrollIterations;
            Core::JSON::Boolean ToPersist;
        };
    private:
        FrontPanel(const FrontPanel&) = delete;
        FrontPanel& operator=(const FrontPanel&) = delete;

    public:
        FrontPanel()
            : _skipURL(0)
        {
        }

        virtual ~FrontPanel()
        {
        }

        BEGIN_INTERFACE_MAP(FrontPanel)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service);
        virtual void Deinitialize(PluginHost::IShell* service);
        virtual string Information() const;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index);

    private:
        uint8_t _skipURL;
        FrontPanelHAL _controller;
    };

} // Namespace Plugin.
}

#endif // __FRONTPANEL_H
