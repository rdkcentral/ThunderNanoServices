#define crc32 crc32_func
#include "Module.h"
#undef crc32
#include "ParserATSC.h"

#include <errno.h>
#include <libucsi/atsc/caption_service_descriptor.h>
#include <libucsi/atsc/content_advisory_descriptor.h>
#include <libucsi/atsc/section.h>

#define ONEPART_CHANNEL_NUMBER_MASK 0X03FF0

using namespace WPEFramework;

ParserATSC::ParserATSC(ISIHandler* siHandler, uint32_t homeTS)
    : WPEFramework::Core::Thread(0, _T("SIControlParser"))
    , _epgDB(EPGDataBase::GetInstance())
    , _eitPidIndex(0)
    , _isVCTParsed(false)
    , _isTimeParsed(false)
    , _eitPidCountCurrent(0)
    , _setFilters(false)
    , _clientInitialised(false)
    , _siHandler(siHandler)
    , _freqHandler(this)
    , _currentParsingFrequency(0)
    , _homeTS(homeTS)
    , _parserRunning(false)
    , _isStreaming(false)
{
    TRACE(Trace::Information, (_T("Parser constructor is invoked")));
    _epgDB.CreateChannelTable();
    _epgDB.CreateProgramTable();
}

IParser* IParser::GetInstance(ISIHandler* siHandler, uint32_t homeTS)
{
    static ParserATSC* parserATSC = new ParserATSC(siHandler, homeTS);
    return parserATSC;
}

ISIHandler* ParserATSC::GetSIHandler()
{
    return _siHandler;
}

ParserATSC::~ParserATSC()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    if (_setFilters)
        ReleaseFilters();
    TRACE(Trace::Information, (_T("Filters stopped")));

    Block();
    DataQueue::GetInstance().Clear();
    Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);

    _vctHeaderMap.clear();
    TRACE(Trace::Information, (_T("Queue cleared")));

   _clientInitialised = false;
    TRACE(Trace::Information, (_T("Destructor Completed")));
}

void ParserATSC::ReleaseFilters()
{
    if (_setFilters) {
        GetSIHandler()->StopFilter(ATSC_BASE_PID, 0);
        for (auto pid : _eitPidVector) {
            GetSIHandler()->StopFilter(pid, 0/*Placeholder*/);
        }
    }
    _setFilters = false;
}

void ParserATSC::SendBaseTableRequest()
{
    TRACE(Trace::Information, (_T("Requesting TVCT and MGT")));
    GetSIHandler()->StartFilter(ATSC_BASE_PID, 0);
    _setFilters = true;
}

void ParserATSC::UpdateCurrentFrequency(uint32_t currentFrequency)
{
    _isStreaming = true;
    SendBaseTableRequest();
}

void ParserATSC::ConfigureParser()
{
    if (_homeTS) {
        _parserRunning = true;
        GetSIHandler()->SetHomeTS(_homeTS, 0);
        SendBaseTableRequest();
    } else {
        if (_epgDB.ReadFrequency(_frequencyList)) {
            if (_frequencyList.size()) {
                _parserRunning = true;
                _freqHandler.SetFrequencyList(_frequencyList);
                _freqHandler.Run();
            }
        }
    }
}

void ParserATSC::UpdateFrequencyList()
{
    if (!_homeTS) {
        std::vector<uint32_t> previousFrequencyList;
        if (_frequencyList.size())
            previousFrequencyList = _frequencyList;

        _epgDB.ReadFrequency(_frequencyList);
        if (_frequencyList.size() && (previousFrequencyList != _frequencyList)) {
            if (_parserRunning) {
                _freqHandler.Stop();
                GetSIHandler()->StopFilters();
            }
            _parserRunning = true;
            _freqHandler.SetFrequencyList(_frequencyList);
            _freqHandler.Run();
        }
    }
}

