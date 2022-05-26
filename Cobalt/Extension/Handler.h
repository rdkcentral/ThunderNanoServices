/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#pragma once

#include "../Module.h"
#include "extension/platform_service.h"

namespace WPEFramework {
namespace Plugin {

    struct IHandler {
        virtual ~IHandler() = default;
        virtual void* Send(void* data, uint64_t length, uint64_t* outputLength) = 0;
    };

    class HandlerAdministrator {
    private:
        struct IFactory {
            virtual ~IFactory() = default;
            virtual IHandler* Factory(void* context, const char* name, ReceiveMessageCallback receiveCallback) = 0;
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
            ~Entry() override = default;
            IHandler* Factory(void* context, const char* name, ReceiveMessageCallback receiveCallback) override
            {
                return (new HANDLER(context, name, receiveCallback));
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
        IHandler* Handler(void* context, const char* name, ReceiveMessageCallback receiveCallback)
        {
            IHandler* result = nullptr;
            string servicename(name);
            string classname = servicename.substr(servicename.find_last_of(".") + 1);
            if (!classname.empty()) {
                std::map<const string, IFactory*>::iterator index = _factories.find(classname);
                if (index != _factories.end()) {
                    result = index->second->Factory(context, name, receiveCallback);
                }
            }
            return (result);
        }
        bool Exists(const char* name)
        {
            bool result = false;
            string servicename(name);
            string classname = servicename.substr(servicename.find_last_of(".") + 1);
            if (!classname.empty()) {
                std::map<const string, IFactory*>::iterator index = _factories.find(classname);
                if (index != _factories.end()) {
                    result = true;
                }
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

} // namespace Plugin
} // namespace WPEFramework
