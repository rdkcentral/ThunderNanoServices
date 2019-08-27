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
            TRACE_L1("Source location : [%s]\n", _source.c_str());
        }
        if (location.Download.IsSet() == true) {
            _destination = location.Download.Value();
            TRACE_L1("Destination location : [%s]\n", _destination.c_str());
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
        TRACE(Trace::Information, (string(__FUNCTION__)));
        if (path.empty() != true) {
            _source = path + "/" + name;
        }
        _downloadSignal.ResetEvent();

        _adminLock.Lock();
        _type = type;
        _interval = interval;
        _hash = hash;

        _upgrader.Schedule();
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
    }

    uint32_t FirmwareControl::Download() {

        TRACE(Trace::Information, (string(__FUNCTION__)));
        Notifier notifier(this);
        string destination = _destination + "/" + Name;

        PluginHost::DownloadEngine downloadEngine(&notifier, destination);

        uint32_t status = downloadEngine.Start(_source, destination, _hash);
        if ((status == Core::ERROR_NONE) || status == Core::ERROR_INPROGRESS)) {

            Status(UpgradeStatus::DOWNLOAD_STARTED, status, 0);
            status = _downloadSignal.Lock(_waitTime);
            if ((status == Core::ERROR_NONE) && (DownloadStatus() == Core::ERROR_NONE)) {
                 Status(UpgradeStatus::DOWNLOAD_COMPLETED, ErrorType::ERROR_NONE, 0);
            } else {
                status = ((status != Core::ERROR_NONE)? status: DownloadStatus());
                Status(UpgradeStatus::DOWNLOAD_ABORTED, status, 0);
            }
        } else {
            Status(UpgradeStatus::DOWNLOAD_ABORTED, status, 0);
        }

        TRACE(Trace::Information, (string(__FUNCTION__)));
        return status;
    }

} // namespace Plugin
} // namespace WPEFramework