void ParserATSC::ParseData(struct section_ext* sectionExt, uint32_t frequency)
{
    if (_currentParsingFrequency != frequency) {
        ResetTables();
        _currentParsingFrequency = frequency;
    }
    struct atsc_section_psip* psip;
    TRACE(Trace::Information, (_T("Parser")));

    psip = atsc_section_psip_decode(sectionExt);
    if (!psip) {
        TRACE(Trace::Error, (_T("error calling atsc_section_psip_decode()")));
        return;
    }
    uint8_t tableId = sectionExt->table_id;
    TRACE(Trace::Information, (_T("Table type =  %02X "), tableId));

    switch (tableId) {
    case stag_atsc_terrestrial_virtual_channel:
        if (GetSIHandler()->IsTSInfoPresent() && !GetSIHandler()->IsScanning())
            ParseTVCT(psip, frequency);
        break;
    case stag_atsc_master_guide:
        ParseMGT(psip);
        if (_isVCTParsed && _isTimeParsed) {
            if (!_eitPidCountCurrent) { // FIXME Temporary fix till version check is implemented.
                _eitPidCountCurrent = _eitPidVector.size();
                PushEitStartRequest();
            }
        }
        break;
    case stag_atsc_event_information:
        ParseEIT(psip);
        break;
    case stag_atsc_cable_virtual_channel:
        if (GetSIHandler()->IsTSInfoPresent() && !GetSIHandler()->IsScanning())
            ParseCVCT(psip, frequency);
        break;
    case stag_atsc_system_time: {
        if (!_isTimeParsed)
            ParseSTT(psip);
        }
        break;
    case stag_atsc_rating_region:
        break;
    default:
        TRACE(Trace::Information, (_T("SI Table type unknown")));
        break;
    }
}    

bool ParserATSC::ParseEIT(struct atsc_section_psip* psip)
{
    TRACE(Trace::Information, (_T("Parse EIT")));
    uint16_t sourceId;
    uint8_t versionNo = psip->ext_head.version_number;
    uint8_t sectionNo = psip->ext_head.section_number;

    struct atsc_eit_section* eit;
    eit = atsc_eit_section_codec(psip);
    if (!eit) {
        TRACE(Trace::Error, (_T("error decode table section")));
        return false;
    }
    sourceId = atsc_eit_section_source_id(eit);
    if (_programMap.find(sourceId) == _programMap.end()) {
        for (uint8_t sectionNumber = 0; sectionNumber <= eit->head.ext_head.last_section_number; sectionNumber++)
            _programMap[sourceId].insert(sectionNumber);
    } else {
        if (_programMap[sourceId].empty())
            return true;
    }

    _programMap[sourceId].erase(eit->head.ext_head.section_number);

    if (!ParseEvents(eit, sourceId)) {
        TRACE(Trace::Error, (_T("error calling parse_events()")));
        return false;
    }
    if (IsEITParsingCompleted()) {
        GetSIHandler()->EitBroadcasted();
        TRACE(Trace::Information, (_T("Completed EIT Parsing ")));
        PushEitStopRequest();
        _eitPidIndex++;
        if ((_eitPidIndex == _eitPidVector.size()) && (_frequencyList.size() > 1) && !_isStreaming)
            _freqHandler.Run();
        else
            PushEitStartRequest();
        _programMap.clear();
    }
    return true;
}

void ParserATSC::PushEitStopRequest()
{
    if (_eitPidVector.size()) {
        TRACE(Trace::Information, (_T("PushEitStopRequest with pid =%d"), (_eitPidIndex)));
        GetSIHandler()->StopFilter(_eitPidVector[_eitPidIndex], 0/*Placeholder*/);
    }
}

void ParserATSC::PushEitStartRequest()
{
    if (_eitPidVector.size()) {
        TRACE(Trace::Information, (_T("PushEitStartRequest with pid =%d"), _eitPidIndex));
        if (_setFilters) {
            _eitPidIndex %= _eitPidVector.size();
            GetSIHandler()->StartFilter(_eitPidVector[_eitPidIndex], 0/*Placeholder*/);
        }
    }
}

bool ParserATSC::IsEITParsingCompleted()
{
    bool isCompleted = false;
    if (_programMap.size() == _channelSet.size()) {
        for (auto& sectionsList : _programMap) {
            if (!sectionsList.second.empty())
                return isCompleted;
        }
        isCompleted = true;
    }
    return isCompleted;
}

bool ParserATSC::ResetTables()
{
    bool ret = true;
    TRACE(Trace::Information, (_T("Reset Tables on Backend restart")));
    _eitPidCountCurrent = 0;
    _isVCTParsed = false;
    _vctHeaderMap.clear();
    _mgtHeader.reset();
    _eitPidIndex = 0;
    _eitPidVector.clear();
    _programMap.clear();
    _channelSet.clear();
    return ret;
}

