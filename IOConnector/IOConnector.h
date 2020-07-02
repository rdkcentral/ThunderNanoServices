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
 
#ifndef IOCONNECTOR_H
#define IOCONNECTOR_H

#include "GPIO.h"
#include "Handler.h"
#include "Module.h"

#include <interfaces/IExternal.h>

namespace WPEFramework {
namespace Plugin {

    class IOConnector
        : public PluginHost::IPlugin,
          public PluginHost::IWeb,
          public Exchange::IExternal::ICatalog,
          public PluginHost::JSONRPC {
    private:
        class Sink : public Exchange::IExternal::INotification {
        public:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator= (const Sink&) = delete;

            Sink(IOConnector* parent)
                : _parent(*parent)
            {
            }
            ~Sink() override
            {
            }

        public:
            void Update(const uint32_t /* id */) override
            {
                _parent.Activity();
            }

            BEGIN_INTERFACE_MAP(Sink)
            INTERFACE_ENTRY(Exchange::IExternal::INotification)
            END_INTERFACE_MAP

        private:
            IOConnector& _parent;
        };
        class PinHandler {
        public:
            PinHandler() = delete;
            PinHandler(const PinHandler&) = delete;
            PinHandler& operator= (const PinHandler&) = delete;

            PinHandler(GPIO::Pin* pin) 
                : _pin(pin)
                , _handlers() {

                ASSERT (pin != nullptr);

                _pin->AddRef();
            }
            ~PinHandler() {
                std::list<IHandler*>::iterator index (_handlers.begin());

                while (index != _handlers.end()) {
                    delete (*index);
                    index++;
                }
                _handlers.clear();

                _pin->Release();;
            }

        public:
            bool Add(PluginHost::IShell* service, const string& name, const string& config, const uint32_t begin, const uint32_t end) {
                IHandler* method = HandlerAdministrator::Instance().Handler(name, service, config, begin, end);
                if (method != nullptr) {
                    _handlers.emplace_back(method);
                }
                return (method != nullptr);
            }
            void Handle() {
                std::list<IHandler*>::iterator index (_handlers.begin());

                while (index != _handlers.end()) {
                    (*index)->Trigger(*_pin);
                    index++;
                }
            }
            GPIO::Pin* Pin() {
                return (_pin);
            }
            bool Get() const {
                return (_pin->Get());
            }
            void Set(const bool value) {
                return (_pin->Set(value));
            }
            void Unsubscribe(Exchange::IExternal::INotification* sink) {
                _pin->Unsubscribe(sink);
                _pin->Deactivate();
            }
            bool HasHandlers() const {
                return (_handlers.size() > 0);
            }

        private:
            GPIO::Pin* _pin;
            std::list<IHandler*> _handlers;
        };

        using NotificationList = std::list< Exchange::IExternal::ICatalog::INotification* >;
        using Pins = std::map<const uint32_t, PinHandler>;

    public:
        class Config : public Core::JSON::Container {
        public:
            class Pin : public Core::JSON::Container {
            public:
                class Handler : public Core::JSON::Container {
                public:
                    Handler()
                        : Name()
                        , Config()
                        , Start(0)
                        , End(~0)
                    {
                        Add(_T("name"), &Name);
                        Add(_T("config"), &Config);
                        Add(_T("start"), &Start);
                        Add(_T("end"), &End);
                    }
                    Handler(const Handler& copy)
                        : Name(copy.Name)
                        , Config(copy.Config)
                        , Start(copy.Start)
                        , End(copy.End)
                    {
                        Add(_T("name"), &Name);
                        Add(_T("config"), &Config);
                        Add(_T("start"), &Start);
                        Add(_T("end"), &End);
                    }
                    ~Handler() override
                    {
                    }

                    Handler& operator=(const Handler& RHS)
                    {
                        Name = RHS.Name;
                        Config = RHS.Config;
                        Start = RHS.Start;
                        End = RHS.End;
                        return (*this);
                    }

                public:
                    Core::JSON::String Name;
                    Core::JSON::String Config;
                    Core::JSON::DecUInt16 Start;
                    Core::JSON::DecUInt16 End;
                };

                enum mode {
                    LOW, /* input  */
                    HIGH, /* input  */
                    BOTH, /* input  */
                    ACTIVE, /* output */
                    INACTIVE, /* output */
                    OUTPUT /* output */
                };

            public:
                Pin()
                    : Id(~0)
                    , Mode(LOW)
                    , ActiveLow(false)
                    , Handlers()
                {
                    Add(_T("id"), &Id);
                    Add(_T("mode"), &Mode);
                    Add(_T("activelow"), &ActiveLow);
                    Add(_T("handlers"), &Handlers);
                }
                Pin(const Pin& copy)
                    : Id(copy.Id)
                    , Mode(copy.Mode)
                    , ActiveLow(copy.ActiveLow)
                    , Handlers(copy.Handlers)
                {
                    Add(_T("id"), &Id);
                    Add(_T("mode"), &Mode);
                    Add(_T("activelow"), &ActiveLow);
                    Add(_T("handlers"), &Handlers);
                }
                ~Pin() override
                {
                }

                Pin& operator=(const Pin& RHS)
                {
                    Id = RHS.Id;
                    Mode = RHS.Mode;
                    ActiveLow = RHS.ActiveLow;
                    Handlers = RHS.Handlers;

                    return (*this);
                }

            public:
                Core::JSON::DecUInt16 Id;
                Core::JSON::EnumType<mode> Mode;
                Core::JSON::Boolean ActiveLow;
                Core::JSON::ArrayType<Handler> Handlers;
            };

        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
            {
                Add(_T("pins"), &Pins);
            }
            ~Config() override
            {
            }

        public:
            Core::JSON::ArrayType<Pin> Pins;
        };

        class Data : public Core::JSON::Container {
        public:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;

            Data()
                : Core::JSON::Container()
                , Id()
                , Value()
            {
                Add(_T("id"), &Id);
                Add(_T("value"), &Value);
            }
            ~Data() override
            {
            }

        public:
            Core::JSON::DecUInt16 Id;
            Core::JSON::DecUInt32 Value;
        };

    public:
        IOConnector(const IOConnector&) = delete;
        IOConnector& operator=(const IOConnector&) = delete;

        IOConnector();
        ~IOConnector() override;

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(IOConnector)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(Exchange::IExternal::ICatalog)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IExternal::IFactory methods
        // -------------------------------------------------------------------------------------------------------
        void Register(ICatalog::INotification* sink) override;
        void Unregister(ICatalog::INotification* sink) override;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        void Activity();
        void GetMethod(Web::Response& response, Core::TextSegmentIterator& index, GPIO::Pin& pin);
        void PostMethod(Web::Response& response, Core::TextSegmentIterator& index, GPIO::Pin& pin);

        // JsonRpc methods
        void RegisterAll();
        void UnregisterAll();
        uint32_t get_pin(const string& index, Core::JSON::DecSInt32& response) const;
        uint32_t set_pin(const string& index, const Core::JSON::DecSInt32& param);
        void event_pinactivity(const string& id, const int32_t& value);

    private:
        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        Core::Sink<Sink> _sink;
        Pins _pins;
        uint8_t _skipURL;
        NotificationList _notifications;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // IOCONNECTOR_H
