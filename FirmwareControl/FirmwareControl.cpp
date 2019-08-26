#include "FirmwareControl.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(FirmwareControl, 1, 0);

    /* virtual */ const string FirmwareControl::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        Location location;
        location.FromString(service->ConfigLine());
        if (location.Source.IsSet() == true) {
            _source = location.Source.Value();
        }
        if (location.Download.IsSet() == true) {
            _destination = location.Download.Value();
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
            _source = path + name;
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

        PluginHost::DownloadEngine downloadEngine(&notifier, _destination + Name);
        downloadEngine.Start(_source, _destination + Name, _hash);
        Status(UpgradeStatus::DOWNLOAD_STARTED, ErrorType::ERROR_NONE, 0);
        if (_downloadSignal.Lock(_waitTime) == Core::ERROR_NONE) {
            if(Status() != UpgradeStatus::UPGRADE_CANCELLED) {
                if (_downloadStatus == Core::ERROR_NONE) {
                    Status(UpgradeStatus::DOWNLOAD_COMPLETED, ErrorType::ERROR_NONE, 0);
                    //Setup callback handler;
                    mfrUpgradeStatusNotify_t mfrNotifier;
                    mfrNotifier.cbData = reinterpret_cast<void*>(this);
                    mfrNotifier.interval = static_cast<int>(_interval);
                    mfrNotifier.cb = Callback;

                    // Initiate image install
                    Status(UpgradeStatus::INSTALL_STARTED, ErrorType::ERROR_NONE, 0);
                    mfrError_t mfrStatus = mfrWriteImage(Name, _destination.c_str(), static_cast<mfrImageType_t>(_type), mfrNotifier);
                    if (mfrERR_NONE != mfrStatus) {
                        Status(UpgradeStatus::INSTALL_ABORTED, ConvertMfrStatusToCore(mfrStatus), 0);
                    } else {
                        Status(UpgradeStatus::UPGRADE_COMPLETED, Core::ERROR_NONE, 100);
                    }
                } else {
                    Status(UpgradeStatus::DOWNLOAD_ABORTED, _downloadStatus, 0);
                }
            }
        } else {
            Status(UpgradeStatus::DOWNLOAD_ABORTED, Core::ERROR_UNAVAILABLE, 0);
        }

        return;
    }

} // namespace Plugin
} // namespace WPEFramework
