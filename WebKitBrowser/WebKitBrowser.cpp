#include "WebKitBrowser.h"

namespace WPEFramework {

namespace WebKitBrowser {

    // An implementation file needs to implement this method to return an operational browser, wherever that would be :-)
    extern Exchange::IMemory* MemoryObserver(const uint32_t pid);
}

namespace Plugin {

    SERVICE_REGISTRATION(WebKitBrowser, 1, 0);

    // static Core::ProxyPoolType< Web::JSONLabel < WebKitBrowser::Data > > jsonBodyDataFactory(2);
    static Core::ProxyPoolType<Web::JSONBodyType<WebKitBrowser::Data> > jsonBodyDataFactory(2);

    /* virtual */ const string WebKitBrowser::Initialize(PluginHost::IShell* service)
    {
        Config config;
        string message;

        ASSERT(_service == nullptr);
        ASSERT(_browser == nullptr);
        ASSERT(_memory == nullptr);

        _pid = 0;
        _service = service;
        _skipURL = _service->WebPrefix().length();

        bool outOfProcess (config.OutOfProcess.Value() == true);

        config.FromString(_service->ConfigLine());

	if (EnvironmentOverride(config.EnvironmentOverride.Value())) {
            string value;
            if (Core::SystemInfo::GetEnvironment(_T("WPE_WEBKIT_OUT_OF_PROCESS"), value) == true) {
                outOfProcess = ((value.length() == 1) && (value[0] == '1')) || (((value.length() == 1) || (value.length() == 4)) && (toupper(value[0]) == 'T'));
            }
        }

        // Register the Process::Notification stuff. The Remote process might die before we get a
        // change to "register" the sink for these events !!! So do it ahead of instantiation.
        _service->Register(&_notification);

        if (outOfProcess == true) {
            _browser = service->Instantiate<Exchange::IBrowser>(2000, _T("WebKitImplementation"), static_cast<uint32_t>(~0), _pid, _service->Locator());
        }
        else {
            _browser = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IBrowser>(Core::Library(), _T("WebKitImplementation"), static_cast<uint32_t>(~0));
        }

        if ((_browser != nullptr) && (_service != nullptr)) {
            PluginHost::IStateControl* stateControl(_browser->QueryInterface<PluginHost::IStateControl>());

            // We see that sometimes the WPE implementation crashes before it reaches this point, than there is
            // no StateControl. Cope with this situation.
            if (stateControl == nullptr) {
                _browser->Release();
                _browser = nullptr;
            }
            else {
                _browser->Register(&_notification);
                _memory = WPEFramework::WebKitBrowser::MemoryObserver(_pid);

                ASSERT(_memory != nullptr);

                stateControl->Configure(_service);
                stateControl->Register(&_notification);
                stateControl->Release();
            }
        }

        if (_browser == nullptr) {
            message = _T("WebKitBrowser could not be instantiated.");
            _service->Unregister(&_notification);
            _service = nullptr;
        }

        return message;
    }

    /* virtual */ void WebKitBrowser::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(_service == service);
        ASSERT(_browser != nullptr);
        ASSERT(_memory != nullptr);

        // Make sure we get no longer get any notifications, we are deactivating..
        _service->Unregister(&_notification);
        _browser->Unregister(&_notification);
        _memory->Release();

        PluginHost::IStateControl* stateControl(_browser->QueryInterface<PluginHost::IStateControl>());

        // In case WPE rpcprocess crashed, there is no access to the statecontrol interface, check it !!
        if (stateControl != nullptr) {
            stateControl->Unregister(&_notification);
            stateControl->Release();
        }
        else {
            _notification.Release();
        }

        // Stop processing of the browser:
        if (_browser->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) {

            ASSERT(_pid != 0);

            TRACE_L1("Browser Plugin is not properly destructed. %d", _pid);

            RPC::IRemoteProcess* process(_service->RemoteProcess(_pid));

            // The process can disappear in the meantime...
            if (process != nullptr) {

                // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                process->Terminate();
                process->Release();
            }
        }

        _service = nullptr;
        _browser = nullptr;
        _memory = nullptr;
    }

    /* virtual */ string WebKitBrowser::Information() const
    {
        // No additional info to report.
        return (nullptr);
    }

