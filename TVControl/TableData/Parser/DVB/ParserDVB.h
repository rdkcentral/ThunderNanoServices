#ifndef __TVDVBPARSER_H
#define __TVDVBPARSER_H

#include <EPGData.h>
#include <IParser.h>
#include <cinttypes>
#include <inttypes.h>
#include <map>
#include <set>
#include <time.h>

#define PID_ONLY 0x02

#define PAT_PID 0x0000
#define NIT_PID 0x0010
#define SDT_PID 0x0011
#define EIT_PID 0x0012
#define TDT_PID 0x0014
#define TOT_PID 0x0014

#define PROGRAM_ASSOCIATION_TABLE_ID 0x00
#define PROGRAM_MAP_TABLE_ID 0x02
#define NETWORK_INFORMATION_TABLE_ID 0x40
#define NETWORK_INFORMATION_OTHER_TABLE_ID 0x41
#define SERVICE_DESCRIPTION_OTHER_TABLE_ID 0x46
#define SERVICE_DESCRIPTION_TABLE_ID 0x42
#define EVENT_INFORMATION_TABLE_ID_START 0x50
#define EVENT_INFORMATION_TABLE_ID_END 0x5F
#define BOUQUET_ASSOCIATION_TABLE_ID 0x4A
#define EVENT_INFORMATION_PF_TABLE_ID 0x4E
#define TIME_AND_DATE_TABLE_ID 0x70
#define TIME_OFFSET_TABLE_ID 0x73

#define SERVICE_DESCRIPTOR 0x48
#define SHORT_EVENT_DESCRIPTOR 0x4D
#define COMPONENT_DESCRIPTOR 0x50
#define CONTENT_DESCRIPTOR 0x54
#define LOCAL_TIME_OFFSET_DESCRIPTOR 0x58
#define TERRESTRIAL_DELIVERY_SYSTEM_DESCRIPTOR 0x5A
#define SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR 0x43
#define CABLE_DELIVERY_SYSTEM_DESCRIPTOR 0x44
#define BOUQUET_NAME_DESCRIPTOR 0x47
#define LINKAGE_DESCRIPTOR 0x4A
#define DATA_SPECIFIER_DESCRIPTOR 0x5F
#define FREQUENCY_LIST_DESCRIPTOR 0x62
#define LOGICAL_CHANNEL_DESCRIPTOR 0x83

#define VIDEO_STREAM_CONTENT 0x01
#define AUDIO_STREAM_CONTENT 0x02
#define SUBTITLE_STREAM_CONTENT 0x03

#define TABLE_HEADER_LEN 8 // Table header lenth ie. till the lastSectionNumber field.
#define DESCR_HEADER_LEN 2 // Decriptor tag + descriptor length.
#define CRC_LEN 4 // Last 4 bytes.
#define CHAR_CODE_LEN 3 // Country code and language code.

#define TABLE_ID_EXTENSION_OFFSET 3 // Offset of table id extension field.
#define LAST_SECTION_NUM_OFFSET 7 // Last field of header.
#define PMT_LOOP_BASE_OFFSET 11
#define NIT_LOOP_BASE_OFFSET 11
#define BAT_LOOP_BASE_OFFSET 11
#define SDT_LOOP_BASE_OFFSET 10
#define EIT_LOOP_BASE_OFFSET 13

#define PAT_LOOP_LEN 4
#define PMT_LOOP_LEN 5
#define NIT_LOOP_LEN 6
#define BAT_LOOP_LEN 6
#define SDT_LOOP_LEN 5
#define EIT_LOOP_LEN 12
#define ISO639_LANGUAGE_DESCR_LOOP_LEN 4
#define SERVICE_LIST_DESCR_LOOP_LEN 3
#define CONTENT_DESCR_LOOP_LEN 2
#define LOCAL_TIME_OFFSET_DESCR_LOOP_LEN 13
#define SUBTITLING_DESCR_LOOP_LEN 8
#define LOGICAL_CHANNEL_DESCR_LOOP_LEN 4

#define ISO8859_5    0x01
#define ISO8859_6    0x02
#define ISO8859_7    0x03
#define ISO8859_8    0x04
#define ISO8859_9    0x05
#define ISO8859_10   0x06
#define ISO8859_11   0x07
#define ISO8859_12   0x08
#define ISO8859_13   0x09
#define ISO8859_14   0x0A
#define ISO8859_15   0x0B
#define ISO8859      0x10
#define ISO10646_1   0x11

uint16_t aunUnicode6937[256] =
{
    /*0x00*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /*0x08*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /*0x10*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /*0x18*/ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    /*0x20*/ 0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    /*0x28*/ 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
    /*0x30*/ 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    /*0x38*/ 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
    /*0x40*/ 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    /*0x48*/ 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
    /*0x50*/ 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    /*0x58*/ 0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
    /*0x60*/ 0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    /*0x68*/ 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
    /*0x70*/ 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    /*0x78*/ 0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x0000
};

typedef uint8_t DvbDate[5];
typedef uint8_t DvbDuration[3];
typedef uint8_t DvbHHMM[2];

