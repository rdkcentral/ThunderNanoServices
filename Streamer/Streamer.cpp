#include "Streamer.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(Streamer, 1, 0);

static Core::ProxyPoolType<Web::JSONBodyType<Streamer::Data> > jsonBodyDataFactory(2);

/* virtual */ const string Streamer::Initialize(PluginHost::IShell* service)
{
    string message;
    Config config;

    ASSERT(_service == nullptr);

    // Setup skip URL for right offset.
    _pid = 0;
    _service = service;
    _skipURL = _service->WebPrefix().length();

    config.FromString(_service->ConfigLine());
    _player = _service->Root<Exchange::IPlayer>(_pid, 2000, _T("StreamerImplementation"));

    if ((_player != nullptr) && (_service != nullptr)) {
        TRACE(Trace::Information, (_T("Successfully instantiated Streamer")));
        _player->Configure(_service);
    } else {
        TRACE(Trace::Error, (_T("Streamer could not be initialized.")));
        message = _T("Streamer could not be initialized.");
    }

    return message;
}

/* virtual */ void Streamer::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);
    ASSERT(_player != nullptr);

    // Stop processing of the browser:
    if (_player->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {

        ASSERT(_pid != 0);

        TRACE(Trace::Error, (_T("OutOfProcess Plugin is not properly destructed. PID: %d"), _pid));

        RPC::IRemoteProcess* process(_service->RemoteProcess(_pid));

        // The process can disappear in the meantime...
        if (process != nullptr) {

            // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
            process->Terminate();
            process->Release();
        }
    }

    _player = nullptr;
    _service = nullptr;
}

/* virtual */ string Streamer::Information() const
{
    // No additional info to report.
    return (string());
}

/* virtual */ void Streamer::Inbound(Web::Request& request)
{
    if (request.Verb == Web::Request::HTTP_POST || request.Verb == Web::Request::HTTP_PUT) {
        request.Body(jsonBodyDataFactory.Element());
    }
}

