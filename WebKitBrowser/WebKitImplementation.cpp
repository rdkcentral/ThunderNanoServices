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

#include <memory>

#include "Module.h"

#ifdef WEBKIT_GLIB_API
#include <wpe/webkit.h>
#else
#include <WPE/WebKit.h>
#include <WPE/WebKit/WKCookieManagerSoup.h>
#include <WPE/WebKit/WKGeolocationManager.h> // TODO: add ref to this header in WebKit.h?
#include <WPE/WebKit/WKGeolocationPermissionRequest.h>
#include <WPE/WebKit/WKGeolocationPosition.h>
#include <WPE/WebKit/WKNotification.h>
#include <WPE/WebKit/WKNotificationManager.h>
#include <WPE/WebKit/WKNotificationPermissionRequest.h>
#include <WPE/WebKit/WKNotificationProvider.h>
#include <WPE/WebKit/WKSoupSession.h>
#include <WPE/WebKit/WKUserMediaPermissionRequest.h>

#include "BrowserConsoleLog.h"
#include "InjectedBundle/Tags.h"

#endif

#include <wpe/wpe.h>

#include <glib.h>

#include "HTML5Notification.h"
#include "WebKitBrowser.h"

#include <iostream>

namespace WPEFramework {
namespace Plugin {

#ifndef WEBKIT_GLIB_API
    static void onDidReceiveSynchronousMessageFromInjectedBundle(WKContextRef context, WKStringRef messageName,
        WKTypeRef messageBodyObj, WKTypeRef* returnData, const void* clientInfo);
    static void onNotificationShow(WKPageRef page, WKNotificationRef notification, const void* clientInfo);
    static void didStartProvisionalNavigation(WKPageRef page, WKNavigationRef navigation, WKTypeRef userData, const void* clientInfo);
    static void didFinishDocumentLoad(WKPageRef page, WKNavigationRef navigation, WKTypeRef userData, const void* clientInfo);
    static void onFrameDisplayed(WKViewRef view, const void* clientInfo);
    static void didSameDocumentNavigation(const OpaqueWKPage* page, const OpaqueWKNavigation* nav, unsigned int count, const void* clientInfo, const void* info);
    static void requestClosure(const void* clientInfo);
    static void didRequestAutomationSession(WKContextRef context, WKStringRef sessionID, const void* clientInfo);
    static WKPageRef onAutomationSessionRequestNewPage(WKWebAutomationSessionRef session, const void* clientInfo);

    // -----------------------------------------------------------------------------------------------------
    // Hide all NASTY C details that come with the POC libraries !!!!!
    // -----------------------------------------------------------------------------------------------------
    static WKPageNavigationClientV0 _handlerWebKit = {
        { 0, nullptr },
        // decidePolicyForNavigationAction
        [](WKPageRef, WKNavigationActionRef, WKFramePolicyListenerRef listener, WKTypeRef, const void* customData) {
            WKFramePolicyListenerUse(listener);
        },
        // decidePolicyForNavigationResponse
        [](WKPageRef, WKNavigationResponseRef, WKFramePolicyListenerRef listener, WKTypeRef, const void*) {
            WKFramePolicyListenerUse(listener);
        },
        nullptr, // decidePolicyForPluginLoad
        didStartProvisionalNavigation,
        nullptr, // didReceiveServerRedirectForProvisionalNavigation
        nullptr, // didFailProvisionalNavigation
        nullptr, // didCommitNavigation
        nullptr, // didFinishNavigation
        nullptr, // didFailNavigation
        nullptr, // didFailProvisionalLoadInSubframe
        didFinishDocumentLoad,
        didSameDocumentNavigation, // didSameDocumentNavigation
        nullptr, // renderingProgressDidChange
        nullptr, // canAuthenticateAgainstProtectionSpace
        nullptr, // didReceiveAuthenticationChallenge
        // webProcessDidCrash
        [](WKPageRef page, const void*) {
            SYSLOG(Trace::Fatal, ("CRASH: WebProcess crashed, exiting..."));
            exit(1);
        },
        nullptr, // copyWebCryptoMasterKey
        nullptr, // didBeginNavigationGesture
        nullptr, // willEndNavigationGesture
        nullptr, // didEndNavigationGesture
        nullptr, // didRemoveNavigationGestureSnapshot
    };

    static WKContextInjectedBundleClientV1 _handlerInjectedBundle = {
        { 1, nullptr },
        nullptr, // didReceiveMessageFromInjectedBundle
        // didReceiveSynchronousMessageFromInjectedBundle
        onDidReceiveSynchronousMessageFromInjectedBundle,
        nullptr, // getInjectedBundleInitializationUserData
    };

    WKGeolocationProviderV0 _handlerGeolocationProvider = {
        { 0, nullptr },
        // startUpdating
        [](WKGeolocationManagerRef geolocationManager, const void* clientInfo) {
            std::cerr << "in WKGeolocationProviderV0::startUpdating" << std::endl;
            WKGeolocationPositionRef position = WKGeolocationPositionCreate(0.0, 51.49, 4.40, 1.0);
            WKGeolocationManagerProviderDidChangePosition(geolocationManager, position);
        },
        nullptr, // stopUpdating
    };

    WKPageUIClientV8 _handlerPageUI = {
        { 8, nullptr },
        nullptr, // createNewPage_deprecatedForUseWithV0
        nullptr, // showPage
        // close
        [](const OpaqueWKPage*, const void* clientInfo) {
            requestClosure(clientInfo);
        },
        nullptr, // takeFocus
        nullptr, // focus
        nullptr, // unfocus
        nullptr, // runJavaScriptAlert_deprecatedForUseWithV0
        nullptr, // runJavaScriptConfirm_deprecatedForUseWithV0
        nullptr, // runJavaScriptPrompt_deprecatedForUseWithV0
        nullptr, // setStatusText
        nullptr, // mouseDidMoveOverElement_deprecatedForUseWithV0
        nullptr, // missingPluginButtonClicked_deprecatedForUseWithV0
        nullptr, // didNotHandleKeyEvent
        nullptr, // didNotHandleWheelEvent
        nullptr, // toolbarsAreVisible
        nullptr, // setToolbarsAreVisible
        nullptr, // menuBarIsVisible
        nullptr, // setMenuBarIsVisible
        nullptr, // statusBarIsVisible
        nullptr, // setStatusBarIsVisible
        nullptr, // isResizable
        nullptr, // setIsResizable
        nullptr, // getWindowFrame
        nullptr, // setWindowFrame
        nullptr, // runBeforeUnloadConfirmPanel
        nullptr, // didDraw
        nullptr, // pageDidScroll
        nullptr, // exceededDatabaseQuota
        nullptr, // runOpenPanel
        // decidePolicyForGeolocationPermissionRequest
        [](WKPageRef page, WKFrameRef frame, WKSecurityOriginRef origin, WKGeolocationPermissionRequestRef permissionRequest, const void* clientInfo) {
            WKGeolocationPermissionRequestAllow(permissionRequest);
        },
        nullptr, // headerHeight
        nullptr, // footerHeight
        nullptr, // drawHeader
        nullptr, // drawFooter
        nullptr, // printFrame
        nullptr, // runModal
        nullptr, // unused1
        nullptr, // saveDataToFileInDownloadsFolder
        nullptr, // shouldInterruptJavaScript_unavailable
        nullptr, // createNewPage_deprecatedForUseWithV1
        nullptr, // mouseDidMoveOverElement
        // decidePolicyForNotificationPermissionRequest
        [](WKPageRef page, WKSecurityOriginRef origin, WKNotificationPermissionRequestRef permissionRequest, const void* clientInfo) {
            WKNotificationPermissionRequestAllow(permissionRequest);
        },
        nullptr, // unavailablePluginButtonClicked_deprecatedForUseWithV1
        nullptr, // showColorPicker
        nullptr, // hideColorPicker
        nullptr, // unavailablePluginButtonClicked
        nullptr, // pinnedStateDidChange
        nullptr, // didBeginTrackingPotentialLongMousePress
        nullptr, // didRecognizeLongMousePress
        nullptr, // didCancelTrackingPotentialLongMousePress
        nullptr, // isPlayingAudioDidChange
        // decidePolicyForUserMediaPermissionRequest
        [](WKPageRef, WKFrameRef, WKSecurityOriginRef, WKSecurityOriginRef, WKUserMediaPermissionRequestRef permission, const void*) {
            auto audioDevices = WKUserMediaPermissionRequestAudioDeviceUIDs(permission);
            auto videoDevices = WKUserMediaPermissionRequestVideoDeviceUIDs(permission);
            auto audioDevice = WKStringCreateWithUTF8CString("NO AUDIO DEVICE FOUND");
            if (WKArrayGetSize(audioDevices) > 0)
                audioDevice = static_cast<WKStringRef>(WKArrayGetItemAtIndex(audioDevices, 0));
            auto videoDevice = WKStringCreateWithUTF8CString("NO VIDEO DEVICE FOUND");
            if (WKArrayGetSize(videoDevices) > 0)
                videoDevice = static_cast<WKStringRef>(WKArrayGetItemAtIndex(videoDevices, 0));
            WKUserMediaPermissionRequestAllow(permission, audioDevice, videoDevice);
        },
        nullptr, // didClickAutoFillButton
        nullptr, // runJavaScriptAlert
        nullptr, // runJavaScriptConfirm
        nullptr, // runJavaScriptPrompt
        nullptr, // mediaSessionMetadataDidChange
        nullptr, // createNewPage
        nullptr, // runJavaScriptAlert
        nullptr, // runJavaScriptConfirm
        nullptr, // runJavaScriptPrompt
        nullptr, // checkUserMediaPermissionForOrigin
        nullptr, // runBeforeUnloadConfirmPanel
        nullptr, // fullscreenMayReturnToInline
        // willAddDetailedMessageToConsole
        [](WKPageRef, WKStringRef source, WKStringRef, uint64_t line, uint64_t column, WKStringRef message, WKStringRef, const void* clientInfo) {
            TRACE_GLOBAL(BrowserConsoleLog, (message, line, column));
        },
    };

