/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 
#include "IOConnector.h"

#include <interfaces/IKeyHandler.h>

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Plugin::IOConnector::Config::Pin::mode)

    { Plugin::IOConnector::Config::Pin::LOW, _TXT("Low") },
    { Plugin::IOConnector::Config::Pin::HIGH, _TXT("High") },
    { Plugin::IOConnector::Config::Pin::BOTH, _TXT("Both") },
    { Plugin::IOConnector::Config::Pin::ACTIVE, _TXT("Active") },
    { Plugin::IOConnector::Config::Pin::INACTIVE, _TXT("Inactive") },
    { Plugin::IOConnector::Config::Pin::OUTPUT, _TXT("Output") },

    ENUM_CONVERSION_END(Plugin::IOConnector::Config::Pin::mode)

namespace Plugin
{

    SERVICE_REGISTRATION(IOConnector, 1, 0);

    static Core::ProxyPoolType<Web::JSONBodyType<IOConnector::Data>> jsonBodyDataFactory(1);

    class IOState {
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
        inline IOState(const GPIO::Pin* pin)
        {
            Trace::Format(_text, _T("IO Activity on pin: %d, current state: %s"), (pin->Identifier() & 0xFFFF), (pin->Get() ? _T("true") : _T("false")));
        }
        ~IOState()
        {
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
        : _adminLock()
        , _service(nullptr)
        , _sink(this)
        , _pins()
        , _skipURL(0)
        , _notifications()
    {
        RegisterAll();
    }

    /* virtual */ IOConnector::~IOConnector()
    {
        UnregisterAll();
    }

    /* virtual */ const string IOConnector::Initialize(PluginHost::IShell * service)
    {
        ASSERT(_service == nullptr);

        Config config;
        config.FromString(service->ConfigLine());

        _service = service;
        _skipURL = _service->WebPrefix().length();

        auto index(config.Pins.Elements());

        while (index.Next() == true) {

            GPIO::Pin* pin = Core::Service<GPIO::Pin>::Create<GPIO::Pin>(index.Current().Id.Value(), index.Current().ActiveLow.Value());
            uint8_t mode = 0;

            if (pin != nullptr) {
                switch (index.Current().Mode.Value()) {
                case Config::Pin::LOW: {
                    pin->Mode(GPIO::Pin::INPUT);
                    pin->Trigger(GPIO::Pin::FALLING);
                    pin->Subscribe(&_sink);
                    mode = 1;
                    break;
                }
                case Config::Pin::HIGH: {
                    pin->Mode(GPIO::Pin::INPUT);
                    pin->Trigger(GPIO::Pin::RISING);
                    pin->Subscribe(&_sink);
                    mode = 1;
                    break;
                }
                case Config::Pin::BOTH: {
                    pin->Mode(GPIO::Pin::INPUT);
                    pin->Trigger(GPIO::Pin::BOTH);
                    pin->Subscribe(&_sink);
                    mode = 2;
                    break;
                }
                case Config::Pin::ACTIVE: {
                    pin->Mode(GPIO::Pin::OUTPUT);
                    pin->Set(true);
                    break;
                }
                case Config::Pin::INACTIVE: {
                    pin->Mode(GPIO::Pin::OUTPUT);
                    pin->Set(false);
                    break;
                }
                case Config::Pin::OUTPUT: {
                    pin->Mode(GPIO::Pin::OUTPUT);
                    break;
                }
                default:
                    break;
                }

                if (_pins.find(pin->Identifier()) != _pins.end()) {
                    SYSLOG(Logging::Startup, (_T("Pin [%d] defined multiple times, only the first definitions is used !!"), pin->Identifier() & 0xFFFF));
                }
                else {
                    auto newEntry = _pins.emplace(std::piecewise_construct,
                                                  std::forward_as_tuple(pin->Identifier()),
                                                  std::forward_as_tuple(pin));

                    if (index.Current().Handlers.IsSet() == true) {
                        std::list< std::pair<uint32_t, uint32_t> > intervals;

                        Core::JSON::ArrayType<Config::Pin::Handler>::Iterator handler(index.Current().Handlers.Elements());

                        if (mode == 0) {
                            SYSLOG(Logging::Startup, (_T("Pin [%d] defined as output but handlers associated with it!"), pin->Identifier() & 0xFFFF));
                        }
                        else if (mode == 1) {
                            SYSLOG(Logging::Startup, (_T("Handlers requires input pins where BOTH states are reported. Error on pin [%d]."), pin->Identifier() & 0xFFFF));
                        }

                        while (handler.Next() == true) {
                          
                            Config::Pin::Handler& info(handler.Current());

                            uint32_t start = info.Start.Value();
                            uint32_t end   = info.End.Value();

                            if (start > end) {
                                SYSLOG(Logging::Startup, (_T("Handler [%s] on pin [%d] has an incorrect interval [%d-%d]."), info.Name.Value().c_str(), pin->Identifier() & 0xFFFF, start, end));
                            }
                            else {
                                std::list< std::pair<uint32_t, uint32_t> >::iterator loop(intervals.begin());
                                start *= 1000;
                                end   *= 1000;
                                // Check for overlapping intervals. If they exist report a warning...
                                while ((loop != intervals.end()) && (((start < loop->first) || (start >= loop->second)) && ((end <= loop->first) || (end > loop->second)))) {
                                    loop++;
                                }

                                if (loop != intervals.end()) {
                                    SYSLOG(Logging::Startup, (_T("Interval conflict on pin [%d], at handler [%s]."), pin->Identifier() & 0xFFFF, info.Name.Value().c_str()));
                                }
                                else if (newEntry.first->second.Add(_service, info.Name.Value(), info.Config.Value(), start, end) == false) {
                                    SYSLOG(Logging::Startup, (_T("Could not instantiate handler [%s] on pin [%d]."), info.Name.Value().c_str(), pin->Identifier() & 0xFFFF));
                                }
                                else {
                                    intervals.push_back(std::pair<uint32_t,uint32_t>(start,end));
                                }
                            }
                        }
                    }
                }

                pin->Release();
            }
        }

        // On success return empty, to indicate there is no error text.
        return (_pins.size() > 0 ? string() : _T("Could not instantiate the requested Pin"));
    }

    /* virtual */ void IOConnector::Register(ICatalog::INotification* sink)
    {
        _adminLock.Lock();

        // Make sure a sink is not registered multiple times.
        ASSERT(std::find(_notifications.begin(), _notifications.end(), sink) == _notifications.end());

        _notifications.push_back(sink);
        sink->AddRef();

        // report all the IExternals we have
        for (std::pair<const uint32_t, PinHandler>& product : _pins) {
            sink->Activated(product.second.Pin());
        }

        _adminLock.Unlock();
    }

    /* virtual */ void IOConnector::Unregister(ICatalog::INotification* sink)
    {
        _adminLock.Lock();

        // Make sure a sink is not unregistered multiple times.
        NotificationList::iterator index = std::find(_notifications.begin(), _notifications.end(), sink);

        if (index != _notifications.end()) {
            (*index)->Release();
            _notifications.erase(index);
        }

        _adminLock.Unlock();

    }

    /* virtual */ void IOConnector::Deinitialize(PluginHost::IShell * service)
    {
        ASSERT(_service == service);

        _adminLock.Lock();

        for (std::pair<const uint32_t, PinHandler>& product : _pins) {
            product.second.Unsubscribe(&_sink);

            for (auto client : _notifications) {
                client->Deactivated(product.second.Pin());
            }
        }

        _adminLock.Unlock();

        _pins.clear();

        _service = nullptr;
    }

    /* virtual */ string IOConnector::Information() const
    {
        // No additional info to report.
        return (string());
    }

    //  IWeb methods
    // -------------------------------------------------------------------------------------------------------
    /* virtual */ void IOConnector::Inbound(Web::Request & /* request */)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response> IOConnector::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());
        Core::TextSegmentIterator index(
            Core::TextFragment(request.Path, _skipURL, request.Path.length() - _skipURL), false, '/');

