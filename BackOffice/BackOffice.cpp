// Copyright (c) 2022 Metrological. All rights reserved.
#include "BackOffice.h"

namespace Thunder {

    ENUM_CONVERSION_BEGIN(Plugin::BackOffice::reportstate)

        { Plugin::BackOffice::reportstate::UNAVAILABLE, _TXT("Unavailable")  },
        { Plugin::BackOffice::reportstate::DEACTIVATED, _TXT("Deactivated")  },
        { Plugin::BackOffice::reportstate::ACTIVATED,   _TXT("Activated")    },
        { Plugin::BackOffice::reportstate::SUSPENDED,   _TXT("Suspended")    },
        { Plugin::BackOffice::reportstate::RESUMED,     _TXT("Resumed")      },

    ENUM_CONVERSION_END(Plugin::BackOffice::reportstate);

namespace Plugin {
    namespace {
        Core::OptionalType<string> DeviceIdentifier(PluginHost::ISubSystem* subSystem)
        {
            Core::OptionalType<string> result;
            ASSERT(subSystem != nullptr);

            if (subSystem->IsActive(PluginHost::ISubSystem::IDENTIFIER)) {

                const PluginHost::ISubSystem::IIdentifier* identifier(subSystem->Get<PluginHost::ISubSystem::IIdentifier>());

                if (identifier != nullptr) {
                    uint8_t buffer[64];

                    if ((buffer[0] = identifier->Identifier(sizeof(buffer) - 1, &(buffer[1]))) != 0) {
                        result = Core::SystemInfo::Instance().Id(buffer, ~0);
                    }
                    identifier->Release();
                }
            }
            return result;
        }

        static Metadata<BackOffice> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            { subsystem::PLATFORM, subsystem::INTERNET },
            // Terminations
            {},
            // Controls
            {}
        );
    }

    const string BackOffice::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        string result;
        Config config;
        QueryParameters  queryParameters;

        config.FromString(service->ConfigLine());

        if (CreateQueryParameters(service, config, queryParameters) != Core::ERROR_NONE) {
            result = _T("Could not extract the DeviceIdentifier");
        }
        else if (queryParameters.size() == 1) {
            result = _T("Unable to find any configured query parameters");
        }
        else if (_stateChangeObserver.Initialize(service, config) != Core::ERROR_NONE) {
            result = _T("Could not determine the callsigns to use");
        }
        else if ((config.ServerAddress.IsSet() == false) || (config.ServerAddress.Value().empty() == true) || (config.ServerPort.IsSet() == false)) {
            result = _T("Server address or port not specified!");
        }
        else if (_requestSender.Configure(Core::NodeId(config.ServerAddress.Value().c_str(), config.ServerPort.Value()), config.UserAgent.Value(), queryParameters) != Core::ERROR_NONE) {
            result = _T("Client connection could not be configured correctly!");
        }

        return result;
    }
    void BackOffice::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        _stateChangeObserver.Deinitialize(service);
        _requestSender.Close(Core::infinite);
    }
    string BackOffice::Information() const
    {
        return _T("");
    }
    void BackOffice::Send(const string& reportName, reportstate state)
    {
        Core::EnumerateType<reportstate> event(state);
        _requestSender.Send(event.Data(), reportName);
    }
    uint32_t BackOffice::CreateQueryParameters(PluginHost::IShell* service, const Config& config, QueryParameters& parameters)
    {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        PluginHost::ISubSystem* subSystem = service->SubSystems();

        if (subSystem != nullptr) {
            auto deviceId = DeviceIdentifier(subSystem);
            if (deviceId.IsSet() && !deviceId.Value().empty()) {
                if (config.Customer.IsSet() && !config.Customer.Value().empty()) {
                    parameters.emplace_back(_T("customer"), config.Customer.Value());
                }
                if (config.Platform.IsSet() && !config.Platform.Value().empty()) {
                    parameters.emplace_back(_T("platform"), config.Platform.Value());
                }
                if (config.Country.IsSet() && !config.Country.Value().empty()) {
                    parameters.emplace_back(_T("country"), config.Country.Value());
                }
                if (config.Type.IsSet() && !config.Type.Value().empty()) {
                    parameters.emplace_back(_T("type"), config.Type.Value());
                }
                if (config.Session.IsSet()) {
                    parameters.emplace_back(_T("session"), std::to_string(config.Session.Value()));
                }

                parameters.emplace_back(_T("household"), deviceId.Value());

                result = Core::ERROR_NONE;
            }
            subSystem->Release();
        }
        return result;
    }
}
}
