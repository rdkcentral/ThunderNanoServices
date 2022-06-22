// Copyright (c) 2022 Metrological. All rights reserved.
#include "BackOffice.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(BackOffice, 1, 0);

    const string BackOffice::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        string result;

        std::list<std::pair<string, string>> queryParameters;

        queryParameters.push_back({ _T("key"), _T("value") });
        service->Register(&_stateChangeObserver);
        _requestSender.reset(new RequestSender(Core::NodeId(_T("0.0.0.0"), 8081), queryParameters));

        _stateChangeObserver.Register(std::bind(&BackOffice::Send, this, std::placeholders::_1, std::placeholders::_2));

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
    void BackOffice::Send(PluginHost::IShell::state state, const string& callsign)
    {
        _requestSender->Send(_T("state"), _T("callsign"));
    }

}
}