    WKNotificationProviderV0 _handlerNotificationProvider = {
        { 0, nullptr },
        // show
        onNotificationShow,
        nullptr, // cancel
        nullptr, // didDestroyNotification
        nullptr, // addNotificationManager
        nullptr, // removeNotificationManager
        nullptr, // notificationPermissions
        nullptr, // clearNotifications
    };

    WKViewClientV0 _viewClient = {
        { 0, nullptr },
        // frameDisplayed
        onFrameDisplayed,
    };

    WKContextAutomationClientV0 _handlerAutomation = {
        { 0, nullptr },
        // allowsRemoteAutomation
        [](WKContextRef, const void*) -> bool {
            return true;
        },
        didRequestAutomationSession,
        // browserName
        [](WKContextRef, const void*) -> WKStringRef {
            return WKStringCreateWithUTF8CString("WPEWebKitBrowser");
        },
        // browserVersion
        [](WKContextRef, const void*) -> WKStringRef {
            return WKStringCreateWithUTF8CString("1.0");
        }
    };

    WKWebAutomationsessionClientV0 _handlerAutomationSession = {
        { 0, nullptr },
        // requestNewPage
        onAutomationSessionRequestNewPage
    };

    static string WKStringToString(WKStringRef wkStringRef) {
        size_t bufferSize = WKStringGetMaximumUTF8CStringSize(wkStringRef);
        std::unique_ptr<char[]> buffer(new char[bufferSize]);
        size_t stringLength = WKStringGetUTF8CString(wkStringRef, buffer.get(), bufferSize);
        return Core::ToString(buffer.get(), stringLength - 1);
    }
    static std::vector<string> ConvertWKArrayToStringVector(WKArrayRef array) {
            size_t arraySize = WKArrayGetSize(array);

            std::vector<string> stringVector;

            stringVector.reserve(arraySize);
            for (unsigned int index = 0; index < arraySize; ++index) {
                stringVector.emplace_back(WKStringToString(static_cast<WKStringRef>(WKArrayGetItemAtIndex(array, index))));
            }

            return stringVector;
    }

    /* ---------------------------------------------------------------------------------------------------
struct CustomLoopHandler
{
	GSource source;
	uint32_t attentionPending;
};
static gboolean source_prepare(GSource*, gint*)
{
	return (false);
}
static gboolean source_check(GSource* mySource)
{
	return (static_cast<CustomLoopHandler*>(mySource)->attentionPending != 0);
}
static gboolean source_dispatch (GSource*, GSourceFunc callback, gpointer)
{
	uint32_t attention (static_cast<CustomLoopHandler*>(mySource)->attentionPending);

}
static GSourceFuncs _handlerIntervention =
{
	source_prepare,
	source_check,
	source_dispatch,
	nullptr
};
--------------------------------------------------------------------------------------------------- */
#endif

    static Exchange::IBrowser* implementation = nullptr;

    static void CloseDown()
    {
        // Seems we are destructed.....If we still have a pointer to the implementation, Kill it..
        if (implementation != nullptr) {
            delete implementation;
            implementation = nullptr;
        }
    }

    class WebKitImplementation : public Core::Thread, public Exchange::IBrowser, public PluginHost::IStateControl {
    public:
        class BundleConfig : public Core::JSON::Container {
        private:
            using BundleConfigMap = std::map<string, Core::JSON::String>;

        public:
            using Iterator = Core::IteratorMapType<const BundleConfigMap, const Core::JSON::String&, const string&, BundleConfigMap::const_iterator>;

            BundleConfig(const BundleConfig&) = delete;
            BundleConfig& operator=(const BundleConfig&) = delete;

            BundleConfig() 
                : _configs()
            {
            }
            virtual ~BundleConfig()
            {
            }

            inline bool Config(const string& index, string& value) const
            {
                BundleConfigMap::const_iterator position(_configs.find(index));
                bool result = (position != _configs.cend());

                if (result == true) {
                    value = position->second.Value();
                }

                return (result);
            }

        private:
            virtual bool Request(const TCHAR label[])
            {
                if (_configs.find(label) == _configs.end()) {
                    auto element = _configs.emplace(std::piecewise_construct,
                        std::forward_as_tuple(label),
                        std::forward_as_tuple());
                    Add(element.first->first.c_str(), &(element.first->second));
                }
                return (true);
            }

        private:
            BundleConfigMap _configs;
        };
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            class JavaScriptSettings : public Core::JSON::Container {
            public:
                JavaScriptSettings(const JavaScriptSettings&) = delete;
                JavaScriptSettings& operator=(const JavaScriptSettings&) = delete;

                JavaScriptSettings()
                    : Core::JSON::Container()
                    , UseLLInt(true)
                    , UseJIT(true)
                    , UseDFG(true)
                    , UseFTL(true)
                    , UseDOM(true)
                    , DumpOptions(_T("0"))
                {
                    Add(_T("useLLInt"), &UseLLInt);
                    Add(_T("useJIT"), &UseJIT);
                    Add(_T("useDFG"), &UseDFG);
                    Add(_T("useFTL"), &UseFTL);
                    Add(_T("useDOM"), &UseDOM);
                    Add(_T("dumpOptions"), &DumpOptions);
                }
                ~JavaScriptSettings()
                {
                }

            public:
                Core::JSON::Boolean UseLLInt;
                Core::JSON::Boolean UseJIT;
                Core::JSON::Boolean UseDFG;
                Core::JSON::Boolean UseFTL;
                Core::JSON::Boolean UseDOM;
                Core::JSON::String DumpOptions;
            };

        public:
            Config()
                : Core::JSON::Container()
                , UserAgent()
                , URL(_T("http://www.google.com"))
                , Whitelist()
                , PageGroup(_T("WPEPageGroup"))
                , CookieStorage()
                , LocalStorage()
                , Secure(false)
                , InjectedBundle()
                , Transparent(false)
                , Compositor()
                , Inspector()
                , FPS(false)
                , Cursor(false)
                , Touch(false)
                , MSEBuffers()
                , MemoryProfile()
                , MemoryPressure()
                , MediaContentTypesRequiringHardwareSupport()
                , MediaDiskCache(true)
                , DiskCache()
                , XHRCache(false)
                , Languages()
                , CertificateCheck(true)
                , ClientIdentifier()
                , AllowWindowClose(false)
                , NonCompositedWebGLEnabled(false)
                , EnvironmentOverride(false)
                , Automation(false)
                , WebGLEnabled(true)
                , ThreadedPainting()
                , Width(1280)
                , Height(720)
                , PTSOffset(0)
                , ScaleFactor(1.0)
                , MaxFPS(60)
                , ExecPath()
                , HTTPProxy()
                , HTTPProxyExclusion()
            {
                Add(_T("useragent"), &UserAgent);
                Add(_T("url"), &URL);
                Add(_T("whitelist"), &Whitelist);
                Add(_T("pagegroup"), &PageGroup);
                Add(_T("cookiestorage"), &CookieStorage);
                Add(_T("localstorage"), &LocalStorage);
                Add(_T("secure"), &Secure);
                Add(_T("injectedbundle"), &InjectedBundle);
                Add(_T("transparent"), &Transparent);
                Add(_T("compositor"), &Compositor);
                Add(_T("inspector"), &Inspector);
                Add(_T("fps"), &FPS);
                Add(_T("cursor"), &Cursor);
                Add(_T("touch"), &Touch);
                Add(_T("msebuffers"), &MSEBuffers);
                Add(_T("memoryprofile"), &MemoryProfile);
                Add(_T("memorypressure"), &MemoryPressure);
                Add(_T("mediacontenttypesrequiringhardwaresupport"), &MediaContentTypesRequiringHardwareSupport);
                Add(_T("mediadiskcache"), &MediaDiskCache);
                Add(_T("diskcache"), &DiskCache);
                Add(_T("xhrcache"), &XHRCache);
                Add(_T("languages"), &Languages);
                Add(_T("certificatecheck"), &CertificateCheck);
                Add(_T("javascript"), &JavaScript);
                Add(_T("clientidentifier"), &ClientIdentifier);
                Add(_T("windowclose"), &AllowWindowClose);
                Add(_T("noncompositedwebgl"), &NonCompositedWebGLEnabled);
                Add(_T("environmentoverride"), &EnvironmentOverride);
                Add(_T("automation"), &Automation);
                Add(_T("webgl"), &WebGLEnabled);
                Add(_T("threadedpainting"), &ThreadedPainting);
                Add(_T("width"), &Width);
                Add(_T("height"), &Height);
                Add(_T("ptsoffset"), &PTSOffset);
                Add(_T("scalefactor"), &ScaleFactor);
                Add(_T("maxfps"), &MaxFPS);
                Add(_T("bundle"), &Bundle);
                Add(_T("execpath"), &ExecPath);
                Add(_T("proxy"), &HTTPProxy);
                Add(_T("proxyexclusion"), &HTTPProxyExclusion);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String UserAgent;
            Core::JSON::String URL;
            Core::JSON::String Whitelist;
            Core::JSON::String PageGroup;
            Core::JSON::String CookieStorage;
            Core::JSON::String LocalStorage;
            Core::JSON::Boolean Secure;
            Core::JSON::String InjectedBundle;
            Core::JSON::Boolean Transparent;
            Core::JSON::String Compositor;
            Core::JSON::String Inspector;
            Core::JSON::Boolean FPS;
            Core::JSON::Boolean Cursor;
            Core::JSON::Boolean Touch;
            Core::JSON::String MSEBuffers;
            Core::JSON::String MemoryProfile;
            Core::JSON::String MemoryPressure;
            Core::JSON::String MediaContentTypesRequiringHardwareSupport;
            Core::JSON::Boolean MediaDiskCache;
            Core::JSON::String DiskCache;
            Core::JSON::Boolean XHRCache;
            Core::JSON::ArrayType<Core::JSON::String> Languages;
            Core::JSON::Boolean CertificateCheck;
            JavaScriptSettings JavaScript;
            Core::JSON::String ClientIdentifier;
            Core::JSON::Boolean AllowWindowClose;
            Core::JSON::Boolean NonCompositedWebGLEnabled;
            Core::JSON::Boolean EnvironmentOverride;
            Core::JSON::Boolean Automation;
            Core::JSON::Boolean WebGLEnabled;
            Core::JSON::String ThreadedPainting;
            Core::JSON::DecUInt16 Width;
            Core::JSON::DecUInt16 Height;
            Core::JSON::DecSInt16 PTSOffset;
            Core::JSON::DecUInt16 ScaleFactor;
            Core::JSON::DecUInt8 MaxFPS; // A value between 1 and 100...
            BundleConfig Bundle;
            Core::JSON::String ExecPath;
            Core::JSON::String HTTPProxy;
            Core::JSON::String HTTPProxyExclusion;
        };