// GET <- Return the player numbers in use.
// GET ../Window <- Return the Rectangle in which the player is running.
// POST ../Create/<Type> <- Create an instance of a stream of type <Type>, Body return the stream index for reference in the other calls.
// PUT ../<Number>/Load <- Load the URL given in the body onto this stream
// PUT ../<Number>/Attach <- Attach a decoder to the primer of stream <Number>
// PUT ../<Number>/Speed/<speed> <- Set the stream as a speed of <speed>
// PUT ../<Number>/Detach <- Detach a decoder to the primer of stream <Number>
// PUT ../<Number>/Window/X/Y/Width/Height <- Set Window size
// DELETE ../<Number> <- Drop the stream adn thus the decoder
/* virtual */ Core::ProxyType<Web::Response> Streamer::Process(const Web::Request& request)
{
    ASSERT(_skipURL <= request.Path.length());

    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
    Core::TextSegmentIterator index(
        Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

    // Always skip the first, this is the slash
    index.Next();

    if (_player != nullptr) {
        if (request.Verb == Web::Request::HTTP_GET) {
            result = GetMethod(index);
        }
        else if (request.Verb == Web::Request::HTTP_PUT) {
            result = PutMethod(index, request);
        }
        else if (request.Verb == Web::Request::HTTP_POST) {
            result = PostMethod(index, request);
        }
        else if (request.Verb == Web::Request::HTTP_DELETE) {
            result = DeleteMethod(index);
        }
    }
    else {
        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = "Player instance is not created";
    }
    return result;
}

Core::ProxyType<Web::Response> Streamer::GetMethod(Core::TextSegmentIterator& index)
{
    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
    Core::ProxyType<Web::JSONBodyType<Data> > response(jsonBodyDataFactory.Element());

    if (index.IsValid() == true) {
        if (index.Next() && index.IsValid()) {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            uint8_t position = Core::NumberType<uint8_t> (index.Current());
            Streams::iterator stream = _streams.find(position);
            if ((stream != _streams.end()) && (index.Next() == true)) {
                if (index.Remainder() == _T("Type")) {
                    response->Type = stream->second->Type();
                    result->ErrorCode = Web::STATUS_OK;
                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(response);
                }
                else if (index.Remainder() == _T("DRM")) {
                    response->DRM = stream->second->DRM();
                    result->ErrorCode = Web::STATUS_OK;
                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(response);
                }
                else if (index.Remainder() == _T("Metadata")) {
                    response->Metadata = stream->second->Metadata();
                    result->ErrorCode = Web::STATUS_OK;
                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(response);
                }
                else if (index.Remainder() == _T("State")) {
                    response->State = stream->second->State();
                    result->ErrorCode = Web::STATUS_OK;
                    result->ContentType = Web::MIMETypes::MIME_JSON;
                    result->Body(response);
                }
                else {
                    Controls::iterator control = _controls.find(position);
                    if (control != _controls.end()) {
                        if (index.Remainder() == _T("Speed")) {
                            response->Speed = control->second->Speed();
                            result->ErrorCode = Web::STATUS_OK;
                            result->ContentType = Web::MIMETypes::MIME_JSON;
                            result->Body(response);
                        }
                        else if (index.Remainder() == _T("Position")) {
                            response->Position = control->second->Position();
                            result->ErrorCode = Web::STATUS_OK;
                            result->ContentType = Web::MIMETypes::MIME_JSON;
                            result->Body(response);
                        }
                        else if (index.Remainder() == _T("Window")) {
                            Exchange::IStream::IControl::IGeometry* geometry = control->second->Geometry();
                            response->X = geometry->X();
                            response->Y = geometry->Y();
                            response->Width = geometry->Width();
                            response->Height = geometry->Height();
                            result->ErrorCode = Web::STATUS_OK;
                            result->ContentType = Web::MIMETypes::MIME_JSON;
                            result->Body(response);
                        }
                    }
                }
            }
        }
    }
    else {
        TRACE(Trace::Information, (string(__FUNCTION__)));
        for (int8_t position; position < _streams.size(); ++position) {
             Streams::iterator stream = _streams.find(position);
             if (stream != _streams.end()) {
                 Core::JSON::DecUInt8 id;
                 id = stream->first;
                 response->Ids.Add(id);
             }
        }
        if (response->Ids.Length() != 0) {
            result->ErrorCode = Web::STATUS_OK;
            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->Body(response);
        }
    }
    return result;
}

Core::ProxyType<Web::Response> Streamer::PutMethod(Core::TextSegmentIterator& index, const Web::Request& request)
{
    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
    result->ErrorCode = Web::STATUS_BAD_REQUEST;
    result->Message = _T("Unsupported PUT request.");

    TRACE(Trace::Information, (string(__FUNCTION__)));
    if (index.IsValid() == true) {
        if (index.Next() == true) {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            uint8_t position = Core::NumberType<uint8_t> (index.Current());
            Streams::iterator stream = _streams.find(position);
            if ( (stream != _streams.end()) && (index.Next() == true)) {
                Core::ProxyType<Web::JSONBodyType<Data> > response(jsonBodyDataFactory.Element());
                if (index.Remainder() == _T("Load") && (request.HasBody() == true)) {
                    std::string url = request.Body<const Data>()->Url.Value();
                    stream->second->Load(url);
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = _T("Stream loaded");
                }
                else if (index.Remainder() == _T("Attach")) {
                    Exchange::IStream::IControl* control = stream->second->Control();
                    if (control != nullptr) {
                        _controls.insert(std::make_pair(position, control));
                        result->Message = _T("Decoder Attached");
                        result->ErrorCode = Web::STATUS_OK;
                    }
                }
                else {
                   TRACE(Trace::Information, (string(__FUNCTION__)));
                    Controls::iterator control = _controls.find(position);
                    if (control != _controls.end()) {
                        if (index.Current() == _T("Speed") &&  (index.Next() == true)) {
                            int32_t speed = Core::NumberType<int32_t> (index.Current());
                            control->second->Speed(speed);
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Speed set");
                        }
                        else if (index.Current() == _T("Position") &&  (index.Next() == true)) {
                            TRACE(Trace::Information, (string(__FUNCTION__)));
                            uint64_t absoluteTime = Core::NumberType<uint64_t> (index.Current());
                            control->second->Position(absoluteTime);
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Position set");
                        }
                        else if (index.Current() == _T("Window")) {
                            uint32_t X = 0;
                            if (index.Next() == true) {
                                X = Core::NumberType<uint32_t>(index.Current()).Value();
                            }
                            uint32_t Y = 0;
                            if (index.Next() == true) {
                                Y = Core::NumberType<uint32_t>(index.Current()).Value();
                            }
                            uint32_t width = 0;
                            if (index.Next() == true) {
                                width = Core::NumberType<uint32_t>(index.Current()).Value();
                            }
                            uint32_t height = 0;
                            if (index.Next() == true) {
                                height = Core::NumberType<uint32_t>(index.Current()).Value();
                            }
                            Exchange::IStream::IControl::IGeometry* geometry = Core::Service<Player::Implementation::Geometry>::Create<Player::Implementation::Geometry>(X, Y, control->second->Geometry()->Z(), width, height);
                            control->second->Geometry(geometry);
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Window set");
                        }
                        else if (index.Remainder() == _T("Detach")) {
                            control->second->Release();
                            _controls.erase(position);
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Decoder is detached");
                        }
                    }
                }
            }
        }
    }
    return result;
}

Core::ProxyType<Web::Response> Streamer::PostMethod(Core::TextSegmentIterator& index, const Web::Request& request)
{
    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
    result->ErrorCode = Web::STATUS_BAD_REQUEST;
    result->Message = _T("Unsupported POST requestservice.");
    TRACE(Trace::Information, (string(__FUNCTION__)));
    if (index.IsValid() == true) {
        if (index.Next() == true) {
            if (index.Current() == _T("Create")) {

                if (index.Next() == true) {
                    Core::EnumerateType<Exchange::IStream::streamtype> type (index.Current());
                    if (type.IsSet()) {
                        Core::ProxyType<Web::JSONBodyType<Data> > response(jsonBodyDataFactory.Element());

                        Exchange::IStream *stream = _player->CreateStream(type.Value());
                        if (stream != nullptr) {

                            uint8_t position = 0;
                            for (; position < _streams.size(); ++position) {
                                Streams::iterator stream = _streams.find(position);
                                if (stream == _streams.end()) {
                                    break;
                                }
                            }
                            _streams.insert(std::make_pair(position, stream));
                            response->Id = position;
                            result->Body(response);
                            result->ErrorCode = Web::STATUS_OK;
                            result->Message = _T("Stream is created");
                        }
                    }
                } else {
                    TRACE(Trace::Error, (string(_T("Stream is not yet created"))));
                    result->Message = _T("Stream is not created");
                }
            }
        }
    }
    return result;
}

Core::ProxyType<Web::Response> Streamer::DeleteMethod(Core::TextSegmentIterator& index)
{
    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
    result->ErrorCode = Web::STATUS_BAD_REQUEST;
    result->Message = _T("Unsupported DELETE request.");

    if (index.IsValid() == true) {
        if (index.Next() == true) {
            uint8_t position = Core::NumberType<uint8_t> (index.Current());
            Controls::iterator control = _controls.find(position);
            if (control != _controls.end())  {
                control->second->Release();
                _controls.erase(control);
            }
            Streams::iterator stream = _streams.find(position);
            if (stream != _streams.end()) {
                stream->second->Release();
                _streams.erase(position);
                result->ErrorCode = Web::STATUS_OK;
                result->Message = _T("Stream is released");
            }
        }
    }
    return result;
}

void Streamer::Deactivated(RPC::IRemoteProcess* process)
{
    // This can potentially be called on a socket thread, so the deactivation (wich in turn kills this object) must be done
    // on a seperate thread. Also make sure this call-stack can be unwound before we are totally destructed.
    if (_pid == process->Id()) {
        ASSERT(_service != nullptr);
        PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
    }
}

} //namespace Plugin
} // namespace WPEFramework
