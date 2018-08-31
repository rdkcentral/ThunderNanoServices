#include "RpiServer.h"
#include <interfaces/IComposition.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace WPEFramework {
namespace Plugin {

class CompositorImplementation : public Exchange::IComposition {
private:
    CompositorImplementation(const CompositorImplementation&) = delete;
    CompositorImplementation& operator=(const CompositorImplementation&) = delete;

    class Sink
        : public Rpi::Platform::IClient
        , public Rpi::Platform::IStateChange {

    private:
        Sink() = delete;
        Sink(const Sink&) = delete;
        Sink& operator=(const Sink&) = delete;

    public:
        explicit Sink(CompositorImplementation* parent)
            : _parent(*parent) {
        }
        ~Sink() {
        }

    public:
        void Attached(
                Exchange::IComposition::IClient* client) {
            _parent.Add(client);
        }
        void Detached(const char clientName[]) {
            _parent.Remove(clientName);
        }
        virtual void StateChange(Rpi::Platform::server_state state) {
            if (state == Rpi::Platform::OPERATIONAL){
                _parent.PlatformReady();
            }
        }

    private:
        CompositorImplementation& _parent;
    };

public:

    CompositorImplementation()
    : _adminLock()
    , _service(nullptr)
    , _observers()
    , _clients()
    , _sink(this) {
        TRACE(Trace::Information, (_T("RPI CompositorImplementation")));
    }

    ~CompositorImplementation() {

        if (_rpiserver != nullptr) {
            delete _rpiserver;
        }

        if (_service != nullptr) {
            _service->Release();
        }
    }

    BEGIN_INTERFACE_MAP(CompositorImplementation)
    INTERFACE_ENTRY(Exchange::IComposition)
    END_INTERFACE_MAP

    uint32_t Configure(PluginHost::IShell* service) {

        _service = service;
        _service->AddRef();

        string configuration(service->ConfigLine());
        Config info;
        info.FromString(configuration);

        _rpiserver = new Rpi::Platform(
                service->Callsign(), &_sink, &_sink, configuration);
        return  Core::ERROR_NONE;
    }

    void Register(Exchange::IComposition::INotification* notification) {
        _adminLock.Lock();
        ASSERT(std::find(_observers.begin(),
                _observers.end(), notification) == _observers.end());
        notification->AddRef();
        _observers.push_back(notification);
        std::list<Exchange::IComposition::IClient*>::iterator index(_clients.begin());
        while (index != _clients.end()) {
            notification->Attached(*index);
            index++;
        }
        _adminLock.Unlock();
    }

    void Unregister(Exchange::IComposition::INotification* notification) {
        _adminLock.Lock();
        std::list<Exchange::IComposition::INotification*>::iterator index(
                std::find(_observers.begin(), _observers.end(), notification));
        ASSERT(index != _observers.end());
        if (index != _observers.end()) {
            _observers.erase(index);
            notification->Release();
        }
        _adminLock.Unlock();
    }

    Exchange::IComposition::IClient* Client(const uint8_t id) {
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

    Exchange::IComposition::IClient* Client(const string& name) {
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

    void Resolution(const Exchange::IComposition::ScreenResolution format) {
        fprintf(stderr, "CompositorImplementation::Resolution %d\n", format);
    }

    Exchange::IComposition::ScreenResolution Resolution() const {
        return Exchange::IComposition::ScreenResolution::ScreenResolution_720p;
    }
private:
    void Add(Exchange::IComposition::IClient* client) {

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
                    TRACE(Trace::Information,
                            (_T("Client already registered %s."), name.c_str()));
                }
                else {
                    client->AddRef();
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

    void Remove(const char clientName[]) {
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

    void PlatformReady() {
        PluginHost::ISubSystem* subSystems(_service->SubSystems());
        ASSERT(subSystems != nullptr);
        if (subSystems != nullptr) {
            subSystems->Set(PluginHost::ISubSystem::PLATFORM, nullptr);
            subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
            subSystems->Release();
        }
    }

    Core::CriticalSection _adminLock;
    PluginHost::IShell* _service;
    std::list<Exchange::IComposition::INotification*> _observers;
    std::list<Exchange::IComposition::IClient*> _clients;
    Sink _sink;
    Rpi::Platform* _rpiserver;
};

SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

} // namespace Plugin
} // namespace WPEFramework
