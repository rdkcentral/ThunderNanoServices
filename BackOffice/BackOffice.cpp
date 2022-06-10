// Copyright (c) 2022 Metrological. All rights reserved.
#include "BackOffice.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(BackOffice, 1, 0);

    const string BackOffice::Initialize(PluginHost::IShell* service)
    {
        string result;
        service->Register(&_stateChangeObserver);
        _stateChangeObserver.Register(std::bind(&RequestSender::Send, &_requestSender, std::placeholders::_1, std::placeholders::_2));

        return result;
    }
    void BackOffice::Deinitialize(PluginHost::IShell* service)
    {
        _stateChangeObserver.Unregister();
        service->Unregister(&_stateChangeObserver);
    }
    string BackOffice::Information() const
    {
        return _T("");
    }
}
}