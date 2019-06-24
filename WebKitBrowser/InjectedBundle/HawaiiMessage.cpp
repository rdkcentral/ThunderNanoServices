#include "HawaiiMessage.h"
#include "Utils.h"

#include <iostream>

namespace WPEFramework {
namespace JavaScript {
namespace Amazon {

    static char amazonLibrary[] = _T("libamazon_player.so");

    JSContextRef registerMessageListener::jsContext = nullptr;
    JSObjectRef registerMessageListener::jsListener = nullptr;

    class Loader {
    private:
        Loader()
            : library(amazonLibrary)
        {

            if (library.IsLoaded() == true) {
                std::cerr << "Library loaded: " << amazonLibrary << std::endl;
            } else {
                std::cerr << "FAILED Library loading: %s" << amazonLibrary << std::endl;
            }
        }

    public:
        typedef void (*MessageListenerType)( const std::string& msg );
        typedef bool ( *RegisterMessageListenerType )( MessageListenerType inMessageListener );
        typedef std::string ( *SendMessageType )( const std::string& );

        static Loader& Instance() {
            static Loader singleton;
            return (singleton);
        }

        RegisterMessageListenerType RegisterMessageListener() {
            
            if (!registerMessageListenerFunction) {
                registerMessageListenerFunction = reinterpret_cast<RegisterMessageListenerType>(library.LoadFunction(_T("registerMessageListener")));
                if (registerMessageListenerFunction == nullptr) {
                    std::cerr << "Failed read symbol registerMessageListener" << std::endl;
                }
            }
            return (registerMessageListenerFunction);
        }

        SendMessageType SendMessage() {
            
            if (!sendMessageFunction) {
                sendMessageFunction = reinterpret_cast<SendMessageType >(library.LoadFunction(_T("sendMessage")));
                if (sendMessageFunction == nullptr) {
                    std::cerr << "Failed read symbol sendMessage" << std::endl;
                }
            }
            return (sendMessageFunction);
        }

    private:
        Core::Library library;
        RegisterMessageListenerType registerMessageListenerFunction;
        SendMessageType sendMessageFunction;
    };

    registerMessageListener::registerMessageListener()
    {
    }

    registerMessageListener::~registerMessageListener()
    {
         JSValueUnprotect(jsContext, jsListener);
        jsContext = nullptr;
    }

    /*static*/ void registerMessageListener::listenerCallback (const std::string& msg)
    {
        if (jsContext) {
            //std::cerr << "MESSAGE: " << msg << std::endl;

            JSValueRef passedArgs;

            JSStringRef message = JSStringCreateWithUTF8CString(msg.c_str());
            passedArgs = JSValueMakeString(jsContext, message);

            JSObjectRef globalObject = JSContextGetGlobalObject(jsContext);
            JSObjectCallAsFunction(jsContext, jsListener, globalObject, 1, &passedArgs, NULL);
            JSStringRelease(message);
        }
    }

    JSValueRef registerMessageListener::HandleMessage(JSContextRef context, JSObjectRef,
                                        JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*)
    {
        const int acceptedArgCount = 1;

        if (argumentCount != acceptedArgCount) {
            std::cerr << "registerMessageListener expects one argument" << std::endl;
            return JSValueMakeNull(context);
        }

        jsListener = JSValueToObject(context, arguments[0], NULL);
        
        if (!JSObjectIsFunction(context, jsListener)) {
            std::cerr << "registerMessageListener expects one function as argument" << std::endl;
            return JSValueMakeNull(context);
        }
        JSValueProtect(context, jsListener);
        jsContext = JSContextGetGlobalContext(context);
        
        Loader loader = Loader::Instance();
        Loader::RegisterMessageListenerType registerFunction = reinterpret_cast<Loader::RegisterMessageListenerType>(loader.RegisterMessageListener());
        registerFunction (reinterpret_cast<Loader::MessageListenerType>(registerMessageListener::listenerCallback));

        return JSValueMakeNull(context);
    }

    sendMessage::sendMessage()
    {
    }

    JSValueRef sendMessage::HandleMessage(JSContextRef context, JSObjectRef,
                                                      JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*)
    {

        if (argumentCount != 1) {
            std::cerr << "sendMessage expects one argument" << std::endl;
            return JSValueMakeNull(context);
        }

        if (!JSValueIsString(context, arguments[0])) {
            std::cerr << "sendMessage only accepts string arguments" << std::endl;
            return JSValueMakeNull(context);
        }

        JSStringRef jsString = JSValueToStringCopy(context, arguments[0], nullptr);
        size_t bufferSize = JSStringGetLength(jsString) + 1;
        char stringBuffer[bufferSize];

        JSStringGetUTF8CString(jsString, stringBuffer, bufferSize);

        //std::cerr << "sendMessage:: " << std::string(stringBuffer) << std::endl;

        Loader loader = Loader::Instance();
        Loader::SendMessageType sendFunction = reinterpret_cast<Loader::SendMessageType >(loader.SendMessage());
        sendFunction (std::string(stringBuffer));

        JSStringRelease(jsString);

        return JSValueMakeNull(context);
    }

    static JavaScriptFunctionType<registerMessageListener> _registerInstance(_T("hawaii"));
    static JavaScriptFunctionType<sendMessage> _sendInstance(_T("hawaii"));

} // namespace Amazon
} // namespace JavaScript
} // namespace WPEFramework