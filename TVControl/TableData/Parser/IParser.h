
#ifndef __TVIPARSER_H
#define __TVIPARSER_H
#include <DataQueue.h>
#include <core/core.h>
#include <errno.h>
#include <inttypes.h>

#define THREAD_EXIT_LIMIT 100
#define BUFFER_SIZE 4096
#define DATA_OFFSET 6
#define SIZE_OFFSET 4

class ISIHandler {
public:
    ISIHandler() = default;
    virtual ~ISIHandler() = default;
    virtual void StartFilter(uint16_t, uint16_t) = 0;
    virtual void StopFilter(uint16_t, uint16_t) = 0;
    virtual void SetHomeTS(const uint32_t primaryFreq, const uint32_t secondaryFreq) = 0;
    virtual void StopFilters() = 0;
    virtual void EitBroadcasted() = 0;
    virtual bool IsScanning() = 0;
    virtual bool IsTSInfoPresent() = 0;
    virtual void StartTimer() = 0;
};

class IParser : virtual public WPEFramework::Core::Thread {
public:
    static IParser* GetInstance(ISIHandler*, uint32_t);
    virtual void ReleaseFilters() = 0;
    virtual bool ResetTables() = 0;
    virtual void UpdateFrequencyList() = 0;
    virtual void UpdateCurrentFrequency(uint32_t) = 0;
    virtual void SetCountryDetails(const std::string&, uint8_t) = 0;
};

class FrequencyHandler : virtual public WPEFramework::Core::Thread {
public:
    FrequencyHandler() = delete;
    FrequencyHandler(IParser*);
    ~FrequencyHandler();
    uint32_t Worker();
    void SetFrequencyList(std::vector<uint32_t>& list) { _frequencyList = list; }
private:
    uint32_t _currentFreqIndex;
    std::vector<uint32_t> _frequencyList;
    IParser* _parser;
};

class SectionTableHeader {
private:
    uint8_t _version;
    uint8_t _sectionNo;
    uint16_t _extensionId;

public:
    SectionTableHeader(uint8_t version, uint16_t extensionId, uint8_t sectionNo)
        : _version(version)
        , _extensionId(extensionId)
        , _sectionNo(sectionNo)
    {
    }
    uint8_t Version() { return _version; }
    uint16_t ExtensionId() { return _extensionId; }
    uint8_t SectionNumber() { return _sectionNo; }
};
#endif
