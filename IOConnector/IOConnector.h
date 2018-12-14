#ifndef IOCONNECTOR_H
#define IOCONNECTOR_H

#include "Module.h"
#include "Handler.h"
#include "GPIO.h"

#include <interfaces/IExternal.h>

namespace WPEFramework {
namespace Plugin {

    class IOConnector 
        : public PluginHost::IPlugin 
        , public PluginHost::IWeb
        , public Exchange::IExternal::IFactory {
    private:
        IOConnector(const IOConnector&) = delete;
        IOConnector& operator=(const IOConnector&) = delete;

        class Sink : public Exchange::IExternal::INotification {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator= (const Sink&) = delete;

        public:
            Sink(IOConnector* parent) : _parent(*parent) {
            }
            virtual ~Sink() {
            }

        public:
            virtual void Update() override {
                _parent.Activity();
            }

            BEGIN_INTERFACE_MAP(Sink)
                INTERFACE_ENTRY(Exchange::IExternal::INotification)
            END_INTERFACE_MAP

        private:
            IOConnector& _parent;
        };

        typedef std::pair<GPIO::Pin*, IHandler*> PinHandler;
        typedef std::list<PinHandler> Pins;

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            class Pin : public Core::JSON::Container {
            public:
                class Handle: public Core::JSON::Container {
                public:
                    Handle()
                        : Handler()
                        , Config() {
                        Add(_T("name"), &Handler);
                        Add(_T("config"), &Config);
                    }
                    Handle(const Handle& copy)
                        : Handler(copy.Handler)
                        , Config(copy.Config) {
                        Add(_T("name"), &Handler);
                        Add(_T("config"), &Config);
                    }
                    virtual ~Handle() {
                    }

                    Handle& operator= (const Handle& RHS) {
                        Handler = RHS.Handler;
                        Config = RHS.Config;
                        return(*this);
                    }

                public:
                    Core::JSON::String Handler;
                    Core::JSON::String Config;
                };

                enum mode {
                    LOW,        /* input  */
                    HIGH,       /* input  */
                    BOTH,       /* input  */
                    ACTIVE,     /* output */
                    INACTIVE,   /* output */
                    OUTPUT      /* output */
                };

            public:
                Pin()
                    : Id(~0)
                    , Mode(LOW)
                    , ActiveLow(false)
                    , Handler() {
                    Add(_T("id"), &Id);
                    Add(_T("mode"), &Mode);
                    Add(_T("activelow"), &ActiveLow);
                    Add(_T("handler"), &Handler);
                }
                Pin(const Pin& copy)
                    : Id(copy.Id)
                    , Mode(copy.Mode)
                    , ActiveLow(copy.ActiveLow)
                    , Handler(copy.Handler) {
                    Add(_T("id"), &Id);
                    Add(_T("mode"), &Mode);
                    Add(_T("activelow"), &ActiveLow);
                    Add(_T("handler"), &Handler);
                }
		virtual ~Pin() {
		}

                Pin& operator= (const Pin& RHS) {
                    Id = RHS.Id;
                    Mode = RHS.Mode;
                    ActiveLow = RHS.ActiveLow;
                    Handler = RHS.Handler;
 
                    return(*this);
                }
	
            public:
                Core::JSON::DecUInt8 Id;
                Core::JSON::EnumType<mode> Mode;
                Core::JSON::Boolean ActiveLow;
                Handle Handler;
            };

        public:
            Config()
                : Core::JSON::Container() {
                Add(_T("pins"), &Pins);
            }
            virtual ~Config() {
            }

        public:
            Core::JSON::ArrayType<Pin> Pins;
        };

        class Data : public Core::JSON::Container {
        private:
            Data(const Data&) = delete;
            Data& operator=(const Data&) = delete;

        public:
            Data()
                : Core::JSON::Container()
                , Id()
                , Value()
            {
                Add(_T("id"), &Id);
                Add(_T("value"), &Value);
            }
            ~Data()
            {
            }

        public:
            Core::JSON::DecUInt16 Id;
            Core::JSON::DecUInt32 Value;
        };

    public:
        IOConnector();
        virtual ~IOConnector();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(IOConnector)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_ENTRY(Exchange::IExternal::IFactory)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IExternal::IFactory methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Register(IFactory::INotification* sink) override;
        virtual void Unregister(IFactory::INotification* sink) override;
        virtual Exchange::IExternal* Resource(const uint32_t id) override;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        void Activity ();
        void GetMethod(Web::Response& response, Core::TextSegmentIterator& index, GPIO::Pin& pin);
        void PostMethod(Web::Response& response, Core::TextSegmentIterator& index, GPIO::Pin& pin);

    private:
        PluginHost::IShell* _service;
        Core::Sink<Sink> _sink;
        Pins _pins;
        uint8_t _skipURL;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // IOCONNECTOR_H
