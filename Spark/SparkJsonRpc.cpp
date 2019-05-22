
#include <interfaces/json/JsonData_Spark.h>
#include <interfaces/json/JsonData_StateControl.h>
#include "Spark.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::Spark;
    using namespace JsonData::StateControl;

    // Registration
    //

    void Spark::RegisterAll()
    {
        Register<void,StatusResultData>(_T("status"), &Spark::endpoint_status, this);
        Register<void,void>(_T("suspend"), &Spark::endpoint_suspend, this); /* StateControl */
        Register<void,void>(_T("resume"), &Spark::endpoint_resume, this); /* StateControl */
        Register<void,void>(_T("hide"), &Spark::endpoint_hide, this);
        Register<void,void>(_T("show"), &Spark::endpoint_show, this);
        Register<SeturlParamsData,void>(_T("seturl"), &Spark::endpoint_seturl, this);
    }

    void Spark::UnregisterAll()
    {
        Unregister(_T("seturl"));
        Unregister(_T("show"));
        Unregister(_T("hide"));
        Unregister(_T("resume"));
        Unregister(_T("suspend"));
        Unregister(_T("status"));
    }

    uint32_t Spark::StateControlCommand(PluginHost::IStateControl::command command)
    {
        ASSERT(_spark != nullptr);

        PluginHost::IStateControl* stateControl(_spark->QueryInterface<PluginHost::IStateControl>());
        ASSERT(stateControl != nullptr);

        stateControl->Request(command);

        stateControl->Release();

        return Core::ERROR_NONE;
    }

    // API implementation
    //

    // Retrieves the Spark Engine information.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Spark::endpoint_status(StatusResultData& response)
    {
        printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
        ASSERT(_spark != nullptr);

        PluginHost::IStateControl* stateControl(_spark->QueryInterface<PluginHost::IStateControl>());
        ASSERT(stateControl != nullptr);

        PluginHost::IStateControl::state currentState = stateControl->State();

        response.Url = _spark->GetURL();
        response.Fps = _spark->GetFPS();
        response.Suspended = (currentState == PluginHost::IStateControl::SUSPENDED);
        response.Hidden = _hidden;

        stateControl->Release();

        return Core::ERROR_NONE;
    }

    // Suspends the Spark Browser.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Spark::endpoint_suspend() /* StateControl */
    {
        printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
        return StateControlCommand(PluginHost::IStateControl::SUSPEND);
    }

    // Resumes the Spark Browser.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Spark::endpoint_resume() /* StateControl */
    {
        printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
        return StateControlCommand(PluginHost::IStateControl::RESUME);
    }

    // Hides the Spark Browser.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Spark::endpoint_hide()
    {
        printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
        ASSERT(_spark != nullptr);
        _spark->Hide(true);

        return Core::ERROR_NONE;
    }

    // Shows the Spark Browser.
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Spark::endpoint_show()
    {
        printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
        ASSERT(_spark != nullptr);
        _spark->Hide(false);

        return Core::ERROR_NONE;
    }

    // Sets a URL in the Spark Browser.
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INCORRECT_URL: Incorrect URL given
    uint32_t Spark::endpoint_seturl(const SeturlParamsData& params)
    {
        printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
        ASSERT(_spark != nullptr);
        uint32_t result = Core::ERROR_INCORRECT_URL;

        if (params.Url.IsSet() && !params.Url.Value().empty()) {
            _spark->SetURL(params.Url.Value());
            result = Core::ERROR_NONE;
        }
        return result;
    }

    // Signals a URL change in the browser.
    void Spark::event_urlchange(const string& url, const bool& loaded)
    {
        UrlchangeParamsData params;
        params.Url = url;
        params.Loaded = loaded;

        Notify(_T("urlchange"), params);
    }

    // Signals a state change in the browser.
    void Spark::event_statechange(const bool& suspended) /* StateControl */
    {
        StatechangeParamsData params;
        params.Suspended = suspended;

        Notify(_T("statechange"), params);
    }

    // Signals a visibility change of the browser.
    void Spark::event_visibilitychange(const bool& hidden)
    {
        VisibilitychangeParamsData params;
        params.Hidden = hidden;

        Notify(_T("visibilitychange"), params);
    }

} // namespace Plugin

}

