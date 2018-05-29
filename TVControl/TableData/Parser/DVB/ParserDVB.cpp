#include "Module.h"
#include "ParserDVB.h"

using namespace WPEFramework;

#define READ_16(buf1, buf0) ((uint16_t)(buf1 << 8 | buf0))
#define READ_32(buf3, buf2, buf1, buf0) ((uint32_t)((buf3 << 24) | (buf2 << 16) | (buf1 << 8) | buf0))
#define      GETBITS(Source, MSBit, LSBit)   ( \
            ((((1 << (MSBit - LSBit + 1))-1) << LSBit) & Source) >> LSBit)

uint32_t Bcd32ToInteger(const uint8_t b0, const uint8_t b1, const uint8_t b2, const uint8_t b3)
{
    return ((b0 >> 4) & 0x0f) * 10000000 + (b0 & 0x0f) * 1000000 +
        ((b1 >> 4) & 0x0f) * 100000   + (b1 & 0x0f) * 10000 +
        ((b2 >> 4) & 0x0f) * 1000     + (b2 & 0x0f) * 100 +
        ((b3 >> 4) & 0x0f) * 10       + (b3 & 0x0f);
}

uint32_t BcdToInteger(uint32_t bcd)
{
    uint32_t integer = 0;

    int32_t i;
    for (i = 28; i >= 0; i -= 4) {
        integer += ((bcd >> i) & 0x0f);
        if (i)
            integer *= 10;
    }
    return integer;
}

time_t DvbdateToUnixtime(DvbDate dvbDate)
{
    int32_t k = 0;
    struct tm tm;
    double mjd;

    // Check for the undefined value.
    if ((dvbDate[0] == 0xff) && (dvbDate[1] == 0xff) && (dvbDate[2] == 0xff) && (dvbDate[3] == 0xff) && (dvbDate[4] == 0xff))
        return -1;

    memset(&tm, 0, sizeof(tm));
    mjd = READ_16(dvbDate[0], dvbDate[1]);

    tm.tm_year = (int) ((mjd - 15078.2) / 365.25);
    tm.tm_mon = (int) (((mjd - 14956.1) - (int) (tm.tm_year * 365.25)) / 30.6001);
    tm.tm_mday = (int) mjd - 14956 - (int) (tm.tm_year * 365.25) - (int) (tm.tm_mon * 30.6001);
    if ((tm.tm_mon == 14) || (tm.tm_mon == 15))
        k = 1;
    tm.tm_year += k;
    tm.tm_mon = tm.tm_mon - 2 - k * 12;
    tm.tm_sec = BcdToInteger(dvbDate[4]);
    tm.tm_min = BcdToInteger(dvbDate[3]);
    tm.tm_hour = BcdToInteger(dvbDate[2]);

    return mktime(&tm);
}

time_t DvbdurationToSeconds(DvbDuration dvbDuration)
{
    time_t seconds = 0;

    seconds += (BcdToInteger(dvbDuration[0]) * 60 * 60);
    seconds += (BcdToInteger(dvbDuration[1]) * 60);
    seconds += BcdToInteger(dvbDuration[2]);

    return seconds;
}

time_t DvbhhmmToSeconds(DvbHHMM dvbhhmm)
{
    time_t seconds = 0;

    seconds += (BcdToInteger(dvbhhmm[0]) * 60 * 60);
    seconds += (BcdToInteger(dvbhhmm[1]) * 60);

    return seconds;
}

void ParserDVB::ReleaseFilters()
{
    if (_baseFiltersSet) {
        GetSIHandler()->StopFilter(SDT_PID, 0/*Placeholder*/);
        GetSIHandler()->StopFilter(NIT_PID, 0/*Placeholder*/);
        GetSIHandler()->StopFilter(EIT_PID, 0/*Placeholder*/);
        GetSIHandler()->StopFilter(TDT_PID, 0/*Placeholder*/);
        _baseFiltersSet = false;
    }
}

IParser* IParser::GetInstance(ISIHandler* siHandler, uint32_t homeTS)
{
    static ParserDVB* parserDVB = new ParserDVB(siHandler, homeTS);
    return parserDVB;
}

ISIHandler* ParserDVB::GetSIHandler()
{
    return _siHandler;
}

ParserDVB::ParserDVB(ISIHandler* siHandler, uint32_t homeTS)
    : WPEFramework::Core::Thread(0, _T("SIControlParser"))
    , _epgDB(EPGDataBase::GetInstance())
    , _clientInitialised(false)
    , _parserRunning(false)
    , _siHandler(siHandler)
    , _isTimeParsed(false)
    , _baseFiltersSet(false)
    , _homeTS(homeTS)
    , _currentParsingFrequency(0)
    , _freqHandler(this)
    , _countryCode("")
    , _countryRegionId(0)
    , _localTimeOffset(0)
    , _isCableDeliveryDescriptorPresent(false)
{
    TRACE(Trace::Information, (_T("ParserDVB constructor is invoked")));
    _epgDB.CreateNitTable();
    _epgDB.CreateChannelTable();
    _epgDB.CreateProgramTable();
    _event.audioLanguages.clear();
    _event.subtitleLanguages.clear();
}

