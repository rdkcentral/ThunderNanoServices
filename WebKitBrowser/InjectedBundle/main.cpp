#include "Module.h"

#include <WPE/WebKit.h>

#include <cstdio>
#include <memory>
#include <syslog.h>

#include "WhiteListedOriginDomainsList.h"
#include "NotifyWPEFramework.h"
#include "Utils.h"
#include "ClassDefinition.h"

using namespace WPEFramework;
using WebKit::WhiteListedOriginDomainsList;
using JavaScript::ClassDefinition;

WKBundleRef g_Bundle;

static Core::NodeId GetConnectionNode() {
    string nodeName;

    Core::SystemInfo::GetEnvironment(string(_T("COMMUNICATOR_CONNECTOR")), nodeName);

    return (Core::NodeId (nodeName.c_str()));
}

static class PluginHost 
{
    private:
        PluginHost(const PluginHost&) = delete;
        PluginHost& operator= (const PluginHost&) = delete;

    public:
        PluginHost()
            : _comClient(Core::ProxyType<RPC::CommunicatorClient>::Create(GetConnectionNode()))
	    , _handler(Core::ProxyType< RPC::ObjectMessageHandler >::Create())
            , _invokeServer(Core::ProxyType< RPC::InvokeServerType<16,2> >::Create(Core::Thread::DefaultStackSize()))
        {
	    ASSERT (_comClient.IsValid() == true);
	    ASSERT (_handler.IsValid() == true);
	    ASSERT (_invokeServer.IsValid() == true);

            // Seems like we have enough information, open up the Process communcication Channel.
            _comClient->CreateFactory<RPC::InvokeMessage>(4);
            _comClient->CreateFactory<RPC::ObjectMessage>(2);

            // Make sure we understand inbound requests and that we have a factory to create those elements.
            _comClient->Register(Core::proxy_cast<Core::IIPCServer>(_invokeServer));
            _comClient->Register(Core::proxy_cast<Core::IIPCServer>(_handler));
        }
        ~PluginHost()
        {
            TRACE_L1("Destructing injected bundle stuff!!! [%d]", __LINE__);
            Deinitialize();

            if (_comClient.IsValid() == true) {
                _comClient->Unregister(Core::proxy_cast<Core::IIPCServer>(_handler));
                _comClient->Unregister(Core::proxy_cast<Core::IIPCServer>(_invokeServer));
            }
        }

    public:
        void Initialize (WKBundleRef bundle)
        {
            uint32_t result;

            Trace::TraceType<Trace::Information, &Core::System::MODULE_NAME>::Enable(true);

            // We have something to report back, do so...
            if (_comClient.IsValid() == true) {

                if ((result = _comClient->Open(2 * RPC::CommunicationTimeOut)) != Core::ERROR_NONE) {

                    TRACE_L1("Could not open the connection, error (%d)", result);
                }
                
                _comClient->WaitForCompletion();
            }

            _bundle = bundle;
                
            _whiteListedOriginDomainPairs = WhiteListedOriginDomainsList::RequestFromWPEFramework(bundle);
        }

        void Deinitialize() 
        {
            if (_comClient.IsValid() == true) {
                _comClient->Close(Core::infinite);
            }

	    Core::Singleton::Dispose();
        }

        void WhiteList(WKBundleRef bundle) {

            // Whitelist origin/domain pairs for CORS, if set.
            if (_whiteListedOriginDomainPairs) {
                _whiteListedOriginDomainPairs->AddWhiteListToWebKit(bundle);
            }
        }

    private:
        Core::ProxyType<RPC::CommunicatorClient> _comClient;
        Core::ProxyType<RPC::ObjectMessageHandler> _handler;
        Core::ProxyType<RPC::InvokeServerType<16,2> > _invokeServer;

        // White list for CORS.
        std::unique_ptr<WhiteListedOriginDomainsList> _whiteListedOriginDomainPairs;

        // Handle of bundle.
        WKBundleRef _bundle;

} _wpeFrameworkClient;