bool ParserATSC::ParseSTT(struct atsc_section_psip* psip)
{
    int8_t versionNo = psip->ext_head.version_number;
    uint16_t extensionId = psip->ext_head.table_id_ext;
    uint8_t sectionNo = psip->ext_head.section_number;

    TRACE(Trace::Information, (_T("Parse STT")));
    struct atsc_stt_section* stt;
    stt = atsc_stt_section_codec(psip);
    if (!stt) {
        TRACE(Trace::Error, (_T("error decode table section")));
        return false;
    }

    auto sttHeader = std::make_shared<SectionTableHeader>(versionNo, extensionId, sectionNo);
    if (_sttHeader) {
        if ((sttHeader->Version() == _sttHeader->Version()) && (sttHeader->SectionNumber() == _sttHeader->SectionNumber()))
            return true; //skip
    }
    _sttHeader = sttHeader;

    struct timeval timeVal = {};
    timeVal.tv_sec = atsctime_to_unixtime(stt->system_time - stt->gps_utc_offset);
    if (settimeofday(&timeVal, NULL) < 0)
        TRACE(Trace::Error, (_T("Error in settimeofday, errMsg = %s"), strerror(errno)));
    _isTimeParsed = true;
    if (_frequencyList.size() > 1)
        _freqHandler.Run();

    GetSIHandler()->StartTimer();
    return true;
}

bool ParserATSC::ParseMGT(struct atsc_section_psip* psip)
{
    TRACE(Trace::Information, (_T("Parse MGT")));
    bool ret = true;

    struct atsc_mgt_section* mgt;
    mgt = atsc_mgt_section_codec(psip);
    if (!mgt) {
        TRACE(Trace::Error, (_T("error decode table section")));
        return false;
    }

    uint8_t versionNo = psip->ext_head.version_number;
    uint16_t extensionId = psip->ext_head.table_id_ext;
    uint8_t sectionNo = psip->ext_head.section_number;

    auto mgtHeader = std::make_shared<SectionTableHeader>(versionNo, extensionId, sectionNo);
    if (_mgtHeader) {
        if ((mgtHeader->Version() == _mgtHeader->Version()) && (mgtHeader->SectionNumber() == _mgtHeader->SectionNumber()))
            return true; // Skip.
    }
    _mgtHeader = mgtHeader;

    struct atsc_mgt_table* t;
    uint32_t i, j;
    struct mgtTableName mgtTabNameArray[] = {
        {0x0000, "terrestrial VCT with current_next_indictor=1"},
        {0x0001, "terrestrial VCT with current_next_indictor=0"},
        {0x0002, "cable VCT with current_next_indictor=1"},
        {0x0003, "cable VCT with current_next_indictor=0"},
        {0x0004, "channel ETT"},
        {0x0005, "DCCSCT"},
        {0x00FF, "reserved for future  use"},
        {0x017F, "EIT"},
        {0x01FF, "reserved for future  use"},
        {0x027F, "event ETT"},
        {0x02FF, "reserved for future  use"},
        {0x03FF, "RRT with rating region"},
        {0x0FFF, "user private"},
        {0x13FF, "reserved for future  use"},
        {0x14FF, "DCCT with dcc_id"},
        {0xFFFF, "reserved for future  use"}
    };

    atsc_mgt_section_tables_for_each(mgt, t, i) {
        struct mgtTableName table;
        memset(&table, 0, sizeof(struct mgtTableName));

        for (j = 0; j < (int)(sizeof(mgtTabNameArray) / sizeof(struct mgtTableName)); j++) {
            if (t->table_type > mgtTabNameArray[j]._range)
                continue;
            table = mgtTabNameArray[j];
            if (!j || mgtTabNameArray[j - 1]._range + 1 == mgtTabNameArray[j]._range)
                j = -1;
            else {
                j = t->table_type - mgtTabNameArray[j - 1]._range - 1;
                // Table type is EIT push new request.
                if (t->table_type >= 0x0100 && t->table_type <= 0x017F) {
                    TRACE(Trace::Information, (_T("EIT::  %2d: type = 0x%04X, PID = 0x%04X "), i,
                        t->table_type, t->table_type_PID));
                    if (std::find(_eitPidVector.begin(), _eitPidVector.end(), t->table_type_PID) == _eitPidVector.end())
                        _eitPidVector.push_back(t->table_type_PID);
                }
            }
            break;
        }
        TRACE(Trace::Information, (_T("%2d: type = 0x%04X, PID = 0x%04X, %s "), i,
            t->table_type, t->table_type_PID, table._string));
        if (-1 != j)
            TRACE(Trace::Information, (_T(" %d"), j));
    }

    return ret;
}