ParserDVB::~ParserDVB()
{
    if (_baseFiltersSet)
        ReleaseFilters();
    TRACE(Trace::Information, (_T("Filters stopped")));
    DataQueue::GetInstance().Clear();
    TRACE(Trace::Information, (_T("Queue cleared")));
    _nitHeaderMap.clear();
    _sdtHeaderMap.clear();
    _eitHeaderMap.clear();
#ifdef ENABLE_BOUQUET_PARSING
    _batHeaderMap.clear();
    _batInfo.clear();
#else
    _nitInfo.clear();
#endif
    _sdtInfo.clear();

    ClearLanguages();

    Block();
    Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);

    _clientInitialised = false;
    TRACE(Trace::Information, (_T("Destructor Completed")));
}

std::string ParserDVB::ConvertToUnicode(uint8_t *sourceAddr, uint8_t sourceLength)
{
    std::string unicode;
    switch (*sourceAddr) {
    case ISO8859_5:
    case ISO8859_6:
    case ISO8859_7:
    case ISO8859_8:
    case ISO8859_9:
    case ISO8859_10:
    case ISO8859_11:
    case ISO8859_13:
    case ISO8859_14:
    case ISO8859_15:
        if (sourceLength > 1) {
            sourceAddr++;
            unicode = GetUnicode(sourceAddr, sourceLength-1);
        }
        break;
    case ISO8859:
        sourceAddr += 1;
        if ((sourceLength > 3) && (*(sourceAddr) == 0x00) ) {
            sourceAddr += 1;
            if (*(sourceAddr) > 0x00 && *(sourceAddr) < 0x10) {
                sourceAddr += 1;
                unicode = GetUnicode(sourceAddr, sourceLength-3);
            }
        }
        break;
    case ISO10646_1:
        if (sourceLength > 1) {
            sourceAddr += 1;
            unicode = GetUnicode(sourceAddr, sourceLength-1);
        }
        break;
    case 0x13:
        break;
    case 0x15:
        break;
    default :
        unicode = GetUnicode(sourceAddr, sourceLength);
        break;
    }
    return unicode;
}

std::string ParserDVB::GetUnicode(uint8_t *sourceAddr, uint8_t sourceLength)
{
    std::string name;
    uint8_t length;
    for (length = 0; length < sourceLength; length++) {
        if (*sourceAddr >= 0x80 || aunUnicode6937[*(sourceAddr)] == 34 || aunUnicode6937[*(sourceAddr)] == 39 || aunUnicode6937[*(sourceAddr)] == 92) {
            name.push_back(' ');
            sourceAddr++;
        } else if (*sourceAddr < 0x20) {
            sourceAddr++;
        } else {
            name.push_back(aunUnicode6937[*(sourceAddr++)]);
        }
    }
    return name;
}

void ParserDVB::ClearLanguages()
{
    _event.audioLanguages.clear();
    _event.subtitleLanguages.clear();
}

void ParserDVB::ParseData(uint8_t* siData, uint32_t frequency)
{
    if (_currentParsingFrequency != frequency) {
        ResetTables();
        _currentParsingFrequency = frequency;
    }

    uint8_t tableId = siData[0];
    TRACE(Trace::Information, (_T("Table type =  %02X"), tableId));
    uint16_t sectionLength;
    uint16_t tableIdExt;
    uint8_t versionNo;
    uint8_t sectionNo;

    sectionLength = READ_16(GETBITS(siData[1], 3, 0), siData[2]);
    tableIdExt = READ_16(siData[3], siData[4]);
    versionNo = GETBITS(siData[5], 5, 1);
    sectionNo = siData[6];
    siData += TABLE_HEADER_LEN;
    sectionLength -= (LAST_SECTION_NUM_OFFSET - TABLE_ID_EXTENSION_OFFSET + 1) + CRC_LEN; // Remaining header length + CRC length.

    switch (tableId) {
    case PROGRAM_ASSOCIATION_TABLE_ID: {
        ParsePAT(siData, sectionLength);
        break;
    }

    case PROGRAM_MAP_TABLE_ID: {
        ParsePMT(siData, sectionLength, tableIdExt);
        break;
    }

#ifdef ENABLE_BOUQUET_PARSING
    case NETWORK_INFORMATION_OTHER_TABLE_ID:
#endif
    case NETWORK_INFORMATION_TABLE_ID: {
        ParseNIT(siData, sectionLength, tableIdExt, versionNo, sectionNo);
        // FIXME Temporary addition till simultaneous filter are ready.
        break;
    }
#ifdef ENABLE_BOUQUET_PARSING
    case BOUQUET_ASSOCIATION_TABLE_ID: {
        ParseBAT(siData, sectionLength, tableIdExt, versionNo, sectionNo);
        break;
    }
    case SERVICE_DESCRIPTION_OTHER_TABLE_ID:
#endif
    case SERVICE_DESCRIPTION_TABLE_ID: {
        if (GetSIHandler()->IsTSInfoPresent() && !GetSIHandler()->IsScanning())
            ParseSDT(siData, sectionLength, tableIdExt, versionNo, sectionNo);
        break;
    }

    case EVENT_INFORMATION_PF_TABLE_ID:
    case EVENT_INFORMATION_TABLE_ID_START:
    case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: // EIT section for actual schedule.
    case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e:
    case EVENT_INFORMATION_TABLE_ID_END: {
        if (_isTimeParsed)
            ParseEIT(siData, sectionLength, tableIdExt, versionNo, sectionNo);
        break;
    }

    case TIME_AND_DATE_TABLE_ID: {
        // tdt section header is only till section length field
        if (!_isTimeParsed) {
            ParseTDT(siData - (LAST_SECTION_NUM_OFFSET - TABLE_ID_EXTENSION_OFFSET + 1), sectionLength);
            _isTimeParsed = true;
        }
        break;
    }

    case TIME_OFFSET_TABLE_ID: {
        // TOT section header is only till section length field.
        if (!_isTimeParsed) {
            ParseTOT(siData - (LAST_SECTION_NUM_OFFSET - TABLE_ID_EXTENSION_OFFSET + 1), sectionLength);
            _isTimeParsed = true;
        }
        break;
    }

    default:
        TRACE(Trace::Information, (_T("SI Table type unknown")));
        break;
    }
}    

