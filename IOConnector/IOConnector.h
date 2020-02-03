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
          public Exchange::IExternal::IFactory,
          public PluginHost::JSONRPC {
    private:
        IOConnector(const IOConnector&) = delete;
        IOConnector& operator=(const IOConnector&) = delete;

        class Sink : public Exchange::IExternal::INotification {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

        public:
            Sink(IOConnector* parent)
                : _parent(*parent)
            {
            }
            virtual ~Sink()
            {
            }

        public:
            virtual void Update() override
            {
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
                    Core::JSON::DecUInt8 Start;
                    Core::JSON::DecUInt8 End;
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
                virtual ~Pin()
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
                Core::JSON::DecUInt8 Id;
                Core::JSON::EnumType<mode> Mode;
                Core::JSON::Boolean ActiveLow;
                Core::JSON::ArrayType<Handler> Handlers;
            };

        public:
            Config()
                : Core::JSON::Container()
            {
                Add(_T("pins"), &Pins);
            }
            virtual ~Config()
            {
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
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IExternal::IFactory methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Register(IFactory::IProduced* sink) override;
        virtual void Unregister(IFactory::IProduced* sink) override;
        virtual Exchange::IExternal* Resource(const uint32_t id) override;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

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
        PluginHost::IShell* _service;
        Core::Sink<Sink> _sink;
        Pins _pins;
        uint8_t _skipURL;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // IOCONNECTOR_H
