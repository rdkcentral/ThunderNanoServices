#include "Module.h"
#include "Wayland.h"
#include "../Client/Client.h"
#include <interfaces/IResourceCenter.h>
#include <interfaces/IComposition.h>

#include <virtualinput/VirtualKeyboard.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {
    /* -------------------------------------------------------------------------------------------------------------
     * This is a singleton. Declare all C accessors to this object here
     * ------------------------------------------------------------------------------------------------------------- */
    class PlatformImplementation;

    static PlatformImplementation* _implementation = nullptr;
    static Core::CriticalSection _resourceClientsLock;
    static Core::CriticalSection _compositionClientsLock;

    /* -------------------------------------------------------------------------------------------------------------
     *
     * ------------------------------------------------------------------------------------------------------------- */
    class PlatformImplementation : public Exchange::IResourceCenter,
                                   public Exchange::IComposition,
                                   public Wayland::Display::IProcess {
    private:
        PlatformImplementation(const PlatformImplementation&) = delete;
        PlatformImplementation& operator=(const PlatformImplementation&) = delete;

        /* -------------------------------------------------------------------------------------------------------------
         *  Get some activity in our Display client
         * ------------------------------------------------------------------------------------------------------------- */
        class Job : public Core::Thread {
        private:
            Job() = delete;
            Job(const Job&) = delete;
            Job& operator=(const Job&) = delete;

        public:
            Job(PlatformImplementation& parent)
                : _parent(parent)
            {
            }
            virtual ~Job()
            {
            }

        public:
            uint32_t Worker()
            {
                Block();
                _parent.Process();
                return (Core::infinite);
            }

        private:
            PlatformImplementation& _parent;
        };

        /* -------------------------------------------------------------------------------------------------------------
         *  Client definition info
         * ------------------------------------------------------------------------------------------------------------- */
        class Entry : public Exchange::IComposition::IClient {
        private:
            Entry() = delete;
            Entry(const Entry&) = delete;
            Entry& operator=(const Entry&) = delete;

        protected:
            Entry(Wayland::Display::Surface* surface, Implementation::IServer* server)
                : _surface(*surface)
                , _server(server)
            {
                ASSERT(surface != nullptr);
                ASSERT(server != nullptr);
            }

        public:
            static Entry* Create(Implementation::IServer* server, Wayland::Display* display, const uint32_t id)
            {
                Wayland::Display::Surface surface;
                Entry* result(nullptr);

                display->Get(id, surface);


                if (surface.IsValid() == true) {
                    result = Core::Service<Entry>::Create<Entry>(&surface, server);
                }

                return (result);
            }
            virtual ~Entry()
            {
            }

        public:
            inline uint32_t Id() const
            {
                return _surface.Id();
            }
            inline bool IsActive() const
            {
                return _surface.IsValid();
            }
            virtual string Name() const override
            {
                return _surface.Name();
            }
            virtual void Kill() override
            {
                //TODO: to be implemented.
            }
            virtual void Opacity(const uint32_t value) override
            {
                _surface.Opacity(value);
            }
            virtual void Geometry(const uint32_t X, const uint32_t Y, const uint32_t width, const uint32_t height)
            {
                _surface.Resize(X, Y, width, height);
            }
            virtual void Visible(const bool visible)
            {
                _surface.Visibility(visible);
            }
            virtual void SetTop()
            {
                _surface.SetTop();
            }
            virtual void SetInput()
            {
                if (_server != nullptr) {
                    _server->SetInput(_surface.Name().c_str());
                }
            }

            BEGIN_INTERFACE_MAP(Entry)
            INTERFACE_ENTRY(Exchange::IComposition::IClient)
            END_INTERFACE_MAP

        private:
            Wayland::Display::Surface _surface;
            Implementation::IServer* _server;
        };

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , Display("wayland-0")
            {
                Add(_T("display"), &Display);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Display;
        };

        class Sink : public Wayland::Display::ICallback {
        private:
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;
            Sink() = delete;

        public:
            Sink(PlatformImplementation& parent)
                : _parent(parent)
            {
            }

            // ICalllback methods
            virtual void Attached(const uint32_t id)
            {
                _parent.Add(id);
            }

            virtual void Detached(const uint32_t id)
            {
                _parent.Remove(id);
            }

        private:
            PlatformImplementation& _parent;
        };

    public:
        PlatformImplementation()
            : _config()
            , _hardwarestate(Exchange::IResourceCenter::UNITIALIZED)
            , _resourceClients()
            , _compositionClients()
            , _server(nullptr)
            , _controller(nullptr)
            , _job(*this)
            , _sink(*this)
            , _surface(nullptr)
            , _service()
        {
            // Register an @Exit, in case we are killed, with an incorrect ref count !!
            if (atexit(CloseDown) != 0) {
                TRACE(Trace::Information, (("Could not register @exit handler. Error: %d."), errno));
                exit(EXIT_FAILURE);
            }

            // Wayland can only be instantiated once (it is a process wide singleton !!!!)
            ASSERT(_implementation == nullptr);

            _identifier[0] = 0;
            _implementation = this;
        }

        ~PlatformImplementation()
        {
            TRACE(Trace::Information, (_T("Stopping Wayland")));

            if (_surface != nullptr) {
                delete _surface;
            }

            if (_server != nullptr)
                delete _server;

            if (_controller != nullptr)
                // Exit Wayland loop
                _controller->Signal();
            delete _controller;

            if (_service != nullptr)
                _service->Release();
        }

    public:
        BEGIN_INTERFACE_MAP(PlatformImplementation)
        INTERFACE_ENTRY(Exchange::IResourceCenter)
        INTERFACE_ENTRY(Exchange::IComposition)
        END_INTERFACE_MAP

    public:
        /* virtual */ uint32_t Configure(PluginHost::IShell* service)
        {
            _service = service;
            _service->AddRef();

            _config.FromString(_service->ConfigLine());

            ASSERT(_server == nullptr);

            // As there are currently two flavors of the potential Wayland Compositor Implementations, e.g.
            // Westeros and Weston, we do not want to decide here which one we instantiate. We have given both
            // implemntations a "generic" handle that starts the compositor. Here we just instantiate the
            // one that is build together with this so, which might be westeros (if Westeros.cpp is compiled and
            // linked with this so) or weston, if someone creates, in the near future a Westonr.cpp to be
            // compiled with this so, in stead of the Westeros.cpp :-)
            _server = Implementation::Create(_service->ConfigLine());

            ASSERT(_server != nullptr);

            if (_server != nullptr) {
                // We need a Wayland client to be connected to the Wayland server. This is handled by the C++
                // wrapper layer, found in the Client directory. Her we instantiate the wayland C++ compositor
                // abstraction, to create/request a client from this abstraction.
                // This C++ wayland Compositor abstraction instance, will in-fact, call the server, we just
                // instantiated a few lines above.
                _controller = &(Wayland::Display::Instance(_config.Display.Value()));
                // OK ready to receive new connecting surfaces.
                _controller->Callback(&_sink);
                // We also want to be know the current surfaces.
                _controller->LoadSurfaces();
                // Firing up the compositor controllerr.
                _job.Run();

                TRACE(Trace::Information, (_T("Compositor initialized")));
                _hardwarestate = Exchange::IResourceCenter::OPERATIONAL;
                Notify();
                SetEventPlatformReady();
            }

            return ((_server != nullptr) && (_controller != nullptr)) ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE;
        }
        void SetEventPlatformReady()
        {
            // The platform appears to be ready, set the event
            PluginHost::ISubSystem* systemInfo = _service->QueryInterfaceByCallsign<PluginHost::ISubSystem>(string());

            ASSERT(systemInfo != nullptr);

            if ((systemInfo != nullptr) && (systemInfo->IsActive(PluginHost::ISubSystem::PLATFORM) == false)) {
                systemInfo->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
            }

            if (systemInfo != nullptr) {
                systemInfo->Release();
            }
        }


        // -------------------------------------------------------------------------------------------------------
        //   IResourceCenter methods
        // -------------------------------------------------------------------------------------------------------
        /* virtual */ void Register(Exchange::IResourceCenter::INotification* notification)
        {
            // Do not double register a notification sink.
            _resourceClientsLock.Lock();
            ASSERT(std::find(_resourceClients.begin(), _resourceClients.end(), notification) == _resourceClients.end());

            notification->AddRef();

            _resourceClients.push_back(notification);

            notification->StateChange(_hardwarestate);
            _resourceClientsLock.Unlock();
        }
        /* virtual */ void Unregister(Exchange::IResourceCenter::INotification* notification)
        {
            _resourceClientsLock.Lock();
            std::list<Exchange::IResourceCenter::INotification*>::iterator index(std::find(_resourceClients.begin(), _resourceClients.end(), notification));

            // Do not un-register something you did not register.
            ASSERT(index != _resourceClients.end());

            notification->Release();

            _resourceClients.erase(index);
            _resourceClientsLock.Unlock();
        }
        /* virtual */ Exchange::IResourceCenter::hardware_state HardwareState() const
        {
            return (_hardwarestate);
        }
        /* virtual */ uint8_t Identifier(const uint8_t maxLength, uint8_t buffer[]) const
        {
            // TODO: For now use RawDeviceId, this needs to move or change in future.
            _resourceClientsLock.Lock();

            if (_identifier[0] == 0) {
                const uint8_t* id(Core::SystemInfo::Instance().RawDeviceId());

                _identifier[0] = ((sizeof(_identifier) - 1) < id[0] ? (sizeof(_identifier) - 1) : id[0]);
                memcpy(&(_identifier[1]), &id[1], _identifier[0]);
            }

            _resourceClientsLock.Unlock();

            ::memcpy(buffer, &_identifier[1], (maxLength >= _identifier[0] ? _identifier[0] : maxLength));
            return _identifier[0];
        }

        // -------------------------------------------------------------------------------------------------------
        //   IComposition methods
        // -------------------------------------------------------------------------------------------------------
        /* virtual */ void Register(Exchange::IComposition::INotification* notification)
        {
            // Do not double register a notification sink.
            _compositionClientsLock.Lock();
            ASSERT(std::find(_compositionClients.begin(), _compositionClients.end(), notification) == _compositionClients.end());

            notification->AddRef();

            _compositionClients.push_back(notification);

            std::list<Entry*>::iterator index(_clients.begin());

            while (index != _clients.end()) {

                if ((*index)->IsActive() == true) {
                    notification->Attached(*index);
                }
                index++;
            }
            _compositionClientsLock.Unlock();
        }
        /* virtual */ void Unregister(Exchange::IComposition::INotification* notification)
        {
            _compositionClientsLock.Lock();
            std::list<Exchange::IComposition::INotification*>::iterator index(std::find(_compositionClients.begin(), _compositionClients.end(), notification));

            // Do not un-register something you did not register.
            ASSERT(index != _compositionClients.end());

            if (index != _compositionClients.end()) {

                _compositionClients.erase(index);

                notification->Release();
            }
            _compositionClientsLock.Unlock();
        }

        /* virtual */ Exchange::IComposition::IClient* Client(const string& name)
        {
            Exchange::IComposition::IClient* result = nullptr;

            _compositionClientsLock.Lock();

            std::list<Entry*>::iterator index(_clients.begin());

            while ((index != _clients.end()) && ((*index)->Name() != name)) {
                index++;
            }

            if ((index != _clients.end()) && ((*index)->IsActive() == true)) {
                result = (*index);
                result->AddRef();
            }
            _compositionClientsLock.Unlock();

            return (result);
        }
        /* virtual */ Exchange::IComposition::IClient* Client(const uint8_t id)
        {
            uint8_t modifiableId = id;
            Exchange::IComposition::IClient* result = nullptr;

            _compositionClientsLock.Lock();
            std::list<Entry*>::iterator index(_clients.begin());
            while ((index != _clients.end()) && (modifiableId != 0)) {
                if ((*index)->IsActive() == true) {

                    modifiableId--;
                }
                index++;
            }
            if (index != _clients.end()) {
                result = (*index);
                result->AddRef();
            }
            _compositionClientsLock.Unlock();

            return (result);
        }

        // -------------------------------------------------------------------------------------------------------
        //   IProcess methods
        // -------------------------------------------------------------------------------------------------------
        /* virtual */ bool Dispatch()
        {
            printf("DISPATCH [%s:%d] %s.\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
            return (true);
        }

    private:
        void RecursiveList(std::list<Exchange::IResourceCenter::INotification*>::iterator& index)
        {
            Exchange::IResourceCenter::INotification* callee(*index);

            ASSERT(callee != nullptr);

            if (callee != nullptr) {
                callee->AddRef();

                if (++index != _resourceClients.end()) {
                    RecursiveList(index);
                }
                else {
                    _resourceClientsLock.Unlock();
                }

                callee->StateChange(_hardwarestate);
                callee->Release();
            }
        }
        void Notify()
        {
            _resourceClientsLock.Lock();
            if (_resourceClients.size() > 0) {
                std::list<Exchange::IResourceCenter::INotification*>::iterator index(_resourceClients.begin());
                RecursiveList(index);
            }
            else {
                _resourceClientsLock.Unlock();
            }
        }
        void Add(const uint32_t id)
        {
            std::list<Entry*>::iterator index(_clients.begin());

            while ((index != _clients.end()) && (id != (*index)->Id())) {
                index++;
            }

            if (index == _clients.end()) {
                Entry* entry(Entry::Create(_server, _controller, id));

                _clients.push_back(entry);
                TRACE(Trace::Information, (_T("Added client id[%d] name[%s]."), entry->Id(), entry->Name().c_str()));

                if (_compositionClients.size() > 0) {

                    std::list<Exchange::IComposition::INotification*>::iterator index(_compositionClients.begin());

                    while (index != _compositionClients.end()) {
                        (*index)->Attached(entry);
                        index++;
                    }
                }
            }
            else {
                printf("[%s:%d] %s Client surface id found I guess we should update the the entry\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
            }
        }
        void Remove(const uint32_t id)
        {
            std::list<Entry*>::iterator index(_clients.begin());

            while ((index != _clients.end()) && (id != (*index)->Id())) {
                index++;
            }

            if (index != _clients.end()) {
                Entry* entry(*index);

                _clients.erase(index);

                TRACE(Trace::Information, (_T("Removed client id[%d] name[%s]."), entry->Id(), entry->Name().c_str()));

                if (_compositionClients.size() > 0) {
                    std::list<Exchange::IComposition::INotification*>::iterator index(_compositionClients.begin());

                    while (index != _compositionClients.end()) {
                        (*index)->Detached(entry);
                        index++;
                    }
                }

                entry->Release();
            }
        }
        void Process()
        {
            printf("[%s:%d] Starting wayland loop.\n", __FILE__, __LINE__);
            _controller->Process(this);
            printf("[%s:%d] Exiting wayland loop.\n", __FILE__, __LINE__);
        }
        static void CloseDown()
        {
            // Make sure we get exclusive access to the Destruction of this Resource Center.
            _resourceClientsLock.Lock();

            // Seems we are destructed.....If we still have a pointer to the implementation, Kill it..
            if (_implementation != nullptr) {
                delete _implementation;
                _implementation = nullptr;
            }
            _resourceClientsLock.Unlock();
        }

    private:
        Config _config;
        hardware_state _hardwarestate;
        mutable uint8_t _identifier[16];
        std::list<Exchange::IResourceCenter::INotification*> _resourceClients;
        std::list<Exchange::IComposition::INotification*> _compositionClients;
        std::list<Entry*> _clients;
        Implementation::IServer* _server;
        Wayland::Display* _controller;
        Job _job;
        Sink _sink;
        Wayland::Display::Surface* _surface;
        PluginHost::IShell* _service;
    };

    SERVICE_REGISTRATION(PlatformImplementation, 1, 0);

} // namespace Plugin

} // namespace WPEFramework
