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
        void Attached(Exchange::IComposition::IClient* client) {
        }

        void Detached(const char clientName[]) {
        }

        virtual void StateChange(Rpi::Platform::server_state state) {
        }

    private:
        CompositorImplementation& _parent;
    };

public:
    CompositorImplementation()
        : _adminLock()
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
        notification->AddRef();
        _adminLock.Unlock();
    }

    void Unregister(Exchange::IComposition::INotification* notification) {
        _adminLock.Lock();
        notification->Release();
        _adminLock.Unlock();
    }

    Exchange::IComposition::IClient* Client(const uint8_t id) {
        _adminLock.Lock();
        Exchange::IComposition::IClient* result = nullptr;
        _adminLock.Unlock();
        return (result);
    }

    Exchange::IComposition::IClient* Client(const string& name) {
        _adminLock.Lock();
        Exchange::IComposition::IClient* result = nullptr;
        _adminLock.Unlock();
        return (result);
    }

    void Resolution(const Exchange::IComposition::ScreenResolution format) {
    }

    Exchange::IComposition::ScreenResolution Resolution() const {
        return Exchange::IComposition::ScreenResolution::ScreenResolution_480p;
    }
private:
    Core::CriticalSection _adminLock;
    PluginHost::IShell* _service;
    Sink _sink;
    Rpi::Platform* _rpiserver;
};

SERVICE_REGISTRATION(CompositorImplementation, 1, 0);

} // namespace Plugin
} // namespace WPEFramework
