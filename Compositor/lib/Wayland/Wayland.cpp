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
 
#include "Wayland.h"
#ifdef ENABLE_NXSERVER
#include "NexusServer/Settings.h"
#endif

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
                                     public Exchange::IInputSwitch,
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
                , _rectangle( {0, 0, surface->Width(), surface->Height() } )
            {
                ASSERT(surface != nullptr);
                ASSERT(server != nullptr);
            }

        public:
            static Entry* Create(Implementation::IServer* server, const uint32_t id)
            {
                Wayland::Display::Surface surface;
                Entry* result(nullptr);

                server->Get(id, surface);

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
            string Name() const override
            {
                return _surface.Name();
            }
            void Opacity(const uint32_t value) override
            {
                if ((value == Exchange::IComposition::minOpacity) || (value == Exchange::IComposition::maxOpacity)) {
                    _surface.Visibility(value == Exchange::IComposition::maxOpacity);
                }
                else
                {  
                    _surface.Opacity(value);
                }
            }
            uint32_t Geometry(const Exchange::IComposition::Rectangle& rectangle) override 
            {
                _rectangle = rectangle;
                _surface.Resize(rectangle.x, rectangle.y, rectangle.width, rectangle.height);

                return (Core::ERROR_NONE);
            }
            Exchange::IComposition::Rectangle Geometry() const override 
            {
                return (_rectangle);
            }
            uint32_t ZOrder(const uint16_t index) override
            {
                _layer = index;
                _surface.ZOrder(index);

                return (Core::ERROR_NONE);
            }
            uint32_t ZOrder() const override
            {
                return (_layer);
            }
            BEGIN_INTERFACE_MAP(Entry)
                INTERFACE_ENTRY(Exchange::IComposition::IClient)
            END_INTERFACE_MAP

        private:
            Wayland::Display::Surface _surface;
            Implementation::IServer* _server;
            Exchange::IComposition::Rectangle _rectangle;
            uint16_t _layer;
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
                , Resolution(Exchange::IComposition::ScreenResolution::ScreenResolution_720p)
            {
                Add(_T("join"), &Join);
                Add(_T("display"), &Display);
                Add(_T("resolution"), &Resolution);

            }
            ~Config()
            {
            }

        public:
            Core::JSON::Boolean Join;
            Core::JSON::String Display;
            Core::JSON::EnumType<Exchange::IComposition::ScreenResolution> Resolution;
        };

        class Sink : public Wayland::Display::ICallback
#ifdef ENABLE_NXSERVER
            , public Broadcom::Platform::IStateChange
#endif
        {
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
#ifdef ENABLE_NXSERVER
            virtual void StateChange(Broadcom::Platform::server_state state)
            {
                if (state == Broadcom::Platform::OPERATIONAL) {
                    _parent.StartImplementation();
                }
            }
#endif

        private:
            CompositorImplementation& _parent;
        };

    public:
        CompositorImplementation()
            : _config()
            , _compositionClients()
            , _clients()
            , _server(nullptr)
            , _job(*this)
            , _sink(*this)
            , _surface(nullptr)
            , _service()
#ifdef ENABLE_NXSERVER
            , _nxserver(nullptr)
#endif
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

        ~CompositorImplementation()
        {
            TRACE(Trace::Information, (_T("Stopping Wayland\n")));

            if (_surface != nullptr) {
                delete _surface;
            }

            if (_server != nullptr) {
                delete _server;
            }

#ifdef ENABLE_NXSERVER
            if (_nxserver != nullptr) {
                delete _nxserver;
            }
#endif
            if (_service != nullptr) {
                _service->Release();
            }
            g_implementation = nullptr;
        }

    public:
        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(Exchange::IComposition)
        INTERFACE_ENTRY(Exchange::IInputSwitch)
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

#ifdef ENABLE_NXSERVER

#ifdef V3D_DRM_DISABLE
            ::setenv("V3D_DRM_DISABLE", "1", 1);
