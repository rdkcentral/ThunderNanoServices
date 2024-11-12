/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#define MODULE_NAME SimpleCOMRPCServer

#include <core/core.h>
#include <com/com.h>
#include "../SimpleCOMRPCInterface/ISimpleCOMRPCInterface.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE);

using namespace Thunder;

class COMServer : public RPC::Communicator {
private:
    class Implementation : public Exchange::IWallClock {
    private:
        class WallClockNotifier : public Core::Thread {
        private:
            class Notifier {
            public:
                Notifier() = delete;
                Notifier(const Notifier&) = delete;

                Notifier(const uint16_t period, Exchange::IWallClock::ICallback* callback)
                    : _period(period)
                    , _ticks(Core::Time::Now().Add(_period * 1000).Ticks())
                    , _callback(_callback) {
                    _callback->AddRef();
                }
                ~Notifier() {
                    _callback->Release();
                }

            public:
                bool operator== (const Exchange::IWallClock::ICallback* callback) const {
                    return (callback == _callback);
                }
                bool operator!= (const Exchange::IWallClock::ICallback* callback) const {
                    return (!operator==(callback));
                }
                uint64_t NextSlot(const uint64_t currentSlot) {
                    if (currentSlot >= _ticks) {
                        // Seems we need to execute..
                        _period = _callback->Elapsed(_period);

                        _ticks = (_period != 0 ? (_ticks + ((_period * 1000) * Core::Time::TicksPerMillisecond)) : ~0);
                    }
                    return (_ticks);
                }

            private:
                uint16_t _period;
                uint64_t _ticks;
                Exchange::IWallClock::ICallback* _callback;
            };
            using NotifierList = std::list<Notifier>;

        public:
            WallClockNotifier(const WallClockNotifier&) = delete;
            WallClockNotifier& operator=(const WallClockNotifier&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            WallClockNotifier()
                : Core::Thread(Core::Thread::DefaultStackSize(), _T("WallClockNotifier")) {
            }
POP_WARNING()
            ~WallClockNotifier() = default;

        public:
            uint32_t Arm(const uint16_t seconds, Exchange::IWallClock::ICallback* callback)
            {
                uint32_t result = Core::ERROR_ALREADY_CONNECTED;

                _adminLock.Lock();

                NotifierList::iterator index = std::find(_notifiers.begin(), _notifiers.end(), callback);

                if (index == _notifiers.end()) {
                    _notifiers.emplace_front(seconds, callback);
                    result = Core::ERROR_NONE;
                }

                _adminLock.Unlock();

                Core::Thread::Run();

                return (result);
            }
            uint32_t Disarm(const Exchange::IWallClock::ICallback* callback)
            {
                uint32_t result = Core::ERROR_NOT_EXIST;

                _adminLock.Lock();

                NotifierList::iterator index = std::find(_notifiers.begin(), _notifiers.end(), callback);

                if (index == _notifiers.end()) {
                    _adminLock.Unlock();
                }
                else {
                    _notifiers.erase(index);

                    _adminLock.Unlock();

                    Core::Thread::Run();

                    result = Core::ERROR_NONE;
                }

                return (result);
            }

        private:
            uint32_t Worker() override {
                uint32_t duration = Core::infinite;

                _adminLock.Lock();

                Core::Thread::Block();

                uint64_t nextSlot;
                uint64_t timeSlot = Core::Time::Now().Ticks();

                do {
                    NotifierList::iterator index = _notifiers.begin();

                    nextSlot = ~0;

                    while (index != _notifiers.end()) {
                        uint64_t thisSlot = index->NextSlot(timeSlot);

                        if (thisSlot == static_cast<uint64_t>(~0)) {
                            index = _notifiers.erase(index);
                        }
                        else if (thisSlot >= nextSlot) {
                            index++;
                        }
                        else {
                            index++;
                            nextSlot = thisSlot;
                        }
                    }

                    timeSlot = Core::Time::Now().Ticks();

                } while (nextSlot < timeSlot);

                if (nextSlot != static_cast<uint64_t>(~0)) {
                    duration = static_cast<uint32_t>((nextSlot - timeSlot) / Core::Time::TicksPerMillisecond);
                }

                _adminLock.Unlock();

                return (duration);
            }

        private:
            Core::CriticalSection _adminLock;
            NotifierList _notifiers;
        };

    public:
        Implementation(const Implementation&) = delete;
        Implementation& operator= (const Implementation&) = delete;

        Implementation() 
            : _notifier() {

        }
        ~Implementation() override = default;

    public:
        uint32_t Arm (const uint16_t seconds, Exchange::IWallClock::ICallback* callback) override
        {
            return (_notifier.Arm(seconds, callback));
        }
        uint32_t Disarm(const Exchange::IWallClock::ICallback* callback) override
        {
            return (_notifier.Disarm(callback));
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
        WallClockNotifier _notifier;
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
            Core::ProxyType<Core::IIPCServer>(engine),
            _T("@SimpleComServer"))
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
        Open(Core::infinite);
    }
    ~COMServer() override
    {
        Close(Core::infinite);
    }

private:
    void* Acquire(const string& className, const uint32_t interfaceId, const uint32_t versionId) override
    {
        void* result = nullptr;

        // Currently we only support version 1 of the IRPCLink :-)
        if ((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) {
            
            if (interfaceId == ::Exchange::IWallClock::ID) {

                // Allright, request a new object that implements the requested interface.
                result = Core::ServiceType<Implementation>::Create<Exchange::IWallClock>();
            }
            else if (interfaceId == Core::IUnknown::ID) {

                // Allright, request a new object that implements the requested interface.
                result = Core::ServiceType<Implementation>::Create<Core::IUnknown>();
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
    psPath = _T("./PS");

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
        printf("-path <Path to the location of the ProxyStubs> [default: ./PS]\n");
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
