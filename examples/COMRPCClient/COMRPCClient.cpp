/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
// ThunderPlugin:    The is an application (no need to include it anywhere) to host COMRPC objects out-of-process.
//                This is the application that is instantiated as a seperate process in case that a plugin is run
//                out-of-process.
// Thunder:  This is the Thunder application and Controller Plugin. This is the Application that needs
//                to be started.
//
// To include any of the modules in your project, each module, offers a module header file. The module header 
// file is consistently constructed by <module location name>/<module name>.h. Since the Module location name 
// is equal to the module name, the inclusion of a module is done with Thunder/<Module Name>/<Module Name>.h
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

#define MODULE_NAME COMRPCClient

#include <core/core.h>
#include <com/com.h>
#include <definitions/definitions.h>
#include <plugins/Types.h>
#include <interfaces/IDictionary.h>
#include <iostream>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder;

class Sink : public Exchange::IDictionary::INotification {
public:
    Sink() = delete;
    Sink(Sink&&) = delete;
    Sink(const Sink&) = delete;
    Sink& operator= (const Sink&) = delete;

    Sink(Exchange::IDictionary* source)
        : _source(source) {
        _source->AddRef();
    }
    ~Sink() override {
        _source->Release();
    }

public:
    void Modified(const string& nameSpace, const string& key, const string& value) override {
        printf("In namespace [%s], the key [%s] has a new value: [%s]\n", nameSpace.c_str(), key.c_str(), value.c_str());
    }

    BEGIN_INTERFACE_MAP(Sink)
        INTERFACE_ENTRY(Exchange::IDictionary::INotification);
    END_INTERFACE_MAP

private:
    Exchange::IDictionary* _source;
};

class Dictionary : public RPC::SmartInterfaceType<Exchange::IDictionary > {
private:
    using BaseClass = RPC::SmartInterfaceType<Exchange::IDictionary >;
public:
    Dictionary(const uint32_t waitTime, const Core::NodeId& node, const string& callsign)
        : BaseClass() {
        BaseClass::Open(waitTime, node, callsign);
    }
    ~Dictionary() {
        BaseClass::Close(Core::infinite);
    }

public:
    bool Get(const string& nameSpace, const string& key, string& value ) const {
        bool result = false;
        const Exchange::IDictionary* impl = BaseClass::Interface();

        if (impl != nullptr) {
            result = impl->Get(nameSpace, key, value);
            impl->Release();
        }

        return (result);
    }
    bool Set(const string& nameSpace, const string& key, const string& value) {
        bool result = false;
        Exchange::IDictionary* impl = BaseClass::Interface();

        if (impl != nullptr) {
            result = impl->Set(nameSpace, key, value);
            impl->Release();
        }

        return (result);
    }

private:
    void Operational(const bool upAndRunning) {
        printf("Operational state of Dictionary: %s\n", upAndRunning ? _T("true") : _T("false"));
    }
};

class WorkerPoolImplementation 
    : public Core::IIPCServer
    , public Core::ThreadPool::IDispatcher
    , public Core::WorkerPool {
public:
    WorkerPoolImplementation() = delete;
    WorkerPoolImplementation(WorkerPoolImplementation&&) = delete;
    WorkerPoolImplementation(const WorkerPoolImplementation&) = delete;
    WorkerPoolImplementation& operator=(const WorkerPoolImplementation&) = delete;

    PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
    WorkerPoolImplementation(const uint8_t threads, const uint32_t stackSize, const uint32_t queueSize)
        : WorkerPool(threads, stackSize, queueSize, this, nullptr)
    {
        Core::IWorkerPool::Assign(this);
    }
    POP_WARNING()

    ~WorkerPoolImplementation() override {
        Core::IWorkerPool::Assign(nullptr);
    }

private:
    // ThreadPool::IDispatcher
    // -------------------------------------------------------------
    void Initialize() override {
    }
    void Deinitialize() override {
    }
    void Dispatch(Core::IDispatch* job) override {
        job->Dispatch();
    }

    // IIPCServer
    // -------------------------------------------------------------
    void Procedure(Core::IPCChannel& channel, Core::ProxyType<Thunder::Core::IIPC>& data) override {
        Core::ProxyType<Thunder::RPC::Job> job(RPC::Job::Instance());

        job->Set(channel, data);

        WorkerPool::Submit(Core::ProxyType<Core::IDispatch>(job));
    }
};

