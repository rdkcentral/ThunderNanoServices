#include <interfaces/IPower.h>

#include "Handler.h"

namespace WPEFramework {

namespace Plugin {

    class PowerDown : public IHandler {
    private:
        PowerDown ();
        PowerDown (const PowerDown&);
        PowerDown& operator= (const PowerDown&);

        class Config: public Core::JSON::Container {
        private:
            Config(const Config& copy) = delete;
            Config& operator= (const Config& RHS) = delete;

        public:
            Config()
                : Callsign()
                , State() {
                Add(_T("callsign"), &Callsign);
            }
            virtual ~Config() {
            }

        public:
            Core::JSON::String Callsign;
        };

    public:
        PowerDown (PluginHost::IShell* service, const string& configuration) : _service(service) {
            Config config; config.FromString(configuration);
            _callsign = config.Callsign.Value();
        }
        virtual ~PowerDown() {
        }

    public:
        virtual void Trigger(GPIO::Pin& pin) override {

            ASSERT (_service != nullptr);

            Exchange::IPower* handler (_service->QueryInterfaceByCallsign<Exchange::IPower>(_callsign));

             if (handler != nullptr) {

                handler->SetState();
                handler->Release();
            }
        }

    private:
        PluginHost::IShell* _service;
        string _callsign;
    };

    static HandlerAdministrator::Entry<PowerDown> handler;

} // namespace Plugin
} // namespace WPEFramework