    private:
        WebKitImplementation(const WebKitImplementation&) = delete;
        WebKitImplementation& operator=(const WebKitImplementation&) = delete;

    public:
        WebKitImplementation()
            : Core::Thread(0, _T("WebKitBrowser"))
            , _config()
            , _URL()
            , _dataPath()
            , _view()
#ifdef WEBKIT_GLIB_API
            , _guid(Core::Time::Now().Ticks())
#else
            , _page()
            , _automationSession(nullptr)
#endif
            , _adminLock()
            , _fps(0)
            , _loop(nullptr)
            , _context(nullptr)
            , _state(PluginHost::IStateControl::UNINITIALIZED)
            , _hidden(false)
            , _time(0)
            , _compliant(false)
        {
            // Register an @Exit, in case we are killed, with an incorrect ref count !!
            if (atexit(CloseDown) != 0) {
                TRACE_L1("Could not register @exit handler. Error: %d.", errno);
                exit(EXIT_FAILURE);
            }

            // The WebKitBrowser (WPE) can only be instantiated once (it is a process wide singleton !!!!)
            ASSERT(implementation == nullptr);

            implementation = this;
        }
        virtual ~WebKitImplementation()
        {
            Block();

            if (_loop != nullptr)
                g_main_loop_quit(_loop);

            if (Wait(Core::Thread::STOPPED | Core::Thread::BLOCKED, 6000) == false)
                TRACE_L1("Bailed out before the end of the WPE main app was reached. %d", 6000);

            implementation = nullptr;
        }

    public:
        virtual void SetURL(const string& URL)
        {
            _URL = URL;

            TRACE(Trace::Information, (_T("New URL: %s"), _URL.c_str()));

            if (_context != nullptr) {
                g_main_context_invoke(
                    _context,
                    [](gpointer customdata) -> gboolean {
                        WebKitImplementation* object = static_cast<WebKitImplementation*>(customdata);
#ifdef WEBKIT_GLIB_API
                        webkit_web_view_load_uri(object->_view, object->_URL.c_str());
#else
                        auto shellURL = WKURLCreateWithUTF8CString(object->_URL.c_str());
                        WKPageLoadURL(object->_page, shellURL);
                        WKRelease(shellURL);
#endif
                        return FALSE;
                    },
                    this);
            }
        }
        virtual string GetURL() const
        {
            return _URL;
        }
        virtual uint32_t GetFPS() const
        {
            return _fps;
        }
        virtual PluginHost::IStateControl::state State() const
        {
            return (_state);
        }
        virtual uint32_t Request(PluginHost::IStateControl::command command)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            _adminLock.Lock();

