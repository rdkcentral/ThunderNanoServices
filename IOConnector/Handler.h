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
 
#ifndef __HANDLER_H
#define __HANDLER_H

#include "Module.h"
#include "GPIO.h"

namespace WPEFramework {

namespace Plugin {

    struct IHandler {
        virtual ~IHandler() {}
        virtual void Trigger(GPIO::Pin& pin) = 0;
    };

    class HandlerAdministrator {
    private:
        struct IFactory {
            virtual ~IFactory() {}
            virtual IHandler* Factory(PluginHost::IShell* service, const string& configuration, const uint32_t start, const uint32_t end) = 0;
        };

        HandlerAdministrator(const HandlerAdministrator&) = delete;
        HandlerAdministrator& operator=(const HandlerAdministrator&) = delete;

        HandlerAdministrator()
            : _factories()
        {
        }

    public:
        template <typename HANDLER>
        class Entry : public HandlerAdministrator::IFactory {
        private:
            Entry(const Entry<HANDLER>&);
            Entry<HANDLER>& operator&(const Entry<HANDLER>&);

        public:
            Entry()
            {
                HandlerAdministrator::Instance().Announce(Core::ClassNameOnly(typeid(HANDLER).name()).Text(), this);
            }
            virtual ~Entry()
            {
            }
            virtual IHandler* Factory(PluginHost::IShell* service, const string& configuration, const uint32_t start, const uint32_t end) override
            {
                return (new HANDLER(service, configuration, start, end));
            }
        };

        static HandlerAdministrator& Instance()
        {
            return (_administrator);
        }
        ~HandlerAdministrator()
        {
        }

    public:
        IHandler* Handler(const string& name, PluginHost::IShell* service, const string& configuration, const uint32_t start, const uint32_t end)
        {
            IHandler* result = nullptr;
            std::map<const string, IFactory*>::iterator index = _factories.find(name);
            if (index != _factories.end()) {
                result = index->second->Factory(service, configuration, start, end);
            }
            return (result);
        }

    private:
        void Announce(const string& info, IFactory* factory)
        {
            _factories.insert(std::pair<const string, IFactory*>(info, factory));
        }

    private:
        std::map<const string, IFactory*> _factories;

        static HandlerAdministrator _administrator;
    };
}
} // Namespace WPEFramework::Plugin

#endif // __HANDLER_H
