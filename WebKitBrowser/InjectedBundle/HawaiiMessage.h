#ifndef __HAWAII_MESSAGE_H
#define __HAWAII_MESSAGE_H

#include "JavaScriptFunctionType.h"

namespace WPEFramework {
namespace JavaScript {
namespace Amazon {

class registerMessageListener {

public:
    registerMessageListener();
    ~registerMessageListener();

    JSValueRef HandleMessage(JSContextRef context, JSObjectRef,
                                         JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*);

    static void listenerCallback (const std::string& msg);
    static JSContextRef jsContext;
    static JSObjectRef jsListener;
};

class sendMessage {
public:
    sendMessage();

    JSValueRef HandleMessage(JSContextRef context, JSObjectRef,
                             JSObjectRef, size_t argumentCount, const JSValueRef arguments[], JSValueRef*);
};

} // namespace Amazon
} // namespace JavaScript
} // namespace WPEFramework

#endif //__HAWAII_MESSAGE_H
