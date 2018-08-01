
#include "FrontPanel.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(FrontPanel, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<FrontPanel::Config>> jsonBodyDataFactory(1);

    /* virtual */ const string FrontPanel::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_controller.IsValid() == false);

        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());
        _service = service;

        _controller = WPASupplicant::FrontPanelHAL::Create();
        if ((_controller.IsValid() == true)  && (_controller->IsOperational() == true)) {
        }

        return (_service != nullptr ? _T("") : _T("No service."));
    }

    /* virtual */ void FrontPanel::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        _service = nullptr;
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
        TRACE_L1("Web request %s", request.Path.c_str());
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');
        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();
        if (request.Verb == Web::Request::HTTP_GET)
            result = GetMethod(index);
        else if (request.Verb == Web::Request::HTTP_POST)
            result = PostMethod(index, request);

        return result;
    }

    Core::ProxyType<Web::Response> FrontPanel::GetMethod(Core::TextSegmentIterator& index)
    {
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported GET request.");

        if (index.IsValid() == true) {
            if (index.Next() && index.IsValid()) {
                Core::ProxyType<Web::JSONBodyType<FrontPanel::Config>> response(jsonBodyDataFactory.Element());
                if (index.Remainder() == _T("GetFPTimeFormat")) {
                    uint32_t timeFormat;
                    if (_controller->GetFPTimeFormat(timeFormat)) {
                        response->FPDTimeFormat = timeFormat;
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
        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = _T("Unsupported POST requestservice.");

        if (index.IsValid() == true) {
            if (index.Next() && index.IsValid()) {
                Core::ProxyType<Web::JSONBodyType<FrontPanel::Config>> response(jsonBodyDataFactory.Element());
                if (index.Remainder() == _T("GetFPState") && (request.HasBody())) {
                    uint32_t FPDIndicator = request.Body<const Config>()->FPDIndicator.Value();
                    bool state;
                    if (_controller->GetFPState(FPDIndicator, state)) {
                        response->FPDState = state;
                        result->ErrorCode = Web::STATUS_OK;
                        result->Body(response);
                        result->Message = _T("Get Front Panel State");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Get Front Panel State.");
                    }
                } else if (index.Remainder() == _T("GetFPBrightness") && (request.HasBody())) {
                    uint32_t FPDIndicator = request.Body<const Config>()->FPDIndicator.Value();
                    uint32_t brightness;
                    if (_controller->GetFPBrightness(FPDIndicator, brightness)) {
                        response->FPDBrightness = brightness;
                        result->ErrorCode = Web::STATUS_OK;
                        result->Body(response);
                        result->Message = _T("Get Front Panel Brightness.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Get Front Panel Brightness.");
                    }
                } else if (index.Remainder() == _T("GetFPColor") && (request.HasBody())) {
                    uint32_t FPDIndicator = request.Body<const Config>()->FPDIndicator.Value();
                    uint32_t color;
                    if (_controller->GetFPColor(FPDIndicator, color)) {
                        response->FPDColor = color;
                        result->ErrorCode = Web::STATUS_OK;
                        result->Body(response);
                        result->Message = _T("Get Front Panel Color.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Get Front Panel Color.");
                    }
                } else if (index.Remainder() == _T("GetFPTextBrightness") && (request.HasBody())) {
                    uint32_t FPDTextDisplay = request.Body<const Config>()->FPDTextDisplay.Value();
                    uint32_t brightness;
                    if (_controller->GetFPTextBrightness(FPDTextDisplay, brightness)) {
                        response->FPDBrightness = brightness;
                        result->ErrorCode = Web::STATUS_OK;
                        result->Body(response);
                        result->Message = _T("Get Front Panel Text Brightness.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Get Front Panel Text Brightness.");
                    }
                } else if (index.Remainder() == _T("SetFPBlink") && (request.HasBody())) {
                    uint32_t FPDIndicator = request.Body<const Config>()->FPDIndicator.Value();
                    uint32_t BlinkDuration = request.Body<const Config>()->BlinkDuration.Value();
                    uint32_t BlinkIterations = request.Body<const Config>()->BlinkIterations.Value();
                    if (_controller->SetFPBlink(FPDIndicator, BlinkDuration, BlinkIterations)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Blink.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Blink.");
                    }
                } else if (index.Remainder() == _T("SetFPBrightness") && (request.HasBody())) {
                    uint32_t FPDIndicator = request.Body<const Config>()->FPDIndicator.Value();
                    uint32_t FPDBrightness = request.Body<const Config>()->FPDBrightness.Value();
                    if (_controller->SetFPBrightness(FPDIndicator, FPDBrightness)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Brightness.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Brightness.");
                    }
                } else if (index.Remainder() == _T("SetFPState") && (request.HasBody())) {
                    uint32_t FPDIndicator = request.Body<const Config>()->FPDIndicator.Value();
                    bool FPDState = request.Body<const Config>()->FPDState.Value();
                    if (_controller->SetFPState(FPDIndicator, FPDState)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel State.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel State.");
                    }
                } else if (index.Remainder() == _T("SetFPColor") && (request.HasBody())) {
                    uint32_t FPDIndicator = request.Body<const Config>()->FPDIndicator.Value();
                    uint32_t FPDColor = request.Body<const Config>()->FPDColor.Value();
                    if (_controller->SetFPColor(FPDIndicator, FPDColor)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Color.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Color.");
                    }
                } else if (index.Remainder() == _T("SetFPTime") && (request.HasBody())) {
                    uint32_t FPDTimeFormat = request.Body<const Config>()->FPDTimeFormat.Value();
                    uint32_t Hour = request.Body<const Config>()->Hour.Value();
                    uint32_t Minutes = request.Body<const Config>()->Minutes.Value();
                    if (_controller->SetFPTime(FPDTimeFormat, Hour, Minutes)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Time.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Time.");
                    }
                } else if (index.Remainder() == _T("SetFPText") && (request.HasBody())) {
                    std::string Text = request.Body<const Config>()->Text.Value();
                    if (_controller->SetFPText(Text)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Text.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Text.");
                    }
                } else if (index.Remainder() == _T("SetFPTextBrightness") && (request.HasBody())) {
                    uint32_t FPDTextDisplay = request.Body<const Config>()->FPDTextDisplay.Value();
                    uint32_t FPDBrightness = request.Body<const Config>()->FPDBrightness.Value();
                    if (_controller->SetFPTextBrightness(FPDTextDisplay, FPDBrightness)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Text Brightness.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Text Brightness.");
                    }
                 } else if (index.Remainder() == _T("FPEnableClockDisplay") && (request.HasBody())) {
                    uint32_t Enable = request.Body<const Config>()->Enable.Value();
                    if (_controller->FPEnableClockDisplay(Enable)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Enable Front Panel Clock Display.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Clock Display.");
                    }
                } else if (index.Remainder() == _T("SetFPScroll") && (request.HasBody())) {
                    uint32_t ScrollHoldOnDur = request.Body<const Config>()->ScrollHoldOnDur.Value();
                    uint32_t HorzScrollIterations = request.Body<const Config>()->HorzScrollIterations.Value();
                    uint32_t VertScrollIterations = request.Body<const Config>()->VertScrollIterations.Value();
                    if (_controller->SetFPScroll(ScrollHoldOnDur, HorzScrollIterations, VertScrollIterations)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Scroll.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Scroll.");
                    }
                } else if (index.Remainder() == _T("SetFPDBrightness") && (request.HasBody())) {
                    uint32_t FPDIndicator = request.Body<const Config>()->FPDIndicator.Value();
                    uint32_t FPDBrightness = request.Body<const Config>()->FPDBrightness.Value();
                    bool ToPersist = request.Body<const Config>()->ToPersist.Value();
                    if (_controller->SetFPDBrightness(FPDIndicator, FPDBrightness, ToPersist)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Display Brightness.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Brightness.");
                    }
                } else if (index.Remainder() == _T("SetFPDColor") && (request.HasBody())) {
                    uint32_t FPDIndicator = request.Body<const Config>()->FPDIndicator.Value();
                    uint32_t FPDColor = request.Body<const Config>()->FPDColor.Value();
                    bool ToPersist = request.Body<const Config>()->ToPersist.Value();
                    if (_controller->SetFPDColor(FPDIndicator, FPDColor, ToPersist)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Display Color.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Display Color.");
                    }
                } else if (index.Remainder() == _T("SetFPTimeFormat") && (request.HasBody())) {
                    uint32_t FPDTimeFormat = request.Body<const Config>()->FPDTimeFormat.Value();
                    if (_controller->SetFPTimeFormat(FPDTimeFormat)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = _T("Set Front Panel Time Format.");
                    } else {
                        result->ErrorCode = Web::STATUS_NO_CONTENT;
                        result->Message = _T("Failed to Set Front Panel Time Format.");
                    }
                }
            }
        }
        return result;
    }
}
}
