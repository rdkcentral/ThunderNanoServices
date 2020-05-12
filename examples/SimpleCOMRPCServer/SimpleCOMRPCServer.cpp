/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the License);
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

#include <core/core.h>
#include <com/com.h>
#include "../SimpleCOMRPCInterface/ISimpleCOMRPCInterface.h"

using namespace WPEFramework;

class COMServer : public RPC::Communicator {
private:
    class Implementation : public Exchange::IWallClock {
    private:
        class WallClockNotifier {
        private:
            typedef std::list< std::pair<uint32_t, Implementation*> > NotifierList;

            class TimeHandler {
            public:
                TimeHandler()
                    : _callback(nullptr)
                {
                }
                TimeHandler(Implementation* callback)
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
                    ASSERT(_callback != nullptr);
                    return (_callback->Trigger(scheduledTime));
                }

            private:
                Implementation* _callback;
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
                TimeHandler entry(sink);
                // This is called in a multithreaded environment. This is not Javascript :-)
                // So lets make sure that operations on the NotifierList are atomic.
                _adminLock.Lock();

                _timer.Revoke(entry);

                _timer.Schedule(Core::Time::Now().Add(sleepTime), entry);

                _adminLock.Unlock();
            }
            void Revoke(Implementation* sink)
            {
                // This is called in a multithreaded environment. This is not Javascript :-)
                // So lets make sure that operations on the NotifierList are atomic.
                _adminLock.Lock();

                _timer.Revoke(TimeHandler(sink));

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
            , _callback(nullptr)
            , _interval(~0) {

        }
        ~Implementation() override {
        }

    public:
        uint32_t Callback(ICallback* callback) override
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
        void Interval(const uint16_t interval) override
        {
            _interval = interval;

            // This is called in a multithreaded environment. This is not Javascript :-)
            // So lets make sure that operations on the NotifierList are atomic.
            _adminLock.Lock();

            if (_callback != nullptr) {
                WallClockNotifier::Instance().Update(this, ( _interval * 1000) );
            }

            _adminLock.Unlock();
        }
        uint16_t Interval() const override
        {
            return (_interval);
        }
        uint64_t Now() const override
        {
            // Is a simple implementation but just return the crrent time in ticks...
            return Core::Time::Now().Ticks();
        }

        BEGIN_INTERFACE_MAP(Implementation)
            INTERFACE_ENTRY(Exchange::IWallClock)
        END_INTERFACE_MAP

    private:
        uint64_t Trigger(const uint64_t scheduleTime) {

            uint64_t result = Core::Time(scheduleTime).Add(_interval * 1000).Ticks();

            // This is called in a multithreaded environment. This is not Javascript :-)
            // So lets make sure that operations on the NotifierList are atomic.
            _adminLock.Lock();

            // If the callback has been set, it is time to trigger the sink on the 
            // otherside to tell the that there set time has elapsed.
            if (_callback != nullptr) {

                // This, under the hood is the actual callback over the COMRPC channel to the other side
                _callback->Elapsed(_interval);
            }

            // Done, safe to now have the _callback reset.
            _adminLock.Unlock();

            return (result);
        }

    private:
        Core::CriticalSection _adminLock;
        Exchange::IWallClock::ICallback* _callback;
        uint16_t _interval;
    };

public:
    COMServer() = delete;
    COMServer(const COMServer&) = delete;
    COMServer& operator=(const COMServer&) = delete;