enum TableType {
    PatTable,
    PmtTable,
    SdtTable,
    NitTable,
    EitTable,
    TotTable,
    BatTable
};

enum RunningMode {
    NotRunning = 0x01,
    StartsSoon = 0x02,
    Pausing = 0x03,
    Running = 0x04
};

enum FeModulationT {
    Qpsk,
    Qam16,
    Qam32,
    Qam64,
    Qam128,
    Qam256,
    QamAuto,
    Vsb8,
    Vsb16
};

struct Nit {
    uint16_t networkId;
    uint16_t transportStreamId;
    uint16_t originalNetworkId;
    uint32_t frequency;
    uint8_t modulation;
};

struct Sdt {
    uint16_t channelNum;
    uint32_t frequency;
    uint16_t serviceId;
    uint16_t transportStreamId;
    std::string serviceName;
};

struct Eit {
    uint16_t serviceId;
    uint16_t eventId;
    time_t startTime;
    time_t duration;
    std::string eventName;
    std::vector<std::string> subtitleLanguages;
    std::vector<std::string> audioLanguages;
    std::string genre;
};

class ParserDVB : public IParser {
public:
    ParserDVB() = delete;
    ~ParserDVB();
    ParserDVB(ISIHandler*, uint32_t);
    void SendBaseTableRequest();
    void ReleaseFilters();
    bool SetHomeTS();
    bool ResetTables();
    void UpdateFrequencyList();
    void UpdateCurrentFrequency(uint32_t) {}
    void SetCountryDetails(const std::string&, uint8_t);
    uint32_t Worker();
    ISIHandler* GetSIHandler();
    bool _parserRunning;
    EPGDataBase& _epgDB;

private:
    void ConfigureParser();
    void ClearLanguages();
    void ParseData(uint8_t*, uint32_t);
    void ParseStreamIdentifierDescriptor(uint8_t*);
    void ParseSubtitlingDescriptor(uint8_t*);
    void ParseISO639LanguageDescriptor(uint8_t*);
    void ParseNetworkNameDescriptor(uint8_t*);
    void ParseServiceListDescriptor(uint8_t*);
    void ParseTerrestrialDeliverySystemDescriptor(uint8_t*);
    void ParseCableDeliverySystemDescriptor(uint8_t*);
    void ParseServiceDescriptor(uint8_t*);
    void ParseShortEventDescriptor(uint8_t*);
    void ParseComponentDescriptor(uint8_t*);
    void ParseContentDescriptor(uint8_t*);
    void ParseLocalTimeOffsetDescriptor(uint8_t*);
    void ParseLogicalChannelDescriptor(uint8_t*);
#ifdef ENABLE_BOUQUET_PARSING
    void ParseBouquetNameDescriptor(uint8_t*);
    void ParseBAT(uint8_t*, uint16_t, uint16_t, uint8_t, uint8_t);
#endif
    void ParseDescriptors(TableType, uint8_t*, int32_t);
    void ParsePAT(uint8_t*, uint16_t);
    void ParsePMT(uint8_t*, uint16_t, uint16_t);
    void ParseNIT(uint8_t*, uint16_t, uint16_t, uint8_t, uint8_t);
    void ParseSDT(uint8_t*, uint16_t, uint16_t, uint8_t, uint8_t);
    void ParseEIT(uint8_t*, uint16_t, uint16_t, uint8_t, uint8_t);
    void ParseTDT(uint8_t*, uint16_t);
    void ParseTOT(uint8_t*, uint16_t);
    void GetLanguages(std::string&, std::string&);
    bool SetTime(uint8_t* buf);
    std::string ConvertToUnicode(uint8_t*, uint8_t);
    std::string GetUnicode(uint8_t*, uint8_t);

    Nit _nit;
    Sdt _channel;
    Eit _event;
    std::map<uint16_t, uint16_t> _pmtPidMap;
    uint8_t _countryRegionId;
    time_t _localTimeOffset;
    std::string _countryCode;
    bool _isTimeParsed;
    bool _baseFiltersSet;
    ISIHandler* _siHandler;
    bool _clientInitialised;
    uint32_t _homeTS;
    std::vector<uint32_t> _frequencyList;
    FrequencyHandler _freqHandler;
    uint32_t _currentParsingFrequency;
    std::map<uint16_t, std::shared_ptr<SectionTableHeader>> _nitHeaderMap;
    std::map<std::pair<uint16_t, uint16_t>, std::shared_ptr<SectionTableHeader>> _sdtHeaderMap;
    std::map<std::tuple<uint16_t, uint16_t, uint16_t>, std::shared_ptr<SectionTableHeader>> _eitHeaderMap;
#ifdef ENABLE_BOUQUET_PARSING
    std::map<uint16_t, std::shared_ptr<SectionTableHeader>> _batHeaderMap;
    std::set<std::pair<uint16_t, uint16_t>> _batInfo;
#else
    std::set<std::pair<uint16_t, uint16_t>> _nitInfo;
#endif
    std::set<std::tuple<uint16_t, uint16_t, uint16_t>> _sdtInfo;
    std::map<uint16_t, uint16_t> _serviceIdLCNMap;
    bool _isCableDeliveryDescriptorPresent;
};
#endif
