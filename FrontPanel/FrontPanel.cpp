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

#include "FrontPanel.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(FrontPanel, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<FrontPanel::Config>> jsonBodyDataFactory(1);

    /* virtual */ const string FrontPanel::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        return (service != nullptr ? _T("") : _T("No service."));
    }

    /* virtual */ void FrontPanel::Deinitialize(PluginHost::IShell* service)
    {
    }

    /* virtual */ string FrontPanel::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void FrontPanel::Inbound(Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_POST)
            request.Body(jsonBodyDataFactory.Element());
    }

    /* virtual */ Core::ProxyType<Web::Response> FrontPanel::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();
        if (request.Verb == Web::Request::HTTP_GET)
            result = GetMethod(index, request);
        else if (request.Verb == Web::Request::HTTP_POST)
            result = PostMethod(index, request);

        return result;
    }

    Core::ProxyType<Web::Response> FrontPanel::GetMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported GET request.");

        if (index.IsValid() == true) {
            if (index.Next() && index.IsValid()) {
                Core::ProxyType<Web::JSONBodyType<FrontPanel::Config>> response(jsonBodyDataFactory.Element());
                if (index.Current() == _T("State")) {
                    Core::URL::KeyValue options(request.Query.Value());
                    Core::EnumerateType<FrontPanelHAL::Indicator> indicator(options["indicator"]);
                    FrontPanelHAL::State state;
                    if (_controller.GetFPState(indicator.Value(), state)) {
                        response->State = state;
                        result->ErrorCode = Web::STATUS_OK;
                        result->Body(response);
                        result->Message = _T("Get Front Panel State");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Get Front Panel State.");
                    }
                } else if (index.Current() == _T("Brightness")) {
                    Core::URL::KeyValue options(request.Query.Value());
                    Core::EnumerateType<FrontPanelHAL::Indicator> indicator(options["indicator"]);
                    uint32_t brightness;
                    if (_controller.GetFPBrightness(indicator.Value(), brightness)) {
                        response->Brightness = brightness;
                        result->ErrorCode = Web::STATUS_OK;
                        result->Body(response);
                        result->Message = _T("Get Front Panel Brightness.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Get Front Panel Brightness.");
                    }
                } else if (index.Current() == _T("Color")) {
                    Core::URL::KeyValue options(request.Query.Value());
                    Core::EnumerateType<FrontPanelHAL::Indicator> indicator(options["indicator"]);
                    FrontPanelHAL::Color color;
                    if (_controller.GetFPColor(indicator.Value(), color)) {
                        response->Color = color;
                        result->ErrorCode = Web::STATUS_OK;
                        result->Body(response);
                        result->Message = _T("Get Front Panel Color.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Get Front Panel Color.");
                    }
                } else if (index.Current() == _T("TextBrightness")) {
                    Core::URL::KeyValue options(request.Query.Value());
                    Core::EnumerateType<FrontPanelHAL::TextDisplay> textDisplay(options["textdisplay"]);
                    uint32_t brightness;
                    if (_controller.GetFPTextBrightness(textDisplay.Value(), brightness)) {
                        response->Brightness = brightness;
                        result->ErrorCode = Web::STATUS_OK;
                        result->Body(response);
                        result->Message = _T("Get Front Panel Text Brightness.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Get Front Panel Text Brightness.");
                    }
                } else if (index.Current() == _T("TimeFormat")) {
                    FrontPanelHAL::TimeFormat timeFormat;
                    if (_controller.GetFPTimeFormat(timeFormat)) {
                        response->TimeFormat = timeFormat;
                        result->ErrorCode = Web::STATUS_OK;
                        result->Body(response);
                        result->Message = _T("Get Front Panel Time Format.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Get Front Panel Time Format.");
                    }
                }
            }
        }
        return result;
    }

    Core::ProxyType<Web::Response> FrontPanel::PostMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported POST requestservice.");

        if (index.IsValid() == true) {
            if (index.Next() && index.IsValid()) {
                Core::ProxyType<Web::JSONBodyType<FrontPanel::Config>> response(jsonBodyDataFactory.Element());
                if (index.Current() == _T("State") && (request.HasBody())) {
                    FrontPanelHAL::Indicator indicator = request.Body<const Config>()->Indicator.Value();
                    FrontPanelHAL::State state = request.Body<const Config>()->State.Value();
                    if (_controller.SetFPState(indicator, state)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel State.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel State.");
                    }
                } else if (index.Current() == _T("Brightness") && (request.HasBody())) {
                    FrontPanelHAL::Indicator indicator = request.Body<const Config>()->Indicator.Value();
                    uint32_t brightness = request.Body<const Config>()->Brightness.Value();
                    if (_controller.SetFPBrightness(indicator, brightness)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Brightness.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Brightness.");
                    }
                } else if (index.Current() == _T("Color") && (request.HasBody())) {
                    FrontPanelHAL::Indicator indicator = request.Body<const Config>()->Indicator.Value();
                    FrontPanelHAL::Color color = request.Body<const Config>()->Color.Value();
                    if (_controller.SetFPColor(indicator, color)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Color.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Color.");
                    }
                } else if (index.Current() == _T("TextBrightness") && (request.HasBody())) {
                    FrontPanelHAL::TextDisplay textDisplay = request.Body<const Config>()->TextDisplay.Value();
                    uint32_t brightness = request.Body<const Config>()->Brightness.Value();
                    if (_controller.SetFPTextBrightness(textDisplay, brightness)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Text Brightness.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Text Brightness.");
                    }
                } else if (index.Current() == _T("TimeFormat") && (request.HasBody())) {
                    FrontPanelHAL::TimeFormat timeFormat = request.Body<const Config>()->TimeFormat.Value();
                    if (_controller.SetFPTimeFormat(timeFormat)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Time Format.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Time Format.");
                    }
                } else if (index.Current() == _T("Blink") && (request.HasBody())) {
                    FrontPanelHAL::Indicator indicator = request.Body<const Config>()->Indicator.Value();
                    uint32_t blinkDuration = request.Body<const Config>()->BlinkDuration.Value();
                    uint32_t blinkIterations = request.Body<const Config>()->BlinkIterations.Value();
                    if (_controller.SetFPBlink(indicator, blinkDuration, blinkIterations)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Blink.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Blink.");
                    }
                } else if (index.Current() == _T("Time") && (request.HasBody())) {
                    FrontPanelHAL::TimeFormat timeFormat = request.Body<const Config>()->TimeFormat.Value();
                    uint32_t hour = request.Body<const Config>()->Hour.Value();
                    uint32_t minutes = request.Body<const Config>()->Minutes.Value();
                    if (_controller.SetFPTime(timeFormat, hour, minutes)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Time.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Time.");
                    }
                } else if (index.Current() == _T("Text") && (request.HasBody())) {
                    std::string text = request.Body<const Config>()->Text.Value();
                    if (_controller.SetFPText(text)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Text.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Text.");
                    }
                } else if (index.Current() == _T("ClockDisplay") && (request.HasBody())) {
                    bool enable = request.Body<const Config>()->Enable.Value();
                    if (_controller.FPEnableClockDisplay(enable)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Enable Front Panel Clock Display.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Clock Display.");
                    }
                } else if (index.Current() == _T("Scroll") && (request.HasBody())) {
                    uint32_t scrollHoldOnDur = request.Body<const Config>()->ScrollHoldOnDur.Value();
                    uint32_t horzScrollIterations = request.Body<const Config>()->HorzScrollIterations.Value();
                    uint32_t vertScrollIterations = request.Body<const Config>()->VertScrollIterations.Value();
                    if (_controller.SetFPScroll(scrollHoldOnDur, horzScrollIterations, vertScrollIterations)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Scroll.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Scroll.");
                    }
                } else if (index.Current() == _T("DBrightness") && (request.HasBody())) {
                    FrontPanelHAL::Indicator indicator = request.Body<const Config>()->Indicator.Value();
                    uint32_t brightness = request.Body<const Config>()->Brightness.Value();
                    bool toPersist = request.Body<const Config>()->ToPersist.Value();
                    if (_controller.SetFPDBrightness(indicator, brightness, toPersist)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Display Brightness.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Brightness.");
                    }
                } else if (index.Current() == _T("DColor") && (request.HasBody())) {
                    FrontPanelHAL::Indicator indicator = request.Body<const Config>()->Indicator.Value();
                    FrontPanelHAL::Color color = request.Body<const Config>()->Color.Value();
                    bool toPersist = request.Body<const Config>()->ToPersist.Value();
                    if (_controller.SetFPDColor(indicator, color, toPersist)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Display Color.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Display Color.");
                    }
                }
            }
        }
        return result;
    }
}
}