void ParserDVB::SendBaseTableRequest()
{
    TRACE(Trace::Information, (_T("Requesting SDT")));
    GetSIHandler()->StartFilter(SDT_PID, 0/*Placeholder*/);
    TRACE(Trace::Information, (_T("Requesting NIT")));
    GetSIHandler()->StartFilter(NIT_PID, 0/*Placeholder*/);
    TRACE(Trace::Information, (_T("Requesting EIT")));
    GetSIHandler()->StartFilter(EIT_PID, 0/*Placeholder*/);
    GetSIHandler()->StartFilter(TDT_PID, 0/*Placeholder*/);
    _baseFiltersSet  =  true;
}

void ParserDVB::ParseTerrestrialDeliverySystemDescriptor(uint8_t* buf)
{
    buf += DESCR_HEADER_LEN;
    static const FeModulationT mTab[] = { Qpsk, Qam16, Qam64, QamAuto };
    uint32_t frequency;
    frequency = READ_32(buf[0], buf[1], buf[2], buf[3]);
    frequency *= 10;
    _nit.frequency = frequency;
    FeModulationT modulation;
    modulation = mTab[GETBITS(buf[5], 7, 6)];
    _nit.modulation = GETBITS(buf[5], 7, 6);
    TRACE(Trace::Information, (_T("frequency = %u"), frequency));
    TRACE(Trace::Information, (_T("modulation = 0x%0x"), modulation));
}

void ParserDVB::ParseCableDeliverySystemDescriptor(uint8_t* buf)
{
    uint32_t frequency = Bcd32ToInteger(buf[2], buf[3], buf[4], buf[5]);
    frequency *= 100;
    uint8_t modulation = buf[8];
    _nit.modulation = modulation;
    _nit.frequency = frequency;
}

void ParserDVB::ParseServiceDescriptor(uint8_t* buf)
{
    buf += DESCR_HEADER_LEN;
    uint8_t providerNameLen;
    uint8_t serviceNameLen;

    providerNameLen = buf[1];
    buf += (2 + providerNameLen);
    serviceNameLen = *buf;
    _channel.serviceName.clear();
    _channel.serviceName = ConvertToUnicode(buf + 1, serviceNameLen);
    TRACE(Trace::Information, (_T("service_name = %s"), _channel.serviceName.c_str()));
}

void ParserDVB::ParseShortEventDescriptor(uint8_t* buf)
{
    buf += DESCR_HEADER_LEN;
    uint8_t eventNameLen;
    buf += CHAR_CODE_LEN;
    eventNameLen = *buf;
    _event.eventName.clear();
    _event.eventName = ConvertToUnicode(buf + 1, eventNameLen);
    TRACE(Trace::Information, (_T("event_name = %s"), _event.eventName.c_str()));
}

void ParserDVB::ParseComponentDescriptor(uint8_t* buf)
{
    uint8_t streamContent;
    uint8_t componentType;
    buf += DESCR_HEADER_LEN;
    streamContent = GETBITS(buf[0], 3, 0);
    componentType = buf[1];
    TRACE(Trace::Information, (_T("stream_content = 0x%0x"), streamContent));
    TRACE(Trace::Information, (_T("component_type = 0x%0x"), componentType));
    switch (streamContent) {
    case VIDEO_STREAM_CONTENT:
        break;
    case AUDIO_STREAM_CONTENT: {
        bool isPresent = false;
        if (!_event.audioLanguages.empty()) {
            for (auto& audio : _event.audioLanguages) {
                if (!strncmp(audio.c_str(), (char*)buf + 3, CHAR_CODE_LEN)) {
                    isPresent = true;
                    break;
                }
            }
        }
        if (!isPresent) {
            std::string audio;
            audio = ConvertToUnicode(buf + 3, CHAR_CODE_LEN);
            TRACE(Trace::Information, (_T("audio = %s"), audio.c_str()));
            _event.audioLanguages.push_back(audio);
        }
        break;
    }
    case SUBTITLE_STREAM_CONTENT: {
        bool isPresent = false;
        if (!_event.subtitleLanguages.empty()) {
            for (auto& subtitle : _event.subtitleLanguages) {
                if (!strncmp(subtitle.c_str(), (char*)buf + 3, CHAR_CODE_LEN)) {
                    isPresent = true;
                    break;
                }
            }
        }
        if (!isPresent) {
            std::string subtitle;
            subtitle = ConvertToUnicode(buf + 3, CHAR_CODE_LEN);
            TRACE(Trace::Information, (_T("subtitle = %s"), subtitle.c_str()));
            _event.subtitleLanguages.push_back(subtitle);
        }
        break;
    }
    default:
        break;
    }
}

