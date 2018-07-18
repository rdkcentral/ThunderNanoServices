#include <stdio.h>
#include <string.h>
#include "Module.h"

#include <interfaces/IComposition.h>
#include <NexusServer.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {

    /* -------------------------------------------------------------------------------------------------------------
     *
     * ------------------------------------------------------------------------------------------------------------- */
    class CompositorImplementation : public Exchange::IComposition {
    private:
        const std::map<const Exchange::IComposition::ScreenResolution, const NEXUS_VideoFormat > formatLookup = {
                { Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown, NEXUS_VideoFormat_eUnknown },
                { Exchange::IComposition::ScreenResolution::ScreenResolution_480i, NEXUS_VideoFormat_eNtsc },
                { Exchange::IComposition::ScreenResolution::ScreenResolution_480p, NEXUS_VideoFormat_e480p },
                { Exchange::IComposition::ScreenResolution::ScreenResolution_720p, NEXUS_VideoFormat_e720p },
                { Exchange::IComposition::ScreenResolution::ScreenResolution_720p50Hz, NEXUS_VideoFormat_e720p50hz },
                { Exchange::IComposition::ScreenResolution::ScreenResolution_1080p24Hz, NEXUS_VideoFormat_e1080p24hz },
                { Exchange::IComposition::ScreenResolution::ScreenResolution_1080i50Hz, NEXUS_VideoFormat_e1080i50hz },
                { Exchange::IComposition::ScreenResolution::ScreenResolution_1080p50Hz, NEXUS_VideoFormat_e1080p50hz },
                { Exchange::IComposition::ScreenResolution::ScreenResolution_1080p60Hz, NEXUS_VideoFormat_e1080p60hz }
        };

    private:
        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                    : Core::JSON::Container()
                    , HardwareDelay(0)
            {
                Add(_T("hardwareready"), &HardwareDelay);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt8 HardwareDelay;
        };

        class Sink : public Broadcom::Platform::IClient, public Broadcom::Platform::IStateChange {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

            class Postpone : public Core::Thread {
            private:
                Postpone() = delete;
                Postpone(const Postpone&) = delete;
                Postpone& operator= (const Postpone&) = delete;

            public:
                Postpone(Sink& parent, const uint16_t delay)
                    : _parent(parent)
                    , _trigger(false, true)
                    , _delay(delay * 1000) {
                }
                virtual ~Postpone() {
                    Block();
                    _trigger.SetEvent();
                }

            public:
                uint32_t Worker() {
                    if (_trigger.Lock(_delay) == Core::ERROR_TIMEDOUT) {
                        _parent.PlatformReady();
                    }
                    Stop();
                    return (Core::infinite);
                }

            private:
                Sink& _parent;
                Core::Event _trigger;
                uint32_t _delay;
            };

        public:
            explicit Sink(CompositorImplementation* parent)
                : _parent(*parent)
                , _delay(nullptr)
            {
                ASSERT(parent != nullptr);
            }
            ~Sink()
            {
                if (_delay != nullptr) {
                    delete _delay;
                }
            }

        public:
            inline void HardwareDelay(const uint16_t time) {
                ASSERT (_delay == nullptr);
                if ( (time != 0) && (_delay == nullptr) ) {
                    _delay = new Postpone(*this, time);
                }
            }

            // -------------------------------------------------------------------------------------------------------
            //   Broadcom::Platform::ICallback methods
            // -------------------------------------------------------------------------------------------------------
            /* virtual */ void Attached(Exchange::IComposition::IClient* client)
            {
                _parent.Add(client);

            }

            /* virtual */ void Detached(const char clientName[])
            {
                _parent.Remove(clientName);
            }

            /* virtual */ virtual void StateChange(Broadcom::Platform::server_state state)
            {
                if (state == Broadcom::Platform::OPERATIONAL){
                    if (_delay != nullptr) {
                        _delay->Run();
                    } 
                    else {
                        _parent.PlatformReady(); 
                    }
                }
            }

        private:
            inline void PlatformReady() {
                _parent.PlatformReady();
            }

        private:
            CompositorImplementation& _parent;
            Postpone* _delay;
        };

    public:
        CompositorImplementation()
            : _adminLock()
            , _service(nullptr)
            , _observers()
            , _clients()
            , _sink(this)
            , _nxserver(nullptr)
        {
        }

        ~CompositorImplementation()
        {
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
            string configuration (service->ConfigLine());

            _service = service;
            _service->AddRef();

            ASSERT(_nxserver == nullptr);

            if (_nxserver != nullptr) {
                delete _nxserver;
            }
            Config info; info.FromString(configuration);

            _sink.HardwareDelay(info.HardwareDelay.Value());

            _nxserver = new Broadcom::Platform(&_sink, &_sink, configuration);

            ASSERT(_nxserver != nullptr);

            return  Core::ERROR_NONE;
        }

        /* virtual */ void Register(Exchange::IComposition::INotification* notification)
        {
            // Do not double register a notification sink.
            _adminLock.Lock();

            ASSERT(std::find(_observers.begin(), _observers.end(), notification) == _observers.end());

            notification->AddRef();

            _observers.push_back(notification);

            std::list<Exchange::IComposition::IClient*>::iterator index(_clients.begin());

            while (index != _clients.end()) {

                notification->Attached(*index);
                index++;
            }

            _adminLock.Unlock();
        }

        /* virtual */ void Unregister(Exchange::IComposition::INotification* notification)
        {
            _adminLock.Lock();

            std::list<Exchange::IComposition::INotification*>::iterator index(std::find(_observers.begin(), _observers.end(), notification));

            // Do not un-register something you did not register.
            ASSERT(index != _observers.end());

            if (index != _observers.end()) {

                _observers.erase(index);

                notification->Release();
            }

            _adminLock.Unlock();
        }

        /* virtual */ Exchange::IComposition::IClient* Client(const uint8_t id)
        {
            Exchange::IComposition::IClient* result = nullptr;

            uint8_t count = id;

            _adminLock.Lock();

            std::list<Exchange::IComposition::IClient*>::iterator index(_clients.begin());

            while ((count != 0) && (index != _clients.end())) {
                count--;
                index++;
            }

            if (index != _clients.end()) {
                result = (*index);
                result->AddRef();
            }

            _adminLock.Unlock();

            return (result);
        }

        /* virtual */ Exchange::IComposition::IClient* Client(const string& name)
        {
            Exchange::IComposition::IClient* result = nullptr;

            _adminLock.Lock();

            std::list<Exchange::IComposition::IClient*>::iterator index(_clients.begin());

            while ((index != _clients.end()) && ((*index)->Name() != name)) {
                index++;
            }

            if (index != _clients.end()) {
                result = (*index);
                result->AddRef();
            }

            _adminLock.Unlock();

            return (result);
        }

        /* virtual */ void SetResolution(const Exchange::IComposition::ScreenResolution format)
        {

            NEXUS_Error rc;
            NxClient_DisplaySettings displaySettings;

            NxClient_GetDisplaySettings(&displaySettings);

            const auto index(formatLookup.find(format));

            if (index == formatLookup.cend() || index->second == NEXUS_VideoFormat_eUnknown
                || index->second == displaySettings.format) {

                TRACE(Trace::Information, (_T("Output resolution is already %d"), format));
                return;
            }

            if (displaySettings.format != index->second) {

                displaySettings.format = index->second;
                rc = NxClient_SetDisplaySettings(&displaySettings);
                if (rc) {

                    TRACE(Trace::Information, (_T("Setting resolution is failed: %d"), index->second));
                    return;
                }
            }
            TRACE(Trace::Information, (_T("New resolution is %d "), index->second));
            return;
        }

        /* virtual */ const ScreenResolution GetResolution()
        {
            NEXUS_Error rc;
            NxClient_DisplaySettings displaySettings;
            NEXUS_VideoFormat format;

            NxClient_GetDisplaySettings(&displaySettings);
            format = displaySettings.format;

            const auto index = std::find_if(formatLookup.cbegin(), formatLookup.cend(),
                      [format](const std::pair<const Exchange::IComposition::ScreenResolution, const NEXUS_VideoFormat>& found)
                      { return found.second == format; });

            if (index != formatLookup.cend()) {
                TRACE(Trace::Information, (_T("Resolution is %d "), index->second));
                return index->first;
            }
            else {
                TRACE(Trace::Information, (_T("Resolution is unknown")));
                return Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown;
            }
        }

    private:
        void Add(Exchange::IComposition::IClient* client)
        {
            ASSERT (client != nullptr);

            if (client != nullptr) {

                const std::string name(client->Name());

                if (name.empty() == true) {
                    TRACE(Trace::Information, (_T("Registration of a nameless client.")));
                }
                else {
                    std::list<Exchange::IComposition::IClient*>::iterator index(_clients.begin());

                    while ((index != _clients.end()) && (name != (*index)->Name())) {
                        index++;
                    }

                    if (index != _clients.end()) {
                        TRACE(Trace::Information, (_T("Client already registered %s."), name.c_str()));
                    }
                    else {
                        _clients.push_back(client);

                        TRACE(Trace::Information, (_T("Added client %s."), name.c_str()));

                        if (_observers.size() > 0) {

                            std::list<Exchange::IComposition::INotification*>::iterator index(_observers.begin());

                            while (index != _observers.end()) {
                                (*index)->Attached(client);
                                index++;
                            }
                        }
                    }
                }
            }
        }
        void Remove(const char clientName[])
        {
            const std::string name(clientName);

            if (name.empty() == true) {
                TRACE(Trace::Information, (_T("Unregistering a nameless client.")));
            }
            else {
                std::list<Exchange::IComposition::IClient*>::iterator index(_clients.begin());

                while ((index != _clients.end()) && (name != (*index)->Name())) {
                    index++;
                }

                if (index == _clients.end()) {
                    TRACE(Trace::Information, (_T("Client already unregistered %s."), name.c_str()));
                }
                else {
                    Exchange::IComposition::IClient* entry(*index);

                    _clients.erase(index);

                    TRACE(Trace::Information, (_T("Removed client %s."), name.c_str()));

                    if (_observers.size() > 0) {
                        std::list<Exchange::IComposition::INotification*>::iterator index(_observers.begin());

                        while (index != _observers.end()) {
                            (*index)->Detached(entry);
                            index++;
                        }
                    }

                    entry->Release();
                }
            }
        }
        void PlatformReady()
        {
            PluginHost::ISubSystem* subSystems(_service->SubSystems());

            ASSERT(subSystems != nullptr);

            if (subSystems != nullptr) {
                // Set Graphics event. We need to set up a handler for this at a later moment
                subSystems->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
                subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
                subSystems->Release();
            }
        }

    private:
        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        std::list<Exchange::IComposition::INotification*> _observers;
        std::list<Exchange::IComposition::IClient*> _clients;
        Sink _sink;
        Broadcom::Platform* _nxserver;
    };

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

} // namespace Plugin
} // namespace WPEFramework