        // Always skip the first, this is the slash
        index.Next();

        if (index.Next() != true) {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = "No Pin instance number found";
        } else {
            uint32_t pinId(Core::NumberType<uint16_t>(index.Current()).Value());

            // Find the pin..
            Pins::iterator entry = _pins.begin();

            while ((entry != _pins.end()) && ((entry->first & 0xFFFF) != pinId)) {
                entry++;
            }

            if (entry != _pins.end()) {
                GPIO::Pin* pin(entry->second.Pin());

                ASSERT (pin != nullptr);

                if (request.Verb == Web::Request::HTTP_GET) {
                    GetMethod(*result, index, *pin);
                } else if ((request.Verb == Web::Request::HTTP_POST) && (index.Next() == true)) {
                    PostMethod(*result, index, *pin);
                }
            }
        }

        return (result);
    }

    void IOConnector::GetMethod(Web::Response & result, Core::TextSegmentIterator & index, GPIO::Pin & pin)
    {
        Core::ProxyType<Web::JSONBodyType<IOConnector::Data>> element = jsonBodyDataFactory.Element();
        if (element.IsValid() == true) {
            element->Value = pin.Get();
            element->Id = pin.Identifier();
            result.Body(element);
            result.ErrorCode = Web::STATUS_OK;
            result.Message = "Pin value retrieved";
            TRACE(IOState, (&pin));
        }
    }

    void IOConnector::PostMethod(Web::Response & result, Core::TextSegmentIterator & index, GPIO::Pin & pin)
    {
        int32_t value(Core::NumberType<int32_t>(index.Current()));
        pin.Set(value);
        result.ErrorCode = Web::STATUS_OK;
        result.Message = "Pin <" + Core::NumberType<uint16_t>(pin.Identifier() & 0xFFFF).Text() + "> changed state to: " + string(value != 0 ? _T("SET") : _T("CLEARED"));
        TRACE(IOState, (&pin));
    }

    void IOConnector::Activity()
    {
        ASSERT(_service != nullptr);

        // Lets find the pin and trigger if posisble...
        Pins::iterator index = _pins.begin();

        while (index != _pins.end()) {

            GPIO::Pin* pin(index->second.Pin());

            ASSERT (pin != nullptr);

            if (pin->HasChanged()) {

                pin->Align();

                TRACE(IOState, (pin));

                if (index->second.HasHandlers() == true) {
                    index->second.Handle();
                } else {
                    int32_t value = (pin->Get() ? 1 : 0);
                    string pinAsText (Core::NumberType<uint16_t>(pin->Identifier() & 0xFFFF).Text());
                    _service->Notify(_T("{ \"id\": ") + pinAsText + _T(", \"state\": \"") + (value != 0 ? _T("Set\" }") : _T("Clear\" }")));

                    event_pinactivity(pinAsText, value);
                }
            }

            index++;
        }
    }

} // namespace Plugin
} // namespace WPEFramework
