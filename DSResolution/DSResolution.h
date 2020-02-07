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
 
#ifndef __DSRESOLUTION_H
#define __DSRESOLUTION_H

#include "DSResolutionHAL.h"
#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class DSResolution : public PluginHost::IPlugin, public PluginHost::IWeb {
    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Resolution()
            {
                Add(_T("resolution"), &Resolution);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::EnumType<DSResolutionHAL::PixelResolution> Resolution;
        };

    private:
        DSResolution(const DSResolution&) = delete;
        DSResolution& operator=(const DSResolution&) = delete;

    public:
        DSResolution()
            : _skipURL(0)
            , _controller()
        {
        }

        virtual ~DSResolution()
        {
        }

        BEGIN_INTERFACE_MAP(DSResolution)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        END_INTERFACE_MAP

    public:
        // IPlugin Methods
        // ------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service);
        virtual void Deinitialize(PluginHost::IShell* service);
        virtual string Information() const;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);

    private:
        uint8_t _skipURL;
        DSResolutionHAL _controller;
    };

} //Plugin
} //WPEFramework

#endif // __DSRESOLUTION_H
