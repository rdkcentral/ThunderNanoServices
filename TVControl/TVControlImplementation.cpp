#include "TVControlImplementation.h"

extern "C" {
    typedef ::TVPlatform::ISystemTVPlatform* (*GetTVPlatformSystemFunction)();
}

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(TVControlImplementation, 1, 0);

TVControlImplementation::TVControlImplementation()
    : _adminLock()
    , _service(nullptr)
    , _streamingClients()
    , _externalAccess(nullptr)
    , _nodeId(TUNER_PROCESS_NODE_ID)
{
    _externalAccess = new ExternalAccess(Core::NodeId(_nodeId), this);
    uint32_t result = _externalAccess->Open(RPC::CommunicationTimeOut);
    if (result != Core::ERROR_NONE) {
        TRACE(Trace::Information, (_T("Could not open TVControlImplementation server.")));
        delete _externalAccess;
    }
}

TVControlImplementation::~TVControlImplementation()
{
    TRACE(Trace::Information, (_T("~TVControlImplementation Invoked")));
    // Nothing to do here.
    _service = nullptr;

    if (_externalAccess != nullptr) {
        delete _externalAccess;
        _externalAccess = nullptr;
    }
    ITableData::DeleteInstance();
}

uint32_t TVControlImplementation::Configure(PluginHost::IShell* service)
{
    // Start loading the configured factories
    Config config;
    config.FromString(service->ConfigLine());
    const string locator(service->DataPath() + config.Location.Value());

    Core::Directory entry(locator.c_str(), _T("TVControl.tvplatform"));
    while (entry.Next() == true) {
        _tvPlatformSystem = Core::Library(entry.Current().c_str());
        if (_tvPlatformSystem.IsLoaded() == true) {
            GetTVPlatformSystemFunction handle = reinterpret_cast<GetTVPlatformSystemFunction>(_tvPlatformSystem.LoadFunction(_T("GetSystemTVPlatform")));
            if (handle != nullptr) {
                TVPlatform::ISystemTVPlatform* systemTVPlatform = handle();
                TVPlatform::ITVPlatform* tvPlatform = systemTVPlatform->GetInstance();
                _tuner = ITuner::GetInstance(tvPlatform);
                _tableData = ITableData::GetInstance(*this, tvPlatform);
                _tuner->Initialize(service);
            }
        } else {
            TRACE(Trace::Information, (_T("Could not load tvcontrol library")));
        }
    }
}

void TVControlImplementation::Register(Exchange::IStreaming::INotification* sink)
{
    _adminLock.Lock();

    // Make sure a sink is not registered multiple times.
    ASSERT(std::find(_streamingClients.begin(), _streamingClients.end(), sink) == _streamingClients.end());


    _streamingClients.push_back(sink);
    sink->AddRef();

    TRACE(Trace::Information, (_T("IStreaming::INotification Registered in TVControlImplementation: %p"), sink));
    _adminLock.Unlock();
}

void TVControlImplementation::Unregister(Exchange::IStreaming::INotification* sink)
{
    _adminLock.Lock();

    std::list<Exchange::IStreaming::INotification*>::iterator index(std::find(_streamingClients.begin(), _streamingClients.end(), sink));

    // Make sure you do not unregister something you did not register !!!
    ASSERT(index != _streamingClients.end());

    if (index != _streamingClients.end()) {
        TRACE(Trace::Information, (_T("IStreaming::INotification Removing registered listener from TVControlImplementation")));
        (*index)->Release();
        _streamingClients.erase(index);
    }

    _adminLock.Unlock();
}

void TVControlImplementation::Register(Exchange::IGuide::INotification* sink)
{
    _adminLock.Lock();

    // Make sure a sink is not registered multiple times.
    ASSERT(std::find(_guideClients.begin(), _guideClients.end(), sink) == _guideClients.end());

    _guideClients.push_back(sink);
    sink->AddRef();

    TRACE(Trace::Information, (_T("IGuide::INotification Registered in TVControlImplementation: %p"), sink));
    _adminLock.Unlock();
}

void TVControlImplementation::Unregister(Exchange::IGuide::INotification* sink)
{
    _adminLock.Lock();

    std::list<Exchange::IGuide::INotification*>::iterator index(std::find(_guideClients.begin(), _guideClients.end(), sink));

    // Make sure you do not unregister something you did not register !!!
    ASSERT(index != _guideClients.end());

    if (index != _guideClients.end()) {
        TRACE(Trace::Information, (_T("IGuide::INotification Removing registered listener from TVControlImplementation %d")));
        (*index)->Release();
        _guideClients.erase(index);
    }

    _adminLock.Unlock();
}

void TVControlImplementation::StartScan()
{
    TRACE(Trace::Information, (_T("Start scan")));
    _tuner->StartScan(*this);
}

void TVControlImplementation::StopScan()
{
    TRACE(Trace::Information, (_T("Stop scan")));
    _tuner->StopScan();
}

bool TVControlImplementation::IsScanning()
{
    TRACE(Trace::Information, (_T("Is scanning")));
    return _tuner->IsScanning();
}

void TVControlImplementation::SetCurrentChannel(const string& channelId)
{
    TRACE(Trace::Information, (_T("SetCurrentChannel : Channeld Id = %s"), channelId.c_str()));
    _tuner->SetCurrentChannel(channelId, *this);
}

uint32_t TVControlImplementation::StartParser(PluginHost::IShell* service)
{
    return (_tableData->StartParser(service));
}

const std::string TVControlImplementation::GetCurrentChannel()
{
    TRACE(Trace::Information, (_T("Get Current Channel")));
    return _tuner->GetCurrentChannel();
}

