#include "TVControl.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(TVControl, 1, 0);

static Core::ProxyPoolType<Web::JSONBodyType<TVControl::Data> > jsonBodyDataFactory(2);
static Core::ProxyPoolType<Web::JSONBodyType<TVControl::Data> > jsonResponseFactory(4);

/* virtual */ const string TVControl::Initialize(PluginHost::IShell* service)
{
    string message;
    Config config;

    ASSERT(_tuner == nullptr);
    ASSERT(_service == nullptr);

    // Setup skip URL for right offset.
    _pid = 0;
    _service = service;
    _skipURL = _service->WebPrefix().length();
    _service->Register(&_notification);

    config.FromString(_service->ConfigLine());

    if (config.OutOfProcess.Value())
        _tuner = _service->Instantiate<Exchange::IStreaming>(8000, _T("TVControlImplementation"), static_cast<uint32_t>(~0), _pid, service->Locator()); // FIXME: 8sec sleep added to handle rpi tuner init sequence delay. Need to revisit later.
    else
        _tuner = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IStreaming>(Core::Library(), _T("TVControlImplementation"), static_cast<uint32_t>(~0));


    if (_tuner != nullptr) {
        _streamingListener = Core::Service<TVControl::StreamingNotification>::Create<TVControl::StreamingNotification>(this);
        if (_streamingListener != nullptr)
            _tuner->Register(_streamingListener);
        _tuner->Configure(_service);
        _guide = _tuner->QueryInterface<Exchange::IGuide>();
        if (_guide != nullptr) {
            _guideListener = Core::Service<TVControl::GuideNotification>::Create<TVControl::GuideNotification>(this);
            if (_guideListener != nullptr)
                _guide->Register(_guideListener);
            _guide->StartParser(_service);
        }
    } else {
        RPC::IRemoteProcess* process(_service->RemoteProcess(_pid));

        // The process can disappear in the meantime...
        if (process != nullptr) {
            process->Terminate();
            process->Release();
        }
        _service->Unregister(&_notification);
        _service = nullptr;
        message = _T("TVControl could not be instantiated.");
    }

    return message;
}

/* virtual */ void TVControl::Deinitialize(PluginHost::IShell* service)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    ASSERT(_service == service);
    ASSERT(_tuner != nullptr);

    if ((_tuner != nullptr) && (_streamingListener != nullptr))
        _tuner->Unregister(_streamingListener);

    if ((_guide != nullptr) && (_guideListener != nullptr))
        _guide->Unregister(_guideListener);

    _service->Unregister(&_notification);

    // Stop processing of the tvcontrol module:
     _guide->Release();
    if (_tuner->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {

        ASSERT(_pid);
        TRACE_L1("OutOfProcess Plugin is not properly destructed. PID: %d", _pid);

        RPC::IRemoteProcess* process(_service->RemoteProcess(_pid));

        // The process can disappear in the meantime...
        if (process != nullptr) {
            process->Terminate();
            process->Release();
        }
    }

    _tuner = nullptr;
    _guide = nullptr;
    _service = nullptr;
}

/* virtual */ string TVControl::Information() const
{
    // No additional info to report.
    return (string());
}

/* virtual */ void TVControl::Deactivated(RPC::IRemoteProcess* process)
{
    // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
    // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
    if (_pid == process->Id()) {

    ASSERT(_service != nullptr);

    PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
    }
}

/* virtual */ void TVControl::Inbound(Web::Request& request)
{
    if (request.Verb == Web::Request::HTTP_POST)
        request.Body(jsonBodyDataFactory.Element());
}

