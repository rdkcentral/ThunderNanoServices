#include "Power.h"

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(Power, 1, 0);

static Core::ProxyPoolType<Web::JSONBodyType<Power::Data> > jsonBodyDataFactory(2);
static Core::ProxyPoolType<Web::JSONBodyType<Power::Data> > jsonResponseFactory(4);

/* virtual */ const string Power::Initialize(PluginHost::IShell* service)
{
    string message;

    ASSERT(_power == nullptr);
    ASSERT(_service == nullptr);

    // Setup skip URL for right offset.
    _pid = 0;
    _service = service;
    _skipURL = static_cast<uint8_t>(_service->WebPrefix().length());

    _power = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IPower>(Core::Library(), _T("PowerImplementation"), static_cast<uint32_t>(~0));

    if (_power != nullptr) {
        PluginHost::VirtualInput* keyHandler (PluginHost::InputHandler::KeyHandler());

        ASSERT (keyHandler != nullptr);

        keyHandler->Register(KEY_POWER, &_sink);

        // Receive all plugin information on state changes.
        _service->Register(&_sink);
        
        _power->Configure(_service->ConfigLine());

    } else {
        _service = nullptr;

        message = _T("Power could not be instantiated.");
    }

    return message;
}

/* virtual */ void Power::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);
    ASSERT(_power != nullptr);

    // No need to monitor the Process::Notification anymore, we will kill it anyway.
    _service->Unregister(&_sink);

    // Remove all registered clients
    _clients.clear();

    // Also we are nolonger interested in the powerkey events, we have been requested to shut down our services!
    PluginHost::VirtualInput* keyHandler (PluginHost::InputHandler::KeyHandler());

    ASSERT (keyHandler != nullptr);
    keyHandler->Unregister(KEY_POWER, &_sink);

    _power = nullptr;
    _service = nullptr;
}

/* virtual */ string Power::Information() const
{
    // No additional info to report.
    return (string());
}

/* virtual */ void Power::Inbound(Web::Request& request)
{
    if (request.Verb == Web::Request::HTTP_POST)
        request.Body(jsonBodyDataFactory.Element());
}

/* virtual */ Core::ProxyType<Web::Response> Power::Process(const Web::Request& request)
{
    ASSERT(_skipURL <= request.Path.length());

    Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
    Core::TextSegmentIterator index(
        Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

    result->ErrorCode = Web::STATUS_BAD_REQUEST;
    result->Message = "Unknown error";

    ASSERT (_power != nullptr);

    if ((request.Verb == Web::Request::HTTP_GET) && ((index.Next() == true) && (index.Next() == true))) {
        TRACE(Trace::Information, (string(_T("GET request"))));
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";
        if (index.Remainder() == _T("State")) {
            TRACE(Trace::Information, (string(_T("State"))));
            Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
            response->PowerState = _power->GetState();
            if (response->PowerState) {
                result->ContentType = Web::MIMETypes::MIME_JSON;
                result->Body(Core::proxy_cast<Web::IBody>(response));
            } else {
                result->Message = "Invalid State";
            }
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = "Unknown error";
        }
    } else if ((request.Verb == Web::Request::HTTP_POST) && (index.Next() == true) && (index.Next() == true)) {
        TRACE(Trace::Information, (string(_T("POST request"))));
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";
        if (index.Remainder() == _T("State")) {
            TRACE(Trace::Information, (string(_T("State "))));
            uint32_t timeout = request.Body<const Data>()->Timeout.Value();
            Exchange::IPower::PCState state = static_cast<Exchange::IPower::PCState>(request.Body<const Data>()->PowerState.Value());

            ControlClients(state);

            Core::ProxyType<Web::JSONBodyType<Data> > response(jsonResponseFactory.Element());
            response->Status = _power->SetState(state, timeout);
            result->ContentType = Web::MIMETypes::MIME_JSON;
            result->Body(Core::proxy_cast<Web::IBody>(response));
        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = "Unknown error";
        }
    }

    return result;
}

void Power::KeyEvent(const uint32_t keyCode) {

    // We only subscribed for the KEY_POWER event so do not 
    // expect anything else !!!
    ASSERT (keyCode == KEY_POWER)

    if (keyCode == KEY_POWER) {
        _power->PowerKey();
    }
}

void Power::StateChange(PluginHost::IShell* plugin)
{
    const string callsign (plugin->Callsign());

    _adminLock.Lock();

    Clients::iterator index(_clients.find(callsign));

    if (plugin->State() == PluginHost::IShell::ACTIVATED) {

        if (index == _clients.end()) {
            PluginHost::IStateControl* stateControl(plugin->QueryInterface<PluginHost::IStateControl>());

            if (stateControl != nullptr) {
                _clients.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(callsign),
                                 std::forward_as_tuple(stateControl));
                TRACE(Trace::Information, (_T("%s plugin is add to power control list"), callsign.c_str()));
                stateControl->Release();
            }
        }
    } else if (plugin->State() == PluginHost::IShell::DEACTIVATED) {

        if (index != _clients.end()) { // Remove from the list, if it is already there
            _clients.erase(index);
            TRACE(Trace::Information, (_T("%s plugin is removed from power control list"), plugin->Callsign().c_str()));
        }
    }

    _adminLock.Unlock();
}

void Power::ControlClients(Exchange::IPower::PCState state)
{
    Clients::iterator client(_clients.begin());

    switch (state) {
    case Exchange::IPower::PCState::On:
         TRACE(Trace::Information, (_T("Change state to RESUME for")));
         while (client != _clients.end()) {
             client->second.Resume();
             client++;
         }
         //Nothing to be done
         break;
    case Exchange::IPower::PCState::ActiveStandby:
    case Exchange::IPower::PCState::PassiveStandby:
    case Exchange::IPower::PCState::SuspendToRAM:
    case Exchange::IPower::PCState::Hibernate:
    case Exchange::IPower::PCState::PowerOff:
         while (client != _clients.end()) {
             client->second.Suspend();
             client++;
         }
         break;
    default:
        ASSERT(false);
        break;
    }
}

} //namespace Plugin
} // namespace WPEFramework
