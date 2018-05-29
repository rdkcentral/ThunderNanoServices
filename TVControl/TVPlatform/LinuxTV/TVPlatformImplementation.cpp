#include "Module.h"
#include "TVPlatformImplementation.h"

using namespace WPEFramework;

namespace TVPlatform {

TVPlatformImplementation::TVPlatformImplementation()
    : _tunerCount(0)
    , _isRunning(true)
    , _isStreaming(false)
{
    InitializeTuners();
    _tunerThread = std::thread(&TVPlatformImplementation::TunerChangedListener, this);
}

TVPlatformImplementation::~TVPlatformImplementation()
{
    _isRunning = false;
    _tunerThread.join();
    _tunerList.clear();
}

LinuxDVB::TvTunerBackend* TVPlatformImplementation::GetTuner(bool isForStreaming)
{
    if (_tunerCount) {
        if (_tunerCount == 1)
            return _tunerList[0].get();
        if (isForStreaming)
            return _tunerList[1].get();
        return _tunerList[0].get();
    }
    return nullptr;
}

TvmRc TVPlatformImplementation::Scan(std::vector<uint32_t> freqList, ITunerHandler& tunerHandler)
{
    LinuxDVB::TvTunerBackend* tuner = GetTuner(true);
    TvmRc ret = TvmError;
    if (tuner) {
        if ((ret = tuner->StartScanning(freqList, tunerHandler)) == TvmSuccess)
            _isStreaming = true;
    }
    return ret;
}

TvmRc TVPlatformImplementation::StopScanning()
{
    LinuxDVB::TvTunerBackend* tuner = GetTuner(true);
    if (tuner)
        return tuner->StopScanning();
}

TvmRc TVPlatformImplementation::Tune(TSInfo& tsInfo, ITunerHandler& tunerHandler)
{
    TvmRc ret = TvmError;
    LinuxDVB::TvTunerBackend* tuner = GetTuner(true);
    if (tuner) {
        if ((ret = tuner->Tune(tsInfo, tunerHandler)) == TvmSuccess)
            _isStreaming = true;
    }
    return ret;
}

TvmRc TVPlatformImplementation::SetHomeTS(uint32_t primaryFreq, uint32_t secondaryFreq)
{
    LinuxDVB::TvTunerBackend* tuner = GetTuner(false);
    if (tuner && (((_tunerCount == 1) && !_isStreaming) || (_tunerCount > 1)))
        return tuner->SetHomeTS(primaryFreq);
    return TvmError;
}

TvmRc TVPlatformImplementation::StopFilter(uint16_t pid, uint8_t tid)
{
    LinuxDVB::TvTunerBackend* tuner = GetTuner(false);
    if (tuner)
        return tuner->StopFilter(pid);
}

TvmRc TVPlatformImplementation::StopFilters()
{
    LinuxDVB::TvTunerBackend* tuner = GetTuner(false);
    if (tuner)
        return tuner->StopFilters();
}

TvmRc TVPlatformImplementation::StartFilter(uint16_t pid, uint8_t tid, ISectionHandler* pSectionHandler)
{
    LinuxDVB::TvTunerBackend* tuner = GetTuner(false);
    if (tuner)
        return tuner->StartFilter(pid, pSectionHandler);
}

bool TVPlatformImplementation::IsScanning()
{
    LinuxDVB::TvTunerBackend* tuner = GetTuner(true);
    if (tuner)
        return tuner->IsScanning();
}

std::vector<uint32_t>& TVPlatformImplementation::GetFrequencyList()
{
    LinuxDVB::TvTunerBackend* tuner = GetTuner(true);
    _isStreaming = false;
    if (tuner)
        return tuner->GetFrequencyList();
}

TvmRc TVPlatformImplementation::GetTSInfo(TSInfoList& tsInfoList)
{
    LinuxDVB::TvTunerBackend* tuner = GetTuner(true);
    if (tuner)
        return tuner->GetTSInfo(tsInfoList);
}

TvmRc TVPlatformImplementation::GetChannelMap(ChannelMap& chanMap)
{
    LinuxDVB::TvTunerBackend* tuner = GetTuner(true);
    if (tuner)
        return tuner->GetChannelMap(chanMap);
}

void TVPlatformImplementation::InitializeTuners()
{
    int32_t feOpenMode = 0;
    // FIXME Temporary fix for tuner release.
    char command[1024];
    snprintf(command, 1024, "STOP");
    TRACE(Trace::Information, (_T("Command : %s"), command));
    std::ofstream myfile;
    myfile.open("/etc/TVTune.txt", std::ios::trunc);
    if (myfile.is_open()) {
        myfile << command;
        myfile.close();
        sleep(1);
    } else
        TRACE(Trace::Error, (_T("Failed to open TVTune.txt file.")));

    _tunerCount = 0;
    for (uint32_t i = 0; i < DVB_ADAPTER_SCAN; i++) {
        for (uint32_t j = 0; j < DVB_ADAPTER_SCAN; j++) {
            struct dvbfe_handle* feHandleTmp = dvbfe_open(i, j, feOpenMode);
            if (!feHandleTmp)
                continue;

            std::unique_ptr<TunerData> tunerData = std::make_unique<TunerData>();
            std::string tunerIdStr;
            CreateTunerId(i, j, tunerIdStr);
            TRACE(Trace::Information, (_T("Tuner identified as  %s  Adapter: %d Frontend: %d "), feHandleTmp->name, i, j));
            TRACE(Trace::Information, (_T("Tuner id %s"), tunerIdStr.c_str()));
            tunerData->tunerId.assign(tunerIdStr);
            dvbfe_close(feHandleTmp);
            std::unique_ptr<LinuxDVB::TvTunerBackend> tInfo = std::make_unique<LinuxDVB::TvTunerBackend>(_tunerCount, std::move(tunerData), _isDvb);
            // Update the  private tuner list.
            _tunerList.push_back(std::move(tInfo));
            _tunerCount += 1;
        }
    }
    for (auto& tuner : _tunerList)
        tuner->UpdateTunerCount(_tunerCount);
}

// TunerChangedListener waits for the addition/removal of Tuner and triggers the event.
void TVPlatformImplementation::TunerChangedListener()
{
    struct udev* udev;

    // Create the udev object.
    udev = udev_new();
    if (!udev) {
        TRACE(Trace::Error, (_T("Can't create udev")));
        exit(1);
    }

    // Create a list of the devices in the 'dvb' subsystem.(Initially found Tuners).
    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "dvb");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* devListEntry;
    udev_list_entry_foreach(devListEntry, devices)
    {
        const char* path = udev_list_entry_get_name(devListEntry);
        struct udev_device* dev = udev_device_new_from_syspath(udev, path);

        TRACE(Trace::Information, (_T("Device Node Path: %s"), udev_device_get_devnode(dev)));

        dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (!dev) {
            TRACE(Trace::Error, (_T("Unable to find parent usb device.")));
            exit(1);
        }
    }
    struct udev_monitor* mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "dvb", nullptr);
    udev_monitor_enable_receiving(mon);
    int32_t fd = udev_monitor_get_fd(mon);
    // Loop and wait for new Tuners.
    while (_isRunning) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int32_t ret = select(fd + 1, &fds, nullptr, nullptr, &tv);

        if (ret > 0 && FD_ISSET(fd, &fds)) {
            TRACE(Trace::Information, (_T("select() says there should be data")));
            struct udev_device* dev = udev_monitor_receive_device(mon);
            if (dev) {
                std::string fpath = udev_device_get_devnode(dev);
                if (fpath.find("frontend") != std::string::npos) {
                    int32_t i, j;
                    std::string tunerId;
                    sscanf(udev_device_get_devnode(dev), "/%*[a-z-A-Z]/%*[a-z-A-Z]/%*[a-z-A-Z]%d/%*[a-z-A-Z]%d", &i, &j);
                    CreateTunerId(i, j, tunerId);

                    // The data from udev is in string format. So.converting the operation into the appropriate data type.
                    TunerChangedOperation operation;
                    if (!strcmp("add", udev_device_get_action(dev)))
                        operation = Added;
                    else if (!strcmp("remove", udev_device_get_action(dev)))
                        operation = Removed;
                    UpdateTunerList(tunerId, operation);
                    for (auto& tuner : _tunerList)
                        tuner->UpdateTunerCount(_tunerCount);
                    // TVControlPushEvent(1, TunerChanged, tunerId.c_str(), operation, operation); FIXME: Events to be implemented.
                }
                TRACE(Trace::Information, (_T("***Tuner***")));
                TRACE(Trace::Information, (_T("   Node: %s"), udev_device_get_devnode(dev)));
                TRACE(Trace::Information, (_T("   Action: %s"), udev_device_get_action(dev)));
                udev_device_unref(dev);
            } else
                TRACE(Trace::Error, (_T("No Device from receive_device(). An error occured.")));
        }
        usleep(250 * 1000);
    }
    udev_unref(udev);
}

