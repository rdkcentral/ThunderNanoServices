
#include "Module.h"
#include "SubsystemControl.h"

/*
    // Copy the code below to Butler class definition
    // Note: The Butler class must inherit from PluginHost::JSONRPC


*/

namespace Thunder {

namespace Plugin {

    // Registration
    //
    void SubsystemControl::RegisterAll()
    {

        PluginHost::JSONRPC::Register<JsonData::SubsystemControl::ActivateParamsData, Core::JSON::DecUInt32 >(_T("activate"), &SubsystemControl::activate, this);
        PluginHost::JSONRPC::Register<Core::JSON::EnumType<JsonData::SubsystemControl::SubsystemType>, void >(_T("deactivate"), &SubsystemControl::deactivate, this);
    }

    void SubsystemControl::UnregisterAll()
    {
        PluginHost::JSONRPC::Unregister(_T("deactivate"));
        PluginHost::JSONRPC::Unregister(_T("activate"));
    }

    // API implementation
    //

    uint32_t SubsystemControl::activate(const JsonData::SubsystemControl::ActivateParamsData& parameter, Core::JSON::DecUInt32& response) {

        PluginHost::ISubSystem::subsystem system;

        if (_subsystemFactory.Lookup(parameter.System.Value(), system) == true) {
            Core::IUnknown* metadata = _subsystemFactory.Metadata(parameter.System.Value(), parameter.Configuration.Value());
            _service->Set(system, metadata);

            if (metadata != nullptr) {
                metadata->Release();
            }
            return (Core::ERROR_NONE);
        }

        return (Core::ERROR_UNKNOWN_KEY);
    }

    uint32_t SubsystemControl::deactivate(const Core::JSON::EnumType<JsonData::SubsystemControl::SubsystemType>& parameter) {

        PluginHost::ISubSystem::subsystem system;

        if (_subsystemFactory.Lookup(parameter.Value(), system) == true) {
            system = static_cast<PluginHost::ISubSystem::subsystem>(system | PluginHost::ISubSystem::subsystem::NEGATIVE_START);
            _service->Set(system, nullptr);

            return (Core::ERROR_NONE);
        }

        return (Core::ERROR_UNKNOWN_KEY);
    }

    // Event: activity - Notifies about device activity
    void SubsystemControl::event_activity()
    {
        PluginHost::JSONRPC::Notify(_T("activity"));
    }

} // namespace Plugin

}

