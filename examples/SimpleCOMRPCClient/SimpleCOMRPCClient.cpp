#include <core/core.h>
#include <com/com.h>
#include "../SimpleCOMRPCInterface/ISimpleCOMRPCInterface.h"

using namespace WPEFramework;


int main(int argc, char* argv[])
{
    // The core::NodeId can hold an IPv4, IPv6, domain, HCI, L2CAP or netlink address
    // Here we create a domain socket address
    #ifdef __WINDOWS__
    Core::NodeId nodeId("127.0.0.1:63000");
    #else
    Core::NodeId nodeId("/tmp/simplecomrpclink");
    #endif


    // Since the COMRPC, in comparison to the JSONRPC framework, is host/process/plugin 
    // agnostic, the COMRPC mechanism can only "connect" to a port (Thunder application)
    // and request an existing interface or create an object.
    // 
    // Plugins (like OCDM server) that support specific functionality will host there 
    // own COMRPC connection point (server). If the OCDM client attaches to this instance
    // it can creat a new interface instance (OCDMDecryptSession) or it can request an 
    // existing interface with a specific functionality. It is up to the implementation
    // behind this COMRPC connection point what happens.
    //
    // As for the Thunder framework, the only service offered on this connection point
    // at the time of this writing is to "offer an interface (created on client side) 
    // and return it to the process that requested this interface for out-of-process.
    // The calls in the plugins (WebKitBrowser plugin):
    // service->Root<Exchange::IBrowser>(_connectionId, 2000, _T("WebKitImplementation"));
    // will trigger the fork of a new process, that starts WPEProcess, WPEProcess, will 
    // load a plugin (libWebKitBrowser.so), instantiate an object called 
    // "WebKitImplementation" and push the interface that resides on this object 
    // (Exchange::IBrowser) back over the opened COMRPC channel to the Thunder Framework.
    // Since the Thunder Framework initiated this, it knows the sequenceId issued with 
    // this request and it will forward the interface (which is now actually a Proxy) 
    // to the call that started this fork.
    //
    // So that means that currently there is no possibility to request an interface 
    // on this end-poiunt however since it might be interesting to request at least 
    // the IShell interface of the ControllerPlugin (from there one could navigate 
    // to any other plugin) and to explore areas which where not required yet, I will
    // prepare this request to be demoed next week when we are on side and then walk 
    // with you through the flow of events. So what you are doing here, accessing
    // the Thunder::Exchange::IDictionary* from an executable outside of the Thunder
    // is available on the master, as of next week :-)
    // The code that follows now is pseudo code (might still require some changes) but 
    // will be operational next week

    SleepMs(4000);

    // client->Open<Thunder::Exchange::IDictionary>("Dictionary");


    // Two options to do this:
    // 1) 
    Thunder::PluginHost::IShell* controller = client->Open<Thunder::PluginHost::IShell>(_T("Dictionary"), ~0, 3000);

    // Or option 
    // 2)
    // if (client->Open(3000) == Thunder::Core::ERROR_NONE) {
    //    controller = client->Aquire<Thunder::PluginHost::IShell>(10000, _T("Controller"), ~0);
    //}

    // Once we have the controller interface, we can use this interface to navigate through tho other interfaces.
    if (controller != nullptr) {

        Thunder::Exchange::IDictionary* dictionary = controller->QueryInterface<Thunder::Exchange::IDictionary>();

        // Do whatever you want to do on Thunder::Exchange::IDictionary*
        std::cout << "client.IsValid:" << client.IsValid() << std::endl;
        std::cout << "client.IsOpen :" << client->IsOpen() << std::endl;

        if (dictionary == nullptr)
        {
            std::cout << "failed to get dictionary proxy" << std::endl;
            return -1;
        }
        else
        {
            std::cout << "have proxy to dictionary" << std::endl;
            dictionary->Release();
            dictionary = nullptr;
        }
    }

    // You can do this explicitely but it should not be nessecary as the destruction of the Client will close the link anyway.
    client->Close(1000);

    Thunder::Core::Singleton::Dispose();

    return 0;
}
