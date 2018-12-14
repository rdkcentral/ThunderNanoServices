#include <interfaces/IKeyHandler.h>

#include "Handler.h"

namespace WPEFramework {

namespace Plugin {

    class RemotePairing : public IHandler {
    private:
        RemotePairing ();
        RemotePairing (const RemotePairing&);
        RemotePairing& operator= (const RemotePairing&);

        class Config: public Core::JSON::Container {
        private:
            Config(const Config& copy) = delete;
            Config& operator= (const Config& RHS) = delete;

        public:
            Config()
                : Callsign()
                , Producer() {
                Add(_T("callsign"), &Callsign);
                Add(_T("producer"), &Producer);
            }
            virtual ~Config() {
            }

        public:
            Core::JSON::String Callsign;
            Core::JSON::String Producer;
        };

    public:
        RemotePairing (PluginHost::IShell* service, const string& configuration) : _service(service) {
            Config config; config.FromString(configuration);
            _producer = config.Producer.Value();
            _callsign = config.Callsign.Value();
        }
        virtual ~RemotePairing() {
        }

    public:
        virtual void Trigger(GPIO::Pin& pin) override {

            ASSERT (_service != nullptr);

            Exchange::IKeyHandler* handler (_service->QueryInterfaceByCallsign<Exchange::IKeyHandler>(_callsign));

            if (handler != nullptr) {
                Exchange::IKeyProducer* producer (handler->Producer(_producer));

                if (producer != nullptr) {
                    producer->Pair();
                    producer->Release();
                }

                handler->Release();
            }
        }

    private:
        PluginHost::IShell* _service;
        string _callsign;
        string _producer;
    };

    static HandlerAdministrator::Entry<RemotePairing> handler;

} // namespace Plugin
} // namespace WPEFramework