int main(int /* argc */, char** /* argv */)
{
    bool directLink = true;

    // Create an engine that can deserialize the invoke COMRPC messages that have been 
    // received. The parameters her <4,1> stand for the queue length, 4 which means that
    // if 4 elements have been queued, the submission of the 5th element will be a 
    // blocking call, untill there is a free spot again. Which will cuase the system to
    // throttle.
    // The 1 paramater indicates the number of threads that this engine will create to
    // handle the work. So it holds a threadpool of 1 thread.
    // Thunder::Core::ProxyType< Thunder::RPC::InvokeServerType<1, 0, 4> > engine(
    //    Thunder::Core::ProxyType<Thunder::RPC::InvokeServerType<1, 0, 4>>::Create());

    // Thunder::Core::ProxyType<Thunder::RPC::CommunicatorClient> client(
    //    Thunder::Core::ProxyType<Thunder::RPC::CommunicatorClient>::Create(
    //        nodeId,
    //        Thunder::Core::ProxyType< Thunder::Core::IIPCServer >(engine)));

    // Once the socket is opened the first exchange between client and server is an 
    // announce message. This announce message hold information the otherside requires
    // like, where can I find the ProxyStubs that I need to load, what Trace categories
    // need to be enabled.
    // Extensibility allows to be "in the middle" of these messages and to chose on 
    // which thread this message should be executes. Since the message is coming in 
    // over socket, the announce message could be handled on the communication thread
    // or better, if possible, it can be run on the thread of the engine we have just 
    // created.
    // engine->Announcements(client->Announcement());

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
    // will trigger the fork of a new process, that starts ThunderPlugin, then it will
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


    // Three options to do this:
    // 1) 
    //    Thunder::PluginHost::IShell* controller = client->Open<Thunder::PluginHost::IShell>(_T("Dictionary"), ~0, 3000);
    //
    // Or 
    // 2)
    //    if (client->Open(3000) == Thunder::Core::ERROR_NONE) {
    //        controller = client->Acquire<Thunder::PluginHost::IShell>(10000, _T("Controller"), ~0);
    //
    // Or
    // 3)
    // WorkerPoolImplementation workerPool(2, Thunder::Core::Thread::DefaultStackSize(), 6);
    
    if (directLink == true)
    {
        #ifdef __WINDOWS__
        Core::NodeId nodeId("127.0.0.1:5522");
        #else
        Core::NodeId nodeId("/tmp/Dictionary/communicator");
        #endif
        Core::ProxyObject<RPC::CommunicatorClient> client(nodeId);
        client.AddRef();

        Exchange::IDictionary* pluginOnly = client.Open<Exchange::IDictionary>(_T(""));

        if (pluginOnly != nullptr) {
            Core::SinkType<Sink> sink(pluginOnly);

            pluginOnly->Register(_T("/name"), &sink);

            int32_t counter = 0;
            char keyPress;

            do {
                keyPress = toupper(getchar());

                switch (keyPress) {
                case 'S': {

                    string value = Core::NumberType<int32_t>(counter++).Text();
                    if (pluginOnly->Set(_T("/name"), _T("key"), value) == Core::ERROR_NONE) {
                        printf("Set value: %s\n", value.c_str());
                    }
                    break;
                }
                case 'G': {
                    string value;
                    if (pluginOnly->Get(_T("/name"), _T("key"), value) == Core::ERROR_NONE) {
                        printf("Get value: %s\n", value.c_str());
                    }
                    break;

                }
                case 'Q': break;
                default: break;
                };
            } while (keyPress != 'Q');

            pluginOnly->Unregister(_T("/"), &sink);
            pluginOnly->Release();
            pluginOnly = nullptr;
        }
        client.CompositRelease();
    }
    else {
        // The core::NodeId can hold an IPv4, IPv6, domain, HCI, L2CAP or netlink address
        // Here we create a domain socket address
        #ifdef __WINDOWS__
                Core::NodeId nodeId("127.0.0.1:62000");
        #else
                Core::NodeId nodeId("/tmp/communicator");
        #endif


        Dictionary  dictionary(3000, nodeId, _T("Dictionary"));
        char keyPress;
        uint32_t counter = 8;

        do {
            keyPress = toupper(getchar());
            
            switch (keyPress) {
            case 'O': {
                printf("Operations state issue: %s\n", dictionary.IsOperational() ? _T("true") : _T("false"));
                break;
            }
            case 'S': {

                string value = Core::NumberType<int32_t>(counter++).Text();
                if (dictionary.Set(_T("/name"), _T("key"), value) == true) {
                    printf("Set value: %s\n", value.c_str());
                }
                break;
            }
            case 'G': {
                string value;
                if (dictionary.Get(_T("/name"), _T("key"), value) == true) {
                    printf("Get value: %s\n", value.c_str());
                }
                break;

            }
            case 'X': {
                uint32_t count = 0;
                while (count++ != 500000) {
                    string value = Core::NumberType<int32_t>(counter++).Text();
                    if (dictionary.Set(_T("/name"), _T("key"), value) == true) {
                        if (dictionary.Get(_T("/name"), _T("key"), value) == true) {
                            printf("Iteration %6i: Set/Get value: %s\n", count, value.c_str());
                        }
                    }
                }
                break;

            }
            case 'Q': break;
            default: break;
            };
        } while (keyPress != 'Q');
    }

    printf("Prior to the call Dispose\n");
    Core::Singleton::Dispose();
    printf("Completed the call Dispose\n");

    return 0;
}