extern "C" {

__attribute__((destructor))
static void unload() {
    _wpeFrameworkClient.Deinitialize();
}

// Adds class to JS world.
void InjectInJSWorld(ClassDefinition& classDef, WKBundleFrameRef frame, WKBundleScriptWorldRef scriptWorld)
{
    // @Zan: for how long should "ClassDefinition.staticFunctions" remain valid? Can it be
    // released after "JSClassCreate"?

    JSGlobalContextRef context = WKBundleFrameGetJavaScriptContextForWorld(frame, scriptWorld);

    ClassDefinition::FunctionIterator function = classDef.GetFunctions();
    uint32_t functionCount = function.Count();

    // We need an extra entry that we set to all zeroes, to signal end of data.
    // TODO: memleak.
    JSStaticFunction* staticFunctions = new JSStaticFunction[functionCount + 1];

    int index = 0;
    while (function.Next()) {
        staticFunctions[index++] = (*function)->BuildJSStaticFunction();
    }

    staticFunctions[functionCount] = { nullptr, nullptr, 0 };

    // TODO: memleak.
    JSClassDefinition* JsClassDefinition = new JSClassDefinition{
        0, // version
        kJSClassAttributeNone, //attributes
        classDef.GetClassName().c_str(), // className
        0, // parentClass
        nullptr, // staticValues
        staticFunctions, // staticFunctions
        nullptr, //initialize
        nullptr, //finalize
        nullptr, //hasProperty
        nullptr, //getProperty
        nullptr, //setProperty
        nullptr, //deleteProperty
        nullptr, //getPropertyNames
        nullptr, //callAsFunction
        nullptr, //callAsConstructor
        nullptr, //hasInstance
        nullptr, //convertToType
    };

    JSClassRef jsClass = JSClassCreate(JsClassDefinition);
    JSValueRef jsObject = JSObjectMake(context, jsClass, nullptr);
    JSClassRelease(jsClass);

    // @Zan: can we make extension name same as ClassName?
    JSStringRef extensionString = JSStringCreateWithUTF8CString(classDef.GetExtName().c_str());
    JSObjectSetProperty(context, JSContextGetGlobalObject(context), extensionString, jsObject,
        kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, nullptr);
    JSStringRelease(extensionString);
}

static WKBundlePageLoaderClientV6 s_pageLoaderClient = {
    { 6, nullptr },
    nullptr, // didStartProvisionalLoadForFrame
    nullptr, // didReceiveServerRedirectForProvisionalLoadForFrame
    nullptr, // didFailProvisionalLoadWithErrorForFrame
    nullptr, // didCommitLoadForFrame
    nullptr, // didFinishDocumentLoadForFrame
    nullptr, // didFinishLoadForFrame
    nullptr, // didFailLoadWithErrorForFrame
    nullptr, // didSameDocumentNavigationForFrame
    nullptr, // didReceiveTitleForFrame
    nullptr, // didFirstLayoutForFrame
    nullptr, // didFirstVisuallyNonEmptyLayoutForFrame
    nullptr, // didRemoveFrameFromHierarchy
    nullptr, // didDisplayInsecureContentForFrame
    nullptr, // didRunInsecureContentForFrame
    // didClearWindowObjectForFrame
    [](WKBundlePageRef, WKBundleFrameRef frame, WKBundleScriptWorldRef scriptWorld, const void*) {
        // Add JS classes to JS world.
        ClassDefinition::Iterator ite = ClassDefinition::GetClassDefinitions();
        while (ite.Next()) {
            InjectInJSWorld(*ite, frame, scriptWorld);
        }
    },
    nullptr, // didCancelClientRedirectForFrame
    nullptr, // willPerformClientRedirectForFrame
    nullptr, // didHandleOnloadEventsForFrame
    nullptr, // didLayoutForFrame
    nullptr, // didNewFirstVisuallyNonEmptyLayout_unavailable
    nullptr, // didDetectXSSForFrame
    nullptr, // shouldGoToBackForwardListItem
    nullptr, // globalObjectIsAvailableForFrame
    nullptr, // willDisconnectDOMWindowExtensionFromGlobalObject
    nullptr, // didReconnectDOMWindowExtensionToGlobalObject
    nullptr, // willDestroyGlobalObjectForDOMWindowExtension
    nullptr, // didFinishProgress
    nullptr, // shouldForceUniversalAccessFromLocalURL
    nullptr, // didReceiveIntentForFrame_unavailable
    nullptr, // registerIntentServiceForFrame_unavailable
    nullptr, // didLayout
    nullptr, // featuresUsedInPage
    nullptr, // willLoadURLRequest
    nullptr, // willLoadDataRequest
};

static WKBundlePageUIClientV4 s_pageUIClient = {
    { 4, nullptr },
    nullptr, // willAddMessageToConsole
    nullptr, // willSetStatusbarText
    nullptr, // willRunJavaScriptAlert
    nullptr, // willRunJavaScriptConfirm
    nullptr, // willRunJavaScriptPrompt
    nullptr, // mouseDidMoveOverElement
    nullptr, // pageDidScroll
    nullptr, // unused1
    nullptr, // shouldGenerateFileForUpload
    nullptr, // generateFileForUpload
    nullptr, // unused2
    nullptr, // statusBarIsVisible
    nullptr, // menuBarIsVisible
    nullptr, // toolbarsAreVisible
    nullptr, // didReachApplicationCacheOriginQuota
    nullptr, // didExceedDatabaseQuota
    nullptr, // createPlugInStartLabelTitle
    nullptr, // createPlugInStartLabelSubtitle
    nullptr, // createPlugInExtraStyleSheet
    nullptr, // createPlugInExtraScript
    nullptr, // unused3
    nullptr, // unused4
    nullptr, // unused5
    nullptr, // didClickAutoFillButton
    //willAddDetailedMessageToConsole
    [](WKBundlePageRef page, WKConsoleMessageSource source, WKConsoleMessageLevel level, WKStringRef message, uint32_t lineNumber,
        uint32_t columnNumber, WKStringRef url, const void* clientInfo) {
        string messageString = WebKit::Utils::WKStringToString(message);

        const uint16_t maxStringLength = Trace::TRACINGBUFFERSIZE - 1;
        if (messageString.length() > maxStringLength) {
            messageString = messageString.substr(0, maxStringLength);
        }

        // TODO: use "Trace" classes for different levels.
        TRACE_GLOBAL(Trace::Information, (messageString));
    }
};

static WKBundleClientV1 s_bundleClient = {
    { 1, nullptr },
    // didCreatePage
    [](WKBundleRef bundle, WKBundlePageRef page, const void* clientInfo) {
        // Register page loader client, for javascript callbacks.
        WKBundlePageSetPageLoaderClient(page, &s_pageLoaderClient.base);

        // Register UI client, this one will listen to log messages.
        WKBundlePageSetUIClient(page, &s_pageUIClient.base);

        _wpeFrameworkClient.WhiteList(bundle);
    },
    nullptr, // willDestroyPage
    nullptr, // didInitializePageGroup
    nullptr, // didReceiveMessage
    nullptr, // didReceiveMessageToPage
};

// Declare module name for tracer.
MODULE_NAME_DECLARATION(BUILD_REFERENCE)


void WKBundleInitialize(WKBundleRef bundle, WKTypeRef)
{
	g_Bundle = bundle;

    _wpeFrameworkClient.Initialize(bundle);

    WKBundleSetClient(bundle, &s_bundleClient.base);
}

}
