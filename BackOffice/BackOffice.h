// Copyright (c) 2022 Metrological. All rights reserved.

#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

    class BackOffice : public PluginHost::IPlugin {
    public:
        BackOffice()
            : _service(nullptr)
        {
        }
        ~BackOffice() override = default;

        BackOffice(const BackOffice&) = delete;
        BackOffice& operator=(const BackOffice&) = delete;

        BEGIN_INTERFACE_MAP(BackOffice)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        PluginHost::IShell* _service;
    };

} // namespace
} // namespace