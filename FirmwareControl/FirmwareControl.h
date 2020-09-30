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

#pragma once

#include "Module.h"
#include "DownloadEngine.h"
#include <interfaces/json/JsonData_FirmwareControl.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include <mfrApi.h>
#ifdef __cplusplus
}
#endif

namespace WPEFramework {
namespace Plugin {

    class FirmwareControl : public PluginHost::IPlugin, public PluginHost::JSONRPC {

    public:
        enum Type {
            IMAGE_TYPE_CDL,
            IMAGE_TYPE_RCDL
        };
        enum UpgradeStatus {
            NONE,
            UPGRADE_STARTED,
            DOWNLOAD_STARTED,
            DOWNLOAD_ABORTED,
            DOWNLOAD_COMPLETED,
            INSTALL_INITIATED,
            INSTALL_NOT_STARTED,
            INSTALL_ABORTED,
            INSTALL_STARTED,
            UPGRADE_COMPLETED,
            UPGRADE_CANCELLED
        };
        enum ErrorType {
            ERROR_NONE,
            GENERIC,
            INVALID_PARAMETERS,
            INVALID_STATES,
            NO_ENOUGH_SPACE,
            OPERATION_NOT_PERMITTED,
            INCORRECT_HASH,
            UNAUTHENTICATED,
            UNAVAILABLE,
            TIMEDOUT,
            DOWNLOAD_DIR_NOT_EXIST,
            RESUME_NOT_SUPPORTED,
            INVALID_RANGE,
            UNKNOWN
        };
    private:
        static constexpr const TCHAR* Name = "imageTemp";
        static int32_t constexpr WaitTime = Core::infinite;

    private:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&);
            Config& operator=(const Config&);

            Config()
                : Core::JSON::Container()
                , Source()
                , Download()
                , WaitTime()
            {
                Add(_T("source"), &Source);
                Add(_T("download"), &Download);
                Add(_T("waittime"), &WaitTime);
            }

            ~Config() {}

        public:
            Core::JSON::String Source;
            Core::JSON::String Download;
            Core::JSON::DecSInt32 WaitTime;
        };

        class Notifier : public INotifier {
        public:
            Notifier() = delete;
            Notifier(const Notifier&) = delete;
            Notifier& operator=(const Notifier&) = delete;

            Notifier(FirmwareControl* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            virtual ~Notifier()
            {
            }
            virtual void NotifyStatus(const uint32_t status) override
            {
                _parent.NotifyDownloadStatus(status);
            }
            virtual void NotifyProgress(const uint32_t transferred) override
            {
                _parent.NotifyProgress(UpgradeStatus::DOWNLOAD_STARTED, ErrorType::ERROR_NONE, transferred);
            }

        private:
            FirmwareControl& _parent;
        };

        class Upgrader : public Core::IDispatch {
        public:
            Upgrader() = delete;
            Upgrader(const Upgrader&) = delete;
            Upgrader& operator=(const Upgrader&) = delete;

            Upgrader(FirmwareControl* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            virtual ~Upgrader()
            {
            }
            void Schedule() {
                Core::ProxyType<Core::IDispatch> job(*this);
                Core::IWorkerPool::Instance().Submit(job);
            }
            void Stop() {
                Core::ProxyType<Core::IDispatch> job(*this);
                Core::IWorkerPool::Instance().Revoke(job);
            }
            virtual void Dispatch() override {
                _parent.Upgrade();
            }

        private:
            FirmwareControl& _parent;
        };
    public:
        FirmwareControl(const FirmwareControl&) = delete;
        FirmwareControl& operator=(const FirmwareControl&) = delete;

        FirmwareControl()
            : _source("http://test.org/image")
            , _destination("/tmp")
            , _adminLock()
            , _type(IMAGE_TYPE_CDL)
            , _hash()
            , _resume(false)
            , _interval(0)
            , _position(0)
            , _waitTime(WaitTime)
            , _downloadStatus(Core::ERROR_NONE)
            , _upgradeStatus(UpgradeStatus::NONE)
            , _installStatus()
            , _upgrader(Core::ProxyType<Upgrader>::Create(this))
            , _signal(false, true)
        {
            RegisterAll();
        }

        virtual ~FirmwareControl()
        {
            UnregisterAll();

            _adminLock.Lock();
            if (_upgradeStatus != UpgradeStatus::NONE) {
                _upgradeStatus = UPGRADE_CANCELLED;
                _signal.SetEvent();
                _upgrader->Stop();
            }
            _adminLock.Unlock();
        }

