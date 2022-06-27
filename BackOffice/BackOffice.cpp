// Copyright (c) 2022 Metrological. All rights reserved.
#include "BackOffice.h"
#include <regex>

namespace WPEFramework {
namespace Plugin {
    namespace {
        std::list<std::pair<string, string>> CreateQueryParameters(const BackOffice::Config& config)
        {
            std::list<std::pair<string, string>> result;
            if (config.Customer.IsSet() && !config.Customer.Value().empty()) {
                result.emplace_back(_T("customer"), config.Customer.Value());
            }
            if (config.Platform.IsSet() && !config.Platform.Value().empty()) {
                result.emplace_back(_T("platform"), config.Platform.Value());
            }
            if (config.Country.IsSet() && !config.Country.Value().empty()) {
                result.emplace_back(_T("country"), config.Country.Value());
            }
            result.emplace_back(_T("type"), _T("app"));
            return result;
        }

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

        std::vector<std::string> TokenizeString(const std::string& buffer, char delimiter)
        {
            std::vector<std::string> result;

            if (!buffer.empty()) {
                std::istringstream iss(buffer);
                std::string token;

                while (iss.good()) {
                    getline(iss, token, delimiter);
                    result.push_back(token);
                }
            }

            return result;
        }

        template <typename T>
        string GetMappings(T& mapping, Core::JSON::ArrayType<Core::JSON::String>::Iterator iterator)
        {
            string result;
            while (iterator.Next()) {
                auto tokens = TokenizeString(iterator.Current().Value(), _T(','));
                if (tokens.size() == 2) {
                    mapping.emplace(tokens[0], tokens[1]);
                } else {
                    result = _T("Failed parsing callsign mapping!");
                }
            }
            return result;
        }
    }

    SERVICE_REGISTRATION(BackOffice, 1, 0);

    const string BackOffice::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        string result;

        _config.FromString(service->ConfigLine());
        auto queryParameters = CreateQueryParameters(_config);
        if (queryParameters.size() > 0) {
            result = GetMappings(_callsignMappings, _config.CallsignMapping.Elements());
            if (result.empty()) {
                result = GetMappings(_stateMappings, _config.StateMapping.Elements());
            }
        } else {
            result = _T("Unable to create query parameters");
        }

        if (result.empty()) {
            _subSystem = service->SubSystems();
            if (_subSystem != nullptr) {
                _subSystem->AddRef();
                auto deviceId = DeviceIdentifier(_subSystem);
                if (deviceId.IsSet()) {
                    auto deviceIdNormalized = deviceId.Value();
                    deviceIdNormalized = std::regex_replace(deviceIdNormalized, std::regex(_T("WPE")), "");
                    queryParameters.emplace_back(_T("household"), deviceIdNormalized);
                }
            } else {
                result = _T("Unable to obtain subsystem");
            }
        }

        if (result.empty()) {
            if (_config.ServerAddress.IsSet() && !_config.ServerAddress.Value().empty() && _config.ServerPort.IsSet()) {
                _requestSender.reset(new RequestSender(Core::NodeId(_config.ServerAddress.Value().c_str(), _config.ServerPort.Value()), queryParameters));
                if (_requestSender != nullptr) {
                    for (const auto& callsign : _callsignMappings) {
                        _stateChangeObserver.RegisterObservable(callsign.first);
                    }
                    _stateChangeObserver.Register(std::bind(&BackOffice::Send, this, std::placeholders::_1, std::placeholders::_2));
                    service->Register(&_stateChangeObserver);
                }
            } else {
                result = _T("Server address or port not specified!");
            }
        }

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
    void BackOffice::Send(const string& state, const string& callsign)
    {
        auto id = _callsignMappings.find(callsign);
        auto event = _stateMappings.find(state);

        if (id != _callsignMappings.end() && event != _stateMappings.end()) {
            _requestSender->Send(event->second, id->second);
        }
    }
}
}