void ParserDVB::ParseContentDescriptor(uint8_t* buf)
{
    int16_t descriptorLength = (int16_t)buf[1];
    buf += DESCR_HEADER_LEN;
    uint8_t contentNibbleLevel1;
    uint8_t contentNibbleLevel2;
    char gen[16][50] = {"Undefined Content", "Movie/Drama", "News/Current Affairs",
        "Show/Game Show", "Sports", "Children's/Youth Programmes", "Music/Ballet/Dance",
        "Arts/Culture (without music)", "Social/Political Issues/Economics",
        "Education/Science/Factual Topics", "Leisure Hobbies",
        "Special Characteristics", "reserved for future use",
        "reserved for future use", "reserved for future use", "User Defined"};
    while (descriptorLength > 0) {
        contentNibbleLevel1 = GETBITS(buf[0], 7, 4);
        contentNibbleLevel2 = GETBITS(buf[0], 3, 0);
        size_t strLen = contentNibbleLevel1 > 16 ? strlen("Not available") : strlen(gen[contentNibbleLevel1]);

        _event.genre.assign(contentNibbleLevel1 > 16 ? "Not available" : gen[contentNibbleLevel1], strLen + 1);
        TRACE(Trace::Information, (_T("content_nibble_level_1 = 0x%0x"), contentNibbleLevel1));
        TRACE(Trace::Information, (_T("content_nibble_level_2 = 0x%0x"), contentNibbleLevel2));
        TRACE(Trace::Information, (_T("genre = %s"), _event.genre.c_str()));
        descriptorLength -= CONTENT_DESCR_LOOP_LEN;
        buf += CONTENT_DESCR_LOOP_LEN;
    }
}

void ParserDVB::ParseLocalTimeOffsetDescriptor(uint8_t* buf)
{
    int16_t descriptorLength = (int16_t)buf[1];
    buf += DESCR_HEADER_LEN;
    while (descriptorLength > 0) {
        std::string countryCode;
        countryCode = ConvertToUnicode(buf, CHAR_CODE_LEN);
        uint8_t countryRegionId = GETBITS(buf[3], 5, 0);
        if ((!strncmp(_countryCode.c_str(), countryCode.c_str(), CHAR_CODE_LEN)) && (_countryRegionId == countryRegionId)) {
            uint8_t localTimeOffsetPolarity = GETBITS(buf[3], 0, 0);

            DvbHHMM dvbLocalTimeOffset;
            memcpy(dvbLocalTimeOffset, buf + 4, 2);
            _localTimeOffset = DvbhhmmToSeconds(dvbLocalTimeOffset);
            DvbDate utcTimeOfChange;
            memcpy(utcTimeOfChange, buf + 6, 5);
            time_t timeOfChange;
            timeOfChange = DvbdateToUnixtime(utcTimeOfChange);

            DvbHHMM dvbNextTimeOffset;
            memcpy(dvbNextTimeOffset, buf + 11, 2);
            time_t nextTimeOffset = DvbhhmmToSeconds(dvbNextTimeOffset);
            TRACE(Trace::Information, (_T("countryCode = %s"), countryCode));
            TRACE(Trace::Information, (_T("countryRegionId = %d"), countryRegionId));
            TRACE(Trace::Information, (_T("localTimeOffsetPolarity = %d"), localTimeOffsetPolarity));
            TRACE(Trace::Information, (_T("localTimeOffset = %d"), _localTimeOffset));
            TRACE(Trace::Information, (_T("timeOfChange = %ld"), timeOfChange));
            TRACE(Trace::Information, (_T("nextTimeOffset = %d"), nextTimeOffset));
            break;
        }

        descriptorLength -= LOCAL_TIME_OFFSET_DESCR_LOOP_LEN;
        buf += LOCAL_TIME_OFFSET_DESCR_LOOP_LEN;
    }
}

