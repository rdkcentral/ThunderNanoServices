#ifndef IOCONNECTOR_H
#define IOCONNECTOR_H

#include "Module.h"
#include "GPIO.h"

namespace WPEFramework {
namespace Plugin {

    class IOConnector : public PluginHost::IPlugin {
    private:
        IOConnector(const IOConnector&) = delete;
        IOConnector& operator=(const IOConnector&) = delete;

        class Sink : public GPIO::Pin::IObserver {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator= (const Sink&) = delete;

        public:
            Sink(IOConnector& parent) : _parent(parent) {
            }
            virtual ~Sink() {
            }

        public:
            virtual void Activity (GPIO::Pin&  pin) override {
                _parent.Activity(pin);
            }

        private:
            IOConnector& _parent;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Pin(16)
                , Callsign(_T("RemoeControl"))
                , Producer(_T("RF4CE")) {
                Add(_T("pin"), &Pin);
                Add(_T("callsign"), &Callsign);
                Add(_T("producer"), &Producer);
            }
            virtual ~Config() {
            }

        public:
            Core::JSON::DecUInt8 Pin;
            Core::JSON::String   Callsign;
            Core::JSON::String   Producer;
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
        void Activity (GPIO::Pin& pin);

    private:
        PluginHost::IShell* _service;
        Sink _sink;
        GPIO::Pin* _pin;
	string _callsign;
        string _producer;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // IOCONNECTOR_H
