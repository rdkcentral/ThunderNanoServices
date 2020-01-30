
#include "Module.h"
#include "InputSwitch.h"
#include <interfaces/json/JsonData_InputSwitch.h>

/*
    // Copy the code below to InputSwitch class definition
    // Note: The InputSwitch class must inherit from PluginHost::JSONRPC

    private:
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_channel(const JsonData::InputSwitch::ChannelParamsInfo& params);
        uint32_t endpoint_status(const JsonData::InputSwitch::StatusParamsData& params, Core::JSON::ArrayType<JsonData::InputSwitch::ChannelParamsInfo>& response);
*/

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::InputSwitch;

    // Registration
    //

    void InputSwitch::RegisterAll()
    {
        Register<ChannelParamsInfo,void>(_T("channel"), &InputSwitch::endpoint_channel, this);
        Register<StatusParamsData,Core::JSON::ArrayType<ChannelParamsInfo>>(_T("status"), &InputSwitch::endpoint_status, this);
    }

    void InputSwitch::UnregisterAll()
    {
        Unregister(_T("status"));
        Unregister(_T("channel"));
    }

    // API implementation
    //

    // Method: channel - Enable or Disable the throughput throough the given channel
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Failed to scan
    uint32_t InputSwitch::endpoint_channel(const ChannelParamsInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const bool enabled = params.Enabled.Value();

        if (params.Name.IsSet() == true) {
            if (FindChannel(params.Name.Value()) == false) {
                result = Core::ERROR_UNKNOWN_KEY;
            }
            else {
                _handler.Enable(enabled);

                _handler.Reset();
            }
        }
        else {
            // Set all channels to the requested status..
            _handler.Reset();
            while (_handler.Next() == true) {
                _handler.Enable(enabled);
            }
        }

        return result;
    }

    // Method: status - Check the status of the requested channel
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_UNKNOWN_KEY: Could not find the designated channel
    uint32_t InputSwitch::endpoint_status(const StatusParamsData& params, Core::JSON::ArrayType<ChannelParamsInfo>& response)
    {
        uint32_t result = Core::ERROR_NONE;

        if (params.Name.IsSet() == true) {
            if (FindChannel(params.Name.Value()) == false) {
                result = Core::ERROR_UNKNOWN_KEY;
            }
            else {
                ChannelParamsInfo& element(response.Add());

                element.Name = _handler.Name();
                element.Enabled = _handler.Enabled();

                _handler.Reset();
            }
        }
        else {
            // Insert all channels with there status..
            _handler.Reset();
            while (_handler.Next() == true) {
                ChannelParamsInfo& element(response.Add());

                element.Name = _handler.Name();
                element.Enabled = _handler.Enabled();
            }
        }

        return result;
    }

} // namespace Plugin

}

