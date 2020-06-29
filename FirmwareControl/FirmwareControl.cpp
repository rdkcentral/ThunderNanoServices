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

#include "FirmwareControl.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(FirmwareControl, 1, 0);

    /* virtual */ const string FirmwareControl::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        Config config;
        config.FromString(service->ConfigLine());
        if (config.Source.IsSet() == true) {
            _source = config.Source.Value();
            TRACE_L1("Source location : [%s]\n", _source.c_str());
        }
        if (config.Download.IsSet() == true) {
            _destination = config.Download.Value() + "/";
            TRACE_L1("Destination location : [%s]\n", _destination.c_str());
        }
        if (config.WaitTime.IsSet() == true) {
            _waitTime = config.WaitTime.Value();
        }

        string message;
        uint32_t status = ConvertMfrStatusToCore(mfrFWUpgradeInit());
        if (status != Core::ERROR_NONE) {
            message = _T("Error in MFR library initialization");
        }

        return (message);
    }

    /* virtual */ void FirmwareControl::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        string message;
        uint32_t status = ConvertMfrStatusToCore(mfrFWUpgradeTerm());
        if (status != Core::ERROR_NONE) {
            message = _T("Error in MFR library deinitialization");
        }
    }

    /* virtual */ string FirmwareControl::Information() const
    {
        // No additional info to report.
        return (string());
    }

    uint32_t FirmwareControl::Schedule(const std::string& name, const std::string& path, const FirmwareControl::Type& type, const uint16_t& interval, const std::string& hash)
    {
        TRACE(Trace::Information, (string(__FUNCTION__)));
        if (path.empty() != true) {
            _source = path + "/" + name;
        }
        _adminLock.Lock();
        _type = type;
        _interval = interval;
        _hash = hash;

        _upgrader->Schedule();
        _adminLock.Unlock();

        return Core::ERROR_NONE;
    }

    void FirmwareControl::Upgrade() {
        TRACE(Trace::Information, (string(__FUNCTION__)));
        uint32_t status = Download();
        if (status == Core::ERROR_NONE && (Status() != UpgradeStatus::UPGRADE_CANCELLED)) {
            Install();
        }
    }

    void FirmwareControl::Install() {
        TRACE(Trace::Information, (string(__FUNCTION__)));
        //Setup callback handler;
        mfrUpgradeStatusNotify_t mfrNotifier;
        mfrNotifier.cbData = reinterpret_cast<void*>(this);
        mfrNotifier.interval = static_cast<int>(_interval);
        mfrNotifier.cb = Callback;

        // Initiate image install
        mfrError_t mfrStatus = mfrWriteImage(Name, _destination.c_str(), static_cast<mfrImageType_t>(_type), mfrNotifier);
        if (mfrERR_NONE != mfrStatus) {
            Status(UpgradeStatus::INSTALL_ABORTED, ConvertMfrStatusToCore(mfrStatus), 0);
        } else {
            Status(UpgradeStatus::INSTALL_INITIATED, ErrorType::ERROR_NONE, 0);
            uint32_t status = WaitForCompletion(_waitTime); // To avoid hang situation
            if (status != Core::ERROR_NONE) {
                Status(UpgradeStatus::INSTALL_ABORTED, Core::ERROR_TIMEDOUT, 0);
            } else {
                _adminLock.Lock();
                mfrUpgradeStatus_t installStatus = _installStatus;
                _adminLock.Unlock();
                UpgradeStatus upgradeStatus = ConvertMfrWriteStatusToUpgradeStatus(installStatus.progress);
                TRACE(Trace::Information, (_T("Installation completed with status : %s\n"), ((upgradeStatus == UPGRADE_COMPLETED) ? "Success": "Failure")));
                Status(upgradeStatus, ConvertMfrWriteErrorToUpgradeType(installStatus.error), installStatus.percentage);
            }
        }
    }

    uint32_t FirmwareControl::Download() {

        TRACE(Trace::Information, (string(__FUNCTION__)));
        Notifier notifier(this);

        PluginHost::DownloadEngine downloadEngine(&notifier, _destination + Name);

        uint32_t status = downloadEngine.Start(_source, _destination, _hash);
        if ((status == Core::ERROR_NONE) || (status == Core::ERROR_INPROGRESS)) {

            Status(UpgradeStatus::DOWNLOAD_STARTED, ErrorType::ERROR_NONE, 0);
            downloadEngine.StartProgressNotifier(_interval);
            status = WaitForCompletion(_waitTime);
            if ((status == Core::ERROR_NONE) && (DownloadStatus() == Core::ERROR_NONE)) {
                 if (_hash.empty() != true) {
                     status = downloadEngine.CheckHMAC();
                 }
            }
        }

        status = ((status != Core::ERROR_NONE)? status: DownloadStatus());
        if (status == Core::ERROR_NONE) {
            Status(UpgradeStatus::DOWNLOAD_COMPLETED, ErrorType::ERROR_NONE, 100);
        } else {
            Status(UpgradeStatus::DOWNLOAD_ABORTED, status, 0);
        }

        return status;
    }

} // namespace Plugin
} // namespace WPEFramework