            if (_state == PluginHost::IStateControl::UNINITIALIZED) {
                // Seems we are passing state changes before we reached an operational browser.
                // Just move the state to what we would like it to be :-)
                _state = (command == PluginHost::IStateControl::SUSPEND ? PluginHost::IStateControl::SUSPENDED : PluginHost::IStateControl::RESUMED);
                result = Core::ERROR_NONE;
            } else {
                switch (command) {
                case PluginHost::IStateControl::SUSPEND:
                    if (_state == PluginHost::IStateControl::RESUMED) {
                        Suspend();
                        result = Core::ERROR_NONE;
                    }
                    break;
                case PluginHost::IStateControl::RESUME:
                    if (_state == PluginHost::IStateControl::SUSPENDED) {
                        Resume();
                        result = Core::ERROR_NONE;
                    }
                    break;
                default:
                    break;
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        virtual void Register(PluginHost::IStateControl::INotification* sink)
        {
            _adminLock.Lock();

            // Make sure a sink is not registered multiple times.
            ASSERT(std::find(_stateControlClients.begin(), _stateControlClients.end(), sink) == _stateControlClients.end());

            _stateControlClients.push_back(sink);
            sink->AddRef();

            _adminLock.Unlock();

            TRACE_L1("Registered a sink on the browser %p", sink);
        }
        virtual void Unregister(PluginHost::IStateControl::INotification* sink)
        {
            _adminLock.Lock();

            std::list<PluginHost::IStateControl::INotification*>::iterator index(std::find(_stateControlClients.begin(), _stateControlClients.end(), sink));

            // Make sure you do not unregister something you did not register !!!
            ASSERT(index != _stateControlClients.end());

            if (index != _stateControlClients.end()) {
                (*index)->Release();
                _stateControlClients.erase(index);
                TRACE_L1("Unregistered a sink on the browser %p", sink);
            }

            _adminLock.Unlock();
        }
        virtual void Hide(const bool hidden)
        {
            if (hidden == true) {
                Hide();
            } else {
                Show();
            }
        }
        virtual void Register(Exchange::IBrowser::INotification* sink)
        {
            _adminLock.Lock();

            // Make sure a sink is not registered multiple times.
            ASSERT(std::find(_notificationClients.begin(), _notificationClients.end(), sink) == _notificationClients.end());

            _notificationClients.push_back(sink);
            sink->AddRef();

            _adminLock.Unlock();

            TRACE_L1("Registered a sink on the browser %p", sink);
        }

        virtual void Unregister(Exchange::IBrowser::INotification* sink)
        {
            _adminLock.Lock();

            std::list<Exchange::IBrowser::INotification*>::iterator index(std::find(_notificationClients.begin(), _notificationClients.end(), sink));

            // Make sure you do not unregister something you did not register !!!
            ASSERT(index != _notificationClients.end());

            if (index != _notificationClients.end()) {
                (*index)->Release();
                _notificationClients.erase(index);
                TRACE_L1("Unregistered a sink on the browser %p", sink);
            }

            _adminLock.Unlock();
        }

        void OnURLChanged(const string& URL)
        {
            _adminLock.Lock();

            _URL = URL;

            std::list<Exchange::IBrowser::INotification*>::iterator index(_notificationClients.begin());

            while (index != _notificationClients.end()) {
                (*index)->URLChanged(URL);
                index++;
            }

            _adminLock.Unlock();
        }
#ifdef WEBKIT_GLIB_API
        void OnLoadFinished()
        {
            string URL = Core::ToString(webkit_web_view_get_uri(_view));
            OnLoadFinished(URL);
        }
#endif
        void OnLoadFinished(const string& URL)
        {
            _adminLock.Lock();

            _URL = URL;

            std::list<Exchange::IBrowser::INotification*>::iterator index(_notificationClients.begin());

            while (index != _notificationClients.end()) {
                (*index)->LoadFinished(URL);
                index++;
            }

            _adminLock.Unlock();
        }
        void OnStateChange(const PluginHost::IStateControl::state newState)
        {
            _adminLock.Lock();

            if (_state != newState) {
                _state = newState;

                std::list<PluginHost::IStateControl::INotification*>::iterator index(_stateControlClients.begin());

                while (index != _stateControlClients.end()) {
                    (*index)->StateChange(newState);
                    index++;
                }
            }

            _adminLock.Unlock();
        }
        void Hidden(const bool hidden)
        {
            _adminLock.Lock();

            if (hidden != _hidden) {
                _hidden = hidden;

                std::list<Exchange::IBrowser::INotification*>::iterator index(_notificationClients.begin());

                while (index != _notificationClients.end()) {
                    (*index)->Hidden(hidden);
                    index++;
                }
            }

            _adminLock.Unlock();
        }
        void OnJavaScript(const std::vector<string>& text) const
        {
            for (const string& line : text) {
                std::cout << "  " << line << std::endl;
            }
        }
        virtual uint32_t Configure(PluginHost::IShell* service)
        {
            _dataPath = service->DataPath();
            _config.FromString(service->ConfigLine());

            bool environmentOverride(WebKitBrowser::EnvironmentOverride(_config.EnvironmentOverride.Value()));

            if ((environmentOverride == false) || (Core::SystemInfo::GetEnvironment(_T("WPE_WEBKIT_URL"), _URL) == false)) {
                _URL = _config.URL.Value();
            }

            Core::SystemInfo::SetEnvironment(_T("QUEUEPLAYER_FLUSH_MODE"), _T("3"), false);
            Core::SystemInfo::SetEnvironment(_T("HOME"), service->PersistentPath());

            if (_config.ClientIdentifier.IsSet() == true) {
                string value(service->Callsign() + ',' + _config.ClientIdentifier.Value());
                Core::SystemInfo::SetEnvironment(_T("CLIENT_IDENTIFIER"), value, !environmentOverride);
            } else {
                Core::SystemInfo::SetEnvironment(_T("CLIENT_IDENTIFIER"), service->Callsign(), !environmentOverride);
            }

            // Set dummy window for gst-gl
            Core::SystemInfo::SetEnvironment(_T("GST_GL_WINDOW"), _T("dummy"), !environmentOverride);

            // MSE Buffers
            if (_config.MSEBuffers.Value().empty() == false)
                Core::SystemInfo::SetEnvironment(_T("MSE_MAX_BUFFER_SIZE"), _config.MSEBuffers.Value(), !environmentOverride);

            // Memory Pressure
            if (_config.MemoryPressure.Value().empty() == false)
                Core::SystemInfo::SetEnvironment(_T("WPE_POLL_MAX_MEMORY"), _config.MemoryPressure.Value(), !environmentOverride);

            // Memory Profile
            if (_config.MemoryProfile.Value().empty() == false)
                Core::SystemInfo::SetEnvironment(_T("WPE_RAM_SIZE"), _config.MemoryProfile.Value(), !environmentOverride);

            // GStreamer on-disk buffering
            if (_config.MediaDiskCache.Value() == false)
                Core::SystemInfo::SetEnvironment(_T("WPE_SHELL_DISABLE_MEDIA_DISK_CACHE"), _T("1"), !environmentOverride);
            else
                Core::SystemInfo::SetEnvironment(_T("WPE_SHELL_MEDIA_DISK_CACHE_PATH"), service->PersistentPath(), !environmentOverride);

            // Disk Cache
            if (_config.DiskCache.Value().empty() == false)
                Core::SystemInfo::SetEnvironment(_T("WPE_DISK_CACHE_SIZE"), _config.DiskCache.Value(), !environmentOverride);

            if (_config.XHRCache.Value() == false)
                Core::SystemInfo::SetEnvironment(_T("WPE_DISABLE_XHR_RESPONSE_CACHING"), _T("1"), !environmentOverride);

            // Enable cookie persistent storage
            if (_config.CookieStorage.Value().empty() == false)
                Core::SystemInfo::SetEnvironment(_T("WPE_SHELL_COOKIE_STORAGE"), _T("1"), !environmentOverride);

            // Use cairo noaa compositor
            if (_config.Compositor.Value().empty() == false)
                Core::SystemInfo::SetEnvironment(_T("CAIRO_GL_COMPOSITOR"), _config.Compositor.Value(), !environmentOverride);

            // WebInspector
            if (_config.Inspector.Value().empty() == false) {
                if (_config.Automation.Value())
                    Core::SystemInfo::SetEnvironment(_T("WEBKIT_INSPECTOR_SERVER"), _config.Inspector.Value(), !environmentOverride);
                else
                    Core::SystemInfo::SetEnvironment(_T("WEBKIT_LEGACY_INSPECTOR_SERVER"), _config.Inspector.Value(), !environmentOverride);
            }

            // RPI mouse support
            if (_config.Cursor.Value() == true)
                Core::SystemInfo::SetEnvironment(_T("WPE_BCMRPI_CURSOR"), _T("1"), !environmentOverride);

            // RPI touch support
            if (_config.Touch.Value() == true)
                Core::SystemInfo::SetEnvironment(_T("WPE_BCMRPI_TOUCH"), _T("1"), !environmentOverride);

            // WPE allows the LLINT to be used if true
            if (_config.JavaScript.UseLLInt.Value() == false) {
                Core::SystemInfo::SetEnvironment(_T("JSC_useLLInt"), _T("false"), !environmentOverride);
            }

            // WPE allows the baseline JIT to be used if true
            if (_config.JavaScript.UseJIT.Value() == false) {
                Core::SystemInfo::SetEnvironment(_T("JSC_useJIT"), _T("false"), !environmentOverride);
            }

            // WPE allows the DFG JIT to be used if true
            if (_config.JavaScript.UseDFG.Value() == false) {
                Core::SystemInfo::SetEnvironment(_T("JSC_useDFGJIT"), _T("false"), !environmentOverride);
            }

            // WPE allows the FTL JIT to be used if true
            if (_config.JavaScript.UseFTL.Value() == false) {
                Core::SystemInfo::SetEnvironment(_T("JSC_useFTLJIT"), _T("false"), !environmentOverride);
            }

            // WPE allows the DOM JIT to be used if true
            if (_config.JavaScript.UseDOM.Value() == false) {
                Core::SystemInfo::SetEnvironment(_T("JSC_useDOMJIT"), _T("false"), !environmentOverride);
            }

            // WPE DumpOptions
            if (_config.JavaScript.DumpOptions.Value().empty() == false) {
                Core::SystemInfo::SetEnvironment(_T("JSC_dumpOptions"), _config.JavaScript.DumpOptions.Value(), !environmentOverride);
            }

            // ThreadedPainting
            if (_config.ThreadedPainting.Value().empty() == false) {
                Core::SystemInfo::SetEnvironment(_T("WEBKIT_NICOSIA_PAINTING_THREADS"), _config.ThreadedPainting.Value(), !environmentOverride);
            }

            // PTSOffset
            if (_config.PTSOffset.IsSet() == true) {
                string ptsoffset(Core::NumberType<int16_t>(_config.PTSOffset.Value()).Text());
                Core::SystemInfo::SetEnvironment(_T("PTS_REPORTING_OFFSET_MS"), ptsoffset, !environmentOverride);
            }

            // ExecPath
            if (_config.ExecPath.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("WEBKIT_EXEC_PATH"), _config.ExecPath.Value(), !environmentOverride);
            }

            //  HTTPProxy
            if (_config.HTTPProxy.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("http_proxy"), _config.HTTPProxy.Value(), !environmentOverride);
            }

            // HTTPProxyExclusion
            if (_config.HTTPProxyExclusion.IsSet() == true) {
                Core::SystemInfo::SetEnvironment(_T("no_proxy"), _config.HTTPProxyExclusion.Value(), !environmentOverride);
            }

            string width(Core::NumberType<uint16_t>(_config.Width.Value()).Text());
            string height(Core::NumberType<uint16_t>(_config.Height.Value()).Text());
            string maxFPS(Core::NumberType<uint16_t>(_config.MaxFPS.Value()).Text());
            Core::SystemInfo::SetEnvironment(_T("WEBKIT_RESOLUTION_WIDTH"), width, !environmentOverride);
            Core::SystemInfo::SetEnvironment(_T("WEBKIT_RESOLUTION_HEIGHT"), height, !environmentOverride);
            Core::SystemInfo::SetEnvironment(_T("WEBKIT_MAXIMUM_FPS"), maxFPS, !environmentOverride);

            if (width.empty() == false) {
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_DISP_WIDTH"), width, !environmentOverride);
            }

            if (height.empty() == false) {
                Core::SystemInfo::SetEnvironment(_T("GST_VIRTUAL_DISP_HEIGHT"), height, !environmentOverride);
            }

            // Oke, so we are good to go.. Release....
            Core::Thread::Run();

            return (Core::ERROR_NONE);
        }

        void NotifyClosure()
        {
            _adminLock.Lock();

            std::list<Exchange::IBrowser::INotification*>::iterator index(_notificationClients.begin());

            while (index != _notificationClients.end()) {
                (*index)->Closure();
                index++;
            }

            _adminLock.Unlock();
        }

        void SetFPS(const uint32_t fps)
        {
            _fps = fps;
        }

