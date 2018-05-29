#include "Module.h"
#include "ITuner.h"

#include "JSData.h"

namespace WPEFramework {
namespace Plugin {

ITuner::ITuner(TVPlatform::ITVPlatform* tvPlatform)
    : _adminLock()
    , _service(nullptr)
    , _epgDB(EPGDataBase::GetInstance())
    , _tvPlatform(tvPlatform)
    , _config()
    , _currentChannel("")
{
    TRACE(Trace::Information, (_T("%s"), __FUNCTION__));
}

ITuner::~ITuner()
{
    TRACE(Trace::Information, (_T("%s"), __FUNCTION__));
}

ITuner* ITuner::GetInstance(TVPlatform::ITVPlatform* tvPlatform)
{
    static ITuner tunerInstance(tvPlatform);
    return &tunerInstance;
}

void ITuner::ParseFrequencies(const std::string strFrequency, std::vector<uint32_t>& frequencyList)
{
    size_t prev = 0, pos = 0;
    do {
        pos = strFrequency.find(',', prev);
        if (pos == string::npos)
            pos = strFrequency.length();
        string token = strFrequency.substr(prev, pos - prev);
        size_t pos2 = token.find('-');
        if (pos2 != string::npos) {
            int32_t start = atoi(token.substr(0, pos2).c_str());
            int32_t end  = atoi(token.substr(pos2 + 1).c_str());
            for (int32_t i = start; i <= end; ++i)
                frequencyList.push_back((uint32_t)(i * 1000000));
        } else
            frequencyList.push_back((uint32_t)(atoi(token.c_str()) * 1000000));
        prev = pos + 1;
    } while (pos < strFrequency.length());
}

void ITuner::StoreFrequencyListInDB()
{
    std::vector<uint32_t> frequencyList = _tvPlatform->GetFrequencyList();
    if (frequencyList.size() && frequencyList != _frequencyList) {
        _epgDB.CreateFrequencyTable();
        _epgDB.InsertFrequencyInfo(frequencyList);
    }
}

const string ITuner::Initialize(PluginHost::IShell* service)
{
    _config.FromString(service->ConfigLine());
    bool isDvb = _config.Dvb.Value();
    _tvPlatform->SetDbs(isDvb);
    _tvPlatform->SetTuneParameters(_config.TuneParameters.Value());
    std::string strFreqList = _config.FrequencyList.Value();
    std::vector<uint32_t> frequencyList;
    if (strFreqList.size() && strcmp(strFreqList.c_str(), "0")) {
        TRACE(Trace::Information, (_T("%s: %s:%d FREQUENCY_LIST=%s"), __FUNCTION__, __FILE__, __LINE__, strFreqList.c_str()));
        ParseFrequencies(strFreqList, frequencyList);
        _frequencyList = frequencyList;
        _epgDB.CreateFrequencyTable();
        _epgDB.InsertFrequencyInfo(frequencyList);
    } else
        TRACE(Trace::Information, (_T("%s: %s:%d FREQUENCY_LIST is NULL"), __FUNCTION__, __FILE__, __LINE__));
    _tvPlatform->Init();
    return (_T(""));
}

void ITuner::Deinitialize(PluginHost::IShell* service)
{
}

void ITuner::StartScan(TVPlatform::ITVPlatform::ITunerHandler& tunerHandler)
{
    TvmRc rc = TvmError;
    if (!_tvPlatform->IsScanning())
        rc = _tvPlatform->Scan(_frequencyList, tunerHandler);
}

void ITuner::StopScan()
{
    TvmRc rc = TvmError;
    if (_tvPlatform->IsScanning())
        rc = _tvPlatform->StopScanning();
}

bool ITuner::IsScanning()
{
    return (_tvPlatform->IsScanning());
}

std::string ITuner::GetCurrentChannel()
{
    TRACE(Trace::Information, (_T("Get current Channel")));
    Channel channel;
    std::string currentChannel;
    if (_epgDB.ReadChannel(_currentChannel, channel))
        channel.ToString(currentChannel);
    return currentChannel;
}

void ITuner::SetCurrentChannel(const string& channelId, TVPlatform::ITVPlatform::ITunerHandler& tunerHandler)
{
    uint16_t progNum;
    uint32_t frequency;
    if (!_epgDB.IsParentalLocked(channelId)) {
        if (_epgDB.GetFrequencyFromChannelInfo(channelId, frequency) && _epgDB.GetServiceIdFromChannelInfo(channelId, progNum)) {
            TRACE(Trace::Information, (_T("SetCurrentChannel to  %s with freq = %u program num = %d"), channelId.c_str(), frequency, progNum));
#if RECORDED_STREAM
            // FIXME: Added for testing purpose only.
            const char* freq = getenv("TVC_FREQUENCY_LIST");
            if (freq) {
                std::string strFrequency = freq;
                if (strFrequency.size()) {
                    frequency = (uint32_t)std::stoi(strFrequency);
                    frequency *= 1000000;
                    TRACE(Trace::Information, (_T("TVC_FREQUENCY_LIST=%s"), freq));
                } else
                    TRACE(Trace::Information, (_T("TVC_FREQUENCY_LIST is NULL")));
            }
#endif
            // FIXME: Remove later.

            TRACE(Trace::Information, (_T("SetCurrentChannel to %s with freq = %u program num = %d"), channelId.c_str(), frequency, progNum));
            TSInfo tsInfo;
            memset(&tsInfo, 0, sizeof(tsInfo));
            tsInfo.frequency = frequency;
            tsInfo.programNumber = progNum;
            if (_epgDB.ReadTSInfo(tsInfo)) {
                TRACE(Trace::Information, (_T("SetCurrentChannel to %s with freq = %u program num = %d \
                    videoPid = %d, videocodec= %d, videoPCRpid = %d, audioPid = %d, \
                    audiocodec = %d, audioPCRPid = %d pmtpid =%d"),
                    channelId.c_str(), tsInfo.frequency,
                    tsInfo.programNumber, tsInfo.videoPid, tsInfo.videoCodec, tsInfo.videoPcrPid,
                    tsInfo.audioPid, tsInfo.audioCodec, tsInfo.audioPcrPid, tsInfo.pmtPid));
                _tvPlatform->Disconnect();
                if( _tvPlatform->Tune(tsInfo, tunerHandler) == TvmSuccess) {
                    _currentChannel.clear();
                    _currentChannel.assign(channelId);
                    tunerHandler.CurrentChannelChanged(_currentChannel);
                    TRACE(Trace::Information, (_T("Current channel is = %s"), channelId.c_str()));
                }
            } else
                TRACE(Trace::Information, (_T("Failed get TS details for channel ID = %s, freq = %u, program number = %d form DB"), channelId.c_str(), tsInfo.frequency, tsInfo.programNumber));
        } else
            TRACE(Trace::Error, (_T("Failed to get frequency and program number for channel ID = %s"), channelId.c_str()));
    } else
        TRACE(Trace::Error, (_T("Failed to set channel since it is LOCKED")));
}

bool ITuner::StoreTSInfoInDB()
{
    TSInfoList tsInfoList;
    memset(&tsInfoList, 0, sizeof(TSInfoList));
    bool ret = false;
    TRACE(Trace::Information, (_T("TS Information\n")));
    if ((_tvPlatform->GetTSInfo(tsInfoList) == TvmSuccess) && tsInfoList.size() && _epgDB.InsertTSInfo(tsInfoList))
        ret = true;
    TRACE(Trace::Information, (_T("Failed to store TS Information\n")));
    return ret;
}
}
} // namespace WPEFramework::Plugin
