/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
        Register<ResumeParamsData,void>(_T("resume"), &FirmwareControl::endpoint_resume, this);
        Property<Core::JSON::EnumType<StatusType>>(_T("status"), &FirmwareControl::get_status, nullptr, this);
        Property<Core::JSON::DecUInt64>(_T("downloadsize"), &FirmwareControl::get_downloadsize, nullptr, this);
    }

    void FirmwareControl::UnregisterAll()
    {
        Unregister(_T("resume"));
        Unregister(_T("upgrade"));
        Unregister(_T("downloadsize"));
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
    //  - ERROR_ILLEGAL_STATE: Invalid state of device
    //  - ERROR_INCORRECT_HASH: Incorrect hash given
    uint32_t FirmwareControl::endpoint_upgrade(const UpgradeParamsData& params)
    {
        TRACE(Trace::Information, (string(__FUNCTION__)));

        uint32_t result = Core::ERROR_NONE;
        const string& name = params.Name.Value();

        _adminLock.Lock();
        UpgradeStatus upgradeStatus = _upgradeStatus;
        _adminLock.Unlock();

        TRACE(Trace::Information, (_T("status = [%s] \n"), Core::EnumerateType<JsonData::FirmwareControl::StatusType>(upgradeStatus).Data()));

        if (upgradeStatus == UpgradeStatus::NONE) {
            if (result == Core::ERROR_NONE) {
                if (name.empty() != true) {

                    _adminLock.Lock();
                    _upgradeStatus = UPGRADE_STARTED;
                    _adminLock.Unlock();

                    string locator = _source;
                    if (params.Location.IsSet() == true) {
                        locator = params.Location.Value();
                    }
                    TRACE(Trace::Information, (_T("Image = [%s/%s]\n"), locator.c_str(), name.c_str()));

                    Type type = IMAGE_TYPE_CDL;
                    if (params.Type.IsSet() == true) {
                        type = static_cast<Type>(params.Type.Value()); 
                    }
                    TRACE(Trace::Information, (_T("Image Type = [%d]\n"), type));

                    uint16_t progressInterval = 0; 
                    if (params.Progressinterval.IsSet() == true) {
                        progressInterval = params.Progressinterval.Value();
                    }
                    TRACE(Trace::Information, (_T("Progress Interval = [%d]\n"), progressInterval));
                    string hash;
                    if (params.Hmac.IsSet() == true) {
                        if (params.Hmac.Value().size() == (Crypto::HASH_SHA256 * 2)) {
                            hash = params.Hmac.Value();
                        } else {
                            result = Core::ERROR_INCORRECT_HASH;
                        }
                        TRACE(Trace::Information, (_T("Hash = [%s]\n"), hash.c_str()));
                    }
                    if (result == Core::ERROR_NONE) {
                        TRACE(Trace::Information, (_T("Scheduling the upgrade \n")));
                        result = Schedule(name, locator, type, progressInterval, hash, false);
                    }
      
                } else {
                    result = Core::ERROR_BAD_REQUEST;
                }
            }
        } else {
            result = Core::ERROR_INPROGRESS;
        }

        if ((result != Core::ERROR_NONE) && (result != Core::ERROR_INPROGRESS)) {
            ResetStatus();
        }
        TRACE(Trace::Information, (_T("Status of upgrade request = %d\n"), result));
        return result;
    }

    // Method: resume - Resume download from the last paused position"
    // Return codes:
    //  - ERROR_NONE: Success
    //  - ERROR_INPROGRESS: Operation in progress
    //  - ERROR_INCORRECT_URL: Invalid location given
    //  - ERROR_UNAVAILABLE: Error in download
    //  - ERROR_BAD_REQUEST: Bad file name given
    //  - ERROR_ILLEGAL_STATE: Invalid state of device
    uint32_t FirmwareControl::endpoint_resume(const ResumeParamsData& params)
    {
        TRACE(Trace::Information, (string(__FUNCTION__)));

        uint32_t result = Core::ERROR_NONE;
        const string& name = params.Name.Value();

        _adminLock.Lock();
        UpgradeStatus upgradeStatus = _upgradeStatus;
        _adminLock.Unlock();

        TRACE(Trace::Information, (_T("status = [%s] \n"), Core::EnumerateType<JsonData::FirmwareControl::StatusType>(upgradeStatus).Data()));

        if (upgradeStatus == UpgradeStatus::NONE) {
            if (result == Core::ERROR_NONE) {
                if (name.empty() != true) {

                    _adminLock.Lock();
                    _upgradeStatus = UPGRADE_STARTED;
                    _adminLock.Unlock();

                    string locator = _source;
                    if (params.Location.IsSet() == true) {
                        locator = params.Location.Value();
                    }
                    TRACE(Trace::Information, (_T("Image = [%s/%s]\n"), locator.c_str(), name.c_str()));

                    if (result == Core::ERROR_NONE) {
                        TRACE(Trace::Information, (_T("Scheduling the upgrade \n")));
                        result = Schedule(name, locator);
                    }
                } else {
                    result = Core::ERROR_BAD_REQUEST;
                }
            }
        } else {
            result = Core::ERROR_INPROGRESS;
        }

        if ((result != Core::ERROR_NONE) && (result != Core::ERROR_INPROGRESS)) {
            ResetStatus();
        }
        TRACE(Trace::Information, (_T("Status of resume request = %d\n"), result));
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
        TRACE(Trace::Information, (_T("status = [%s] \n"), Core::EnumerateType<JsonData::FirmwareControl::StatusType>(response).Data()));

        return Core::ERROR_NONE;
    }

    // Property: downloadsize - Max free space available to download image
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t FirmwareControl::get_downloadsize(Core::JSON::DecUInt64& response) const
    {
        response = DownloadMaxSize();

        return Core::ERROR_NONE;
    }

    // Event: upgradeprogress - Notifies progress of upgrade
    void FirmwareControl::event_upgradeprogress(const StatusType& status, const UpgradeprogressParamsData::ErrorType& error, const uint32_t& progress)
    {
        UpgradeprogressParamsData params;
        params.Status = status;
        params.Error = error;
        params.Progress = progress;

        TRACE(Trace::Information, (_T("status = [%s] error = [%s] progress = [%d]\n"),
              Core::EnumerateType<JsonData::FirmwareControl::StatusType>(status).Data(),
              Core::EnumerateType<JsonData::FirmwareControl::UpgradeprogressParamsData::ErrorType>(error).Data(),
              progress));
        Notify(_T("upgradeprogress"), params);
    }

} // namespace Plugin

}

