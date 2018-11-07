#ifndef __HANDLER_H
#define __HANDLER_H

#include "Module.h"
#include "GPIO.h"

namespace WPEFramework {

namespace Plugin {

    struct IHandler{
        virtual ~IHandler() {}
        virtual void Trigger(GPIO::Pin& pin) = 0;
    };

    class HandlerAdministrator {
    private:
        struct IFactory {
            virtual ~IFactory() {}
            virtual IHandler* Factory(PluginHost::IShell* service, const string& configuration) = 0;
        };

        HandlerAdministrator(const HandlerAdministrator&) = delete;
        HandlerAdministrator& operator= (const HandlerAdministrator&) = delete;

        HandlerAdministrator() : _factories() {
        }

    public:
        template<typename HANDLER>
        class Entry : public HandlerAdministrator::IFactory {
        private:
            Entry(const Entry<HANDLER>&);
            Entry<HANDLER>& operator&(const Entry<HANDLER>&);

        public:
            Entry() {
                HandlerAdministrator::Instance().Announce (Core::ClassNameOnly(typeid(HANDLER).name()).Text(), this);
            }
            virtual ~Entry() {
            }
            virtual IHandler* Factory(PluginHost::IShell* service, const string& configuration) override {
                return (new HANDLER(service, configuration));
            }
        };
 
        static HandlerAdministrator& Instance() {
            return(_administrator);
        }
        ~HandlerAdministrator() {
        }

    public:
       IHandler* Handler(const string& name, PluginHost::IShell* service, const string& configuration) {
            IHandler* result = nullptr;
            std::map<const string, IFactory*>::iterator index = _factories.find(name);
            if (index != _factories.end()) {
                result = index->second->Factory(service, configuration);
            }
            return (result);
        }

    private:
        void Announce(const string& info, IFactory* factory) {
            _factories.insert(std::pair<const string, IFactory*>(info, factory));
        }

    private:
        std::map<const string, IFactory*> _factories;

        static HandlerAdministrator _administrator;
    };

} } // Namespace WPEFramework::Plugin

#endif // __HANDLER_H
