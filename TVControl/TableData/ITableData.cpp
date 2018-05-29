#include "Module.h"
#include "ITableData.h"

#include <DataQueue.h>

#define ONE_MINUTE_IN_MILLISECONDS (1 * 60 * 1000)
#define DB_CACHING_TIMER 2

namespace WPEFramework {
namespace Plugin {

void ITableData::SectionDataCB(const std::string& siData)
{
    std::pair<uint8_t*, uint16_t> dataElement;
    std::get<0>(dataElement) = reinterpret_cast<uint8_t*>(malloc(BUFFER_SIZE));
    if (std::get<0>(dataElement)) {
        memcpy(std::get<0>(dataElement), siData.c_str(), siData.length());
        uint16_t len;
        memcpy(&len, std::get<0>(dataElement) + SIZE_OFFSET, sizeof(uint16_t));
        std::get<1>(dataElement) = len;
        DataQueue::GetInstance().Push(dataElement);
    }
}

ITableData::ITableData(ITableDataHandler &tableDataHandler, TVPlatform::ITVPlatform* tvPlatform)
    : _epgDB(EPGDataBase::GetInstance())
    , _tvPlatform(tvPlatform)
    , _cleanupTimer(WPEFramework::Core::Thread::DefaultStackSize(), _T("ClearSIData"))
    , _clearSIDataTimer(0)
    , _config()
    , _isParentalControlled(false)
    , _parentalControlPin("Metro123#") // Default pin.
    , _tableDataHandler(tableDataHandler)
    , _homeTS(0)
    , _clearSIDataCounter(0)
    , _dbCacheCounter(0)
    , _isTSInfoPresent(false)
{
    _epgDB.CreateTSTable();
    TRACE(Trace::Information, (_T("SIControl instantiation check")));
    DataQueue::GetInstance().Clear();
    TRACE(Trace::Information, (_T("SIControl parental control Init")));
    ParentalControlInit();
    UpdateScanStatus();
}

ITableData* ITableData::_tblInstance = nullptr;

ITableData* ITableData::GetInstance(ITableDataHandler &tableDataHandler, TVPlatform::ITVPlatform* tvPlatform)
{
    _tblInstance = new ITableData(tableDataHandler, tvPlatform);
    return _tblInstance;
}

void ITableData::DeleteInstance()
{
    if (_tblInstance)
        delete _tblInstance;
}

void ITableData::StopFilter(uint16_t pid, uint16_t tid)
{
    _tvPlatform->StopFilter(pid, tid);
}

void ITableData::StopFilters()
{
    TRACE(Trace::Information, (_T("Stop Filters")));
    _tvPlatform->StopFilters();
}

void ITableData::StartFilter(uint16_t pid, uint16_t tid)
{
    TRACE(Trace::Information, (_T("Start Filter for PID- 0x%02x"), pid));
    _tvPlatform->StartFilter(pid, tid, this);
}

void ITableData::EitBroadcasted()
{
    TRACE(Trace::Information, (_T("EitBroadcasted")));
    _tableDataHandler.EITBroadcast();
}

void ITableData::SetHomeTS(const uint32_t primaryFreq, const uint32_t secondaryFreq)
{
    _tvPlatform->SetHomeTS(primaryFreq, secondaryFreq);
}

bool ITableData::IsScanning()
{
    return (_tvPlatform->IsScanning());
}

bool ITableData::IsTSInfoPresent()
{
    return _isTSInfoPresent;
}

void ITableData::UpdateScanStatus()
{
    std::string table("TSINFO");
    if (!_epgDB.IsTableEmpty(table))
        _isTSInfoPresent = true;
}

ITableData::~ITableData()
{
    TRACE(Trace::Information, (_T("Destructor invoked")));
    if (_tvParser) {
        TRACE(Trace::Information, (_T("Destructor invoked delete parser instance")));
        delete _tvParser;
        _tvParser = nullptr;
        TRACE(Trace::Information, (_T("Destructor deleted parser instance")));
    }
    TRACE(Trace::Information, (_T("Destructor completed")));
}

uint32_t ITableData::StartParser(PluginHost::IShell* service)
{
    service->AddRef();
    _config.FromString(service->ConfigLine());
    _clearSIDataTimer = _config.Idletime.Value(); // _clearSIDataTimer in milliseconds.
    uint32_t homeTS = (_config.HomeTS.Value()) * 1000000;

    uint32_t result = Core::ERROR_NONE;
    _tvParser = IParser::GetInstance(this, homeTS); // Get an GetInstance of current parser.
    _tvParser->SetCountryDetails(_config.CountryCode.Value(), _config.CountryRegionId.Value());
    _tvParser->Run();

    return (result);
}

void ITableData::StartTimer()
{
    Core::Time NextTick(Core::Time::Now());
    NextTick.Add(ONE_MINUTE_IN_MILLISECONDS);
    _cleanupTimer.Schedule(NextTick.Ticks(), TimeHandler(*this));
}

std::string ITableData::GetChannels()
{
    TRACE(Trace::Information, (_T("GetChannels")));
    Core::JSON::ArrayType<Channel> channelList;
    std::string strChanList;
    if (_epgDB.ReadChannels(channelList))
        channelList.ToString(strChanList);
    return strChanList;
}

std::string ITableData::GetPrograms()
{ 
    // Get all programs of all channels for 3hr.
    TRACE(Trace::Information, (_T("GetPrograms")));
    Core::JSON::ArrayType<Program> programList;
    std::string strPgmList;
    if (_epgDB.ReadPrograms(programList))
        programList.ToString(strPgmList);
    return strPgmList;
}

std::string ITableData::GetCurrentProgram(const string& channelNum)
{
    TRACE(Trace::Information, (_T("GetCurrentProgram")));
    Program program;
    std::string strPgm;
    uint16_t serviceId;
    _epgDB.GetServiceIdFromChannelInfo(channelNum, serviceId);
    if (_epgDB.ReadProgram(serviceId, program))
        program.ToString(strPgm);
    return strPgm;
}

std::string ITableData::GetAudioLanguages(const uint32_t eventId)
{
    TRACE(Trace::Information, (_T("GetAudioLanguages")));
    std::string strLanguageList;
    std::vector<std::string> audioLanguages;
    if (_epgDB.ReadAudioLanguages(eventId, audioLanguages)) {
        Core::JSON::ArrayType<Core::JSON::String> jsonLanguageList;
        for (auto& language : audioLanguages) {
            Core::JSON::String jsonLanguage;
            jsonLanguage = language;
            jsonLanguageList.Add(jsonLanguage);
        }
        jsonLanguageList.ToString(strLanguageList);
    }
    return strLanguageList;
}

std::string ITableData::GetSubtitleLanguages(const uint32_t eventId)
{
    TRACE(Trace::Information, (_T("GetSubtitleLanguages")));
    std::string strLanguageList;
    std::vector<std::string> subtitleLanguages;
    if (_epgDB.ReadSubtitleLanguages(eventId, subtitleLanguages)) {
        Core::JSON::ArrayType<Core::JSON::String> jsonLanguageList;
        for (auto& language : subtitleLanguages) {
            Core::JSON::String jsonLanguage;
            jsonLanguage = language;
            jsonLanguageList.Add(jsonLanguage);
        }
        jsonLanguageList.ToString(strLanguageList);
    }
    return strLanguageList;
}

bool ITableData::UpdateParentalLockFile()
{
    std::fstream file;
    file.open(PARENTAL_LOCK_FILE, std::fstream::trunc | std::fstream::in | std::fstream::out);
    if (file.is_open()) {
        file << "IsParentalControlled:" << _isParentalControlled << "\nPin:" << _parentalControlPin.c_str();
        file.close();
        return true;
    }
    TRACE(Trace::Information, (_T("Failed to open Lock File\n")));
    return false;
}

bool ITableData::ParentalControlInit()
{
    TRACE(Trace::Information, (_T("Parental control Init")));
    bool ret = false;
    std::fstream file;
    file.open(PARENTAL_LOCK_FILE);
    if (file.is_open()) {
        std::string line;
        while (getline(file, line))
        {
            std::string item, status;
            std::stringstream sstr(line);
            getline(sstr, item, ':');
            if (!item.compare("IsParentalControlled")) {
                getline(sstr, status, '\n');
                if (!status.compare("1"))
                    _isParentalControlled = true;
            }
            if (!item.compare("Pin"))
                getline(sstr, _parentalControlPin, '\n');
        }
        ret = true;
        file.close();
    } else
        ret = UpdateParentalLockFile();
    return ret;
}

bool ITableData::SetParentalControlPin(const string& oldPin, const string& newPin)
{
    TRACE(Trace::Information, (_T("Set Parental control pin - old pin : %s new pin : %s."), oldPin.c_str(), newPin.c_str()));
    if (!oldPin.compare(_parentalControlPin)) {
        TRACE(Trace::Information, (_T("Given and old pin is match, so set new pin.")));
        _parentalControlPin = newPin;
        return UpdateParentalLockFile();
    }
    TRACE(Trace::Error, (_T("Given and old pin does not match, can not set new pin.")));
    return false;
}

bool ITableData::SetParentalControl(const string& pin, const bool isLocked)
{
    TRACE(Trace::Information, (_T("Set Parental control - %d  pin : %s"), isLocked, pin.c_str()));
    bool ret = false;
    if (!pin.compare(_parentalControlPin)) {
        if (isLocked != _isParentalControlled) {
            _isParentalControlled = isLocked;
            if (!_isParentalControlled)
                _epgDB.UnlockChannels();
            ret = UpdateParentalLockFile();
            if (ret)
                _tableDataHandler.ParentalControlChanged();
        }
    } else
        TRACE(Trace::Error, (_T("Pin does not match, can not set parental control.")));
    return ret;
}

bool ITableData::IsParentalControlled()
{
    TRACE(Trace::Information, (_T("Is system wide parental control  set")));
    return(_isParentalControlled ? true : false);

}

bool ITableData::IsParentalLocked(const string& channelNum)
{
    TRACE(Trace::Information, (_T("Is parental lock set for channel")));
    return _epgDB.IsParentalLocked(channelNum);
}

bool ITableData::SetParentalLock(const string& pin, const bool isLocked, const string& channelNum)
{
    bool ret = false;
    TRACE(Trace::Information, (_T("Set Parental Lock for channel LCN - %d : pin - %s : enable - %d"), channelNum, pin.c_str(), isLocked));
    if (_isParentalControlled) {
        if(!pin.compare(_parentalControlPin)) {
            if (isLocked !=  _epgDB.IsParentalLocked(channelNum)) {
                ret = _epgDB.SetParentalLock(isLocked, channelNum);
                if (ret)
                    _tableDataHandler.ParentalLockChanged(channelNum);
            }
        } else
            TRACE(Trace::Error, (_T("Pin does not match, can not set parental lock.")));
    } else
        TRACE(Trace::Error, (_T("Not parental controlled, can not set parental lock.")));
    return ret;
}

uint64_t ITableData::Timed(const uint64_t scheduledTime)
{
    if (_clearSIDataTimer) {
        _clearSIDataCounter++;
        if (_clearSIDataCounter == _clearSIDataTimer) {
            _epgDB.ClearSIData();
            _clearSIDataCounter = 0;
        }
    }
    _dbCacheCounter++;
    if (_dbCacheCounter == DB_CACHING_TIMER) {
        _epgDB.LoadOrSaveDB(true);
        _dbCacheCounter = 0;
    }

    Core::Time NextTick(Core::Time::Now());
    NextTick.Add(ONE_MINUTE_IN_MILLISECONDS);
    return (NextTick.Ticks());
}

void ITableData::NotifyFrequencyListUpdate()
{
    _tvParser->UpdateFrequencyList();
}

void ITableData::NotifyTSInfoUpdate() {
    UpdateScanStatus();
}

void ITableData::NotifyStreamingFrequencyChange(uint32_t currentFrequency)
{
    _tvParser->UpdateCurrentFrequency(currentFrequency);
}

}
} // namespace Solutions::SIControl.