    /* virtual */ void WebKitBrowser::Inbound(Web::Request& request)
    {
        if (request.Verb == Web::Request::HTTP_POST) {
            request.Body(jsonBodyDataFactory.Element());
        }
    }

    /* virtual */ Core::ProxyType<Web::Response> WebKitBrowser::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        TRACE(Trace::Information, (string(_T("Received request"))));

        Core::ProxyType<Web::Response> result(PluginHost::Factories::Instance().Response());
        Core::TextSegmentIterator index(
            Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        result->ErrorCode = Web::STATUS_BAD_REQUEST;
        result->Message = "Unknown error";

        if (_browser != nullptr) {

            PluginHost::IStateControl* stateControl(_browser->QueryInterface<PluginHost::IStateControl>());

            ASSERT(stateControl != nullptr);

            if (request.Verb == Web::Request::HTTP_GET) {
                PluginHost::IStateControl::state currentState = stateControl->State();
                Core::ProxyType<Web::JSONBodyType<WebKitBrowser::Data> > body(jsonBodyDataFactory.Element());
                body->URL = _browser->GetURL();
                body->FPS = _browser->GetFPS();
                body->Suspended = (currentState == PluginHost::IStateControl::SUSPENDED);
                body->Hidden = _hidden;
                result->ErrorCode = Web::STATUS_OK;
                result->Message = "OK";
                result->Body<Web::JSONBodyType<WebKitBrowser::Data> >(body);
            }
            else if ((request.Verb == Web::Request::HTTP_POST) && (index.Next() == true) && (index.Next() == true)) {
                result->ErrorCode = Web::STATUS_OK;
                result->Message = "OK";

                // We might be receiving a plugin download request.
                if (index.Remainder() == _T("Suspend")) {
                    stateControl->Request(PluginHost::IStateControl::SUSPEND);
                }
                else if (index.Remainder() == _T("Resume")) {
                    stateControl->Request(PluginHost::IStateControl::RESUME);
                }
                else if (index.Remainder() == _T("Hide")) {
                    _browser->Hide(true);
                }
                else if (index.Remainder() == _T("Show")) {
                    _browser->Hide(false);
                }
                else if ((index.Remainder() == _T("URL")) && (request.HasBody() == true) && (request.Body<const Data>()->URL.Value().empty() == false)) {
                    _browser->SetURL(request.Body<const Data>()->URL.Value());
                }
                else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = "Unknown error";
                }
            }
            stateControl->Release();
        }

        return result;
    }
    void WebKitBrowser::LoadFinished(const string& URL) {
        string message(string("{ \"url\": \"") + URL + string("\", \"loaded\":true }"));
        TRACE(Trace::Information, (_T("LoadFinished: %s"), message.c_str()));
        _service->Notify(message);
    }
    void WebKitBrowser::URLChanged(const string& URL) {
        string message(string("{ \"url\": \"") + URL + string("\" }"));
        TRACE(Trace::Information, (_T("URLChanged: %s"), message.c_str()));
        _service->Notify(message);
    }
    void WebKitBrowser::Hidden(const bool hidden) {
        TRACE(Trace::Information, (_T("Hidden: %s }"), (hidden ? "true" : "false")));
        string message(string("{ \"hidden\": ") + (hidden ? _T("true") : _T("false")) + string("}"));
        _hidden = hidden;
        _service->Notify(message);
    }
    void WebKitBrowser::Closure() {
        TRACE(Trace::Information, (_T("Closure: \"true\"")));
        _service->Notify(_T("{\"Closure\": true }"));
    }
    void WebKitBrowser::StateChange(const PluginHost::IStateControl::state state) {
        TRACE(Trace::Information, (_T("StateChange: { \"State\": %d }"), state));

        string message(
            string("{ \"suspended\": ") + (state == PluginHost::IStateControl::SUSPENDED ? _T("true") : _T("false")) + string(" }"));

        _service->Notify(message);
    }
    void WebKitBrowser::Deactivated(RPC::IRemoteProcess* process)
    {
        if (process->Id() == _pid) {

            ASSERT(_service != nullptr);

            PluginHost::WorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }
}
} // namespace Plugin