bool ParserATSC::ParseCVCT(struct atsc_section_psip* psip, uint32_t frequency)
{
    TRACE(Trace::Information, (_T("Parse CVCT")));

    uint8_t versionNo = psip->ext_head.version_number;
    uint16_t transportStreamId = psip->ext_head.table_id_ext;
    uint8_t sectionNo = psip->ext_head.section_number;

    struct atsc_cvct_section *cvct;
    if ((cvct = atsc_cvct_section_codec(psip)) == NULL) {
        fprintf(stderr, "SCT XXXX CVCT section decode error\n");
        return false;
    }

    auto cvctHeader = std::make_shared<SectionTableHeader>(versionNo, transportStreamId, sectionNo);
    auto it = _vctHeaderMap.find(transportStreamId);
    if (it != _vctHeaderMap.end()) {
        if ((cvctHeader->Version() == it->second->Version()) && (cvctHeader->SectionNumber() == it->second->SectionNumber()))
            return true; //skip

        if (cvctHeader->Version() != it->second->Version()) {
            _channelSet.clear();
            _programMap.clear();
        }
        it->second = cvctHeader;
    } else
        _vctHeaderMap.insert(std::make_pair(transportStreamId, cvctHeader));

    struct atsc_cvct_channel *channel;
    struct descriptor *desc;
    uint32_t idx;
    struct PmtPidInfo* pmtInfo;
    pmtInfo = reinterpret_cast<struct PmtPidInfo*>(malloc(sizeof(struct PmtPidInfo)));
    TRACE(Trace::Information, (_T("\tSCT tranport_stream_id:0x%04x"),
        atsc_cvct_section_transport_stream_id(cvct)));
    atsc_cvct_section_channels_for_each(cvct, channel, idx) {
        char* name;
        uint16_t k;
        name = (char*)malloc(sizeof(char) * MAX_TITLE_SIZE);
        for (k = 0; k < MAX_TITLE_SIZE; k++)
            name[k] = GetBits((const uint8_t *)channel->short_name, k * 16, 16);
        name[MAX_TITLE_SIZE] = '\0';
        std::string logicalChannelNumber;
        if ((channel->major_channel_number && ONEPART_CHANNEL_NUMBER_MASK) == ONEPART_CHANNEL_NUMBER_MASK)
            logicalChannelNumber.assign(std::to_string((channel->major_channel_number & 0x00F) << 10 + channel->minor_channel_number));
        else {
            logicalChannelNumber.assign(std::to_string(channel->major_channel_number));
            logicalChannelNumber += ".";
            logicalChannelNumber += std::to_string(channel->minor_channel_number);
        }
        TRACE(Trace::Information, (_T("TSID : %d"), channel->channel_TSID));
        TRACE(Trace::Information, (_T("ProgramNumber : %d"), channel->program_number));
        TRACE(Trace::Information, (_T("LCN : %s"), logicalChannelNumber.c_str()));
        TRACE(Trace::Information, (_T("ServiceID : %d"), channel->source_id));

        uint16_t srcId = channel->source_id;
        _channelSet.insert(srcId);
        TRACE(Trace::Information, (_T("Service Name : %s"), name))

        TRACE(Trace::Information, (_T("\tSCT major_channel_number:%04x minor_channel_number:%04x modulation_mode:%02x carrier_frequency:%i channel_TSID:%04x program_number:%04x ETM_location:%i access_controlled:%i hidden:%i path_select:%i out_of_band:%i hide_guide:%i service_type:%02x source_id:%04x"),
            channel->major_channel_number,
            channel->minor_channel_number,
            channel->modulation_mode,
            channel->carrier_frequency,
            channel->channel_TSID,
            channel->program_number,
            channel->ETM_location,
            channel->access_controlled,
            channel->hidden,
            channel->path_select,
            channel->out_of_band,
            channel->hide_guide,
            channel->service_type,
            channel->source_id));
        std::string language;
        atsc_cvct_channel_descriptors_for_each(channel, desc)
        {
            TRACE(Trace::Information, (_T("Descriptor : %x"), desc->tag));
            switch (desc->tag) {
            case DTagAtscExtendedChannelName:
                ParseAtscExtendedChannelNameDescriptor(&name, const_cast<const unsigned char*>(reinterpret_cast<unsigned char*>(desc)));
                break;
            case DTagAtscServiceLocation:
                ParseAtscServiceLocationDescriptor(pmtInfo, const_cast<const unsigned char*>(reinterpret_cast<unsigned char*>(desc)));
                for (uint16_t audioIndex = 0; audioIndex < pmtInfo->_audioNum; ++audioIndex) {
                    language += pmtInfo->_audioLang[audioIndex];
                    if (audioIndex != pmtInfo->_audioNum - 1)
                        language += ',';
                }
                break;
            default:
                break;
            }
        }
        if (_epgDB.IsServicePresentInTSInfo(channel->program_number))
            _epgDB.InsertChannelInfo(frequency, channel->modulation_mode, name, channel->source_id, channel->channel_TSID, 0,
                logicalChannelNumber, channel->program_number, language);
        if (name)
            free(name);
    }
    _isVCTParsed = true;  //FIXME  Todo recheck same flag could be used
    return true;
}

