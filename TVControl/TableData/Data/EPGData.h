#ifndef __EPGDATABASE_H
#define __EPGDATABASE_H

#include <JSData.h>
#include <fcntl.h>
#include <interfaces/ITVPlatform.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <tracing/tracing.h>
#include <unistd.h>
#include <vector>

class EPGDataBase {
private:
    sqlite3* _dataBase;
    char* _errMsg;
    int32_t _fd;
    sqlite3_stmt* _stmt;

public:
    EPGDataBase();
    ~EPGDataBase();

private:
    bool DBLock();
    bool DBUnlock();
    bool OpenDB();
    bool CloseDB();
    bool ExecuteSQLQuery(char const*);
public:
    static EPGDataBase& GetInstance();
    bool IsTableEmpty(const std::string&);
    bool CreateFrequencyTable();
    bool CreateNitTable();
    bool CreateChannelTable();
    bool CreateProgramTable();
    bool CreateTSTable();
    bool InsertFrequencyInfo(std::vector<uint32_t>);
    bool InsertChannelInfo(uint32_t, uint32_t, const char*, uint16_t, uint16_t,
         uint16_t, const std::string&, uint16_t, const std::string&);
    bool InsertProgramInfo(uint16_t, uint16_t, time_t, time_t, const char*,
        const std::string&, const std::string&, const std::string&, const std::string&);
    bool InsertNitInfo(uint16_t, uint16_t, uint16_t, uint32_t, uint8_t);
    bool GetServiceIdFromChannelInfo(const string&, uint16_t&);
    bool GetFrequencyFromChannelInfo(const string&, uint32_t&);
    bool ReadFrequency(std::vector<uint32_t>&);
    bool ReadChannel(const string&, WPEFramework::Channel&);
    bool GetFrequencyListFromNit(std::vector<uint32_t>&);
    bool GetFrequencyAndModulationFromNit(uint16_t, uint16_t, uint32_t&, uint8_t&);
    void ClearSIData();
    int LoadOrSaveDB(bool);
    bool ReadChannels(WPEFramework::Core::JSON::ArrayType<WPEFramework::Channel>&);
    bool ReadPrograms(WPEFramework::Core::JSON::ArrayType<WPEFramework::Program>&);
    bool ReadProgram(uint16_t, WPEFramework::Program&);
    bool ReadSubtitleLanguages(uint64_t, std::vector<std::string>&);
    bool ReadAudioLanguages(uint64_t, std::vector<std::string>&);
    bool IsParentalLocked(const string&);
    bool SetParentalLock(const bool, const string&);
    bool InsertTSInfo(TSInfoList&);
    bool ReadTSInfo(TSInfo&);
    bool IsServicePresentInTSInfo(int32_t);
    bool UnlockChannels();
    bool IsServicePresentInChannelTable(uint16_t, uint16_t, uint16_t&);
};
#endif
