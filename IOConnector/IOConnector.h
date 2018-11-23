#ifndef IOCONNECTOR_H
#define IOCONNECTOR_H

#include "Module.h"
#include "Handler.h"
#include "GPIO.h"

namespace WPEFramework {
namespace Plugin {

    class IOConnector : public PluginHost::IPlugin {
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
                        Add(_T("handler"), &Handler);
                        Add(_T("config"), &Config);
                    }
                    Handle(const Handle& copy)
                        : Handler(copy.Handler)
                        , Config(copy.Config) {
                        Add(_T("handler"), &Handler);
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
                    , Handler() {
                    Add(_T("id"), &Id);
                    Add(_T("mode"), &Mode);
                    Add(_T("handler"), &Handler);
                }
                Pin(const Pin& copy)
                    : Id(copy.Id)
                    , Mode(copy.Mode)
                    , Handler(copy.Handler) {
                    Add(_T("id"), &Id);
                    Add(_T("mode"), &Mode);
                    Add(_T("handler"), &Handler);
                }
		virtual ~Pin() {
		}

                Pin& operator= (const Pin& RHS) {
                    Id = RHS.Id;
                    Mode = RHS.Mode;
                    Handler = RHS.Handler;
                    return(*this);
                }
	
            public:
                Core::JSON::DecUInt8 Id;
                Core::JSON::EnumType<mode> Mode;
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

    public:
        IOConnector();
        virtual ~IOConnector();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(IOConnector)
            INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

    private:
        void Activity ();

    private:
        PluginHost::IShell* _service;
        Core::Sink<Sink> _sink;
        Pins _pins;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // IOCONNECTOR_H