bool ParserATSC::ParseTVCT(struct atsc_section_psip* psip, uint32_t frequency)
{
    TRACE(Trace::Information, (_T("Parse TVCT")));

    uint8_t versionNo = psip->ext_head.version_number;
    uint16_t transportStreamId = psip->ext_head.table_id_ext;
    uint8_t sectionNo = psip->ext_head.section_number;

    struct atsc_tvct_section* tvct;
    bool ret = true;

    tvct = atsc_tvct_section_codec(psip);
    if (!tvct) {
        TRACE(Trace::Error, (_T("error decode table section")));
        return false;
    }

    if (MAX_NUM_CHANNELS < tvct->num_channels_in_section) {
        TRACE(Trace::Error, (_T("no support for more than %d "
            "virtual channels in a pyhsical channel"), MAX_NUM_CHANNELS));
        return false;
    }

    auto tvctHeader = std::make_shared<SectionTableHeader>(versionNo, transportStreamId, sectionNo);
    auto it = _vctHeaderMap.find(transportStreamId);
    if (it != _vctHeaderMap.end()) {
        if ((tvctHeader->Version() == it->second->Version()) && (tvctHeader->SectionNumber() == it->second->SectionNumber()))
            return true; // Skip.

        if (tvctHeader->Version() != it->second->Version()) {
            _channelSet.clear();
            _programMap.clear();
        }
        it->second = tvctHeader;
    } else
        _vctHeaderMap.insert(std::make_pair(transportStreamId, tvctHeader));

    struct PmtPidInfo* pmtInfo;
    pmtInfo = reinterpret_cast<struct PmtPidInfo*>(malloc(sizeof(struct PmtPidInfo)));
    uint32_t i, k;
    struct atsc_tvct_channel* ch;
    atsc_tvct_section_channels_for_each(tvct, ch, i)
    {
        char* name;
        name = (char*)malloc(sizeof(char) * MAX_TITLE_SIZE);
        for (k = 0; k < MAX_TITLE_SIZE; k++)
            name[k] = GetBits((const uint8_t *)ch->short_name, k * 16, 16);
        name[MAX_TITLE_SIZE] = '\0';
        uint16_t majorNum = (uint16_t)ch->major_channel_number;
        uint16_t minorNum = (uint16_t)ch->minor_channel_number;
        std::string logicalChannelNumber;
        logicalChannelNumber.assign(std::to_string(ch->major_channel_number));
        logicalChannelNumber += ".";
        logicalChannelNumber += std::to_string(ch->minor_channel_number);

        TRACE(Trace::Information, (_T("TSID : %d"), ch->channel_TSID));
        TRACE(Trace::Information, (_T("ProgramNumber : %d"), ch->program_number));
        TRACE(Trace::Information, (_T("LCN : %s"), logicalChannelNumber.c_str()));
        TRACE(Trace::Information, (_T("ServiceID : %d"), ch->source_id));

        uint16_t srcId = ch->source_id;
        _channelSet.insert(srcId);
        TRACE(Trace::Information, (_T("Service Name : %s"), name));
        struct descriptor* desc;
        std::string language;
        atsc_tvct_channel_descriptors_for_each(ch, desc)
        {
            TRACE(Trace::Information, (_T("Descriptor : %x"), desc->tag));
            switch (desc->tag) {
            case DTagAtscExtendedChannelName:
                ParseAtscExtendedChannelNameDescriptor(&name, const_cast<const unsigned char*>(reinterpret_cast<unsigned char*>(desc)));
                break;
            case DTagAtscServiceLocation:
                ParseAtscServiceLocationDescriptor(pmtInfo, const_cast<const unsigned char*>(reinterpret_cast<unsigned char*>(desc)));
                for (uint16_t audioIndex = 0; audioIndex < pmtInfo->_audioNum; ++audioIndex) {
                    language += pmtInfo->_audioLang[audioIndex];
                    if (audioIndex != pmtInfo->_audioNum - 1)
                        language += ',';
                }
                break;
            default:
                break;
            }
        }
        if (_epgDB.IsServicePresentInTSInfo(ch->program_number)) {
            _epgDB.InsertChannelInfo(frequency, ch->modulation_mode, name, ch->source_id, ch->channel_TSID, 0
                , logicalChannelNumber, ch->program_number, language);
        }
        if (name)
            free(name);
    }
    ret = true;
    free(pmtInfo);
    _isVCTParsed = true;
    return ret;
}

