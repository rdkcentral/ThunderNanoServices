/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#include "Module.h"

namespace Thunder {
namespace Plugin {

    class ConfigUpdateExample
        : public PluginHost::IPlugin {

    private:
        class Config : public Core::JSON::Container {
        public:
            class Plugin : public Core::JSON::Container {
            public:
                Plugin& operator=(const Plugin&) = delete;
                Plugin()
                    : Core::JSON::Container()
		    , Callsign()
                    , SystemRootPath()
		{
		    Add(_T("callsign"), &Callsign);
		    Add(_T("systemrootpath"), &SystemRootPath);
		}
                Plugin(const Plugin& copy)
                    : Core::JSON::Container()
		    , Callsign(copy.Callsign)
                    , SystemRootPath(copy.SystemRootPath)
		{
		    Add(_T("callsign"), &Callsign);
		    Add(_T("systemrootpath"), &SystemRootPath);
		}
		~Plugin() override = default;

            public:
                Core::JSON::String Callsign;
		Core::JSON::String SystemRootPath;
	    };

        public:
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , Plugins()
            {
                Add(_T("plugins"), &Plugins);
            }
            Config(const Config& copy)
                : Core::JSON::Container()
                , Plugins(copy.Plugins)
	    {
                Add(_T("plugins"), &Plugins);
	    }
            ~Config() override = default;

        public:
            Core::JSON::ArrayType<Plugin> Plugins;
        };

    public:
        ConfigUpdateExample(const ConfigUpdateExample&) = delete;
        ConfigUpdateExample& operator=(const ConfigUpdateExample&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        ConfigUpdateExample()
            : _service(nullptr)
        {
        }
POP_WARNING()

        ~ConfigUpdateExample() override
        {
        }

    public:
        BEGIN_INTERFACE_MAP(ConfigUpdateExample)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

    public:
        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        const string Initialize(PluginHost::IShell* service) override;

        // The plugin is unloaded from Thunder. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After theis call, the lifetime of the Service object ends.
        void Deinitialize(PluginHost::IShell* service) override;

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetData plugin to publish this information to the ouside world.
        string Information() const override;

    private:

        PluginHost::IShell* _service;
    };
} //namespace Plugin
} //namespace Thunder
