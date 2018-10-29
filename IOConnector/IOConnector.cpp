#include "IOConnector.h"

#include <interfaces/IKeyHandler.h>

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(IOConnector, 1, 0);

    class EXTERNAL IOState {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        IOState(const IOState& a_Copy) = delete;
        IOState& operator=(const IOState& a_RHS) = delete;

    public:
        inline IOState(const GPIO::Pin& pin) {
            Trace::Format(_text, _T("IO Activity on pin: %d, current state: %s"), pin.Id(), (pin.Get() ? _T("true") : _T("false")));
        }
        ~IOState() {
        }

    public:
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };

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
            _pin->Trigger(GPIO::Pin::FALLING);
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

        TRACE(IOState, (pin));

        if (pin.Get() == false) {
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