uint32_t ParserATSC::GetBits(const uint8_t* buf, uint32_t startbit, uint32_t bitlen)
{
    const uint8_t* b = &buf[startbit / 8];
    startbit %= 8;

    uint32_t bitHigh = 8;
    uint32_t tmpLong = b[0];
    for (uint32_t i = 0; i < ((bitlen - 1) >> 3); i++) {
        tmpLong <<= 8;
        tmpLong |= b[i + 1];
        bitHigh += 8;
    }

    startbit = bitHigh - startbit - bitlen;
    tmpLong = tmpLong >> startbit;
    uint32_t mask = (1ULL << bitlen) - 1;
    return tmpLong & mask;
}

void ParserATSC::ParseAtscExtendedChannelNameDescriptor(char** serviceName, const unsigned char* buf)
{
    unsigned char* b = (unsigned char*)buf + 2;
    uint8_t numStr = b[0];
#define uncompressed_string 0x00
    b++;
    for (uint16_t i = 0; i < numStr; i++) {
        uint8_t numSeg = b[3];
        b += 4; // Skip lang code.
        for (uint16_t j = 0; j < numSeg; j++) {
            uint8_t compressionType = b[0]; // mode = b[1].
            uint8_t numBytes = b[2];

            switch (compressionType) {
            case uncompressed_string:
                memcpy(*serviceName, &b[3], numBytes);
                *(serviceName[numBytes]) = '\0';
                break;
            default:
                break;
            }
            b += 3 + numBytes;
        }
    }
}

struct ATSCServiceLocationDescriptor ParserATSC::ReadATSCServiceLocationDescriptor(const uint8_t *b)
{
    struct ATSCServiceLocationDescriptor v;
    v._descriptorTag = GetBits(b,  0, 8);
    v._descriptorLength = GetBits(b,  8, 8);
    v._reserved = GetBits(b, 16, 3);
    v._pcrPid = GetBits(b, 19, 13);
    v._numberElements = GetBits(b, 32, 8);
    return v;
}

struct ATSCServiceLocationElement ParserATSC::ReadATSCServiceLocationElement(const uint8_t *b)
{
    struct ATSCServiceLocationElement v;
    v._streamType = GetBits(b,  0, 8);
    v._reserved = GetBits(b,  8, 3);
    v._elementaryPid = GetBits(b, 11, 13);
    v._ISO639LanguageCode = GetBits(b, 24, 24);
    return v;
}

void ParserATSC::ParseAtscServiceLocationDescriptor(struct PmtPidInfo* s, const unsigned char *buf)
{
    struct ATSCServiceLocationDescriptor d = ReadATSCServiceLocationDescriptor(buf);
    unsigned char* b = (unsigned char *)buf + 5;
    s->_pcrPid = d._pcrPid;
    s->_audioNum = 0;
    for (uint16_t count = 0; count < d._numberElements; count++) {
        struct ATSCServiceLocationElement e = ReadATSCServiceLocationElement(b);
        switch (e._streamType) {
        case 0x02: // Video.
            s->_videoPid = e._elementaryPid;
            TRACE(Trace::Information, (_T("  VIDEO     : PID 0x%04x"), e._elementaryPid));
            break;
        case 0x81: // Audio.
            if (s->_audioNum < AUDIO_CHAN_MAX) {
                s->_audioPid[s->_audioNum] = e._elementaryPid;
                s->_audioLang[s->_audioNum][0] = (e._ISO639LanguageCode >> 16) & 0xff;
                s->_audioLang[s->_audioNum][1] = (e._ISO639LanguageCode >> 8)  & 0xff;
                s->_audioLang[s->_audioNum][2] =  e._ISO639LanguageCode        & 0xff;
                s->_audioLang[s->_audioNum][3] = '\0';
                s->_audioNum++;
            }
            TRACE(Trace::Information, (_T("\tAUDIO\t: PID 0x%04x lang: %s"), e._elementaryPid, s->_audioLang[s->_audioNum-1]));
            break;
        default:
            TRACE(Trace::Information, (_T("unhandled stream_type: %x"), e._streamType));
            break;
        }
        b += 6;
    }
}

