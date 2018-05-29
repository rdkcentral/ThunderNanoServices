#ifndef __TVATSCPARSER_H
#define __TVATSCPARSER_H
#include <EPGData.h>
#include <IParser.h>
#include <algorithm>
#include <libucsi/atsc/section.h>
#include <libucsi/atsc/types.h>
#include <map>
#include <tracing/tracing.h>
#include <unordered_set>
#include <vector>

#define AUDIO_CHAN_MAX (32)
#define MAX_NUM_EVENT_TABLES 128
#define MAX_NUM_CHANNELS 16
#define MAX_NUM_EVENTS_PER_CHANNEL (4 * 24 * 7)
#define PID_ONLY 0x02
#define ATSC_BASE_PID 0x1ffb
#define MAX_TITLE_SIZE 7

struct ATSCServiceLocationDescriptor {
    uint8_t _descriptorTag : 8;
    uint8_t _descriptorLength : 8;
    uint8_t _reserved : 3;
    uint16_t _pcrPid : 13;
    uint8_t _numberElements : 8;
};

struct ATSCServiceLocationElement {
    uint8_t _streamType : 8;
    uint8_t _reserved : 3;
    uint16_t _elementaryPid : 13;
    uint32_t _ISO639LanguageCode : 24;
};

enum AtscDescriptorTag {
    DTagAtscStuffing = 0X80,
    DTagAtscAC3Audio = 0X81,
    DTagAtscCaptionService = 0X86,
    DTagAtscContentAdvisory = 0X87,
    DTagAtscExtendedChannelName = 0Xa0,
    DTagAtscServiceLocation = 0Xa1,
    DTagAtscTimeShiftedService = 0Xa2,
    DTagAtscComponentName = 0xa3,
    DTagAtscDCCDepartingRequest = 0xa8,
    DTagAtscDCCArrivingRequest = 0xa9,
    DTagAtscRedistributionControl = 0xaa,
    DTagAtscPrivateInformation = 0xad,
    DTagAtscContentIdentifier = 0xb6,
    DTagAtscGenre = 0xab,
};

struct PmtPidInfo {
    uint16_t _pcrPid;
    uint16_t _videoPid;
    uint16_t _audioPid[AUDIO_CHAN_MAX];
    char _audioLang[AUDIO_CHAN_MAX][4];
    uint16_t _audioNum;
};

struct mgtTableName {
    uint16_t _range;
    const char* _string;
};

class ParserATSC : public IParser {
public:
    ParserATSC() = delete;
    ParserATSC(ISIHandler*, uint32_t);
    ~ParserATSC();
    void SendBaseTableRequest();
    void UpdateFrequencyList();
    void ReleaseFilters();
    bool ResetTables();
    uint32_t Worker();
    bool IsParserRunning() { return _parserRunning; }
    bool IsStreaming() { return _isStreaming; }
    ISIHandler* GetSIHandler();
    void UpdateCurrentFrequency(uint32_t);
    void SetCountryDetails(const std::string&, uint8_t) {}
    void ClearEITPids();

private:
    void ConfigureParser();
    void ParseData(struct section_ext*, uint32_t);
    void ParseAtscExtendedChannelNameDescriptor(char**, const unsigned char*);
    void ParseAtscServiceLocationDescriptor(struct PmtPidInfo*, const unsigned char*);
    struct ATSCServiceLocationDescriptor ReadATSCServiceLocationDescriptor(const uint8_t*);
    struct ATSCServiceLocationElement ReadATSCServiceLocationElement(const uint8_t*);
    char* ParseAtscCaptionServiceDescriptor(struct descriptor*);
    std::string ParseAtscContentAdvisoryDescriptor(struct descriptor*);
    bool ParseMGT(struct atsc_section_psip*);
    bool ParseEIT(struct atsc_section_psip*);
    bool ParseTVCT(struct atsc_section_psip*, uint32_t);
    bool ParseEvents(struct atsc_eit_section*, uint8_t);
    bool ParseSTT(struct atsc_section_psip* psip);
    bool ParseCVCT(atsc_section_psip*, uint32_t);

    std::string GetUnicode(uint8_t*, size_t);
    std::string ATSCTextDecode(atsc_text*, uint8_t);
    bool GetSectionExt(uint8_t*, uint32_t, struct section_ext**);
    uint32_t GetBits(const uint8_t*, uint32_t, uint32_t);
    bool IsEITParsingCompleted();
    bool PushRequest(uint8_t, uint8_t, uint16_t, uint8_t, uint32_t);
    void PushEitStartRequest();
    void PushEitStopRequest();

    EPGDataBase& _epgDB;
    std::vector<uint16_t> _eitPidVector;
    unsigned _eitPidIndex;
    std::map<uint16_t, std::unordered_set<uint8_t> > _programMap;
    std::unordered_set<uint16_t> _channelSet;
    bool _isVCTParsed;
    bool _isTimeParsed;
    unsigned _eitPidCountCurrent;
    bool _setFilters;
    ISIHandler* _siHandler;
    bool _clientInitialised;
    bool _parserRunning;
    std::vector<uint32_t> _frequencyList;
    FrequencyHandler _freqHandler;
    uint32_t _currentParsingFrequency;
    bool _isStreaming;
    uint32_t _homeTS;
    std::shared_ptr<SectionTableHeader> _mgtHeader;
    std::shared_ptr<SectionTableHeader> _sttHeader;
    std::map<uint16_t, std::shared_ptr<SectionTableHeader>> _vctHeaderMap;
};
#endif
