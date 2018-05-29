#include "Module.h"

namespace WPEFramework {
namespace Plugin {
namespace Command {

    class PluginControl {
    private:
        PluginControl(const PluginControl&) = delete;
        PluginControl& operator=(const PluginControl&) = delete;

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            enum state {
                ACTIVATE,
                DEACTIVATE
            };

        public:
            Config()
                : Core::JSON::Container()
                , Callsign()
                , State()
                , Success()
                , Failure()
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("state"), &State);
                Add(_T("success"), &Success);
                Add(_T("failure"), &Failure);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Callsign;
            Core::JSON::EnumType<state> State;
            Core::JSON::String Success;
            Core::JSON::String Failure;
        };

    public:
        PluginControl(const string& configuration)
            : _config()
        {

            _config.FromString(configuration);
        }
        ~PluginControl()
        {
        }

        const string Execute(PluginHost::IShell* service)
        {

            string result = _config.Failure;
            PluginHost::IShell* pluginShell = service->QueryInterfaceByCallsign<PluginHost::IShell>(_config.Callsign);

            if (pluginShell != nullptr) {

                switch (_config.State) {

                case Config::state::ACTIVATE:
                    if (pluginShell->State() == PluginHost::IShell::ACTIVATED) {
                        result = _config.Success;
                    }
                    else {
                        // Time to activate it...
                        if (pluginShell->Activate(PluginHost::IShell::reason::REQUESTED) == Core::ERROR_NONE) {
                            result = _config.Success;
                        }
                    }
                    break;
                case Config::state::DEACTIVATE:
                    if (pluginShell->State() == PluginHost::IShell::DEACTIVATED) {
                        result = _config.Success;
                    }
                    else {
                        // Time to deactivate it...
                        if (pluginShell->Deactivate(PluginHost::IShell::reason::REQUESTED) == Core::ERROR_NONE) {
                            result = _config.Success;
                        }
                    }
                    break;
                }
            }

            return (result);
        }

    private:
        Config _config;
    };

    class PluginObserver {
    private:
        PluginObserver(const PluginObserver&) = delete;
        PluginObserver& operator=(const PluginObserver&) = delete;

    private:
        class Observer : public PluginHost::IPlugin::INotification {
        private:
            Observer() = delete;
            Observer(const Observer&) = delete;
            Observer& operator=(Observer&) = delete;

        public:
            Observer(PluginObserver* parent)
                : _parent(*parent)
            {
            }
            ~Observer()
            {
            }

        public:
            virtual void StateChange(PluginHost::IShell* plugin)
            {
                _parent.StateChange(plugin);
            }

			BEGIN_INTERFACE_MAP(Observer)
				INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
			END_INTERFACE_MAP

        private:
            PluginObserver& _parent;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Callsign()
                , Active()
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("active"), &Active);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Callsign;
            Core::JSON::Boolean Active;
        };

    public:
		#ifdef __WIN32__ 
		#pragma warning( disable : 4355 )
		#endif
        PluginObserver(const string& configuration)
            : _config()
            , _waitEvent(true, false)
            , _observer(Core::Service<Observer>::Create<Observer>(this))
        {

            _config.FromString(configuration);
        }
		#ifdef __WIN32__ 
		#pragma warning( default : 4355 )
		#endif
        ~PluginObserver()
        {
        }

        const string Execute(PluginHost::IShell* service)
        {

            _waitEvent.ResetEvent();

            service->Register(_observer);

            _waitEvent.Lock();

            service->Unregister(_observer);

            return (EMPTY_STRING);
        }

        void Abort()
        {
            _waitEvent.SetEvent();
        }

    private:
        void StateChange(PluginHost::IShell* plugin)
        {

            if (_config.Callsign.Value() == plugin->Callsign()) {

                if ( ((_config.Active.Value() == true)  && (plugin->State() == PluginHost::IShell::ACTIVATED))   || 
					 ((_config.Active.Value() == false) && (plugin->State() == PluginHost::IShell::DEACTIVATED)) ) {
                    _waitEvent.SetEvent();
                }
            }
        }

    private:
        Config _config;
        Core::Event _waitEvent;
        Observer* _observer;
    };
}
}
}