        string GetConfig(const string& key) const
        {
            string value;
            _config.Bundle.Config(key,value);
            return (value);
        }
#ifndef WEBKIT_GLIB_API
        void OnNotificationShown(uint64_t notificationID) const
        {
            WKNotificationManagerProviderDidShowNotification(_notificationManager, notificationID);
        }

        void OnRequestAutomationSession(WKContextRef context, WKStringRef sessionID)
        {
            _automationSession = WKWebAutomationSessionCreate(sessionID);
            _handlerAutomationSession.base.clientInfo = static_cast<void*>(this);
            WKWebAutomationSessionSetClient(_automationSession, &_handlerAutomationSession.base);
            WKContextSetAutomationSession(context, _automationSession);
        }

        WKPageRef GetPage() const
        {
            return _page;
        }
#endif
        BEGIN_INTERFACE_MAP(WebKitImplementation)
        INTERFACE_ENTRY(Exchange::IBrowser)
        INTERFACE_ENTRY(PluginHost::IStateControl)
        END_INTERFACE_MAP

    private:
        void Hide()
        {
            if (_context != nullptr) {
                _time = Core::Time::Now().Ticks();
                g_main_context_invoke(
                    _context,
                    [](gpointer customdata) -> gboolean {
                        WebKitImplementation* object = static_cast<WebKitImplementation*>(customdata);
#ifdef WEBKIT_GLIB_API
                        webkit_web_view_hide(object->_view);
#else
                        WKViewSetViewState(object->_view, (object->_state == PluginHost::IStateControl::RESUMED ? kWKViewStateIsInWindow : 0));
#endif
                        object->Hidden(true);
                        TRACE_L1("Internal Hide Notification took %d mS.", static_cast<uint32_t>(Core::Time::Now().Ticks() - object->_time));

                        return FALSE;
                    },
                    this);
            }
        }
        void Show()
        {
            if (_context != nullptr) {
                _time = Core::Time::Now().Ticks();
                g_main_context_invoke(
                    _context,
                    [](gpointer customdata) -> gboolean {
                        WebKitImplementation* object = static_cast<WebKitImplementation*>(customdata);
#ifdef WEBKIT_GLIB_API
                        webkit_web_view_show(object->_view);
#else
                        WKViewSetViewState(object->_view, (object->_state == PluginHost::IStateControl::RESUMED ? kWKViewStateIsInWindow : 0) | kWKViewStateIsVisible);
#endif
                        object->Hidden(false);

                        TRACE_L1("Internal Show Notification took %d mS.", static_cast<uint32_t>(Core::Time::Now().Ticks() - object->_time));

                        return FALSE;
                    },
                    this);
            }
        }
        void Suspend()
        {
            if (_context == nullptr) {
                _state = PluginHost::IStateControl::SUSPENDED;
            } else {
                _time = Core::Time::Now().Ticks();
                g_main_context_invoke(
                    _context,
                    [](gpointer customdata) -> gboolean {
                        WebKitImplementation* object = static_cast<WebKitImplementation*>(customdata);
#ifdef WEBKIT_GLIB_API
                        webkit_web_view_suspend(object->_view);
#else
                        WKViewSetViewState(object->_view, (object->_hidden ? 0 : kWKViewStateIsVisible));
#endif
                        object->OnStateChange(PluginHost::IStateControl::SUSPENDED);

                        TRACE_L1("Internal Suspend Notification took %d mS.", static_cast<uint32_t>(Core::Time::Now().Ticks() - object->_time));

                        return FALSE;
                    },
                    this);
            }
        }
        void Resume()
        {
            if (_context == nullptr) {
                _state = PluginHost::IStateControl::RESUMED;
            } else {
                _time = Core::Time::Now().Ticks();

                g_main_context_invoke(
                    _context,
                    [](gpointer customdata) -> gboolean {
                        WebKitImplementation* object = static_cast<WebKitImplementation*>(customdata);
#ifdef WEBKIT_GLIB_API
                        webkit_web_view_resume(object->_view);
#else
                        WKViewSetViewState(object->_view, (object->_hidden ? 0 : kWKViewStateIsVisible) | kWKViewStateIsInWindow);
#endif
                        object->OnStateChange(PluginHost::IStateControl::RESUMED);

                        TRACE_L1("Internal Resume Notification took %d mS.", static_cast<uint32_t>(Core::Time::Now().Ticks() - object->_time));

                        return FALSE;
                    },
                    this);
            }
        }
#ifdef WEBKIT_GLIB_API
        static void initializeWebExtensionsCallback(WebKitWebContext* context, WebKitImplementation* browser)
        {
            webkit_web_context_set_web_extensions_directory(context, browser->_dataPath.c_str());
            // FIX it
            GVariant* data = g_variant_new("(sms)", std::to_string(browser->_guid).c_str(), !browser->_config.Whitelist.Value().empty() ? browser->_config.Whitelist.Value().c_str() : nullptr);
            webkit_web_context_set_web_extensions_initialization_user_data(context, data);
        }
        static void wpeNotifyWPEFrameworkMessageReceivedCallback(WebKitUserContentManager*, WebKitJavascriptResult* message, WebKitImplementation* browser)
        {
            JSCValue* args = webkit_javascript_result_get_js_value(message);
            JSCValue* arrayLengthValue = jsc_value_object_get_property(args, "length");
            int arrayLength = jsc_value_to_int32(arrayLengthValue);
            g_object_unref(arrayLengthValue);

            std::vector<string> messageStrings;
            for (int i = 0; i < arrayLength; ++i) {
                JSCValue* itemValue = jsc_value_object_get_property_at_index(args, i);
                char* itemStr = jsc_value_to_string(itemValue);
                g_object_unref(itemValue);
                messageStrings.push_back(Core::ToString(itemStr));
                g_free(itemStr);
            }

            browser->OnJavaScript(messageStrings);
        }
        static gboolean decidePolicyCallback(WebKitWebView*, WebKitPolicyDecision* decision, WebKitPolicyDecisionType)
        {
            webkit_policy_decision_use(decision);
            return TRUE;
        }
        static void uriChangedCallback(WebKitWebView* webView, GParamSpec*, WebKitImplementation* browser)
        {
            browser->OnURLChanged(Core::ToString(webkit_web_view_get_uri(webView)));
        }
        static void loadChangedCallback(WebKitWebView* webView, WebKitLoadEvent loadEvent, WebKitImplementation* browser)
        {
            if (loadEvent == WEBKIT_LOAD_FINISHED)
                browser->OnLoadFinished();
        }
        static void webProcessTerminatedCallback(WebKitWebView* webView, WebKitWebProcessTerminationReason reason)
        {
            switch (reason) {
            case WEBKIT_WEB_PROCESS_CRASHED:
                SYSLOG(Trace::Fatal, ("CRASH: WebProcess crashed: exiting ..."));
                break;
            case WEBKIT_WEB_PROCESS_EXCEEDED_MEMORY_LIMIT:
                SYSLOG(Trace::Fatal, ("CRASH: WebProcess terminated due to memory limit: exiting ..."));
                break;
            }
            exit(1);
        }
        static void closeCallback(WebKitWebView*, WebKitImplementation* browser)
        {
            browser->NotifyClosure();
        }
        static gboolean decidePermissionCallback(WebKitWebView*, WebKitPermissionRequest* permissionRequest)
        {
            webkit_permission_request_allow(permissionRequest);
            return TRUE;
        }
        static gboolean showNotificationCallback(WebKitWebView*, WebKitNotification* notification, WebKitImplementation* browser)
        {
            TRACE_GLOBAL(HTML5Notification, (_T("%s - %s"), webkit_notification_get_title(notification), webkit_notification_get_body(notification)));
            return FALSE;
        }
        static WebKitWebView* createWebViewForAutomationCallback(WebKitAutomationSession* session, WebKitImplementation* browser)
        {
            return browser->_view;
        }
        static void automationStartedCallback(WebKitWebContext* context, WebKitAutomationSession* session, WebKitImplementation* browser)
        {
            WebKitApplicationInfo *info = webkit_application_info_new();
            webkit_application_info_set_name(info, "WPEWebKitBrowser");
            webkit_application_info_set_version(info, 1, 0, 0);
            webkit_automation_session_set_application_info(session, info);
            webkit_application_info_unref(info);

            g_signal_connect(session, "create-web-view", reinterpret_cast<GCallback>(createWebViewForAutomationCallback), browser);
        }
        virtual uint32_t Worker()
        {
            _context = g_main_context_new();
            _loop = g_main_loop_new(_context, FALSE);
            g_main_context_push_thread_default(_context);

            bool automationEnabled = _config.Automation.Value();

            WebKitWebContext* context;
            if (automationEnabled) {
                context = webkit_web_context_new_ephemeral();
                webkit_web_context_set_automation_allowed(context, TRUE);
                g_signal_connect(context, "automation-started", reinterpret_cast<GCallback>(automationStartedCallback), this);
            } else {
                gchar* wpeStoragePath;
                if (_config.LocalStorage.IsSet() == true && _config.LocalStorage.Value().empty() == false)
                    wpeStoragePath = g_build_filename(_config.LocalStorage.Value().c_str(), "wpe", "local-storage", nullptr);
                else
                    wpeStoragePath = g_build_filename(g_get_user_cache_dir(), "wpe", "local-storage", nullptr);
                g_mkdir_with_parents(wpeStoragePath, 0700);

                gchar* wpeDiskCachePath = g_build_filename(g_get_user_cache_dir(), "wpe", "disk-cache", nullptr);
                g_mkdir_with_parents(wpeDiskCachePath, 0700);

                auto* websiteDataManager = webkit_website_data_manager_new("local-storage-directory", wpeStoragePath, "disk-cache-directory", wpeDiskCachePath, nullptr);
                g_free(wpeStoragePath);
                g_free(wpeDiskCachePath);

                context = webkit_web_context_new_with_website_data_manager(websiteDataManager);
                g_object_unref(websiteDataManager);
            }

            if (_config.InjectedBundle.Value().empty() == false) {
                // Set up injected bundle. Will be loaded once WPEWebProcess is started.
                g_signal_connect(context, "initialize-web-extensions", G_CALLBACK(initializeWebExtensionsCallback), this);
            }

            if (!webkit_web_context_is_ephemeral(context)) {
                gchar* cookieDatabasePath;
                if (_config.CookieStorage.IsSet() == true && _config.CookieStorage.Value().empty() == false)
                    cookieDatabasePath = g_build_filename(_config.CookieStorage.Value().c_str(), "cookies.db", nullptr);
                else
                    cookieDatabasePath = g_build_filename(g_get_user_cache_dir(), "cookies.db", nullptr);

                auto* cookieManager = webkit_web_context_get_cookie_manager(context);
                webkit_cookie_manager_set_persistent_storage(cookieManager, cookieDatabasePath, WEBKIT_COOKIE_PERSISTENT_STORAGE_SQLITE);
            }

            if (!_config.CertificateCheck)
                webkit_web_context_set_tls_errors_policy(context, WEBKIT_TLS_ERRORS_POLICY_IGNORE);

            auto* languages = static_cast<char**>(g_new0(char*, _config.Languages.Length() + 1));
            Core::JSON::ArrayType<Core::JSON::String>::Iterator index(_config.Languages.Elements());

            for (unsigned i = 0; index.Next(); ++i)
                languages[i] = g_strdup(index.Current().Value().c_str());

            webkit_web_context_set_preferred_languages(context, languages);
            g_strfreev(languages);

            auto* preferences = webkit_settings_new();

            webkit_settings_set_enable_encrypted_media(preferences, TRUE);
            webkit_settings_set_enable_mediasource(preferences, TRUE);

            // Turn on/off WebGL
            webkit_settings_set_enable_webgl(preferences, _config.WebGLEnabled.Value());

            webkit_settings_set_enable_non_composited_webgl(preferences, _config.NonCompositedWebGLEnabled.Value());

            // Media Content Types Requiring Hardware Support
            if (_config.MediaContentTypesRequiringHardwareSupport.IsSet() == true
                && _config.MediaContentTypesRequiringHardwareSupport.Value().empty() == false) {
                webkit_settings_set_media_content_types_requiring_hardware_support(preferences,
                    _config.MediaContentTypesRequiringHardwareSupport.Value().c_str());
            }

            if (_config.UserAgent.IsSet() == true && _config.UserAgent.Value().empty() == false)
                webkit_settings_set_user_agent(preferences, _config.UserAgent.Value().c_str());

            _view = WEBKIT_WEB_VIEW(g_object_new(WEBKIT_TYPE_WEB_VIEW,
                "backend", webkit_web_view_backend_new(wpe_view_backend_create(), nullptr, nullptr),
                "web-context", context,
                "settings", preferences,
                "is-controlled-by-automation", automationEnabled,
                nullptr));
            g_object_unref(context);
            g_object_unref(preferences);

            unsigned frameDisplayedCallbackID = 0;
            if (_config.FPS.Value() == true) {
                frameDisplayedCallbackID = webkit_web_view_add_frame_displayed_callback(_view, [](WebKitWebView*, gpointer userData) {
                    static unsigned s_frameCount = 0;
                    static gint64 lastDumpTime = g_get_monotonic_time();

                    ++s_frameCount;
                    gint64 time = g_get_monotonic_time();
                    if (time - lastDumpTime >= G_USEC_PER_SEC) {
                        auto* browser = static_cast<WebKitImplementation*>(userData);
                        browser->SetFPS(s_frameCount * G_USEC_PER_SEC * 1.0 / (time - lastDumpTime));
                        s_frameCount = 0;
                        lastDumpTime = time;
                    }
                }, this, nullptr);
            }

            auto* userContentManager = webkit_web_view_get_user_content_manager(_view);
            webkit_user_content_manager_register_script_message_handler_in_world(userContentManager, "wpeNotifyWPEFramework", std::to_string(_guid).c_str());
            g_signal_connect(userContentManager, "script-message-received::wpeNotifyWPEFramework",
                reinterpret_cast<GCallback>(wpeNotifyWPEFrameworkMessageReceivedCallback), this);

            g_signal_connect(_view, "decide-policy", reinterpret_cast<GCallback>(decidePolicyCallback), nullptr);
            g_signal_connect(_view, "notify::uri", reinterpret_cast<GCallback>(uriChangedCallback), this);
            g_signal_connect(_view, "load-changed", reinterpret_cast<GCallback>(loadChangedCallback), this);
            g_signal_connect(_view, "web-process-terminated", reinterpret_cast<GCallback>(webProcessTerminatedCallback), nullptr);
            g_signal_connect(_view, "close", reinterpret_cast<GCallback>(closeCallback), this);
            g_signal_connect(_view, "permission-request", reinterpret_cast<GCallback>(decidePermissionCallback), nullptr);
            g_signal_connect(_view, "show-notification", reinterpret_cast<GCallback>(showNotificationCallback), this);

            SetURL(_URL);

            // Move into the correct state, as requested
            auto* backend = webkit_web_view_backend_get_wpe_backend(webkit_web_view_get_backend(_view));
            _adminLock.Lock();
            if ((_state == PluginHost::IStateControl::SUSPENDED) || (_state == PluginHost::IStateControl::UNINITIALIZED)) {
                _state = PluginHost::IStateControl::UNINITIALIZED;
                wpe_view_backend_add_activity_state(backend, wpe_view_activity_state_visible);
                OnStateChange(PluginHost::IStateControl::SUSPENDED);
            } else {
                _state = PluginHost::IStateControl::UNINITIALIZED;
                wpe_view_backend_add_activity_state(backend, wpe_view_activity_state_visible | wpe_view_activity_state_focused | wpe_view_activity_state_in_window);
                OnStateChange(PluginHost::IStateControl::RESUMED);
            }
            _adminLock.Unlock();

            g_main_loop_run(_loop);

            if (frameDisplayedCallbackID)
                webkit_web_view_remove_frame_displayed_callback(_view, frameDisplayedCallbackID);
            webkit_user_content_manager_unregister_script_message_handler_in_world(userContentManager, "wpeNotifyWPEFramework", std::to_string(_guid).c_str());

            g_clear_object(&_view);
            g_main_context_pop_thread_default(_context);
            g_main_loop_unref(_loop);
            g_main_context_unref(_context);

            return Core::infinite;
        }
#else
        virtual uint32_t Worker()
        {
            _context = g_main_context_new();
            _loop = g_main_loop_new(_context, FALSE);
            g_main_context_push_thread_default(_context);

            auto contextConfiguration = WKContextConfigurationCreate();

            if (_config.InjectedBundle.Value().empty() == false) {
                // Set up injected bundle. Will be loaded once WPEWebProcess is started.
                string injectedBundlePath = _dataPath + _config.InjectedBundle.Value();
                WKStringRef injectedBundlePathString = WKStringCreateWithUTF8CString(injectedBundlePath.c_str());
                WKContextConfigurationSetInjectedBundlePath(contextConfiguration, injectedBundlePathString);
                WKRelease(injectedBundlePathString);
            }

            gchar* wpeStoragePath;
            if (_config.LocalStorage.IsSet() == true && _config.LocalStorage.Value().empty() == false)
                wpeStoragePath = g_build_filename(_config.LocalStorage.Value().c_str(), "wpe", "local-storage", nullptr);
            else
                wpeStoragePath = g_build_filename(g_get_user_cache_dir(), "wpe", "local-storage", nullptr);

            g_mkdir_with_parents(wpeStoragePath, 0700);
            auto storageDirectory = WKStringCreateWithUTF8CString(wpeStoragePath);
            g_free(wpeStoragePath);
            WKContextConfigurationSetLocalStorageDirectory(contextConfiguration, storageDirectory);

            gchar* wpeDiskCachePath = g_build_filename(g_get_user_cache_dir(), "wpe", "disk-cache", nullptr);
            g_mkdir_with_parents(wpeDiskCachePath, 0700);
            auto diskCacheDirectory = WKStringCreateWithUTF8CString(wpeDiskCachePath);
            g_free(wpeDiskCachePath);
            WKContextConfigurationSetDiskCacheDirectory(contextConfiguration, diskCacheDirectory);

            WKContextRef context = WKContextCreateWithConfiguration(contextConfiguration);
            WKSoupSessionSetIgnoreTLSErrors(context, !_config.CertificateCheck);

            WKMutableArrayRef languages = WKMutableArrayCreate();
            Core::JSON::ArrayType<Core::JSON::String>::Iterator index(_config.Languages.Elements());

            while (index.Next() == true) {
                WKStringRef itemString = WKStringCreateWithUTF8CString(index.Current().Value().c_str());
                WKArrayAppendItem(languages, itemString);
                WKRelease(itemString);
            }

            WKSoupSessionSetPreferredLanguages(context, languages);
            WKRelease(contextConfiguration);
            WKRelease(languages);

            WKGeolocationManagerRef geolocationManager = WKContextGetGeolocationManager(context);
            WKGeolocationManagerSetProvider(geolocationManager, &_handlerGeolocationProvider.base);

            _notificationManager = WKContextGetNotificationManager(context);
            _handlerNotificationProvider.base.clientInfo = static_cast<void*>(this);
            WKNotificationManagerSetProvider(_notificationManager, &_handlerNotificationProvider.base);

            auto pageGroupIdentifier = WKStringCreateWithUTF8CString(_config.PageGroup.Value().c_str());
            auto pageGroup = WKPageGroupCreateWithIdentifier(pageGroupIdentifier);
            WKRelease(pageGroupIdentifier);

            auto preferences = WKPreferencesCreate();

            // Allow mixed content.
            bool allowMixedContent = _config.Secure.Value();
            WKPreferencesSetAllowRunningOfInsecureContent(preferences, !allowMixedContent);
            WKPreferencesSetAllowDisplayOfInsecureContent(preferences, !allowMixedContent);

            // WebSecurity
            WKPreferencesSetWebSecurityEnabled(preferences, allowMixedContent);

            // Turn off log message to stdout.
            WKPreferencesSetLogsPageMessagesToSystemConsoleEnabled(preferences, false);

            // Turn on gamepads.
            WKPreferencesSetGamepadsEnabled(preferences, true);

            // Turn on fullscreen API.
            WKPreferencesSetFullScreenEnabled(preferences, true);

            // Turn on/off allowScriptWindowClose
            WKPreferencesSetAllowScriptsToCloseWindow(preferences, _config.AllowWindowClose.Value());

            // Turn on/off non composited WebGL
            WKPreferencesSetNonCompositedWebGLEnabled(preferences, _config.NonCompositedWebGLEnabled.Value());

            //Turn on/off WebGL
            WKPreferencesSetWebGLEnabled(preferences, _config.WebGLEnabled.Value());

            // Media Content Types Requiring Hardware Support
            if (_config.MediaContentTypesRequiringHardwareSupport.IsSet() == true
                && _config.MediaContentTypesRequiringHardwareSupport.Value().empty() == false) {
              auto contentTypes = WKStringCreateWithUTF8CString(
                  _config.MediaContentTypesRequiringHardwareSupport.Value().c_str());
              WKPreferencesSetMediaContentTypesRequiringHardwareSupport(preferences, contentTypes);
              WKRelease(contentTypes);
            }

            WKPageGroupSetPreferences(pageGroup, preferences);

            auto pageConfiguration = WKPageConfigurationCreate();
            WKPageConfigurationSetContext(pageConfiguration, context);
            WKPageConfigurationSetPageGroup(pageConfiguration, pageGroup);

            gchar* cookieDatabasePath;

            if (_config.CookieStorage.IsSet() == true && _config.CookieStorage.Value().empty() == false)
                cookieDatabasePath = g_build_filename(_config.CookieStorage.Value().c_str(), "cookies.db", nullptr);
            else
                cookieDatabasePath = g_build_filename(g_get_user_cache_dir(), "cookies.db", nullptr);

            auto path = WKStringCreateWithUTF8CString(cookieDatabasePath);
            g_free(cookieDatabasePath);
            auto cookieManager = WKContextGetCookieManager(context);
            WKCookieManagerSetCookiePersistentStorage(cookieManager, path, kWKCookieStorageTypeSQLite);

#ifdef WPE_WEBKIT_DEPRECATED_API
            _view = WKViewCreate(pageConfiguration);
#else
            _view = WKViewCreate(wpe_view_backend_create(), pageConfiguration);
#endif
            if (_config.FPS.Value() == true) {
                _viewClient.base.clientInfo = static_cast<void*>(this);
                WKViewSetViewClient(_view, &_viewClient.base);
            }

            //_page = WKRetain(WKViewGetPage(_view));
            _page = WKViewGetPage(_view);

            if (_config.Transparent.Value() == true)
                WKPageSetDrawsBackground(_page, false);

            // Register handlers for page navigation and message from injected bundle.
            _handlerWebKit.base.clientInfo = static_cast<void*>(this);
            WKPageSetPageNavigationClient(_page, &_handlerWebKit.base);

            _handlerInjectedBundle.base.clientInfo = static_cast<void*>(this);
            WKContextSetInjectedBundleClient(context, &_handlerInjectedBundle.base);

            WKPageSetProxies(_page, nullptr);

            WKPageSetCustomBackingScaleFactor(_page, _config.ScaleFactor.Value());

            if (_config.Automation.Value()) {
                _handlerAutomation.base.clientInfo = static_cast<void*>(this);
                WKContextSetAutomationClient(context, &_handlerAutomation.base);
            }

            WKPageSetPageUIClient(_page, &_handlerPageUI.base);

            if (_config.UserAgent.IsSet() == true && _config.UserAgent.Value().empty() == false)
                WKPageSetCustomUserAgent(_page, WKStringCreateWithUTF8CString(_config.UserAgent.Value().c_str()));

            SetURL(_URL);

            // Move into the correct state, as requested
            _adminLock.Lock();
            if ((_state == PluginHost::IStateControl::SUSPENDED) || (_state == PluginHost::IStateControl::UNINITIALIZED)) {
                _state = PluginHost::IStateControl::UNINITIALIZED;
                Suspend();
            } else {
                _state = PluginHost::IStateControl::UNINITIALIZED;
                OnStateChange(PluginHost::IStateControl::RESUMED);
            }
            _adminLock.Unlock();

            g_main_loop_run(_loop);

            // Seems if we stop the mainloop but are not in a suspended state, there is a crash. 
            // Force suspended state first.
            if (_state == PluginHost::IStateControl::RESUMED) {
                WKViewSetViewState(_view, 0);
            }

            // WKRelease(_page);
            if (_automationSession) WKRelease(_automationSession);

            WKRelease(_view);
            WKRelease(pageConfiguration);
            WKRelease(pageGroup);
            WKRelease(context);
            WKRelease(preferences);

            g_main_context_pop_thread_default(_context);
            g_main_loop_unref(_loop);
            g_main_context_unref(_context);

            return Core::infinite;
        }
#endif

