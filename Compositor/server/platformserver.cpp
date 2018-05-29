#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>

#include <interfaces/IResourceCenter.h>
#include <interfaces/IComposition.h>

using namespace WPEFramework;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

class MyCompositionListener : public Exchange::IComposition::INotification
{
public:
    virtual void Attached(Exchange::IComposition::IClient* client)
    {
        string clientName = client->Name();
        TRACE_L1("plaformserver, client attached: %s", clientName.c_str());
    }

    virtual void Detached(Exchange::IComposition::IClient* client)
    {
        string clientName = client->Name();
        TRACE_L1("plaformserver, client detached: %s", clientName.c_str());
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
    Exchange::IResourceCenter * resourceCenter = nullptr;

    if (resource.IsLoaded() == true) {
        TRACE_L1("Compositor started in process %s implementation", libPath.c_str());
    } else {
        TRACE_L1("FAILED to start in process %s implementation", libPath.c_str());
        return 1;
    }

    // TODO: straight to composition.
    resourceCenter = Core::ServiceAdministrator::Instance().Instantiate<Exchange::IResourceCenter>(resource, _T("PlatformImplementation"), static_cast<uint32_t>(~0));
    if (resourceCenter) {
        TRACE_L1("Instantiated resource center [%d]", __LINE__);
    } else {
        TRACE_L1("Failed to instantiate resource center [%d]", __LINE__);
        Core::Singleton::Dispose();
        return 1;
    }

    Exchange::IComposition* composition = resourceCenter->QueryInterface<Exchange::IComposition>();
    if (!composition) {
        TRACE_L1("Failed to get composition interface [%d]", __LINE__);
        return 1;
    }

    MyCompositionListener * listener = Core::Service<MyCompositionListener>::Create<MyCompositionListener>();
    composition->Register(listener);

    // TODO: this sets default values, should we also allow for string/path passed along on command line?
    uint32_t confResult = resourceCenter->Configure("{}");
    TRACE_L1("Configured resource center [%d]", confResult);

    TRACE_L1("platformserver: dropping into while-true [%d]", __LINE__);
    while(true);

    Core::Singleton::Dispose();

    return result;
}