void ParserDVB::ParseLogicalChannelDescriptor(uint8_t* buf)
{
    int16_t descriptorLength = (int16_t)buf[1];
    buf += DESCR_HEADER_LEN;
    uint16_t serviceId, lcn;
    while (descriptorLength > 0) {
        serviceId = READ_16(buf[0], buf[1]);
        lcn = READ_16(GETBITS(buf[2], 1, 0), buf[3]);
        TRACE(Trace::Information, (_T("serviceId = 0x%02x"), serviceId));
        TRACE(Trace::Information, (_T("LCN = %d"), lcn));

        _serviceIdLCNMap[serviceId] = lcn;

        descriptorLength -= LOGICAL_CHANNEL_DESCR_LOOP_LEN;
        buf += LOGICAL_CHANNEL_DESCR_LOOP_LEN;
    }
}
#ifdef ENABLE_BOUQUET_PARSING
void ParserDVB::ParseBouquetNameDescriptor(uint8_t* buf)
{
    uint8_t descriptorLength = buf[1];
    buf += DESCR_HEADER_LEN;
    std::string bouquetName = ConvertToUnicode(buf, descriptorLength);
}
#endif
void ParserDVB::ParseDescriptors(TableType t, uint8_t* buf, int32_t descriptorsLoopLen)
{
    while (descriptorsLoopLen > 0) {
        uint8_t descriptorTag = buf[0];
        uint8_t descriptorLen = buf[1] + 2;

        if (!descriptorLen)
            break;

        switch (descriptorTag) {

        case TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR:
            if ((t == NitTable) && (!_isCableDeliveryDescriptorPresent))
                ParseTerrestrialDeliverySystemDescriptor(buf);
            break;

        case CABLE_DELIVERY_SYSTEM_DESCRIPTOR:
            if (t == NitTable) {
                _isCableDeliveryDescriptorPresent = true;
                ParseCableDeliverySystemDescriptor(buf);
            }
            break;

        case LOGICAL_CHANNEL_DESCRIPTOR:
            if (t == NitTable)
                ParseLogicalChannelDescriptor(buf);
            break;

        case SERVICE_DESCRIPTOR:
            if (t == SdtTable)
                ParseServiceDescriptor(buf);
            break;

        case SHORT_EVENT_DESCRIPTOR:
            if (t == EitTable)
                ParseShortEventDescriptor(buf);
            break;

        case COMPONENT_DESCRIPTOR:
            if (t == EitTable)
                ParseComponentDescriptor(buf);
            break;

        case CONTENT_DESCRIPTOR:
            if (t == EitTable)
                ParseContentDescriptor(buf);
            break;

        case LOCAL_TIME_OFFSET_DESCRIPTOR:
            if (t == TotTable)
                ParseLocalTimeOffsetDescriptor(buf);
            break;
#ifdef ENABLE_BOUQUET_PARSING
        case BOUQUET_NAME_DESCRIPTOR:
            if (t == BatTable) {
                ParseBouquetNameDescriptor(buf);
            }
            break;
#endif
        default:
            break;
        }

        buf += descriptorLen;
        descriptorsLoopLen -= descriptorLen;
    }
}

void ParserDVB::ParsePAT(uint8_t* buf, uint16_t sectionLength)
{
    uint16_t progNum;
    uint16_t pid;
    while (sectionLength >= PAT_LOOP_LEN) {
        progNum = READ_16(buf[0], buf[1]);
        pid = READ_16(GETBITS(buf[2], 4, 0), buf[3]);
        if (progNum)
            _pmtPidMap[progNum] = pid;
        TRACE(Trace::Information, (_T("progNum = %d"), progNum));
        TRACE(Trace::Information, (_T("PMT pid = 0x%04X"), pid));
        sectionLength -= PAT_LOOP_LEN;
        buf += PAT_LOOP_LEN;
    }
}

void ParserDVB::ParsePMT(uint8_t* buf, uint16_t sectionLength, uint16_t programNum)
{
    uint16_t progInfoLen = READ_16(GETBITS(buf[2], 3, 0), buf[3]);
    buf += (PMT_LOOP_BASE_OFFSET - LAST_SECTION_NUM_OFFSET + progInfoLen);
    while (sectionLength >= PMT_LOOP_LEN) {
        uint8_t streamType = buf[0];
        uint16_t elementaryPID = READ_16(GETBITS(buf[1], 4, 0), buf[2]);
        uint16_t descriptorsLoopLen = READ_16(GETBITS(buf[3], 3, 0), buf[4]);

        if (sectionLength < descriptorsLoopLen) {
            TRACE(Trace::Error, (_T("section too short: service_id == 0x%02x, sectionLength == %i, "
                "descriptorsLoopLen == %i"),
                programNum, sectionLength, descriptorsLoopLen));
            break;
        }

        ParseDescriptors(PmtTable, buf + PMT_LOOP_LEN, descriptorsLoopLen);

        sectionLength -= descriptorsLoopLen + PMT_LOOP_LEN;
        buf += descriptorsLoopLen + PMT_LOOP_LEN;
    }
}