    private:
        Config _config;
        string _URL;
        string _dataPath;

#ifdef WEBKIT_GLIB_API
        WebKitWebView* _view;
        uint64_t _guid;
#else
        WKViewRef _view;
        WKPageRef _page;
        WKNotificationManagerRef _notificationManager;
        WKWebAutomationSessionRef _automationSession;
#endif
        Core::CriticalSection _adminLock;
        uint32_t _fps;
        GMainLoop* _loop;
        GMainContext* _context;
        std::list<Exchange::IBrowser::INotification*> _notificationClients;
        std::list<PluginHost::IStateControl::INotification*> _stateControlClients;
        PluginHost::IStateControl::state _state;
        bool _hidden;
        uint64_t _time;
        bool _compliant;
    };

    SERVICE_REGISTRATION(WebKitImplementation, 1, 0);

#ifndef WEBKIT_GLIB_API

    // Handles synchronous messages from injected bundle.
    /* static */ void onDidReceiveSynchronousMessageFromInjectedBundle(WKContextRef context, WKStringRef messageName,
        WKTypeRef messageBodyObj, WKTypeRef* returnData, const void* clientInfo)
    {
        static int configLen = strlen(Tags::Config);
        const WebKitImplementation* browser = static_cast<const WebKitImplementation*>(clientInfo);

        string name = WKStringToString(messageName);

        // Depending on message name, select action.
        if (name == Tags::Notification) {
            // Message contains strings from custom JS handler "NotifyWebbridge".
            WKArrayRef messageLines = static_cast<WKArrayRef>(messageBodyObj);

            std::vector<string> messageStrings = ConvertWKArrayToStringVector(messageLines);
            browser->OnJavaScript(messageStrings);
        } else if (name == Tags::URL) {
            *returnData = WKStringCreateWithUTF8CString(browser->GetURL().c_str());
        } else if (name.compare(0, configLen, Tags::Config) == 0) {
            // Second part of this string is the key we are looking for, extract it...
            std::string utf8Json = Core::ToString(browser->GetConfig(name.substr(configLen)));
            *returnData = WKStringCreateWithUTF8CString(utf8Json.c_str());
        } else {
            // Unexpected message name.
            std::cerr << "WebBridge received synchronous message (" << name << "), but didn't process it." << std::endl;
        }
    }

