
#include <interfaces/json/JsonData_WebKitBrowser.h>
#include <interfaces/json/JsonData_StateControl.h>
#include "WebKitBrowser.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::WebKitBrowser;
    using namespace JsonData::StateControl;

    // Registration
    //

    void WebKitBrowser::RegisterAll()
    {
        Register<void,StatusResultData>(_T("status"), &WebKitBrowser::endpoint_status, this);
        Register<void,void>(_T("suspend"), &WebKitBrowser::endpoint_suspend, this); /* StateControl */
        Register<void,void>(_T("resume"), &WebKitBrowser::endpoint_resume, this); /* StateControl */
        Register<void,void>(_T("hide"), &WebKitBrowser::endpoint_hide, this);
        Register<void,void>(_T("show"), &WebKitBrowser::endpoint_show, this);
        Register<SeturlParamsData,void>(_T("seturl"), &WebKitBrowser::endpoint_seturl, this);
    }

    void WebKitBrowser::UnregisterAll()
    {
        Unregister(_T("seturl"));
        Unregister(_T("show"));
        Unregister(_T("hide"));
        Unregister(_T("resume"));
        Unregister(_T("suspend"));
        Unregister(_T("status"));
    }

    uint32_t WebKitBrowser::StateControlCommand(PluginHost::IStateControl::command command)
    {
        ASSERT(_browser != nullptr);

        PluginHost::IStateControl* stateControl(_browser->QueryInterface<PluginHost::IStateControl>());
        ASSERT(stateControl != nullptr);

        stateControl->Request(command);

        stateControl->Release();

        return Core::ERROR_NONE;
    }

    // API implementation
    //

    // Retrieves the WebKit Browser information.
    uint32_t WebKitBrowser::endpoint_status(StatusResultData& response)
    {
        ASSERT(_browser != nullptr);

        PluginHost::IStateControl* stateControl(_browser->QueryInterface<PluginHost::IStateControl>());
        ASSERT(stateControl != nullptr);

        PluginHost::IStateControl::state currentState = stateControl->State();

        response.Url = _browser->GetURL();
        response.Fps = _browser->GetFPS();
        response.Suspended = (currentState == PluginHost::IStateControl::SUSPENDED);
        response.Hidden = _hidden;

        stateControl->Release();

        return Core::ERROR_NONE;
    }

    // Suspends the WebKit Browser.
    uint32_t WebKitBrowser::endpoint_suspend() /* StateControl */
    {
        return StateControlCommand(PluginHost::IStateControl::SUSPEND);
    }

    // Resumes the WebKit Browser.
    uint32_t WebKitBrowser::endpoint_resume() /* StateControl */
    {
        return StateControlCommand(PluginHost::IStateControl::RESUME);
    }

    // Hides the WebKit Browser.
    uint32_t WebKitBrowser::endpoint_hide()
    {
        ASSERT(_browser != nullptr);
        _browser->Hide(true);

        return Core::ERROR_NONE;
    }

    // Shows the WebKit Browser.
    uint32_t WebKitBrowser::endpoint_show()
    {
        ASSERT(_browser != nullptr);
        _browser->Hide(false);

        return Core::ERROR_NONE;
    }

    // Sets a URL in the WebKit Browser.
    uint32_t WebKitBrowser::endpoint_seturl(const SeturlParamsData& params)
    {
        ASSERT(_browser != nullptr);
        uint32_t result = Core::ERROR_INCORRECT_URL;

        if (params.Url.IsSet() && !params.Url.Value().empty()) {
            _browser->SetURL(params.Url.Value());
            result = Core::ERROR_NONE;
        }

        return result;
    }

    // Signals a URL change in the browser.
    void WebKitBrowser::event_urlchange(const string& url, const bool& loaded)
    {
        UrlchangeParamsData params;
        params.Url = url;
        params.Loaded = loaded;

        Notify(_T("urlchange"), params);
    }

    // Signals a state change in the browser.
    void WebKitBrowser::event_statechange(const bool& suspended) /* StateControl */
    {
        StatechangeParamsData params;
        params.Suspended = suspended;

        Notify(_T("statechange"), params);
    }

    // Signals a visibility change of the browser.
    void WebKitBrowser::event_visibilitychange(const bool& hidden)
    {
        VisibilitychangeParamsData params;
        params.Hidden = hidden;

        Notify(_T("visibilitychange"), params);
    }

    // Notifies that the web page requests to close its window.
    void WebKitBrowser::event_pageclosure()
    {
        Notify(_T("pageclosure"));
    }

} // namespace Plugin

} // namespace WPEFramework