void TVPlatformImplementation::CreateTunerId(int32_t i, int32_t j, std::string& tunerId)
{
    tunerId.assign(std::to_string(i));
    tunerId.append(":"); // delimiter.
    tunerId.append(std::to_string(j));
    TRACE(Trace::Information, (_T("Tuner id %s"), tunerId.c_str()));
}

void TVPlatformImplementation::UpdateTunerList(std::string& tunerId, TunerChangedOperation operation)
{
    if (operation == Added) {
        std::unique_ptr<TunerData> tunerData = std::make_unique<TunerData>();
        tunerData->tunerId.assign(tunerId);
        std::unique_ptr<LinuxDVB::TvTunerBackend> tInfo = std::make_unique<LinuxDVB::TvTunerBackend>(_tunerCount, std::move(tunerData), _isDvb);
        _tunerList.push_back(std::move(tInfo));
        TRACE(Trace::Information, (_T("Found and Added the Tuner")));
        _tunerCount += 1;
    } else if (operation == Removed) {
        int32_t position = 0;
        // Iterate  private tuner list and get the particular tuner info.
        for (auto& tuner : _tunerList) {
            TRACE(Trace::Information, (_T("Id of this tuner %s"), tuner->_tunerData->tunerId.c_str()));
            TRACE(Trace::Information, (_T("Id of required tuner %s"), tunerId));

            if (!strncmp(tuner->_tunerData->tunerId.c_str(), tunerId.c_str(), tuner->_tunerData->tunerId.length())) {
                _tunerList.erase(_tunerList.begin() + position);
                TRACE(Trace::Information, (_T("Found and deleted the Tuner")));
                break;
            }
            position++;
        }
    }
}

static SystemTVPlatformType<TVPlatformImplementation> g_instance;

} // namespace TVPlatform

TVPlatform::ISystemTVPlatform* GetSystemTVPlatform() {

    return &TVPlatform::g_instance;
}