    /* static */ void didStartProvisionalNavigation(WKPageRef page, WKNavigationRef navigation, WKTypeRef userData, const void* clientInfo)
    {
        WebKitImplementation* browser = const_cast<WebKitImplementation*>(static_cast<const WebKitImplementation*>(clientInfo));

        WKURLRef urlRef = WKPageCopyActiveURL(page);
        WKStringRef urlStringRef = WKURLCopyString(urlRef);

        string url = WKStringToString(urlStringRef);

        browser->OnURLChanged(url);

        WKRelease(urlRef);
        WKRelease(urlStringRef);
    }

    /* static */ void didSameDocumentNavigation(const OpaqueWKPage* page, const OpaqueWKNavigation* nav, WKSameDocumentNavigationType type, const void* clientInfo, const void* info)
    {
        if (type == kWKSameDocumentNavigationAnchorNavigation) {
            WebKitImplementation* browser = const_cast<WebKitImplementation*>(static_cast<const WebKitImplementation*>(info));

            WKURLRef urlRef = WKPageCopyActiveURL(page);
            WKStringRef urlStringRef = WKURLCopyString(urlRef);

            string url = WKStringToString(urlStringRef);

            browser->OnURLChanged(url);

            WKRelease(urlRef);
            WKRelease(urlStringRef);
        }
    }

