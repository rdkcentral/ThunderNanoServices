#include "FirmwareControl.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(FirmwareControl, 1, 0);

    /* virtual */ const string FirmwareControl::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        Config config;
        config.FromString(service->ConfigLine());
        if (config.Locator.IsSet() == true) {
            _locator = config.Locator.Value();
        }
        if (config.Destination.IsSet() == true) {
            _destination = config.Destination.Value();
        }
        return (string());
    }

    /* virtual */ void FirmwareControl::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
    }

    /* virtual */ string FirmwareControl::Information() const
    {
        // No additional info to report.
        return (string());
    }

    uint32_t FirmwareControl::Schedule(const std::string& name, const std::string& path, const FirmwareControl::Type& type, const uint16_t& interval, const std::string& hash)
    {
        uint32_t status = Core::ERROR_NONE;

        //FIXME Add code to check the upgrade is already happening (here or jsonrpc call)
        if (path.empty() != true) {
            _locator = path + name;
        }

        if (status == Core::ERROR_NONE) {
            _downloadSignal.ResetEvent();

            _adminLock.Lock();
            _type = type;
            _interval = interval;
            _hash = hash;

            _upgrader.Schedule();
            _adminLock.Unlock();
        }

        return status;
    }

    void FirmwareControl::Upgrade() {

        Notifier notifier(this);

        uint32_t status = Core::ERROR_NONE;
        ProgressStatus progressStatus = ProgressStatus::NONE;
        PluginHost::DownloadEngine downloadEngine(&notifier, _destination + Name);
        downloadEngine.Start(_locator, _destination + Name, _hash);
        _adminLock.Lock();
        bool upgradeInProgress = _upgradeInProgress;
        _adminLock.Unlock();
        if ((_downloadSignal.Lock(_waitTime) == Core::ERROR_NONE) && (upgradeInProgress == true)) {
            if (_downloadStatus == Core::ERROR_NONE) {
                if (_interval) {
                    NotifyProgress(ProgressStatus::DOWNLOAD_COMPLETED, ErrorType::ERROR_NONE, 100); //FIXME: relook later about the incremental values
                }
                //Setup callback handler;
                mfrUpgradeStatusNotify_t mfrNotifier;
                mfrNotifier.cbData = reinterpret_cast<void*>(this);
                mfrNotifier.interval = static_cast<int>(_interval);
                mfrNotifier.cb = Callback;

                // Initiate image install
                mfrError_t mfrStatus = mfrWriteImage(Name, _destination.c_str(), static_cast<mfrImageType_t>(_type), mfrNotifier);
                if (mfrERR_NONE != mfrStatus) {
                    progressStatus = ProgressStatus::INSTALL_ABORTED;
                    status = ConvertMfrStatusToCore(mfrStatus);
                }
            } else {
                progressStatus = ProgressStatus::DOWNLOAD_ABORTED;
                status = _downloadStatus;
            }
        } else {
            progressStatus = ProgressStatus::DOWNLOAD_ABORTED;
            status = Core::ERROR_UNAVAILABLE;
        }

        _adminLock.Lock();
        upgradeInProgress = _upgradeInProgress;
        _adminLock.Unlock();

        if ((progressStatus !=  ProgressStatus::NONE) && (upgradeInProgress == true)) {
            NotifyProgress(progressStatus, ConvertCoreErrorToUpgradeError(status), 0);
        }
        return;
    }

} // namespace Plugin
} // namespace WPEFramework
