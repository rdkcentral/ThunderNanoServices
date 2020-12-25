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
 
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <interfaces/IComposition.h>
#include <interfaces/IResourceCenter.h>

using namespace WPEFramework;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

class MyCompositionListener : public Exchange::IComposition::INotification {
public:
    virtual void Attached(Exchange::IComposition::IClient* client)
    {
        string clientName = client->Name();
        TRACE(Trace::Information, (_T("plaformserver, client attached: %s"), clientName.c_str()));
    }

    virtual void Detached(Exchange::IComposition::IClient* client)
    {
        string clientName = client->Name();
        TRACE(Trace::Information, (_T("plaformserver, client detached: %s"), clientName.c_str()));
    }

    BEGIN_INTERFACE_MAP(MyCompositionListener)
    INTERFACE_ENTRY(Exchange::IComposition::INotification)
    END_INTERFACE_MAP
};

int main(int argc, char* argv[])
{
    int result = 0;

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " [so-file containing platform impl]" << std::endl;
        std::cerr << "For example:" << std::endl;
        std::cerr << "   " << argv[0] << " /usr/share/WPEFramework/Compositor/libplatformplugin.so" << std::endl;

        return 1;
    }

    string libPath(argv[1]);
    Core::Library resource(libPath.c_str());
    Exchange::IResourceCenter* resourceCenter = nullptr;

    if (resource.IsLoaded() == true) {
        TRACE_GLOBAL(Trace::Information, (_T("Compositor started in process %s implementation"), libPath.c_str()));
    } else {
        TRACE_GLOBAL(Trace::Error, (_T("FAILED to start in process %s implementation"), libPath.c_str()));
        return 1;
    }

    // TODO: straight to composition.
    resourceCenter = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IResourceCenter>(resource, _T("PlatformImplementation"), static_cast<uint32_t>(~0));
    if (resourceCenter) {
        TRACE_GLOBAL(Trace::Information, (_T("Instantiated resource center [%d]"), __LINE__));
    } else {
        TRACE_GLOBAL(Trace::Error, (_T("Failed to instantiate resource center [%d]"), __LINE__));
        Core::Singleton::Dispose();
        return 1;
    }

    Exchange::IComposition* composition = resourceCenter->QueryInterface<Exchange::IComposition>();
    if (composition == nullptr) {
        TRACE_GLOBAL(Trace::Error, (_T("Failed to get composition interface [%d]"), __LINE__));
    }
    else {
        MyCompositionListener* listener = Core::Service<MyCompositionListener>::Create<MyCompositionListener>();
        composition->Register(listener);

        // TODO: this sets default values, should we also allow for string/path passed along on command line?
        uint32_t confResult = resourceCenter->Configure("{}");
        TRACE_GLOBAL(Trace::Information, (_T("Configured resource center [%d]", confResult);

        TRACE_GLOBAL(Trace::Information, (_T("platformserver: dropping into while-true [%d]"), __LINE__));
        while (true)
            ;

        composition->Release();
    }

    Core::Singleton::Dispose();

    return result;
}
