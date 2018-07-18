#include "Module.h"
#include "Wayland.h"
#include "../Client/Implementation.h"
#include <interfaces/IComposition.h>

#include <virtualinput/VirtualKeyboard.h>

#include "../NexusServer/NexusServer.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {
    /* -------------------------------------------------------------------------------------------------------------
     * This is a singleton. Declare all C accessors to this object here
     * ------------------------------------------------------------------------------------------------------------- */
    class CompositorImplementation;

    static CompositorImplementation* g_implementation = nullptr;
    static Core::CriticalSection g_implementationLock;

    class CompositorImplementation : public Exchange::IComposition,
                                     public Wayland::Display::IProcess {
    private:
        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

        /* -------------------------------------------------------------------------------------------------------------
         *  Get some activity in our Display client
         * ------------------------------------------------------------------------------------------------------------- */
        class Job : public Core::Thread {
        private:
            Job() = delete;
            Job(const Job&) = delete;
            Job& operator=(const Job&) = delete;

        public:
            Job(CompositorImplementation& parent)
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
            CompositorImplementation& _parent;
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
                , Join(false)
                , Display("wayland-0")
            {
                Add(_T("join"), &Join);
                Add(_T("display"), &Display);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::Boolean Join;
            Core::JSON::String Display;
        };

        class Sink : public Wayland::Display::ICallback, public Broadcom::Platform::IStateChange {
        private:
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;
            Sink() = delete;

        public:
            Sink(CompositorImplementation& parent)
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
            virtual void StateChange(Broadcom::Platform::server_state state)
            {
                if (state == Broadcom::Platform::OPERATIONAL) {
                    _parent.StartImplementation();
                }
            }

        private:
            CompositorImplementation& _parent;
        };

    public:
        CompositorImplementation()
            : _config()
            , _compositionClients()
            , _clients()
            , _server(nullptr)
            , _controller(nullptr)
            , _job(*this)
            , _sink(*this)
            , _surface(nullptr)
            , _service()
            , _nxserver(nullptr)
        {
            // Register an @Exit, in case we are killed, with an incorrect ref count !!
            if (atexit(CloseDown) != 0) {
                TRACE(Trace::Information, (_T("Could not register @exit handler. Error: %d.\n"), errno));
                exit(EXIT_FAILURE);
            }

            // make sure we have one compositor in the system
            ASSERT(g_implementation == nullptr);

            g_implementation = this;
        }

        ~CompositorImplementation() {
            TRACE(Trace::Information, (_T("Stopping Wayland\n")));

            if (_surface != nullptr) {
                delete _surface;
            }

            if (_server != nullptr) {
                delete _server;
            }

            if (_controller != nullptr) {
                // Exit Wayland loop
                _controller->Signal();
                delete _controller;
            }

            if (_nxserver != nullptr) {
                delete _nxserver;
            }

            if (_service != nullptr) {
                _service->Release();
            }
        }

    public:
        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(Exchange::IComposition)
        END_INTERFACE_MAP

    public:
        // -------------------------------------------------------------------------------------------------------
        //   IComposition methods
        // -------------------------------------------------------------------------------------------------------
        /* virtual */ uint32_t Configure(PluginHost::IShell* service)
        {
            _service = service;
            _service->AddRef();

            _config.FromString(_service->ConfigLine());

            ASSERT(_server == nullptr);

            // If we run on a broadcom platform first start the nxserver, then we start the compositor...
            ASSERT(_nxserver == nullptr);

            if (_nxserver != nullptr) {
                delete _nxserver;
            }

            if (_config.Join == false) {
                _nxserver = new Broadcom::Platform( &_sink, nullptr, _service->ConfigLine());
            }
            else {
                StartImplementation();
            }

            ASSERT(_nxserver != nullptr);

            return (((_nxserver != nullptr) || (_server != nullptr)) ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }
        /* virtual */ void Register(Exchange::IComposition::INotification* notification)
        {
            // Do not double register a notification sink.
            g_implementationLock.Lock();
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
            g_implementationLock.Unlock();
        }
        /* virtual */ void Unregister(Exchange::IComposition::INotification* notification)
        {
            g_implementationLock.Lock();
            std::list<Exchange::IComposition::INotification*>::iterator index(std::find(_compositionClients.begin(), _compositionClients.end(), notification));

            // Do not un-register something you did not register.
            ASSERT(index != _compositionClients.end());

            if (index != _compositionClients.end()) {

                _compositionClients.erase(index);

                notification->Release();
            }
            g_implementationLock.Unlock();
        }
        /* virtual */ Exchange::IComposition::IClient* Client(const string& name)
        {
            Exchange::IComposition::IClient* result = nullptr;

            g_implementationLock.Lock();

            std::list<Entry*>::iterator index(_clients.begin());

            while ((index != _clients.end()) && ((*index)->Name() != name)) {
                index++;
            }

            if ((index != _clients.end()) && ((*index)->IsActive() == true)) {
                result = (*index);
                result->AddRef();
            }
            g_implementationLock.Unlock();

            return (result);
        }
        /* virtual */ Exchange::IComposition::IClient* Client(const uint8_t id)
        {
            uint8_t modifiableId = id;
            Exchange::IComposition::IClient* result = nullptr;

            g_implementationLock.Lock();
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
            g_implementationLock.Unlock();

            return (result);
        }
        /* virtual */ void SetResolution(const Exchange::IComposition::ScreenResolution format)
        {
            /*TODO: TO BE IMPLEMENTED*/
        }
        /* virtual */ const ScreenResolution GetResolution()
        {
            /*TODO: TO BE IMPLEMENTED*/
            return Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown;
        }

        // -------------------------------------------------------------------------------------------------------
        //   IProcess methods
        // -------------------------------------------------------------------------------------------------------
        /* virtual */ bool Dispatch()
        {
            return (true);
        }

    private:
        void Add(const uint32_t id)
        {
            std::list<Entry*>::iterator index(_clients.begin());

            while ((index != _clients.end()) && (id != (*index)->Id())) {
                index++;
            }

            if (index == _clients.end()) {
                Entry* entry(Entry::Create(_server, _controller, id));

                _clients.push_back(entry);
                TRACE(Trace::Information, (_T("Added client id[%d] name[%s].\n"), entry->Id(), entry->Name().c_str()));

                if (_compositionClients.size() > 0) {

                    std::list<Exchange::IComposition::INotification*>::iterator index(_compositionClients.begin());

                    while (index != _compositionClients.end()) {
                        (*index)->Attached(entry);
                        index++;
                    }
                }
            }
            else {
                TRACE(Trace::Information, (_T("[%s:%d] %s Client surface id found I guess we should update the the entry\n"), __FILE__, __LINE__, __PRETTY_FUNCTION__));
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

                TRACE(Trace::Information, (_T("Removed client id[%d] name[%s].\n"), entry->Id(), entry->Name().c_str()));

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
            TRACE(Trace::Information, (_T("[%s:%d] Starting wayland loop.\n"), __FILE__, __LINE__));
            _controller->Process(this);
            TRACE(Trace::Information, (_T("[%s:%d] Exiting wayland loop.\n"), __FILE__, __LINE__));
        }
        static void CloseDown()
        {
            // Make sure we get exclusive access to the Destruction of this Resource Center.
            g_implementationLock.Lock();

            // Seems we are destructed.....If we still have a pointer to the implementation, Kill it..
            if (g_implementation != nullptr) {
                delete g_implementation;
                g_implementation = nullptr;
            }
            g_implementationLock.Unlock();
        }
        void StartImplementation()
        {
            ASSERT(_service != nullptr);

            PluginHost::ISubSystem* subSystems = _service->SubSystems();

            ASSERT (subSystems != nullptr);

            if (subSystems != nullptr) {
                subSystems->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
            }

            // As there are currently two flavors of the potential Wayland Compositor Implementations, e.g.
            // Westeros and Weston, we do not want to decide here which one we instantiate. We have given both
            // implemntations a "generic" handle that starts the compositor. Here we just instantiate the
            // one that is build together with this so, which might be westeros (if Westeros.cpp is compiled and
            // linked with this so) or weston, if someone creates, in the near future a Westonr.cpp to be
            // compiled with this so, in stead of the Westeros.cpp :-)
            _server = Implementation::Create(_service);

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

                TRACE(Trace::Information, (_T("Compositor initialized\n")));

                if (subSystems != nullptr) {
                    // Set Graphics event. We need to set up a handler for this at a later moment
                    subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
                }
            }

            if (subSystems != nullptr) {
                subSystems->Release();
            }
        }

    private:
        Config _config;
        std::list<Exchange::IComposition::INotification*> _compositionClients;
        std::list<Entry*> _clients;
        Implementation::IServer* _server;
        Wayland::Display* _controller;
        Job _job;
        Sink _sink;
        Wayland::Display::Surface* _surface;
        PluginHost::IShell* _service;
        Broadcom::Platform* _nxserver;
    };

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

} // namespace Plugin

} // namespace WPEFramework
