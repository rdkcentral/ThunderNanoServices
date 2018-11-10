#include "IOConnector.h"

#include <interfaces/IKeyHandler.h>

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Plugin::IOConnector::Config::Pin::mode)

    { Plugin::IOConnector::Config::Pin::LOW,      _TXT("Low")      },
    { Plugin::IOConnector::Config::Pin::HIGH,     _TXT("High")     },
    { Plugin::IOConnector::Config::Pin::BOTH,     _TXT("Both")     },
    { Plugin::IOConnector::Config::Pin::ACTIVE,   _TXT("Active")   },
    { Plugin::IOConnector::Config::Pin::INACTIVE, _TXT("Inactive") },
    { Plugin::IOConnector::Config::Pin::OUTPUT,   _TXT("Output")   },

ENUM_CONVERSION_END(Plugin::IOConnector::Config::Pin::mode)

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
        , _pins()
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

        auto index (config.Pins.Elements());

        while (index.Next() == true) {
            
            GPIO::Pin* pin = new GPIO::Pin(index.Current().Id.Value());

            if (pin != nullptr) {
                switch(index.Current().Mode.Value()) {
                case Config::Pin::LOW:
                {
                    pin->Mode(GPIO::Pin::INPUT);
                    pin->Trigger(GPIO::Pin::FALLING); 
                    break;
                }
                case Config::Pin::HIGH:
                {
                    pin->Mode(GPIO::Pin::INPUT);
                    pin->Trigger(GPIO::Pin::RISING); 
                    break;
                }
                case Config::Pin::BOTH:
                {
                    pin->Mode(GPIO::Pin::INPUT);
                    pin->Trigger(GPIO::Pin::BOTH); 
                    break;
                }
                case Config::Pin::ACTIVE:
                {
                    pin->Mode(GPIO::Pin::OUTPUT);
                    pin->Set(true); 
                    break;
                }
                case Config::Pin::INACTIVE:
                {
                    pin->Mode(GPIO::Pin::OUTPUT);
                    pin->Set(false); 
                    break;
                }
                case Config::Pin::OUTPUT:
                {
                    pin->Mode(GPIO::Pin::OUTPUT);
                    break;
                }
                default: break;
                }
			
                pin->Register(&_sink);

                IHandler* method = nullptr;

                if (index.Current().Handler.IsSet() != false) {
                    Config::Pin::Handle&  handler (index.Current().Handler);
                    method = HandlerAdministrator::Instance().Handler(handler.Handler.Value(), _service, handler.Config.Value());
                }

                _pins.push_back(PinHandler(pin, method));
            }
        }
        
        // On success return empty, to indicate there is no error text.
        return (_pins.size() > 0 ? string() : _T("Could not instantiate the requested Pin"));
    }

    /* virtual */ void IOConnector::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT (_service == service);
        ASSERT (_pin != nullptr);

        while (_pins.size() > 0) {
            delete _pins.front().first;
            if (_pins.front().second != nullptr) {
                delete _pins.front().second;
            }
            _pins.pop_front();
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

        // Lets find the pin and trigger if posisble...
        Pins::iterator index = _pins.begin();

        while ((index != _pins.end()) && (index->first != &pin)) { index++; }

        if (index != _pins.end()) {

            if (index->second != nullptr) {
                index->second->Trigger(pin);
            }
            else {
                _service->Notify(_T("{ \"id\": ") + 
                                Core::NumberType<uint8_t>(pin.Id()).Text() + 
                                _T(", \"state\": \"") + 
                                (pin.Get() ? _T("High\" }") : _T("Low\" }")));
            }
        }
    }

} // namespace Plugin
} // namespace WPEFramework