    COMServer(
        const Core::NodeId& source,
        const string& proxyServerPath,
        const Core::ProxyType< RPC::InvokeServerType<1, 0, 4> >& engine)
        : RPC::Communicator(
            source, 
            proxyServerPath, 
            Core::proxy_cast<Core::IIPCServer>(engine))
        , _remoteEntry(nullptr)
    {
        // Once the socket is opened the first exchange between client and server is an 
        // announce message. This announce message hold information the otherside requires
        // like, where can I find the ProxyStubs that I need to load, what Trace categories
        // need to be enabled.
        // Extensibility allows to be "in the middle" of these messages and to chose on 
        // which thread this message should be executes. Since the message is coming in 
        // over socket, the announce message could be handled on the communication thread
        // or better, if possible, it can be run on the thread of the engine we have just 
        // created.
        engine->Announcements(Announcement());

        Open(Core::infinite);
    }
    ~COMServer() override
    {
        Close(Core::infinite);
    }

private:
    void* Aquire(const string& className, const uint32_t interfaceId, const uint32_t versionId) override
    {
        void* result = nullptr;

        // Currently we only support version 1 of the IRPCLink :-)
        if ((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) {
            
            if (interfaceId == ::Exchange::IWallClock::ID) {

                // Allright, request a new object that implements the requested interface.
                result = Core::Service<Implementation>::Create<Exchange::IWallClock>();
            }
            else if (interfaceId == Core::IUnknown::ID) {

                // Allright, request a new object that implements the requested interface.
                result = Core::Service<Implementation>::Create<Core::IUnknown>();
            }
        }
        return (result);
    }
    void Offer(Core::IUnknown* remote, const uint32_t interfaceId) override
    {
        if (interfaceId == Exchange::IMath::ID) {
            if (_remoteEntry == nullptr) {
                printf("From now on we can do calculations, an IMath is offered!\n");
            }
            else {
                printf("Dropping the IMath interfae we were offered. A new ne is offered!\n");
                _remoteEntry->Release();
            }
            _remoteEntry = remote->QueryInterface<Exchange::IMath>();
        }
    }
    // note: do NOT do a QueryInterface on the IUnknown pointer (or any other method for that matter), the object it points to might already be destroyed
    void Revoke(const Core::IUnknown* remote, const uint32_t interfaceId) override
    {
        if ((interfaceId == Exchange::IMath::ID) && (remote == _remoteEntry)) {
            // Seems we need to drop it..
            printf("Dropping the IMath interfae we were offered. It is revoked!\n");
            _remoteEntry->Release();
            _remoteEntry = nullptr;
        }
    }
private:
    Exchange::IMath* _remoteEntry;
};

bool ParseOptions(int argc, char** argv, Core::NodeId& comChannel, string& psPath)
{
    int index = 1;
    bool showHelp = false;
    comChannel = Core::NodeId(Exchange::SimpleTestAddress);
    psPath = _T(".");

    while ((index < argc) && (!showHelp)) {
        if (strcmp(argv[index], "-listen") == 0) {
            comChannel = Core::NodeId(argv[index + 1]);
            index++;
        }
        else if (strcmp(argv[index], "-path") == 0) {
            psPath = argv[index + 1];
            index++;
        }
        else if (strcmp(argv[index], "-h") == 0) {
            showHelp = true;
        }
        index++;
    }

    return (showHelp);
}


int main(int argc, char* argv[])
{
    // The core::NodeId can hold an IPv4, IPv6, domain, HCI, L2CAP or netlink address
    // Here we create a domain socket address
    Core::NodeId comChannel;
    string psPath;

    printf("\npierre is old and builds a COMServer for funs and giggles :-)\n");

    if (ParseOptions(argc, argv, comChannel, psPath) == true) {
        printf("Options:\n");
        printf("-listen <IP/FQDN>:<port> [default: %s]\n", Exchange::SimpleTestAddress);
        printf("-path <Path to the location of the ProxyStubs> [default: .]\n");
        printf("-h This text\n\n");
    }
    else
    {
        int element;
        COMServer server(comChannel, psPath, Core::ProxyType< RPC::InvokeServerType<1, 0, 4> >::Create());
        printf("Channel:        %s:[%d]\n", comChannel.HostAddress().c_str(), comChannel.PortNumber());
        printf("ProxyStub path: %s\n\n", psPath.c_str());

        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
            case 'Q': break;
            default: break;
            }

        } while (element != 'Q');
    }

    Core::Singleton::Dispose();

    return 0;
}
