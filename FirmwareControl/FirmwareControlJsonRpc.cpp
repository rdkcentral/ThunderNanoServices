
#include "Module.h"
#include "FirmwareControl.h"
#include <interfaces/json/JsonData_FirmwareControl.h>

namespace WPEFramework {

namespace Plugin {

    using namespace JsonData::FirmwareControl;

    // Registration
    //

    void FirmwareControl::RegisterAll()
    {
        Register<UpgradeParamsData,void>(_T("upgrade"), &FirmwareControl::endpoint_upgrade, this);
        Property<Core::JSON::EnumType<StatusType>>(_T("status"), &FirmwareControl::get_status, nullptr, this);
    }

    void FirmwareControl::UnregisterAll()
    {
        Unregister(_T("upgrade"));
        Unregister(_T("status"));
    }

    // API implementation
    //

    // Method: upgrade - Upgrade the device to the given firmware
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INPROGRESS: Operation in progress
    //  - ERROR_INCORRECT_URL: Invalid location given
    //  - ERROR_UNAVAILABLE: Error in download
    //  - ERROR_BAD_REQUEST: Bad file name given
    //  - ERROR_UNKNOWN_KEY: Bad hash value given
    //  - ERROR_ILLEGAL_STATE: Invalid state of device
    //  - ERROR_INCORRECT_HASH: Incorrect hash given
    uint32_t FirmwareControl::endpoint_upgrade(const UpgradeParamsData& params)
    {
        uint32_t result = Core::ERROR_NONE;
        const string& name = params.Name.Value();

        _adminLock.Lock();
        UpgradeStatus upgradeStatus = _upgradeStatus;
        _adminLock.Unlock();
        if (upgradeStatus != UpgradeStatus::NONE) {
            if (result == Core::ERROR_NONE) {
                if (name.empty() != true) {

                    _adminLock.Lock();
                    _upgradeStatus = UPGRADE_STARTED;
                    _adminLock.Unlock();

                    string locator = _source;
                    if (params.Location.IsSet() != true) {
                        locator = params.Location.Value();
                    }
                    Type type = IMAGE_TYPE_CDL;
                    if (params.Type.IsSet() == true) {
                        type = static_cast<Type>(params.Type.Value()); 
                    }
                    uint16_t progressInterval = 0; 
                    if (params.Progressinterval.IsSet() == true) {
                        progressInterval = params.Progressinterval.Value();
                    }
                    string hash;
                    if (params.Hmac.IsSet() == true) {
                        if (params.Hmac.Value().size() == (Crypto::HASH_SHA256 * 2)) {
                            hash = params.Hmac.Value();
                        } else {
                            result = Core::ERROR_INCORRECT_HASH;
                        }
                    }
                    if (result == Core::ERROR_NONE) {
                        result = Schedule(name, locator, type, progressInterval, hash);
                    }
      
                } else {
                    result = Core::ERROR_BAD_REQUEST;
                }
            }
        } else {
            result = Core::ERROR_INPROGRESS;
        }
        return result;
    }

    // Property: status - Current status of a upgrade
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t FirmwareControl::get_status(Core::JSON::EnumType<StatusType>& response) const
    {
        _adminLock.Lock();
        response = static_cast<JsonData::FirmwareControl::StatusType>(_upgradeStatus);
        _adminLock.Unlock();
        return Core::ERROR_NONE;
    }

    // Event: upgradeprogress - Notifies progress of upgrade
    void FirmwareControl::event_upgradeprogress(const StatusType& status, const UpgradeprogressParamsData::ErrorType& error, const uint16_t& percentage)
    {
        UpgradeprogressParamsData params;
        params.Status = status;
        params.Error = error;
        params.Percentage = percentage;

        Notify(_T("upgradeprogress"), params);
    }

} // namespace Plugin

}