        BEGIN_INTERFACE_MAP(FirmwareControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP
    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        inline void NotifyDownloadStatus(const uint32_t status)
        {
            _adminLock.Lock();
            _downloadStatus = status;
            _adminLock.Unlock();

            _signal.SetEvent();
        }

        static void Callback(mfrUpgradeStatus_t mfrStatus, void *cbData)
        {
            FirmwareControl* control = static_cast<FirmwareControl*>(cbData);
            ASSERT(control != nullptr);

            if (control != nullptr) {
               control->NotifyInstallStatus(mfrStatus);
            }
        }

        inline void NotifyInstallStatus(mfrUpgradeStatus_t mfrStatus)
        {
            UpgradeStatus upgradeStatus = ConvertMfrWriteStatusToUpgradeStatus(mfrStatus.progress);
            if ((upgradeStatus == UPGRADE_COMPLETED) || (upgradeStatus == INSTALL_ABORTED)) {
                _adminLock.Lock();
                _installStatus = mfrStatus;
                _adminLock.Unlock();

                _signal.SetEvent(); // To exit from the installation wait time
            } else {
                NotifyProgress(upgradeStatus, ConvertMfrWriteErrorToUpgradeType(mfrStatus.error), mfrStatus.percentage);
            }
        }

        inline void NotifyProgress(const UpgradeStatus& upgradeStatus, const ErrorType& errorType, const uint32_t& progress)
        {
            if ((upgradeStatus == UPGRADE_COMPLETED) ||
                (upgradeStatus == INSTALL_ABORTED) ||
                (upgradeStatus == DOWNLOAD_ABORTED)) {
                event_upgradeprogress(static_cast<JsonData::FirmwareControl::StatusType>(upgradeStatus),
                                      static_cast<JsonData::FirmwareControl::UpgradeprogressParamsData::ErrorType>(errorType), progress);
                ResetStatus();
                if (!((upgradeStatus == DOWNLOAD_ABORTED) && (errorType == UNAVAILABLE))) {
                    RemoveDownloadedFile();
                }
            } else if (_interval) { // Send intermediate staus/progress of upgrade
                event_upgradeprogress(static_cast<JsonData::FirmwareControl::StatusType>(upgradeStatus),
                                      static_cast<JsonData::FirmwareControl::UpgradeprogressParamsData::ErrorType>(errorType), progress);
            }
        }

    private:
        uint32_t Schedule(const std::string& name, const std::string& path);
        uint32_t Schedule(const std::string& name, const std::string& path, const Type& type, const uint16_t& interval, const std::string& hash, const bool resume);

    private:
        void Upgrade();
        void Install();
        uint32_t Resume(PluginHost::DownloadEngine& engine);
        uint32_t Download(PluginHost::DownloadEngine& engine);

        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_resume(const JsonData::FirmwareControl::ResumeParamsData& params);
        uint32_t endpoint_upgrade(const JsonData::FirmwareControl::UpgradeParamsData& params);
        uint32_t get_status(Core::JSON::EnumType<JsonData::FirmwareControl::StatusType>& response) const;
        uint32_t get_downloadsize(Core::JSON::DecUInt64& response) const;

        void event_upgradeprogress(const JsonData::FirmwareControl::StatusType& status,
                                   const JsonData::FirmwareControl::UpgradeprogressParamsData::ErrorType& error, const uint32_t& progress);

        inline uint32_t WaitForCompletion(int32_t waitTime)
        {
            uint32_t status = _signal.Lock(waitTime);
            _signal.ResetEvent(); //Clear signalled event
            return status;
        }

        inline void RemoveDownloadedFile()
        {
            Core::File _storage(_destination + Name);
            if (_storage.Exists()) {
                _storage.Destroy();
            }
        }
        inline void ResetStatus()
        {
            _adminLock.Lock();
            _upgradeStatus = UpgradeStatus::NONE;
            _adminLock.Unlock();
        }

        inline uint32_t DownloadStatus() const
        {
            _adminLock.Lock();
            uint32_t status = _downloadStatus;
            _adminLock.Unlock();
            return status;
        }

        inline UpgradeStatus Status() const
        {
            _adminLock.Lock();
            UpgradeStatus status = _upgradeStatus;
            _adminLock.Unlock();

            return status;
        }

        inline void Status(const UpgradeStatus& upgradeStatus, const uint32_t& error, const uint32_t& progress)
        {
            _adminLock.Lock();
            _upgradeStatus = upgradeStatus;
            _adminLock.Unlock();

            NotifyProgress(upgradeStatus, ConvertCoreErrorToUpgradeError(error), progress);
        }

        inline uint64_t DownloadMaxSize() const
        {
            uint64_t availableSize = 0;
            Core::Partition path(_destination.c_str());
            if (path.IsValid()) {
                availableSize = path.Free();
            }

            return availableSize;
        }

        inline ErrorType ConvertCoreErrorToUpgradeError(uint32_t error) const
        {
            ErrorType errorType = ErrorType::UNKNOWN;
            switch (error) {
            case Core::ERROR_NONE:
                errorType = ErrorType::ERROR_NONE;
                break;
            case Core::ERROR_UNAVAILABLE:
            case Core::ERROR_ASYNC_FAILED:
                errorType = ErrorType::UNAVAILABLE;
                break;
            case Core::ERROR_INCORRECT_HASH:
                errorType = ErrorType::INCORRECT_HASH;
                break;
            case Core::ERROR_WRITE_ERROR:
                errorType = ErrorType::NO_ENOUGH_SPACE;
                break;
            case Core::ERROR_UNAUTHENTICATED:
                errorType = ErrorType::UNAUTHENTICATED;
                break;
            case Core::ERROR_TIMEDOUT:
                errorType = ErrorType::TIMEDOUT;
                break;
            case Core::ERROR_NOT_EXIST:
                errorType = ErrorType::DOWNLOAD_DIR_NOT_EXIST;
                break;
            case Core::ERROR_NOT_SUPPORTED:
                errorType = ErrorType::RESUME_NOT_SUPPORTED;
                break;
            case Core::ERROR_INVALID_RANGE:
                errorType = ErrorType::INVALID_RANGE;
                break;
            default: //Expand later on need basis.
                break;
            }
            return errorType;
        }

        inline uint32_t ConvertMfrStatusToCore(mfrError_t mfrStatus) const
        {
            uint32_t status = Core::ERROR_GENERAL;
            switch (mfrStatus) {
            case mfrERR_NONE:
                status = Core::ERROR_NONE;
                break;
            case mfrERR_INVALID_PARAM:
            case mfrERR_OPERATION_NOT_SUPPORTED:
                status = Core::ERROR_BAD_REQUEST;
                break;
            case mfrERR_INVALID_STATE:
                status = Core::ERROR_ILLEGAL_STATE;
                break;
            case mfrERR_UNKNOWN:
            default:
                status = Core::ERROR_GENERAL;
                break;
            }
            return status;
        }

        inline UpgradeStatus ConvertMfrWriteStatusToUpgradeStatus(mfrUpgradeProgress_t progress) const
        {
            UpgradeStatus status = UpgradeStatus::NONE;
            switch (progress) {
            case mfrUPGRADE_PROGRESS_NOT_STARTED:
                status = UpgradeStatus::INSTALL_NOT_STARTED;
                break;
            case mfrUPGRADE_PROGRESS_STARTED:
                status = UpgradeStatus::INSTALL_STARTED;
                break;
            case mfrUPGRADE_PROGRESS_ABORTED:
                status = UpgradeStatus::INSTALL_ABORTED;
                break;
            case mfrUPGRADE_PROGRESS_COMPLETED:
                status = UpgradeStatus::UPGRADE_COMPLETED;
                break;
            default:
                break;
            }
            return status;
        }

        inline ErrorType ConvertMfrWriteErrorToUpgradeType(mfrError_t error) const
        {
            ErrorType errorType = ErrorType::UNKNOWN;
            switch (error) {
            case mfrERR_NONE:
                errorType = ErrorType::ERROR_NONE;
                break;
            case mfrERR_GENERAL:
                errorType = ErrorType::GENERIC;
                break;
            case mfrERR_INVALID_PARAM:
                errorType = ErrorType::INVALID_PARAMETERS;
                break;
            case mfrERR_INVALID_STATE:
                errorType = ErrorType::INVALID_STATES;
                break;
            case mfrERR_OPERATION_NOT_SUPPORTED:
                errorType = ErrorType::OPERATION_NOT_PERMITTED;
                break;
            case mfrERR_UNKNOWN:
            default:
                errorType = ErrorType::UNKNOWN;
                break;
            }
            return errorType;
        }

    private:
        string _source;
        string _destination;
        mutable Core::CriticalSection _adminLock;

        Type _type;
        string _hash;
        bool   _resume;
        uint16_t _interval;

        uint64_t _position;
        int32_t _waitTime;
        uint32_t _downloadStatus;
        UpgradeStatus _upgradeStatus;
        mfrUpgradeStatus_t _installStatus;

        Core::ProxyType<Upgrader> _upgrader;
        Core::Event _signal;
    };

} // namespace Plugin
} // namespace WPEFramework