bool ParserATSC::ParseEvents(struct atsc_eit_section *eit, uint8_t sourceId)
{
    uint32_t i, j, k;
    struct atsc_eit_event* e;
    time_t startTime, endTime;

    if (!eit) {
        TRACE(Trace::Error, (_T("nullptr pointer detected")));
        return false;
    }
    atsc_eit_section_events_for_each(eit, e, i) {
        struct tm start;
        struct tm end;
        startTime = atsctime_to_unixtime(e->start_time);
        endTime = startTime + e->length_in_seconds;
        localtime_r(&startTime, &start);
        localtime_r(&endTime, &end);

        TRACE(Trace::Information, (_T("|%02d:%02d--%02d:%02d| "),
            start.tm_hour, start.tm_min,
            end.tm_hour, end.tm_min));
        uint8_t titleLength = (e->title_length) + 1;
        struct atsc_text* titleText = atsc_eit_event_name_title_text(e);
        if (!titleText)
            continue;

        std::string title = ATSCTextDecode(titleText, titleLength);
        TRACE(Trace::Information, (_T("title = %s"), title.c_str()));

        std::string rating;
        struct descriptor* desc;
        char* captionLanguage = nullptr;
        struct atsc_eit_event_part2* part = atsc_eit_event_part2(e);
        atsc_eit_event_part2_descriptors_for_each(part, desc) {
            if (desc->tag == DTagAtscContentAdvisory)
                rating = ParseAtscContentAdvisoryDescriptor(desc);
            else if (desc->tag == DTagAtscCaptionService)
                captionLanguage = ParseAtscCaptionServiceDescriptor(desc);
        }
        _epgDB.InsertProgramInfo(sourceId, e->event_id, startTime, e->length_in_seconds, title.c_str(), rating, captionLanguage ? captionLanguage : "", "", "");
        if (captionLanguage)
            free(captionLanguage);
        TRACE(Trace::Information, (_T("event_id : %d"), e->event_id));
    }
    return true;
}

char* ParserATSC::ParseAtscCaptionServiceDescriptor(struct descriptor* desc)
{
    struct atsc_caption_service_descriptor* captionDesc = atsc_caption_service_descriptor_codec(desc);
    struct atsc_caption_service_entry* caption;
    uint32_t i = 0;
    atsc_caption_service_descriptor_entries_for_each(captionDesc, caption, i) {
        TRACE(Trace::Information, (_T("Caption lang: %c%c%c "), caption->language_code[0], caption->language_code[1], caption->language_code[2]));
        char* language = reinterpret_cast<char*>(malloc(4));
        language[0] = caption->language_code[0];
        language[1] = caption->language_code[1];
        language[2] = caption->language_code[2];
        language[3] = '\0';
        return language;
    }
    return nullptr;
}

std::string ParserATSC::GetUnicode(uint8_t *sourceAddr, size_t sourceLength)
{
    std::string name;
    size_t length;
    for (length = 0; length < sourceLength; length++) {
        if (*sourceAddr >= 0x80 || (*sourceAddr == 0x22) || (*sourceAddr == 0x27) || (*sourceAddr == 0x5c)) {
            name.push_back(' ');
            sourceAddr++;
        } else if (*sourceAddr < 0x20)
            sourceAddr++;
        else
            name.push_back(*(sourceAddr++));
    }
    return name;
}

std::string ParserATSC::ATSCTextDecode(struct atsc_text* aText, uint8_t length)
{
    std::string returnText;
    if (!length)
        return returnText;

    int strIndex;
    struct atsc_text_string* curString;
    atsc_text_strings_for_each(aText, curString, strIndex) {
        int segIndex;
        struct atsc_text_string_segment* curSegment;
        atsc_text_string_segments_for_each(curString, curSegment, segIndex) {
            if (curSegment->compression_type < 0x3E) {
                uint8_t* decoded = nullptr;
                size_t decodedLen = 0;
                size_t decodedPos = 0;
                if (atsc_text_segment_decode(curSegment, &decoded, &decodedLen, &decodedPos) < 0) {
                    TRACE(Trace::Error, (_T("Decode error")));
                } else
                    returnText = GetUnicode(decoded, decodedPos);
                if (decoded)
                    free(decoded);
            }
        }
    }
    return returnText;
}

