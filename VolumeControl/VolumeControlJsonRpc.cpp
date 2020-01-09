
#include "Module.h"
#include "VolumeControl.h"
#include <interfaces/json/JsonData_VolumeControl.h>

/*
    // Copy the code below to VolumeControl class definition
    // Note: The VolumeControl class must inherit from PluginHost::JSONRPC

    private:
        void RegisterAll();
        void UnregisterAll();
        uint32_t get_volume(Core::JSON::DecUInt8& response) const;
        uint32_t set_volume(const Core::JSON::DecUInt8& param);
        uint32_t get_muted(Core::JSON::Boolean& response) const;
        uint32_t set_muted(const Core::JSON::Boolean& param);
        void event_volumechange(const uint8_t& volume);
        void event_mutedchange(const bool& muted);
*/

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::VolumeControl;

    // Registration
    //

    void VolumeControl::RegisterAll()
    {
        Property<Core::JSON::DecUInt8>(_T("volume"), &VolumeControl::get_volume, &VolumeControl::set_volume, this);
        Property<Core::JSON::Boolean>(_T("muted"), &VolumeControl::get_muted, &VolumeControl::set_muted, this);
    }

    void VolumeControl::UnregisterAll()
    {
        Unregister(_T("muted"));
        Unregister(_T("volume"));
    }

    // API implementation
    //

    // Property: volume - Volume in percents
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t VolumeControl::get_volume(Core::JSON::DecUInt8& response) const
    {
        uint8_t vol = 100;
        uint32_t result = const_cast<const Exchange::IVolumeControl*>(_implementation)->Volume(vol);
        response = vol;
        return result;
    }

    // Property: volume - Volume in percents
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t VolumeControl::set_volume(const Core::JSON::DecUInt8& param)
    {
        return _implementation->Volume(param.Value());
    }

    // Property: muted - Drives whether audop should be muted
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t VolumeControl::get_muted(Core::JSON::Boolean& response) const
    {
        bool muted = false;
        uint32_t result = const_cast<const Exchange::IVolumeControl*>(_implementation)->Muted(muted);
        response = muted;
        return result;
    }

    // Property: muted - Drives whether audop should be muted
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t VolumeControl::set_muted(const Core::JSON::Boolean& param)
    {
        return _implementation->Muted(param.Value());
    }

    // Event: volumechange - Signals change of the volume
    void VolumeControl::event_volumechange(const uint8_t& volume)
    {
        VolumechangeParamsData params;
        params.Volume = volume;

        Notify(_T("volumechange"), params);
    }

    // Event: mutedchange - Signals change of the muted property
    void VolumeControl::event_mutedchange(const bool& muted)
    {
        MutedchangeParamsData params;
        params.Muted = muted;

        Notify(_T("mutedchange"), params);
    }

} // namespace Plugin

}

