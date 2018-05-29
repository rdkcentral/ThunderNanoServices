// This is work-in-progress version of SIControl
#ifndef ITABLEDATA_H
#define ITABLEDATA_H

#include "ITableDataHandler.h"

#include <EPGData.h>
#include <IParser.h>
#include <fstream>
#include <interfaces/IGuide.h>
#include <interfaces/ITVPlatform.h>
#include <unistd.h>

#define PARENTAL_LOCK_FILE "/root/TVParentLock.txt"

namespace WPEFramework {
namespace Plugin {

class ITableData 
    : public TVPlatform::ITVPlatform::ISectionHandler
    , public ISIHandler {

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Idletime(30)
                , HomeTS(0)
            {
                Add(_T("idletime"), &Idletime);
                Add(_T("homets"), &HomeTS);
                Add(_T("countrycode"), &CountryCode);
                Add(_T("countryregionid"), &CountryRegionId);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt16 Idletime;
            Core::JSON::DecUInt32 HomeTS;
            Core::JSON::String CountryCode;
            Core::JSON::DecUInt8 CountryRegionId;
        };
        ITableData(const ITableData&) = delete;
        ITableData& operator=(const ITableData&) = delete;

    public:
        class TimeHandler {
        public:
            TimeHandler()
                : _parent(nullptr)
            {
            }
            TimeHandler(ITableData& parent)
                : _parent(&parent)
            {
            }
            TimeHandler(const TimeHandler& copy)
                : _parent(copy._parent)
            {
            }
            ~TimeHandler()
            {
            }
            TimeHandler& operator=(const TimeHandler& RHS)
            {
                _parent = RHS._parent;
                return (*this);
            }

        public:
            uint64_t Timed(const uint64_t scheduledTime)
            {
                ASSERT(_parent);
                return (_parent->Timed(scheduledTime));
            }

        private:
            ITableData* _parent;
        };

    public:
        ITableData(ITableDataHandler&, TVPlatform::ITVPlatform* tvPlatform);
        virtual ~ITableData();

        static ITableData* GetInstance(ITableDataHandler&, TVPlatform::ITVPlatform* tvPlatform);
        static void DeleteInstance();

        // ISectionHandler interfaces.
        void SectionDataCB(const string&);

        // ISIHandler interfaces.
        void StartFilter(uint16_t, uint16_t);
        void StopFilter(uint16_t, uint16_t);
        void StopFilters();
        void SetHomeTS(const uint32_t, const uint32_t);
        void EitBroadcasted();
        bool IsTSInfoPresent();
        bool IsScanning();
        void UpdateScanStatus();
        void StartTimer();

        // ITableData interfaces.
        uint32_t StartParser(PluginHost::IShell*);
        uint64_t Timed(const uint64_t);
        std::string GetChannels();
        std::string GetPrograms(); // Get all programs of all channels for 3hr.
        std::string GetCurrentProgram(const string&);
        std::string GetAudioLanguages(const uint32_t);
        std::string GetSubtitleLanguages(const uint32_t);
        bool SetParentalControlPin(const string&, const string&);
        bool SetParentalControl(const string&, const bool);
        bool IsParentalControlled();
        bool IsParentalLocked(const string&);
        bool SetParentalLock(const string&, const bool, const string&);
        bool ParentalControlInit();
        void NotifyFrequencyListUpdate();
        void NotifyTSInfoUpdate();
        void NotifyStreamingFrequencyChange(uint32_t);
        bool UpdateParentalLockFile();

    private:
        static ITableData* _tblInstance;
        IParser* _tvParser;
        TVPlatform::ITVPlatform* _tvPlatform;
        EPGDataBase& _epgDB;
        uint16_t _clearSIDataTimer; // in milliseconds.
        uint16_t _clearSIDataCounter;
        uint16_t _dbCacheCounter;
        Core::TimerType<TimeHandler> _cleanupTimer;
        Config _config;
        bool _isParentalControlled;
        std::string _parentalControlPin;
        bool _isParentalLocked;
        ITableDataHandler& _tableDataHandler;
        uint16_t _homeTS;
        volatile bool _isTSInfoPresent;
    };
}
}
#endif
