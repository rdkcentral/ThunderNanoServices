// Copyright (c) 2022 Metrological. All rights reserved.
#include "BackOffice.h"

namespace WPEFramework {
namespace Plugin {

    const string BackOffice::Initialize(PluginHost::IShell* service)
    {
        string result;

        return result;
    }
    void BackOffice::Deinitialize(PluginHost::IShell* service)
    {
    }
    string BackOffice::Information() const
    {
        return _T("");
    }
}
}