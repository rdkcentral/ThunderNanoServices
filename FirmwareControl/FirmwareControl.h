#pragma once

#include "Module.h"
#include "DownloadEngine.h"
#include <interfaces/json/JsonData_FirmwareControl.h>
#include <mfrTypes.h>

namespace WPEFramework {
namespace Plugin {

    class FirmwareControl : public PluginHost::IPlugin, public PluginHost::JSONRPC {

    public:
        enum Type {
            IMAGE_TYPE_CDL,
            IMAGE_TYPE_RCDL
        };
        enum ProgressStatus {
            NONE,
            DOWNLOAD_NOT_STARTED,
            DOWNLOAD_ABORTED,
            DOWNLOAD_COMPLETED,
            INSTALL_NOT_STARTED,
            INSTALL_ABORTED,
            INSTALL_STARTED,
            UPGRADE_COMPLETED
        };
        enum ErrorType {
            ERROR_NONE,
            GENERIC,
            INVALID_PARAMETERS,
            INVALID_STATES,
            OPERATION_NOT_PERMITTED,
            UNAVAILABLE,
            INCORRECT_HASH,
            UNKNOWN
        };
    private:
        static constexpr const TCHAR* Name = "imageTemp";
        static uint32_t constexpr WaitTime = 60 * 60 * 1000;

    private:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&);
            Config& operator=(const Config&);

            Config()
            : Core::JSON::Container()
            , Locator()
            , Destination()
            {
                Add(_T("locator"), &Locator);
                Add(_T("destination"), &Destination);
            }

            ~Config() {}

        public:
            Core::JSON::String Locator;
            Core::JSON::String Destination;
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
            virtual void NotifyDownloadStatus(const uint32_t status) override
            {
                _parent.NotifyDownloadStatus(status);
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
                PluginHost::WorkerPool::Instance().Submit(job);
            }
            void Stop() {
                Core::ProxyType<Core::IDispatch> job(*this);
                PluginHost::WorkerPool::Instance().Revoke(job);
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
            : _waitTime(WaitTime)
            , _locator("http://test.org/image")
            , _destination("/tmp")
            , _adminLock()
            , _type(IMAGE_TYPE_CDL)
            , _hash()
            , _interval(0)
            , _upgradeInProgress(false)
            , _upgrader(this)
            , _downloadSignal(false, true)
        {
            RegisterAll();
        }

        virtual ~FirmwareControl()
        {
            UnregisterAll();

            _adminLock.Lock();
            if (_upgradeInProgress == true) {
                _upgradeInProgress = false;
                _downloadSignal.SetEvent();
                _upgrader.Stop();
            }
            _adminLock.Unlock();
        }

        BEGIN_INTERFACE_MAP(FirmwareControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
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

            _downloadSignal.SetEvent();
        }

        static void Callback(mfrUpgradeStatus_t mfrStatus, void *cbData)
        {
            FirmwareControl* control = static_cast<FirmwareControl*>(cbData);
            ASSERT(control != nullptr);

            if (control != nullptr) {
               control->NotifyProgress(mfrStatus);
            }
        }

        inline void NotifyProgress(mfrUpgradeStatus_t mfrStatus)
        {
            NotifyProgress(ConvertMfrWriteStatusToUpgradeStatus(mfrStatus.progress), ConvertMfrWriteErrorToUpgradeType(mfrStatus.error), mfrStatus.percentage);
        }

        inline void NotifyProgress(const ProgressStatus& progressStatus, const ErrorType& errorType, const uint16_t& percentage)
        {
            if ((progressStatus == UPGRADE_COMPLETED) || (INSTALL_ABORTED) || (DOWNLOAD_ABORTED)) {
                _adminLock.Lock();
                _upgradeInProgress = false;
                _adminLock.Unlock();
            }
            event_upgradeprogress(static_cast<JsonData::FirmwareControl::UpgradeprogressParamsData::StatusType>(progressStatus),
                                  static_cast<JsonData::FirmwareControl::UpgradeprogressParamsData::ErrorType>(errorType), percentage);
        }

    private:
        uint32_t Schedule(const std::string& name, const std::string& path, const Type& type, const uint16_t& interval, const std::string& hash);

    private:
        void Upgrade();
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_upgrade(const JsonData::FirmwareControl::UpgradeParamsData& params);
        void event_upgradeprogress(const JsonData::FirmwareControl::UpgradeprogressParamsData::StatusType& status,
                                   const JsonData::FirmwareControl::UpgradeprogressParamsData::ErrorType& error, const uint16_t& percentage);

        inline ErrorType ConvertCoreErrorToUpgradeError(uint32_t error)
        {
            ErrorType errorType = ErrorType::UNKNOWN;
            switch (error) {
            case Core::ERROR_UNAVAILABLE:
                errorType = ErrorType::UNAVAILABLE;
                break;
            case Core::ERROR_INCORRECT_HASH:
                errorType = ErrorType::INCORRECT_HASH;
                break;
            default: //Expand later on need basis.
                break;
            }
            return errorType;
        }
        inline uint32_t ConvertMfrStatusToCore(mfrError_t mfrStatus)
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

        inline ProgressStatus ConvertMfrWriteStatusToUpgradeStatus(mfrUpgradeProgress_t progress)
        {
            ProgressStatus status = ProgressStatus::NONE;
            switch (progress) {
            case mfrUPGRADE_PROGRESS_NOT_STARTED:
                status = ProgressStatus::INSTALL_NOT_STARTED;
                break;
            case mfrUPGRADE_PROGRESS_STARTED:
                status = ProgressStatus::INSTALL_STARTED;
                break;
            case mfrUPGRADE_PROGRESS_ABORTED:
                status = ProgressStatus::INSTALL_ABORTED;
                break;
            case mfrUPGRADE_PROGRESS_COMPLETED:
                status = ProgressStatus::UPGRADE_COMPLETED;
                break;
            default:
                break;
            }
            return status;
        }

        inline ErrorType ConvertMfrWriteErrorToUpgradeType(mfrError_t error)
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
        uint32_t _waitTime;
        uint32_t _downloadStatus;
        string _locator;
        string _destination;
        Core::CriticalSection _adminLock;

        Type _type;
        string _hash;
        uint16_t _interval;

        bool _upgradeInProgress;

        Upgrader _upgrader;

        Core::Event _downloadSignal;
    };

} // namespace Plugin
} // namespace WPEFramework