/* virtual */ Core::ProxyType<Web::Response> TVControl::Process(const Web::Request& request)
{
    ASSERT(_skipURL <= request.Path.length());

    TRACE(Trace::Information, (string(_T("Received request"))));

    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
    Core::TextSegmentIterator index(
        Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

    result->ErrorCode = Web::STATUS_BAD_REQUEST;
    result->Message = "Unknown error";

    if (_tuner != nullptr) {

        if ((request.Verb == Web::Request::HTTP_GET) && ((index.Next()) && (index.Next()))) {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            result->ErrorCode = Web::STATUS_OK;
            result->Message = "OK";
            if (index.Remainder() == _T("GetChannels")) {
                TRACE(Trace::Information, (string(__FUNCTION__)));
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                std::string channels = _guide->GetChannels();
                if (channels.size() > 0) {
                    response->Channels.FromString(channels);
                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                } else {
                    result->ErrorCode = Web::STATUS_NO_CONTENT;
                    result->Message = "Could not able to get Channel List";
                }

            } else if (index.Remainder() == _T("GetPrograms")) {
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                std::string programs = _guide->GetPrograms();
                if (programs.size() > 0) {
                    response->Programs.FromString(programs);
                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                } else {
                    result->ErrorCode = Web::STATUS_NO_CONTENT;
                    result->Message = "Could not able to get Program List";
                }
            } else if (index.Remainder() == _T("IsParentalControlled")) {
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                response->IsParentalControlled = _guide->IsParentalControlled();
                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            } else if (index.Remainder() == _T("GetCurrentChannel")) {
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                std::string channel = _tuner->GetCurrentChannel();
                if (channel.size() > 0) {
                    response->CurrentChannel.FromString(channel);
                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                } else {
                    result->ErrorCode = Web::STATUS_NO_CONTENT;
                    result->Message = "Could not able to get CurrentChannel";
                }
            } else if (index.Remainder() == _T("IsScanning")) {
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                response->IsScanning = _tuner->IsScanning();
                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            } else {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = "Unknown error";
            }
        } else if ((request.Verb == Web::Request::HTTP_POST) && (index.Next()) && (index.Next())) {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            result->ErrorCode = Web::STATUS_OK;
            result->Message = "OK";
            if (index.Remainder() == _T("StartScan")) {
                _tuner->StartScan();
            } else if (index.Remainder() == _T("StopScan")) {
                _tuner->StopScan();
            } else if ((index.Remainder() == _T("SetCurrentChannel")) && (request.HasBody())) {
                std::string id = request.Body<const Data>()->ChannelId.Value();
                _tuner->SetCurrentChannel(id);
            } else if ((index.Remainder() == _T("IsParentalLocked")) && (request.HasBody())) {
                std::string id = request.Body<const Data>()->ChannelId.Value();
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                response->IsParentalLocked = _guide->IsParentalLocked(id);
                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            } else if ((index.Remainder() == _T("Test")) && (request.HasBody())) {
                std::string str = request.Body<const Data>()->Str.Value();
                _tuner->Test(str);
            } else if ((index.Remainder() == _T("SetParentalControlPin")) && (request.HasBody())) {
                std::string oldPin = request.Body<const Data>()->OldPin.Value();
                std::string newPin = request.Body<const Data>()->NewPin.Value();
                TRACE(Trace::Information, (_T("oldPin = %s, newPin = %s"), oldPin.c_str(), newPin.c_str()));
                if (!(_guide->SetParentalControlPin(oldPin, newPin))) {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = "Could not set parental control pin";
                }
            } else if ((index.Remainder() == _T("SetParentalControl")) && (request.HasBody())) {
                std::string pin = request.Body<const Data>()->Pin.Value();
                bool isLocked = request.Body<const Data>()->IsLocked.Value();
                TRACE(Trace::Information, (_T("pin = %s, islocked = %d"), pin.c_str(), isLocked));
                if (!(_guide->SetParentalControl(pin, isLocked))) {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = "Could not set parental control";
                }
            } else if ((index.Remainder() == _T("SetParentalLock")) && (request.HasBody())) {
                std::string pin = request.Body<const Data>()->Pin.Value();
                bool isLocked = request.Body<const Data>()->IsLocked.Value();
                std::string channelNum = request.Body<const Data>()->ChannelId.Value();
                TRACE(Trace::Information, (_T("pin = %s, islocked = %d, chno. = %s"), pin.c_str(), isLocked, channelNum.c_str()));
                if (!(_guide->SetParentalLock(pin, isLocked, channelNum))) {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = "Could not set parental lock";
                }
            } else if (index.Remainder() == _T("GetCurrentProgram")) {
                std::string lcn = request.Body<const Data>()->ChannelId.Value();
                TRACE(Trace::Information, (_T("LCN = %s"), lcn.c_str()));
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                std::string program = _guide->GetCurrentProgram(lcn);
                if (program.size() > 0) {
                    response->CurrentProgram.FromString(program);
                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(Core::proxy_cast<Web::IBody>(response));
                } else {
                    result->ErrorCode = Web::STATUS_NO_CONTENT;
                    result->Message = "Could not able to get CurrentProgram";
                }
            } else if (index.Remainder() == _T("GetAudioLanguages")) {
                uint16_t eventId = request.Body<const Data>()->EventId.Value();
                TRACE(Trace::Information, (_T("eventId = %d"), eventId));
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                response->AudioLanguages.FromString(_guide->GetAudioLanguages(eventId));
                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            } else if (index.Remainder() == _T("GetSubtitleLanguages")) {
                uint16_t eventId = request.Body<const Data>()->EventId.Value();
                TRACE(Trace::Information, (_T("eventId = %d"), eventId));
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
                response->SubtitleLanguages.FromString(_guide->GetSubtitleLanguages(eventId));
                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            } else {
                result->ErrorCode = Web::STATUS_BAD_REQUEST;
                result->Message = "Unknown error";
            }
        }
    }

    return result;
}
}
}