const std::string TVControlImplementation::GetChannels()
{
    TRACE(Trace::Information, (_T("Get Channels")));
    return _tableData->GetChannels();
}

const std::string TVControlImplementation::GetPrograms()
{
    TRACE(Trace::Information, (_T("Get Programs")));
    return _tableData->GetPrograms();
}

const std::string TVControlImplementation::GetCurrentProgram(const string& channelNum)
{
    TRACE(Trace::Information, (_T("Get current Program : channel number = %s"), channelNum.c_str()));
    return _tableData->GetCurrentProgram(channelNum);
}

const std::string TVControlImplementation::GetAudioLanguages(const uint32_t eventId)
{
    TRACE(Trace::Information, (_T("Get audio languages : event id = %u"), eventId));
    return _tableData->GetAudioLanguages(eventId);
}

const std::string TVControlImplementation::GetSubtitleLanguages(const uint32_t eventId)
{
    TRACE(Trace::Information, (_T("Get subtitle languages : event id = %u"), eventId));
    return _tableData->GetSubtitleLanguages(eventId);
}

bool TVControlImplementation::SetParentalControlPin(const string& oldPin, const string& newPin)
{
    TRACE(Trace::Information, (_T("SetParentalControlPin :old pin = %s\tnew pin = %s"), oldPin.c_str(), newPin.c_str()));
    return _tableData->SetParentalControlPin(oldPin, newPin);
}

bool TVControlImplementation::SetParentalControl(const std::string& pin, const bool isLocked)
{
    TRACE(Trace::Information, (_T("SetParentalControl:pin = %s\tisLocked = %d"), pin.c_str(), isLocked));
    return _tableData->SetParentalControl(pin, isLocked);
}

bool TVControlImplementation::IsParentalControlled()
{
    TRACE(Trace::Information, (_T("Is parental controlled")));
    return _tableData->IsParentalControlled();
}

bool TVControlImplementation::IsParentalLocked(const string& channelNum)
{
    TRACE(Trace::Information, (_T("IsParentalLocked : Channel number = %s"), channelNum.c_str()));
    return _tableData->IsParentalLocked(channelNum);
}

bool TVControlImplementation::SetParentalLock(const string& pin, bool isLocked, const string& channelNum)
{
    TRACE(Trace::Information, (_T("SetParentalLock : pin = %s\tisLocked = %d\tchannel number = %s"), pin, isLocked, channelNum.c_str()));
    return _tableData->SetParentalLock(pin, isLocked, channelNum);
}

void TVControlImplementation::Test(const string& name)
{
    TRACE(Trace::Information, (_T ("Someone triggered test(%s)"), name.c_str()));
    ScanningStateChanged(ScanningState::Stopped);
    StreamingFrequencyChanged(1234);
    CurrentChannelChanged(0);
    EITBroadcast();
    EmergencyAlert();
    ParentalControlChanged();
    ParentalLockChanged(0);
    return;
}

void TVControlImplementation::ScanningStateChanged(const ScanningState state)
{
    if ((state == Completed) || ((state == Stopped))) {
        _tuner->StoreFrequencyListInDB();
        _tableData->NotifyFrequencyListUpdate();
        if(_tuner->StoreTSInfoInDB()) {
            _tableData->NotifyTSInfoUpdate();
        }
    }

    _adminLock.Lock();

    std::list<Exchange::IStreaming::INotification*>::iterator index(_streamingClients.begin());

    while (index != _streamingClients.end()) {
        (*index)->ScanningStateChanged((const uint32_t)state);
        index++;
    }

    _adminLock.Unlock();
}

void TVControlImplementation::StreamingFrequencyChanged(uint32_t currentFrequency)
{
    _tableData->NotifyStreamingFrequencyChange(currentFrequency);
}

void TVControlImplementation::CurrentChannelChanged(const string& lcn)
{
    TRACE(Trace::Information, (_T("CurrentChannelChanged: -----> Current Channel changed <-----")));
    _adminLock.Lock();

    std::list<Exchange::IStreaming::INotification*>::iterator index(_streamingClients.begin());

    while (index != _streamingClients.end()) {
        (*index)->CurrentChannelChanged(lcn);
        index++;
    }

    _adminLock.Unlock();
}

void TVControlImplementation::EITBroadcast()
{
    _adminLock.Lock();

    std::list<Exchange::IGuide::INotification*>::iterator index(_guideClients.begin());

    while (index != _guideClients.end()) {
        (*index)->EITBroadcast();
        index++;
    }

    _adminLock.Unlock();
}

void TVControlImplementation::EmergencyAlert()
{
    _adminLock.Lock();

    std::list<Exchange::IGuide::INotification*>::iterator index(_guideClients.begin());

    while (index != _guideClients.end()) {
        (*index)->EmergencyAlert();
        index++;
    }

    _adminLock.Unlock();
}

void TVControlImplementation::ParentalControlChanged()
{
    _adminLock.Lock();

    std::list<Exchange::IGuide::INotification*>::iterator index(_guideClients.begin());

    while (index != _guideClients.end()) {
        (*index)->ParentalControlChanged();
        index++;
    }

    _adminLock.Unlock();
}

void TVControlImplementation::ParentalLockChanged(const string& lcn)
{
    _adminLock.Lock();

    std::list<Exchange::IGuide::INotification*>::iterator index(_guideClients.begin());

    while (index != _guideClients.end()) {
        (*index)->ParentalLockChanged(lcn);
        index++;
    }

    _adminLock.Unlock();
}
}
} // namespace WPEFramework::Plugin