void ParserDVB::ParseNIT(uint8_t* buf, uint16_t sectionLength, uint16_t networkId, uint8_t versionNo, uint8_t sectionNo)
{
    uint16_t descriptorsLoopLen = READ_16(GETBITS(buf[0], 3, 0), buf[1]);

    if (sectionLength < descriptorsLoopLen) {
        TRACE(Trace::Error, (_T("section too short: networkId == 0x%04x, sectionLength == %i, "
            "descriptorsLoopLen == %i"),
            networkId, sectionLength, descriptorsLoopLen));
        return;
    }

    _nit.networkId = networkId;
    auto nitHeader = std::make_shared<SectionTableHeader>(versionNo, networkId, sectionNo);
    uint16_t key = networkId;
    auto it = _nitHeaderMap.find(key);
    if (it != _nitHeaderMap.end()) {
        if (nitHeader->Version() == it->second->Version()) {
            if (nitHeader->SectionNumber() == it->second->SectionNumber())
                return; // Skip packet.
        }
        it->second = nitHeader;
    } else
        _nitHeaderMap.insert(std::make_pair(key, nitHeader));

    sectionLength -= (descriptorsLoopLen + NIT_LOOP_BASE_OFFSET - LAST_SECTION_NUM_OFFSET);
    buf += (descriptorsLoopLen + NIT_LOOP_BASE_OFFSET - LAST_SECTION_NUM_OFFSET);

    while (sectionLength >= NIT_LOOP_LEN) {
        uint16_t transportStreamId = READ_16(buf[0], buf[1]);
        _nit.transportStreamId = transportStreamId;
        descriptorsLoopLen = READ_16(GETBITS(buf[4], 3, 0), buf[5]);

        if (sectionLength < descriptorsLoopLen)
            break;
        uint16_t originalNetworkId = READ_16(buf[2], buf[3]);
        _nit.originalNetworkId = originalNetworkId;
        TRACE(Trace::Information, (_T("networkId = 0x%0x"), networkId));
        TRACE(Trace::Information, (_T("originalNetworkId = 0x%0x"), originalNetworkId));
        TRACE(Trace::Information, (_T("transportStreamId = 0x%0x"), transportStreamId));
        std::pair<uint16_t, uint16_t> key(originalNetworkId, transportStreamId);
#ifdef ENABLE_BOUQUET_PARSING
        auto iterator = _batInfo.find(key);
        if (iterator == _batInfo.end()) {
            sectionLength -= descriptorsLoopLen + NIT_LOOP_LEN;
            buf += descriptorsLoopLen + NIT_LOOP_LEN;
            continue;
        }
#else
        _nitInfo.insert(key);
#endif
        ParseDescriptors(NitTable, buf + NIT_LOOP_LEN, descriptorsLoopLen);
        _epgDB.InsertNitInfo(_nit.networkId, _nit.transportStreamId, _nit.originalNetworkId, _nit.frequency, _nit.modulation);
        sectionLength -= descriptorsLoopLen + NIT_LOOP_LEN;
        buf += descriptorsLoopLen + NIT_LOOP_LEN;
    }
}

#ifdef ENABLE_BOUQUET_PARSING
void ParserDVB::ParseBAT(uint8_t* buf, uint16_t sectionLength, uint16_t bouquetId, uint8_t versionNo, uint8_t sectionNo)
{
    uint16_t descriptorsLoopLen = READ_16(GETBITS(buf[0], 3, 0), buf[1]);

    if (sectionLength < descriptorsLoopLen) {
        TRACE(Trace::Error, (_T("section too short: bouquetId == 0x%04x, sectionLength == %i, "
            "descriptorsLoopLen == %i"),
            bouquetId, sectionLength, descriptorsLoopLen));
        return;
    }

    auto batHeader = std::make_shared<SectionTableHeader>(versionNo, bouquetId, sectionNo);
    uint16_t key = bouquetId;
    auto it = _batHeaderMap.find(key);
    if (it != _batHeaderMap.end()) {
        if (batHeader->Version() == it->second->Version()) {
            if (batHeader->SectionNumber() == it->second->SectionNumber())
                return; // Skip packet.
        }
        it->second = batHeader;
    } else
        _batHeaderMap.insert(std::make_pair(key, batHeader));

    ParseDescriptors(BatTable, buf + 2, descriptorsLoopLen);
    buf += (descriptorsLoopLen + BAT_LOOP_BASE_OFFSET - LAST_SECTION_NUM_OFFSET);
    sectionLength -= (descriptorsLoopLen + BAT_LOOP_BASE_OFFSET - LAST_SECTION_NUM_OFFSET);

    while (sectionLength >= BAT_LOOP_LEN) {
        uint16_t transportStreamId = READ_16(buf[0], buf[1]);
        _nit.transportStreamId = transportStreamId;
        descriptorsLoopLen = READ_16(GETBITS(buf[4], 3, 0), buf[5]);

        if (sectionLength < descriptorsLoopLen)
            break;
        uint16_t originalNetworkId = READ_16(buf[2], buf[3]);
        TRACE(Trace::Information, (_T("bouquetId = 0x%0x"), bouquetId));
        TRACE(Trace::Information, (_T("originalNetworkId = 0x%0x"), originalNetworkId));
        TRACE(Trace::Information, (_T("transportStreamId = 0x%0x"), transportStreamId));

        std::pair<uint16_t, uint16_t> key(originalNetworkId, transportStreamId);
        _batInfo.insert(key);

        ParseDescriptors(BatTable, buf + BAT_LOOP_LEN, descriptorsLoopLen);
        sectionLength -= descriptorsLoopLen + BAT_LOOP_LEN;
        buf += descriptorsLoopLen + BAT_LOOP_LEN;
    }
}
#endif