std::string ParserATSC::ParseAtscContentAdvisoryDescriptor(struct descriptor* desc)
{
    std::string rating;
    struct atsc_content_advisory_descriptor* content = atsc_content_advisory_descriptor_codec(desc);
    uint32_t i = 0;
    struct atsc_content_advisory_entry* entry;
    atsc_content_advisory_descriptor_entries_for_each(content, entry, i) {
        struct atsc_content_advisory_entry_part2* entry2 = atsc_content_advisory_entry_part2(entry);
        struct atsc_text* descriptionEntry = atsc_content_advisory_entry_part2_description(entry2);
        rating = ATSCTextDecode(descriptionEntry, entry2->rating_description_length);
    }
    return rating;
}

uint32_t ParserATSC::Worker()
{
    TRACE(Trace::Information, (_T("Worker is invoked")));
    if (!_clientInitialised) {
        _clientInitialised = true;
        ConfigureParser();
    }
    while (IsRunning() == true) {
        TRACE(Trace::Information, (_T("Parser running = %d \n"), IsRunning()));
        std::pair<uint8_t*, uint16_t>  dataElement;
        dataElement = DataQueue::GetInstance().Pop();
        // FIXME:Finalising data frame format.
        TRACE(Trace::Information, (_T("Worker data obtained")));
        if ((IsRunning() == true) && std::get<0>(dataElement)) {
            struct section_ext* sectionExt;
            TRACE(Trace::Information, (_T("DBS ATSC")));
            // Has to be modified after Finalising data frame format.
            GetSectionExt((std::get<0>(dataElement) + DATA_OFFSET), std::get<1>(dataElement), &sectionExt);
            uint32_t frequency = 0;
            memcpy(&frequency, std::get<0>(dataElement), sizeof(uint32_t));
            TRACE(Trace::Information, (_T("Frequency = %u\n"), frequency));
            ParseData(sectionExt, frequency);
            free(std::get<0>(dataElement));
        }
    }
    TRACE(Trace::Information, (_T("Worker thread to Block state")));
    return (WPEFramework::Core::infinite);
}

bool ParserATSC::GetSectionExt(uint8_t* siData, uint32_t size, struct section_ext** sectionExt)
{
    TRACE(Trace::Information, (_T("GetSectionExt")));
    struct section* section;
    bool ret = false;

    // Parse section.
    section = section_codec(siData, size);
    if (!section) {
        TRACE(Trace::Error, (_T("error calling section_codec()")));
        return ret;
    }
    TRACE(Trace::Information, (_T("section_ext_decode")));

    *sectionExt = section_ext_decode(section, 0);
    if (!*sectionExt) {
        TRACE(Trace::Error, (_T("error calling section_ext_decode()")));
        return ret;
    }
    return ret;
}

void ParserATSC::ClearEITPids()
{
    _eitPidCountCurrent = 0;
    _eitPidIndex = 0;
    _eitPidVector.clear();
}

FrequencyHandler::FrequencyHandler(IParser* parser)
    : _parser(parser)
    , _currentFreqIndex(0)
{
}

FrequencyHandler::~FrequencyHandler()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    Block();
    TRACE(Trace::Information, (string(__FUNCTION__)));
    Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
    TRACE(Trace::Information, (string(__FUNCTION__)));
}

uint32_t FrequencyHandler::Worker()
{
    ParserATSC* parser = static_cast<ParserATSC*>(_parser);
    TRACE(Trace::Information, (_T("FrequencyHandler::Worker()")));
    if (IsRunning() == true) {
        if (parser->IsParserRunning() && (!parser->IsStreaming())) {
            parser->GetSIHandler()->StopFilters();
            parser->ClearEITPids();
            parser->GetSIHandler()->SetHomeTS(_frequencyList[_currentFreqIndex], 0);
            parser->SendBaseTableRequest();

            if (_frequencyList.size() == 1) {
                TRACE(Trace::Information, (string(__FUNCTION__)));
            } else if (_frequencyList.size() > 1) {
                TRACE(Trace::Information, (string(__FUNCTION__)));
                _currentFreqIndex++;
                _currentFreqIndex %= _frequencyList.size();
                TRACE(Trace::Information, (string(__FUNCTION__)));
            }
        }
    }
    TRACE(Trace::Information, (_T("Worker thread to Block state")));
    Block();
    return (WPEFramework::Core::infinite);
}