    /* static */ void didFinishDocumentLoad(WKPageRef page, WKNavigationRef navigation, WKTypeRef userData, const void* clientInfo)
    {

        WebKitImplementation* browser = const_cast<WebKitImplementation*>(static_cast<const WebKitImplementation*>(clientInfo));

        WKURLRef urlRef = WKPageCopyActiveURL(page);
        WKStringRef urlStringRef = WKURLCopyString(urlRef);

        string url = WKStringToString(urlStringRef);

        browser->OnLoadFinished(url);

        WKRelease(urlRef);
        WKRelease(urlStringRef);
    }

    /* static */ void requestClosure(const void* clientInfo)
    {
        // WebKitImplementation* browser = const_cast<WebKitImplementation*>(static_cast<const WebKitImplementation*>(clientInfo));
        // TODO: @Igalia, make sure the clientInfo is actually holding the correct clientINfo, currently it is nullptr. For
        // now we use the Singleton, this is fine as long as there is only 1 instance (in process) or it is always fine if we
        // are running out-of-process..
        WebKitImplementation* realBrowser = static_cast<WebKitImplementation*>(implementation);
        realBrowser->NotifyClosure();
    }

    /* static */ void onNotificationShow(WKPageRef page, WKNotificationRef notification, const void* clientInfo)
    {
        const WebKitImplementation* browser = static_cast<const WebKitImplementation*>(clientInfo);

        WKStringRef titleRef = WKNotificationCopyTitle(notification);
        WKStringRef bodyRef = WKNotificationCopyBody(notification);

        string title = WKStringToString(titleRef);
        string body = WKStringToString(bodyRef);

        TRACE_GLOBAL(HTML5Notification, (_T("%s - %s"), title.c_str(), body.c_str()));

        // Tell page we've "shown" the notification.
        uint64_t notificationID = WKNotificationGetID(notification);
        browser->OnNotificationShown(notificationID);

        WKRelease(bodyRef);
        WKRelease(titleRef);
    }

    /* static */ void onFrameDisplayed(WKViewRef view, const void* clientInfo)
    {
        WebKitImplementation* browser = const_cast<WebKitImplementation*>(static_cast<const WebKitImplementation*>(clientInfo));

        static unsigned s_frameCount = 0;
        static gint64 lastDumpTime = g_get_monotonic_time();

        ++s_frameCount;
        gint64 time = g_get_monotonic_time();
        if (time - lastDumpTime >= G_USEC_PER_SEC) {
            browser->SetFPS(s_frameCount * G_USEC_PER_SEC * 1.0 / (time - lastDumpTime));
            s_frameCount = 0;
            lastDumpTime = time;
        }
    }

    /* static */ void didRequestAutomationSession(WKContextRef context, WKStringRef sessionID, const void* clientInfo)
    {
        WebKitImplementation* browser = const_cast<WebKitImplementation*>(static_cast<const WebKitImplementation*>(clientInfo));
        browser->OnRequestAutomationSession(context, sessionID);
    }

    /* static */ WKPageRef onAutomationSessionRequestNewPage(WKWebAutomationSessionRef, const void* clientInfo)
    {
        WebKitImplementation* browser = const_cast<WebKitImplementation*>(static_cast<const WebKitImplementation*>(clientInfo));
        return browser->GetPage();
    }

#endif

} // namespace Plugin

namespace WebKitBrowser {

    // TODO: maybe nice to expose this in the config.json
    static const TCHAR* mandatoryProcesses[] = {
        _T("WPENetworkProcess"),
        _T("WPEWebProcess")
    };

    static constexpr uint16_t RequiredChildren = (sizeof(mandatoryProcesses) / sizeof(mandatoryProcesses[0]));
    class MemoryObserverImpl : public Exchange::IMemory {
    private:
        MemoryObserverImpl();
        MemoryObserverImpl(const MemoryObserverImpl&);
        MemoryObserverImpl& operator=(const MemoryObserverImpl&);

        enum { TYPICAL_STARTUP_TIME = 10 }; /* in Seconds */
    public:
        MemoryObserverImpl(const RPC::IRemoteConnection* connection)
            : _main(connection == nullptr ? Core::ProcessInfo().Id() : connection->RemoteId())
            , _children(_main.Id())
            , _startTime(connection == nullptr ? 0 : Core::Time::Now().Add(TYPICAL_STARTUP_TIME * 1000).Ticks())
        { // IsOperation true till calculated time (microseconds)
        }
        ~MemoryObserverImpl()
        {
        }

    public:
        virtual uint64_t Resident() const
        {
            uint32_t result(0);

            if (_startTime != 0) {
                if (_children.Count() < RequiredChildren) {
                    _children = Core::ProcessInfo::Iterator(_main.Id());
                }

                result = _main.Resident();

                _children.Reset();

                while (_children.Next() == true) {
                    result += _children.Current().Resident();
                }
            }

            return (result);
        }
        virtual uint64_t Allocated() const
        {
            uint32_t result(0);

            if (_startTime != 0) {
                if (_children.Count() < RequiredChildren) {
                    _children = Core::ProcessInfo::Iterator(_main.Id());
                }

                result = _main.Allocated();

                _children.Reset();

                while (_children.Next() == true) {
                    result += _children.Current().Allocated();
                }
            }

            return (result);
        }
        virtual uint64_t Shared() const
        {
            uint32_t result(0);

            if (_startTime != 0) {
                if (_children.Count() < RequiredChildren) {
                    _children = Core::ProcessInfo::Iterator(_main.Id());
                }

                result = _main.Shared();

                _children.Reset();

                while (_children.Next() == true) {
                    result += _children.Current().Shared();
                }
            }

            return (result);
        }
        virtual uint8_t Processes() const
        {
            // Refresh the children list !!!
            _children = Core::ProcessInfo::Iterator(_main.Id());
            return ((_startTime == 0) || (_main.IsActive() == true) ? 1 : 0) + _children.Count();
        }
        virtual const bool IsOperational() const
        {
            uint32_t requiredProcesses = 0;

            if (_startTime != 0) {

                //!< We can monitor a max of 32 processes, every mandatory process represents a bit in the requiredProcesses.
                // In the end we check if all bits are 0, what means all mandatory processes are still running.
                requiredProcesses = (0xFFFFFFFF >> (32 - RequiredChildren));

                if (_children.Count() < RequiredChildren) {
                    // Refresh the children list !!!
                    _children = Core::ProcessInfo::Iterator(_main.Id());
                }
                //!< If there are less children than in the the mandatoryProcesses struct, we are done and return false.
                if (_children.Count() >= RequiredChildren) {

                    _children.Reset();

                    //!< loop over all child processes as long as we are operational.
                    while ((requiredProcesses != 0) && (true == _children.Next())) {

                        uint8_t count(0);
                        string name(_children.Current().Name());

                        while ((count < RequiredChildren) && (name != mandatoryProcesses[count])) {
                            ++count;
                        }

                        //<! this is a mandatory process and if its still active reset its bit in requiredProcesses.
                        //   If not we are not completely operational.
                        if ((count < RequiredChildren) && (_children.Current().IsActive() == true)) {
                            requiredProcesses &= (~(1 << count));
                        }
                    }
                }
            }

            // TRACE_L1("requiredProcess = %X, IsStarting = %s, main.IsActive = %s", requiredProcesses, IsStarting() ? _T("true") : _T("false"), _main.IsActive() ? _T("true") : _T("false"));
            return (((requiredProcesses == 0) || (true == IsStarting())) && (true == _main.IsActive()));
        }

        BEGIN_INTERFACE_MAP(MemoryObserverImpl)
        INTERFACE_ENTRY(Exchange::IMemory)
        END_INTERFACE_MAP

    private:
        inline const bool IsStarting() const
        {
            return (_startTime == 0) || (Core::Time::Now().Ticks() < _startTime);
        }

    private:
        Core::ProcessInfo _main;
        mutable Core::ProcessInfo::Iterator _children;
        uint64_t _startTime; // !< Reference for monitor
    };

    Exchange::IMemory* MemoryObserver(const RPC::IRemoteConnection* connection)
    {
        ASSERT(connection != nullptr);
        Exchange::IMemory* result = Core::Service<MemoryObserverImpl>::Create<Exchange::IMemory>(connection);
        return (result);
    }
}
} // namespace WebKitBrowser
