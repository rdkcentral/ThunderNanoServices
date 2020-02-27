#include <core/core.h>
#include <com/com.h>
#include "../SimpleCOMRPCInterface/ISimpleCOMRPCInterface.h"

using namespace WPEFramework;

class EXTERNAL COMServer : public RPC::Communicator {
private:
    class EXTERNAL Implementation : public Exchange::IWallClock {
    private:
        class EXTERNAL WallClockNotifier {
        private:
            typedef std::list< std::pair<uint32_t, Implementation*> > NotifierList;

            class TimeHandler {
            public:
                TimeHandler()
                    : _callback(nullptr)
                {
                }
                TimeHandler(Implementation* callback, const uint32_t interval)
                    : _callback(callback)
                {
                }
                TimeHandler(const TimeHandler& copy)
                    : _callback(copy._callback)
                {
                }
                ~TimeHandler()
                {
                }

                TimeHandler& operator=(const TimeHandler& RHS)
                {
                    _callback = RHS._callback;
                    _interval = RHS._interval;
                    return (*this);
                }
                bool operator==(const TimeHandler& RHS) const
                {
                    return (_callback == RHS._callback);
                }
                bool operator!=(const TimeHandler& RHS) const
                {
                    return (_callback != RHS._callback);
                }

            public:
                uint64_t Timed(const uint64_t scheduledTime)
                {
                    Core::Time(sched)
                }

            private:
                Implementation* _callback;
                uint32_t _interval;
            };

            WallClockNotifier() 
                : _adminLock()
                , _timer(Core::Thread::DefaultStackSize(), _T("WallclockTimer")) {
            }

        public:
            WallClockNotifier(const WallClockNotifier&) = delete;
            WallClockNotifier& operator= (const WallClockNotifier&) = delete;

            static WallClockNotifier& Instance() {
                static WallClockNotifier singleton;
                return (singleton);
            }
            ~WallClockNotifier() {
            }

        public:
            void Update(Implementation* sink, const uint32_t sleepTime)
            {
                // This is called in a multithreaded environment. This is not Javascript :-)
                // So lets make sure that operations on the NotifierList are atomic.
                _adminLock.Lock();


                _adminLock.Unlock();
            }
            void Revoke(Implementation* sink)
            {
                // This is called in a multithreaded environment. This is not Javascript :-)
                // So lets make sure that operations on the NotifierList are atomic.
                _adminLock.Lock();


                _adminLock.Unlock();
            }

        private:
            Core::CriticalSection _adminLock;
            Core::TimerType<TimeHandler> _timer;
        };

    public:
        Implementation(const Implementation&) = delete;
        Implementation& operator= (const Implementation&) = delete;

        Implementation() 
            : _adminLock()
            , _callback(nullptr) {

        }
        ~Implementation() override {
        }

    public:
        virtual uint32_t Callback(ICallback* callback) override
        {
            uint32_t result = Core::ERROR_BAD_REQUEST;

            // This is called in a multithreaded environment. This is not Javascript :-)
            // So lets make sure that operations on the NotifierList are atomic.
            _adminLock.Lock();

            if ((_callback == nullptr) ^ (callback == nullptr)) {

                if (callback != nullptr) {
                    WallClockNotifier::Instance().Update(this, _interval);
                    callback->AddRef();
                }
                else {
                    WallClockNotifier::Instance().Revoke(this);
                    _callback->Release();
                }

                result = Core::ERROR_NONE;
                _callback = callback;
            }

            _adminLock.Unlock();

            return (result);
        }
        void Interval(const uint32_t interval) override
        {
            _interval = interval;

            // This is called in a multithreaded environment. This is not Javascript :-)
            // So lets make sure that operations on the NotifierList are atomic.
            _adminLock.Lock();

            if (_callback != nullptr) {
                WallClockNotifier::Instance().Update(this, _interval);
            }

            _adminLock.Unlock();
        }
        uint32_t Interval() const override
        {
            return (_interval);
        }
        virtual uint64_t Now() const override
        {
            // Is a simple implementation but just return the crrent time in ticks...
            return Core::Time::Now().Ticks();
        }

    private:
        void Callback() {

            // This is called in a multithreaded environment. This is not Javascript :-)
            // So lets make sure that operations on the NotifierList are atomic.
            _adminLock.Lock();

            if (_callback != nullptr) {
                _callback->Elapsed(_interval);
            }

            _adminLock.Unlock();
        }

    private:
        Core::CriticalSection _adminLock;
        Exchange::IWallClock::ICallback* _callback;
        uint32_t _interval;
    };

public:
    COMServer() = delete;
    COMServer(const COMServer&) = delete;
    COMServer& operator=(const COMServer&) = delete;

    COMServer(
        const Core::NodeId& source,
        * parentInterface,
        const Core::ProxyType<RPC::InvokeServer>& engine)
        : RPC::Communicator(source, _T(""), Core::ProxyType<Core::IIPCServer>(engine))
        , _parentInterface(parentInterface)
    {
        engine->Announcements(Announcement());
        Open(Core::infinite);
    }
    ~COMServer()
    {
        Close(Core::infinite);
    }

private:
    virtual void* Aquire(const string& className, const uint32_t interfaceId, const uint32_t versionId)
    {
        void* result = nullptr;

        // Currently we only support version 1 of the IRPCLink :-)
        if (((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) && ((interfaceId == ::OCDM::IAccessorOCDM::ID) || (interfaceId == Core::IUnknown::ID))) {
            // Reference count our parent
            _parentInterface->AddRef();
            TRACE(Trace::Information, ("OCDM interface aquired => %p", this));
            // Allright, respond with the interface.
            result = _parentInterface;
        }
        return (result);
    }

private:
    ::OCDM::IAccessorOCDM* _parentInterface;
};


int main(int argc, char* argv[])
{
    // The core::NodeId can hold an IPv4, IPv6, domain, HCI, L2CAP or netlink address
    // Here we create a domain socket address
    #ifdef __WINDOWS__
    Thunder::Core::NodeId nodeId("127.0.0.1:63000");
    #else
    Thunder::Core::NodeId nodeId("/tmp/comserver");
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
