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

#include "Module.h"
#include <interfaces/IRDKAdapter.h>

namespace WPEFramework {
namespace Plugin {

    class RDKAdapter : public PluginHost::IPlugin, public Exchange::IRDKAdapter {
    public:
        RDKAdapter(const RDKAdapter&) = delete;
        RDKAdapter(RDKAdapter&&) = delete;
        RDKAdapter& operator=(const RDKAdapter&) = delete;
        RDKAdapter& operator=(RDKAdapter&&) = delete;

        RDKAdapter()
            : PluginHost::IPlugin()
            , Exchange::IRDKAdapter()
        {
        }

        ~RDKAdapter() override = default;

        BEGIN_INTERFACE_MAP(RDKAdapter)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(Exchange::IRDKAdapter)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IRDKAdapter methods
        // -------------------------------------------------------------------------------------------------------
        void Test() const override;

    private:
    };

} // namespace Plugin
} // namespace WPEFramework