#endif
            // If we run on a broadcom platform first start the nxserver, then we start the compositor...
            ASSERT(_nxserver == nullptr);

            if (_nxserver != nullptr) {
                delete _nxserver;
            }

            if (_config.Join == false) {
                Config info;
                info.FromString(_service->ConfigLine());

                NEXUS_VideoFormat format = NEXUS_VideoFormat_eUnknown;
                if (info.Resolution.IsSet() == true) {
                    const auto index(formatLookup.find(info.Resolution.Value()));

                    if ((index != formatLookup.cend()) && (index->second != NEXUS_VideoFormat_eUnknown)) {
                        format = index->second;
                    }
                }
                _nxserver = new Broadcom::Platform(&_sink, nullptr, _service->ConfigLine(), format);
            } else {
                StartImplementation();
            }

            ASSERT(_nxserver != nullptr);
            return (((_nxserver != nullptr) || (_server != nullptr)) ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
#else
            StartImplementation();
            return ((_server != nullptr) ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
#endif
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
                    notification->Attached((*index)->Name(), *index);
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

        /* virtual */ uint32_t Resolution(const Exchange::IComposition::ScreenResolution format) override
        {
            return (Implementation::SetResolution(format));
        }

        /* virtual */ Exchange::IComposition::ScreenResolution Resolution() const override
        {
            return (Implementation::GetResolution());
        }




        // -------------------------------------------------------------------------------------------------------
        //   IProcess methods
        // -------------------------------------------------------------------------------------------------------
        /* virtual */ bool Dispatch()
        {
            return (true);
        }
        RPC::IStringIterator* Consumers() const override
        {
            std::list<string> container;
            std::list<Entry*>::const_iterator index(_clients.begin());
            while (index != _clients.end()) {
                container.push_back((*index)->Name());
                index++;
            }
            return (Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(container));
        }
        bool Consumer(const string& name) const override
        {
            return true;
        }
        uint32_t Consumer(const string& name, const mode) override
        {
            return (Core::ERROR_UNAVAILABLE);
        }
        uint32_t Select(const string& callsign) override
        {
            uint32_t status = Core::ERROR_UNKNOWN_KEY;
            std::list<Entry*>::iterator index(_clients.begin());
            while (index != _clients.end()) {
                if ((*index)->Name().find(callsign) == 0) {
                    _server->SetInput((*index)->Name().c_str());
                    status = Core::ERROR_NONE;
                }
                index++;
            }
            return status;
        }

    private:
        void Add(const uint32_t id)
        {
            std::list<Entry*>::iterator index(_clients.begin());

            while ((index != _clients.end()) && (id != (*index)->Id())) {
                index++;
            }

            if (index == _clients.end()) {
                Entry* entry(Entry::Create(_server, id));

                _clients.push_back(entry);
                TRACE(Trace::Information, (_T("Added client id[%d] name[%s].\n"), entry->Id(), entry->Name().c_str()));

                if (_compositionClients.size() > 0) {

                    std::list<Exchange::IComposition::INotification*>::iterator index(_compositionClients.begin());

                    while (index != _compositionClients.end()) {
                        (*index)->Attached(entry->Name(), entry);
                        index++;
                    }
                }
            } else {
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
                        (*index)->Detached(entry->Name());
                        index++;
                    }
                }

                entry->Release();
            }
        }
        void Process()
        {
            TRACE(Trace::Information, (_T("[%s:%d] Starting wayland loop.\n"), __FILE__, __LINE__));
            _server->Process(this);
            TRACE(Trace::Information, (_T("[%s:%d] Exiting wayland loop.\n"), __FILE__, __LINE__));
        }
        static void CloseDown()
        {
            // Seems we are destructed.....If we still have a pointer to the implementation, Kill it..
            if (g_implementation != nullptr) {
                delete g_implementation;
            }
        }
        void StartImplementation()
        {
            ASSERT(_service != nullptr);

            PluginHost::ISubSystem* subSystems = _service->SubSystems();

            ASSERT(subSystems != nullptr);

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

                if (_server->StartController(_config.Display.Value(), &_sink) == true) {
                   // Firing up the compositor controller.
                    _job.Run();

                    TRACE(Trace::Information, (_T("Compositor initialized\n")));
                    if (subSystems != nullptr) {
                        // Set Graphics event. We need to set up a handler for this at a later moment
                        subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
                    }
                } else {
                    delete _server;
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
        Job _job;
        Sink _sink;
        Wayland::Display::Surface* _surface;
        PluginHost::IShell* _service;
#ifdef ENABLE_NXSERVER
        Broadcom::Platform* _nxserver;
#endif
    };

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

} // namespace Plugin

} // namespace WPEFramework