void ParserDVB::ParseSDT(uint8_t* buf, uint16_t sectionLength, uint16_t transportStreamId, uint8_t versionNo, uint8_t sectionNo)
{
    _channel.transportStreamId = transportStreamId;
    uint16_t originalNetworkId = READ_16(buf[0], buf[1]);
    buf += (SDT_LOOP_BASE_OFFSET - LAST_SECTION_NUM_OFFSET); // Skip original network id + reserved field.
#ifdef ENABLE_BOUQUET_PARSING
    auto tableInfo = _batInfo;
#else
    auto tableInfo = _nitInfo;
#endif

    std::pair<uint16_t, uint16_t> key(originalNetworkId, transportStreamId);
    auto iterator = tableInfo.find(key);
    if (iterator == tableInfo.end())
        return;

    auto sdtHeader = std::make_shared<SectionTableHeader>(versionNo, transportStreamId, sectionNo);
    auto it = _sdtHeaderMap.find(key);
    if (it != _sdtHeaderMap.end()) {
        if (sdtHeader->Version() == it->second->Version()) {
            if (sdtHeader->SectionNumber() == it->second->SectionNumber())
                return; // Skip.
        }
        it->second = sdtHeader;
    } else
        _sdtHeaderMap.insert(std::make_pair(key, sdtHeader));
    while (sectionLength >= SDT_LOOP_LEN) {
        uint16_t serviceId = READ_16(buf[0], buf[1]);
        _channel.serviceId = serviceId;
        TRACE(Trace::Information, (_T("transportStreamId = 0x%0x"), transportStreamId));
        TRACE(Trace::Information, (_T("service_id = 0x%x"), serviceId));
        uint16_t descriptorsLoopLen = READ_16(GETBITS(buf[3], 3, 0), buf[4]);

        if (sectionLength < descriptorsLoopLen) {
            TRACE(Trace::Error, (_T("section too short: service_id == 0x%02x, sectionLength == %i, "
                "descriptorsLoopLen == %i"),
            serviceId, sectionLength,
            descriptorsLoopLen));
            break;
        }

        RunningMode running;
        running = static_cast<RunningMode>((buf[3] >> 5) & 0x7);
        TRACE(Trace::Information, (_T("running_status = %s")
            , running == NotRunning ? "not running" : running == StartsSoon ?
            "starts soon" : running == Pausing ? "pausing" : running == Running ? "running" : "???"));

        std::tuple<uint16_t, uint16_t, uint16_t> key(originalNetworkId, transportStreamId, serviceId);
        _sdtInfo.insert(key);

        ParseDescriptors(SdtTable, buf + SDT_LOOP_LEN, descriptorsLoopLen);

        uint32_t frequency;
        uint8_t modulation;
        _epgDB.GetFrequencyAndModulationFromNit(originalNetworkId, transportStreamId, frequency, modulation);
        if (_epgDB.IsServicePresentInTSInfo(_channel.serviceId)) {
            if (_serviceIdLCNMap.find(_channel.serviceId) == _serviceIdLCNMap.end()) {
                uint16_t lcn;
                if (!_epgDB.IsServicePresentInChannelTable(_channel.serviceId, _channel.transportStreamId, lcn))
                    lcn = _serviceIdLCNMap.size() + 1;
                _serviceIdLCNMap[_channel.serviceId] = lcn;
            }
            _channel.channelNum = _serviceIdLCNMap[_channel.serviceId];
            _epgDB.InsertChannelInfo(frequency, modulation, _channel.serviceName.c_str(), _channel.serviceId, _channel.transportStreamId
            , originalNetworkId, std::to_string(_channel.channelNum), _channel.serviceId, "");
        }
        sectionLength -= descriptorsLoopLen + SDT_LOOP_LEN;
        buf += descriptorsLoopLen + SDT_LOOP_LEN;
    }
}

void ParserDVB::ParseEIT(uint8_t* buf, uint16_t sectionLength, uint16_t serviceId, uint8_t versionNo, uint8_t sectionNo)
{
    uint16_t transportStreamId = READ_16(buf[0], buf[1]);
    uint16_t originalNetworkId = READ_16(buf[2], buf[3]);
    TRACE(Trace::Information, (_T("service_id = 0x%0x"), serviceId));
    TRACE(Trace::Information, (_T("transportStreamId = 0x%0x"), transportStreamId));
    TRACE(Trace::Information, (_T("originalNetworkId = 0x%0x"), originalNetworkId));
    buf += (EIT_LOOP_BASE_OFFSET - LAST_SECTION_NUM_OFFSET);
    _event.serviceId = serviceId;

    std::tuple<uint16_t, uint16_t, uint16_t> key(originalNetworkId, transportStreamId, serviceId);
    auto iterator = _sdtInfo.find(key);
    if (iterator == _sdtInfo.end())
        return;

    auto eitHeader = std::make_shared<SectionTableHeader>(versionNo, serviceId, sectionNo);
    auto it = _eitHeaderMap.find(key);
    if (it != _eitHeaderMap.end()) {
        if (eitHeader->Version() == it->second->Version()) {
            if (eitHeader->SectionNumber() == it->second->SectionNumber())
                return; // Skip.
        }
        it->second = eitHeader;
    } else
        _eitHeaderMap.insert(std::make_pair(key, eitHeader));

    while (sectionLength >= EIT_LOOP_LEN) {
        uint16_t eventId = READ_16(buf[0], buf[1]);
        TRACE(Trace::Information, (_T("event_id  = 0x%0x"), eventId));
        _event.eventId = eventId;
        uint16_t descriptorsLoopLen = READ_16(GETBITS(buf[10], 3, 0), buf[11]);
        if (sectionLength < descriptorsLoopLen) {
            TRACE(Trace::Error, (_T("section too short")));
            break;
        }
        DvbDate dvbStartTime;
        memcpy(dvbStartTime, buf + 2, 5);
        time_t startTime;
        startTime = DvbdateToUnixtime(dvbStartTime);
        TRACE(Trace::Information, (_T("start time = %ld(%d:%d:%d)"), startTime, dvbStartTime[2], dvbStartTime[3], dvbStartTime[4]));
        _event.startTime = startTime + _localTimeOffset;
        DvbDuration dvbDuration;
        memcpy(dvbDuration, buf + 7, 3);
        time_t duration;
        duration = DvbdurationToSeconds(dvbDuration);
        TRACE(Trace::Information, (_T("duration = %ld"), duration));
        _event.duration = duration;
        RunningMode running;
        running = static_cast<RunningMode>((buf[10] >> 5) & 0x7);
        TRACE(Trace::Information, (_T("running status = %s"), running == NotRunning ? "not running" : running == StartsSoon ? "starts soon" : running == Pausing ? "pausing" : running == Running ? "running" : "???"));

        _event.genre.clear();
        ClearLanguages();
        ParseDescriptors(EitTable, buf + EIT_LOOP_LEN, descriptorsLoopLen);
        std::string audioLanguages, subtitleLanguages;
        GetLanguages(audioLanguages, subtitleLanguages);
        _epgDB.InsertProgramInfo(_event.serviceId, _event.eventId, _event.startTime, _event.duration, _event.eventName.c_str()
            , "", subtitleLanguages, _event.genre, audioLanguages);
        sectionLength -= descriptorsLoopLen + EIT_LOOP_LEN;
        buf += descriptorsLoopLen + EIT_LOOP_LEN;
    }
}

