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
 
// COMRPCApp.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>


// Thunder is build up, out of modules. There are no circular dependencies. The dependencies are listed 
// here from top to bottom, so all modules depend on core atc...
// At the time of this writing, Thunder has the following modules:
// core:          Platform abstraction layer, this layer exposes a uniform set of C++ interfaces which 
//                should be used in the respective higher layers. This layer is responsible for making 
//                sure that Thunder can run on Windows, Linux and OS-X.
//                This core library contains a JSON parser.
// cryptalgo:     Layer containing C++ interfaces towards cryptographic operations, like SHA/HMAC, random
//                AES encryption/decryption
// tracing:       This module is the runtime tracing system used throughout the modules. This tracing can 
//                be turned on/off @ runtime and allows for creating custom trace classes. Trace classses
//                can be enabled @module (plugin) level or @tracecategory level. This tracing is picked up
//                by the TraceControl plugin that will place the traces in the right sequence again (even 
//                over process boundaries)
// com:           This is the implementation of the Microsoft COM-like framework which allows for process
//                transparent development. It can run over domain socket (only on linux) and over TCP 
//                sockets. The ProxysStub code required is derived/generated using tools from the regular 
//                C headerfiles.
// websocket:     This module exposes a C++ WebSocket class and has the C++ classes for HTPP communication.
//                This module also implements a C++ representation for URL parsing.
// protocols:     This is the combination of com + websocket module!
// plugins:       This module contains all the functionality required to build a plugin. It contains helper 
//                classes and the base interfaces (IPlugin, IShell, ISubsystem) for plugin development and 
//                a helper class to do "easy" development of JSONRPC interfaces.
// WPEProcess:    The is an application (no need to include it anywhere) to host COMRPC objects out-of-process.
//                This is the application that is instantiated as a seperate process in case that a plugin is run
//                out-of-process.
// WPEFramework:  This is the Thunder application and Controller Plugin. This is the Application that needs
//                to be started.
//
// To include any of the modules in your project, each module, offers a module header file. The module header 
// file is consistently constructed by <module location name>/<module name>.h. Since the Module location name 
// is equal to the module name, the inclusion of a module is done with WPEFramework/<Module Name>/<Module Name>.h
// 
// Logging versus Tracing (internal/module)
// Thunder makes a distinction between the Logging concept and the Tracing concept. 
// The audience for Logging is expected to be Testers and Operators. The audience for tracing is expected to 
// be the developper. Tracing, by default, is not enabled. Logging is enabled by default. 
// Since the tracing module is not the "lowest layer (it requires core), some kind of tracing is required 
// for the core module. The core module uses (Trace.h/TRACE_LX pre-processor statement) for its trace 
// definitions. These traces are enabled at compile time (only in DEBUG builds) and only have the levels (1..5). 
// These Traces can thus not be turned on/off @ runtime and thus might, if enabled, generate large log files. 
// The Tracing module adds another type of traces (TRACE() pre-processor statement). These traces are "advanced" 
// traces. They are enabled in the code in DEBUG/RELEASE builds, not in PRODUCTION builds. These Traces are off 
// by default (unless configured differently in the Thunder config file). These traces can be turned on/off at
// runtime, by the Thunder UI, pane Tracing, to enable categories. Once enabled, depending on the configuration 
// of the TraceControl plugin, these traces are send to the console, syslog or to a network port. The TraceControl
// plugin will also reconstruct the traces out of a different process without mixing up parts of the trace.
// Logging is on by default, in all build flavors and report serious issues that are related to exceptional 
// situations observed in the software that can be correctd by a tester/operator.
//


// Since lower levels 
// #define _TRACE_LEVEL 5

#include <core/core.h>
#include <com/com.h>
#include <interfaces/IDictionary.h>
#include <iostream>

namespace Thunder = WPEFramework;

int main(int argc, char* argv[])
{
    // The core::NodeId can hold an IPv4, IPv6, domain, HCI, L2CAP or netlink address
    // Here we create a domain socket address
    #ifdef __WINDOWS__
    Thunder::Core::NodeId nodeId("127.0.0.1:62000");
    #else
    Thunder::Core::NodeId nodeId("/tmp/communicator");
    #endif

    // Create an engine that can deserialize the invoke COMRPC messages that have been 
    // received. The parameters her <4,1> stand for the queue length, 4 which means that
    // if 4 elements have been queued, the submission of the 5th element will be a 
    // blocking call, untill there is a free spot again. Which will cuase the system to
    // throttle.
    // The 1 paramater indicates the number of threads that this engine will create to
    // handle the work. So it holds a threadpool of 1 thread.
    Thunder::Core::ProxyType< Thunder::RPC::InvokeServerType<1, 0, 4> > engine(
        Thunder::Core::ProxyType<Thunder::RPC::InvokeServerType<1, 0, 4>>::Create());

    Thunder::Core::ProxyType<Thunder::RPC::CommunicatorClient> client(
        Thunder::Core::ProxyType<Thunder::RPC::CommunicatorClient>::Create(
            nodeId,
            Thunder::Core::ProxyType< Thunder::Core::IIPCServer >(engine)));

    // Once the socket is opened the first exchange between client and server is an 
    // announce message. This announce message hold information the otherside requires
    // like, where can I find the ProxyStubs that I need to load, what Trace categories
    // need to be enabled.
    // Extensibility allows to be "in the middle" of these messages and to chose on 
    // which thread this message should be executes. Since the message is coming in 
    // over socket, the announce message could be handled on the communication thread
    // or better, if possible, it can be run on the thread of the engine we have just 
    // created.
    engine->Announcements(client->Announcement());

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
