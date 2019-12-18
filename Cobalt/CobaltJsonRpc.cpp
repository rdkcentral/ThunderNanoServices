#include <interfaces/json/JsonData_Browser.h>
#include <interfaces/json/JsonData_StateControl.h>
#include "Cobalt.h"
#include "Module.h"

namespace WPEFramework {
namespace Plugin {

using namespace JsonData::Browser;
using namespace JsonData::StateControl;

// Registration
//
void Cobalt::RegisterAll() {
    Property < Core::JSON::String
            > (_T("url"), &Cobalt::get_url, &Cobalt::set_url, this); /* Browser */
    Property < Core::JSON::EnumType
            < VisibilityType
                    >> (_T("visibility"), &Cobalt::get_visibility, &Cobalt::set_visibility, this); /* Browser */
    Property < Core::JSON::DecUInt32
            > (_T("fps"), &Cobalt::get_fps, nullptr, this); /* Browser */
    Property < Core::JSON::EnumType
            < StateType
                    >> (_T("state"), &Cobalt::get_state, &Cobalt::set_state, this); /* StateControl */
}

void Cobalt::UnregisterAll() {
    Unregister(_T("state"));
    Unregister(_T("fps"));
    Unregister(_T("visibility"));
    Unregister(_T("url"));
}

// API implementation
//

// Property: url - URL loaded in the browser
// Return codes:
//  - ERROR_NONE: Success
uint32_t Cobalt::get_url(Core::JSON::String &response) const /* Browser */
{
    ASSERT(_cobalt != nullptr);
    response = _cobalt->GetURL();
    return Core::ERROR_NONE;
}

// Property: url - URL loaded in the browser
// Return codes:
//  - ERROR_NONE: Success
//  - ERROR_INCORRECT_URL: Incorrect URL given
uint32_t Cobalt::set_url(const Core::JSON::String &param) /* Browser */
{
    ASSERT(_cobalt != nullptr);
    uint32_t result = Core::ERROR_INCORRECT_URL;
    if (param.IsSet() && !param.Value().empty()) {
        _cobalt->SetURL(param.Value());
        result = Core::ERROR_NONE;
    }
    return result;
}

// Property: visibility - Current browser visibility
// Return codes:
//  - ERROR_NONE: Success
uint32_t Cobalt::get_visibility(
        Core::JSON::EnumType<VisibilityType> &response) const /* Browser */ {
    response = (_hidden ? VisibilityType::HIDDEN : VisibilityType::VISIBLE);
    return Core::ERROR_NONE;
}

// Property: visibility - Current browser visibility
// Return codes:
//  - ERROR_NONE: Success
uint32_t Cobalt::set_visibility(
        const Core::JSON::EnumType<VisibilityType> &param) /* Browser */ {
    ASSERT(_cobalt != nullptr);
    uint32_t result = Core::ERROR_BAD_REQUEST;

    if (param.IsSet()) {
        if (param == VisibilityType::VISIBLE) {
            _cobalt->Hide(true);
        } else {
            _cobalt->Hide(false);
        }
        result = Core::ERROR_NONE;
    }
    return result;
}

// Property: fps - Current number of frames per second the browser is rendering
// Return codes:
//  - ERROR_NONE: Success
uint32_t Cobalt::get_fps(Core::JSON::DecUInt32 &response) const /* Browser */
{
    ASSERT(_cobalt != nullptr);
    response = _cobalt->GetFPS();
    return Core::ERROR_NONE;
}

// Property: state - Running state of the service
// Return codes:
//  - ERROR_NONE: Success
uint32_t Cobalt::get_state(Core::JSON::EnumType<StateType> &response) const /* StateControl */
{
    ASSERT(_cobalt != nullptr);
    PluginHost::IStateControl *stateControl(_cobalt->QueryInterface<PluginHost::IStateControl>());
    if (stateControl != nullptr) {
        PluginHost::IStateControl::state currentState = stateControl->State();
        response = (
            currentState == PluginHost::IStateControl::SUSPENDED ?
                    StateType::SUSPENDED : StateType::RESUMED);
        stateControl->Release();
    }
    
    return Core::ERROR_NONE;
}

// Property: state - Running state of the service
// Return codes:
//  - ERROR_NONE: Success
uint32_t Cobalt::set_state(const Core::JSON::EnumType<StateType> &param) /* StateControl */
{
    ASSERT(_cobalt != nullptr);
    uint32_t result = Core::ERROR_BAD_REQUEST;

    if (param.IsSet()) {
        PluginHost::IStateControl *stateControl(
                _cobalt->QueryInterface<PluginHost::IStateControl>());
        if (stateControl != nullptr) {
            stateControl->Request(
                param == StateType::SUSPENDED ?
                        PluginHost::IStateControl::SUSPEND :
                        PluginHost::IStateControl::RESUME);

            stateControl->Release();
        }
        result = Core::ERROR_NONE;
    }
    return result;
}

// Event: urlchange - Signals a URL change in the browser
void Cobalt::event_urlchange(const string &url, const bool &loaded) /* Browser */
{
    UrlchangeParamsData params;
    params.Url = url;
    params.Loaded = loaded;

    Notify(_T("urlchange"), params);
}

// Event: visibilitychange - Signals a visibility change of the browser
void Cobalt::event_visibilitychange(const bool &hidden) /* Browser */
{
    VisibilitychangeParamsData params;
    params.Hidden = hidden;

    Notify(_T("visibilitychange"), params);
}

// Event: statechange - Signals a state change of the service
void Cobalt::event_statechange(const bool &suspended) /* StateControl */
{
    StatechangeParamsData params;
    params.Suspended = suspended;

    Notify(_T("statechange"), params);
}

} // namespace Plugin
} // namespace WPEFramework