void ParserDVB::GetLanguages(std::string& audioLanguages, std::string& subtitleLanguages)
{
    uint16_t count = 0;
    for (auto& audio : _event.audioLanguages) {
        if (count)
            audioLanguages += ',';
        audioLanguages += audio;
        count++;
    }
    count = 0;
    for (auto& subtitle : _event.subtitleLanguages) {
        if (count)
            subtitleLanguages += ',';
        subtitleLanguages += subtitle;
        count++;
    }
}

bool ParserDVB::SetTime(uint8_t* buf)
{
    DvbDate dvbUtcTime;
    memcpy(dvbUtcTime, buf, 5);
    time_t utcTime = DvbdateToUnixtime(dvbUtcTime);
    TRACE(Trace::Information, (_T("utcTime = %ld"), utcTime));
    struct timeval timeVal = {};
    timeVal.tv_sec = utcTime;
    int32_t timeSetStatus = settimeofday(&timeVal, NULL);
    if (timeSetStatus < 0) {
        TRACE(Trace::Error, (_T("Error in settimeofday, errMsg = %s"), strerror(errno)));
        return false;
    }

    GetSIHandler()->StartTimer();
    return true;
}

void ParserDVB::ParseTDT(uint8_t* buf, uint16_t sectionLength)
{
    SetTime(buf);
}

void ParserDVB::ParseTOT(uint8_t* buf, uint16_t sectionLength)
{
    if (SetTime(buf)) {
        uint16_t descriptorsLoopLen = READ_16(GETBITS(buf[1], 3, 0), buf[2]);
        ParseDescriptors(TotTable, buf + 7, descriptorsLoopLen);
    }
}

bool ParserDVB::ResetTables()
{
    _isTimeParsed = false;
    return true;
}

void ParserDVB::ConfigureParser()
{
    uint32_t homeTS = _homeTS;
    if (!homeTS) {
        if (_epgDB.ReadFrequency(_frequencyList))
            homeTS = _frequencyList[0];
    }

    if (homeTS) {
        GetSIHandler()->SetHomeTS(homeTS, 0);
        SendBaseTableRequest();
        _parserRunning = true;
    }
}

void ParserDVB::UpdateFrequencyList()
{
    if (!_homeTS) {
        _freqHandler.SetFrequencyList(_frequencyList);
        _freqHandler.Run();
    }
}

void ParserDVB::SetCountryDetails(const std::string& countryCode, uint8_t countryRegionId)
{
    if (countryCode.size())
        _countryCode = countryCode;
    _countryRegionId = (uint8_t)countryRegionId;
}

uint32_t ParserDVB::Worker()
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
        TRACE(Trace::Information, (_T("Worker data obtained")));
        if (std::get<0>(dataElement)) {
            TRACE(Trace::Information, (_T("DBS DVB")));
            uint32_t frequency = 0;
            memcpy(&frequency, std::get<0>(dataElement), sizeof(uint32_t));
            TRACE(Trace::Information, (_T("Frequency = %u\n"), frequency));
            ParseData(std::get<0>(dataElement) + DATA_OFFSET, frequency);
            free(std::get<0>(dataElement));
        }
    }
    TRACE(Trace::Information, (_T("Worker thread to Block state")));
    return (WPEFramework::Core::infinite);
}

FrequencyHandler::FrequencyHandler(IParser* parser)
    : _parser(parser)
{
}

FrequencyHandler::~FrequencyHandler()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    Block();
    Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
}

uint32_t FrequencyHandler::Worker()
{
    uint32_t previousFrequency = 0;
    if (_frequencyList.size())
        previousFrequency = _frequencyList[0];

    ParserDVB* parser = static_cast<ParserDVB*>(_parser);
    parser->_epgDB.ReadFrequency(_frequencyList);
    if (_frequencyList.size() && (previousFrequency != _frequencyList[0])) {
        if (parser->_parserRunning)
            parser->GetSIHandler()->StopFilters();
        parser->GetSIHandler()->SetHomeTS(_frequencyList[0], 0);
        parser->SendBaseTableRequest();
        parser->_parserRunning = true;
    }

    return (WPEFramework::Core::infinite);
}
