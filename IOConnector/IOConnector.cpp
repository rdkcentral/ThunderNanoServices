#include "IOConnector.h"

#include <interfaces/IKeyHandler.h>

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(IOConnector, 1, 0);

    IOConnector::IOConnector()
        : _service(nullptr)
        , _sink(*this)
        , _pin(nullptr)
        , _callsign()
        , _producer()
    {
    }

    /* virtual */ IOConnector::~IOConnector()
    {
    }

    /* virtual */ const string IOConnector::Initialize(PluginHost::IShell* service)
    {
        ASSERT (_service == nullptr);

        Config config; 
        config.FromString(service->ConfigLine());

        _service = service;
        _pin = new GPIO::Pin(config.Pin.Value());

        if (_pin != nullptr) {
            _pin->Register(&_sink);
            _callsign = config.Callsign.Value();
            _producer = config.Producer.Value();
        }
        
        // On success return empty, to indicate there is no error text.
        return (_pin != nullptr ? string() : _T("Could not instantiate the requested Pin"));
    }

    /* virtual */ void IOConnector::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT (_service == service);
        ASSERT (_pin != nullptr);

        if (_pin != nullptr) {
            _pin->Unregister(&_sink);
            delete _pin;
            _pin = nullptr;
        }

        _service = nullptr;
    }

    /* virtual */ string IOConnector::Information() const
    {
        // No additional info to report.
        return (string());
    }

    void IOConnector::Activity (GPIO::Pin& pin) {

        ASSERT (_pin == &pin);
        ASSERT (_service != nullptr);

        if (pin.Get() == true) {
            Exchange::IKeyHandler* handler (_service->QueryInterfaceByCallsign<Exchange::IKeyHandler>(_callsign));

            if (handler != nullptr) {
                Exchange::IKeyProducer* producer (handler->QueryInterface<Exchange::IKeyProducer>());

                if (producer != nullptr) {
                    producer->Pair();
                    producer->Release();
                }

                handler->Release();
            }
        }
    }

} // namespace Plugin
} // namespace WPEFramework
