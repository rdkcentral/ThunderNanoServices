#include "Module.h"
#include "EPGData.h"

using namespace WPEFramework;

#define EPG_DURATION (3 * 60 * 60)
#define DB_FILE "/root/DVB.db" //FIXME: Change location of database as per platform requirement.

static int Callback(void* notUsed, int argc, char** argv, char** azColName)
{
    for (int i = 0; i < argc; i++)
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    return 0;
}

EPGDataBase& EPGDataBase::GetInstance()
{
    static EPGDataBase s_EPGDataBase;
    return s_EPGDataBase;
}

EPGDataBase::EPGDataBase()
{
    _fd = open("/opt/database.txt", O_RDWR);
    OpenDB();
    LoadOrSaveDB(false);
}

EPGDataBase::~EPGDataBase()
{
    CloseDB();
    close(_fd);
}

int EPGDataBase::LoadOrSaveDB(bool isSave)
{
    sqlite3 *diskFile;
    int rc = sqlite3_open(DB_FILE, &diskFile);
    if (rc == SQLITE_OK) {
        sqlite3 *source = (isSave ? _dataBase : diskFile);
        sqlite3 *destination = (isSave ? diskFile : _dataBase);

        sqlite3_backup *backup = sqlite3_backup_init(destination, "main", source, "main");
        if (backup) {
            (void)sqlite3_backup_step(backup, -1);
            (void)sqlite3_backup_finish(backup);
        }
        rc = sqlite3_errcode(destination);
    }

    (void)sqlite3_close(diskFile);
    return rc;
}

bool EPGDataBase::OpenDB()
{
    if (!sqlite3_open(":memory:", &_dataBase)) {
        TRACE(Trace::Information, (_T("Open Success")));
        return true;
    } 
    TRACE(Trace::Error, (_T("Open Failed")));
    return false;
}

bool EPGDataBase::CloseDB()
{
    TRACE(Trace::Information, (_T("Close invoked")));
    if (sqlite3_close(_dataBase) != SQLITE_OK)
        TRACE(Trace::Error, (_T("ERROR : %s"), sqlite3_errmsg(_dataBase)));
    return true;
}

bool EPGDataBase::CreateFrequencyTable()
{
    char const* sql = "DROP TABLE IF EXISTS [FREQUENCY]";
    if (sqlite3_exec(_dataBase, sql, Callback, 0, &_errMsg) != SQLITE_OK) {
        TRACE(Trace::Error, (_T("Error = %s"), _errMsg));
        sqlite3_free(_errMsg);
    }
    char const* sqlFrequency = "CREATE TABLE FREQUENCY("  \
        "FREQUENCY INT PRIMARY KEY NOT NULL);";
    if (sqlite3_exec(_dataBase, sqlFrequency, Callback, 0, &_errMsg) != SQLITE_OK) {
        TRACE(Trace::Error, (_T("Error = %s"), _errMsg));
        sqlite3_free(_errMsg);
        return false;
    }
    return true;
}

bool EPGDataBase::CreateNitTable()
{
    char const* sqlQuery = "CREATE TABLE IF NOT EXISTS NIT("  \
        "NETWORK_ID              INT       NOT NULL," \
        "TRANSPORT_STREAM_ID INT INT       NOT NULL," \
        "ORIGINAL_NETWORK_ID     INT       DEFAULT NULL," \
        "FREQUENCY               INT       DEFAULT NULL," \
        "MODULATION              INT       DEFAULT NULL," \
        "UNIQUE(TRANSPORT_STREAM_ID, ORIGINAL_NETWORK_ID));";
    return ExecuteSQLQuery(sqlQuery);
}

bool EPGDataBase::ExecuteSQLQuery(char const* sqlQuery)
{
    bool status = true;
    DBLock();
    if (sqlite3_exec(_dataBase, sqlQuery, Callback, 0, &_errMsg) != SQLITE_OK) {
        TRACE(Trace::Error, (_T("Error = %s"), _errMsg));
        sqlite3_free(_errMsg);
        status = false;
    } else
        TRACE(Trace::Information, (_T("Sql query executed successfully")));

    DBUnlock();
    return status;
}

bool EPGDataBase::CreateChannelTable()
{
    char const* sqlQuery = "CREATE TABLE IF NOT EXISTS CHANNEL("  \
        "LCN            TEXT        NOT NULL," \
        "FREQUENCY      INT         NOT NULL," \
        "MODULATION     INT         NOT NULL," \
        "SERVICE_ID     INT         NOT NULL," \
        "TS_ID          INT         NOT NULL," \
        "NETWORK_ID     INT         NOT NULL," \
        "PROGRAM_NUMBER INT         NOT NULL," \
        "NAME           TEXT        NOT NULL," \
        "LANGUAGE       VARCHAR(20) DEFAULT 'und'," \
        "PARENTAL_LOCK  INT         DEFAULT 0, " \
        "UNIQUE(SERVICE_ID, TS_ID));";

    return ExecuteSQLQuery(sqlQuery);
}

bool EPGDataBase::CreateProgramTable()
{
    char const* sqlQuery = "CREATE TABLE IF NOT EXISTS PROGRAM("  \
        "SOURCE_ID       INT      NOT NULL," \
        "EVENT_ID        INT      NOT NULL," \
        "START_TIME      INT      NOT NULL," \
        "DURATION        INT      NOT NULL," \
        "EVENT_NAME      TEXT     NOT NULL," \
        "SUBTITLE_LANG   TEXT     NULL DEFAULT 'und'," \
        "RATING          VARCHAR(10) DEFAULT 'Not Available'," \
        "GENRE           TEXT     NULL DEFAULT 'Not Available'," \
        "AUDIO_LANG      TEXT     NULL DEFAULT 'und'," \
        "UNIQUE(SOURCE_ID, EVENT_ID, START_TIME));";

    return ExecuteSQLQuery(sqlQuery);
}

bool EPGDataBase::CreateTSTable()
{
    char const* sqlQuery = "CREATE TABLE IF NOT EXISTS TSINFO("  \
        "FREQUENCY                 INT       DEFAULT NULL," \
        "PROGRAM_NUMBER            INT       DEFAULT NULL," \
        "VIDEO_PID                 INT       DEFAULT NULL," \
        "VIDEO_CODEC               INT       DEFAULT NULL," \
        "VIDEO_PCR_PID             INT       DEFAULT NULL," \
        "AUDIO_PID                 INT       DEFAULT NULL," \
        "AUDIO_CODEC               INT       DEFAULT NULL," \
        "AUDIO_PCR_PID             INT       DEFAULT NULL," \
        "PMT_PID                   INT       DEFAULT NULL," \
        "UNIQUE(FREQUENCY, PROGRAM_NUMBER, VIDEO_PID, VIDEO_CODEC, VIDEO_PCR_PID, AUDIO_PID, AUDIO_CODEC, AUDIO_PCR_PID, PMT_PID));";

    return ExecuteSQLQuery(sqlQuery);
}


bool EPGDataBase::DBLock()
{
    int32_t ret;
    struct flock f1;
    f1.l_type = F_WRLCK; // F_RDLCK, F_WRLCK, F_UNLCK.
    f1.l_whence = SEEK_SET; // SEEK_SET, SEEK_CUR, SEEK_END.
    f1.l_start = 0; // Offset from l_whence.
    f1.l_len = 0; // length, 0 = to EOF.
    f1.l_pid = getpid(); // our PID.
    ret = fcntl(_fd, F_SETLKW, &f1);  // Set the lock, waiting if necessary.
    if (!ret) {
        TRACE(Trace::Information, (_T("DB LOCKED")));
        return true;
    }

    TRACE(Trace::Information, (_T("DB ALREADY LOCKED")));
    return false;
}

bool EPGDataBase::DBUnlock()
{
    int32_t ret;
    struct flock f1;
    f1.l_type = F_UNLCK; // tell it to unlock the region.
    f1.l_whence = SEEK_SET; // SEEK_SET, SEEK_CUR, SEEK_END.
    f1.l_start = 0; // Offset from l_whence.
    f1.l_len = 0; // length, 0 = to EOF.
    f1.l_pid = getpid(); // our PID.
    ret = fcntl(_fd, F_SETLK, &f1); // set the region to unlocked.
    if (!ret) {
        TRACE(Trace::Information, (_T("DB UNLOCKED")));
        return true;
    }
    TRACE(Trace::Information, (_T("DB ALREADY UNLOCKED")));
    return false;
}

bool EPGDataBase::InsertFrequencyInfo(std::vector<uint32_t> frequencyList)
{
    char sqlQuery[1024];
    for (auto& frequency : frequencyList) {
        snprintf(sqlQuery, 1024, "INSERT OR IGNORE INTO FREQUENCY (FREQUENCY) VALUES (%d);", (int)frequency);
        TRACE(Trace::Information, (_T("QUERY = %s"), sqlQuery));
        if (!ExecuteSQLQuery(sqlQuery)) {
            return false;
        }
    }
    return true;
}

bool EPGDataBase::InsertNitInfo(uint16_t networkId, uint16_t tsId, uint16_t originalNetworkId, uint32_t frequency, uint8_t modulation)
{
    char sqlQuery[1024];

    snprintf(sqlQuery, 1024, "INSERT OR IGNORE INTO NIT (NETWORK_ID, TRANSPORT_STREAM_ID, ORIGINAL_NETWORK_ID, FREQUENCY, MODULATION) VALUES (%d, %d, %d, %d, %d);", (int)networkId, (int)tsId, (int)originalNetworkId, (int)frequency, (int)modulation);

    TRACE(Trace::Information, (_T("QUERY = %s"), sqlQuery));
    return ExecuteSQLQuery(sqlQuery);
}

bool EPGDataBase::GetFrequencyFromChannelInfo(const string& lcn, uint32_t& frequency)
{
    char sqlQuery[1024];
    uint32_t rc = 0;
    bool ret = false;
    std::string table("CHANNEL");
    if (!IsTableEmpty(table)) {
        DBLock();
        uint32_t freq;
        snprintf(sqlQuery, 1024, "SELECT FREQUENCY FROM CHANNEL WHERE LCN=%s;", lcn.c_str());
        if (sqlite3_prepare(_dataBase, sqlQuery, -1, &_stmt, 0) == SQLITE_OK) {
            rc = sqlite3_step(_stmt);
            if ((rc == SQLITE_ROW) || (rc == SQLITE_DONE))
                freq = (uint32_t)sqlite3_column_int(_stmt, 0);
            sqlite3_reset(_stmt);
            if (sqlite3_finalize(_stmt) == SQLITE_OK) {
                frequency = freq;
                ret = true;
            }
        }
        DBUnlock();
    }
    return ret;
}

bool EPGDataBase::GetFrequencyAndModulationFromNit(uint16_t originalNetworkId, uint16_t transportStreamId, uint32_t& frequency, uint8_t& modulation)
{
    char sqlQuery[1024];
    uint32_t rc = 0;
    bool ret = false;
    std::string table("NIT");
    if (!IsTableEmpty(table)) {
        DBLock();
        uint32_t freq;
        uint32_t mod;
        snprintf(sqlQuery, 1024, "SELECT * FROM NIT WHERE ORIGINAL_NETWORK_ID=%d AND TRANSPORT_STREAM_ID=%d;", (int)originalNetworkId, (int)transportStreamId);
        if (sqlite3_prepare(_dataBase, sqlQuery, -1, &_stmt, 0) == SQLITE_OK) {
            rc = sqlite3_step(_stmt);
            if ((rc == SQLITE_ROW) || (rc == SQLITE_DONE)) {
                freq = (uint32_t)sqlite3_column_int(_stmt, 3);
                mod = (uint8_t)sqlite3_column_int(_stmt, 4);
            }
            sqlite3_reset(_stmt);
            if (sqlite3_finalize(_stmt) == SQLITE_OK) {
                frequency = freq;
                modulation = mod;
                ret = true;
            }
        }
        DBUnlock();
    }
    return ret;
}

bool EPGDataBase::GetServiceIdFromChannelInfo(const string& lcn, uint16_t& serviceId)
{
    char sqlQuery[1024];
    uint32_t rc = 0;
    bool ret = false;
    std::string table("CHANNEL");
    if (!IsTableEmpty(table)) {
        DBLock();
        uint16_t svcId;
        snprintf(sqlQuery, 1024, "SELECT PROGRAM_NUMBER FROM CHANNEL WHERE LCN=%s;", lcn.c_str());
        if (sqlite3_prepare(_dataBase, sqlQuery, -1, &_stmt, 0) == SQLITE_OK) {
            rc = sqlite3_step(_stmt);
            if ((rc == SQLITE_ROW) || (rc == SQLITE_DONE))
                svcId = (uint16_t)sqlite3_column_int(_stmt, 0);
            sqlite3_reset(_stmt);
            if (sqlite3_finalize(_stmt) == SQLITE_OK) {
                serviceId = svcId;
                ret = true;
            }
        }
        DBUnlock();
    }
    return ret;
}

bool EPGDataBase::InsertChannelInfo(uint32_t frequency, uint32_t modulation, const char* name, uint16_t serviceId, uint16_t tsId, uint16_t networkId, const std::string& lcn, uint16_t programNo, const std::string& language)
{
    char sqlQuery[1024];

    snprintf(sqlQuery, 1024, "INSERT OR IGNORE INTO CHANNEL (LCN, FREQUENCY, MODULATION, SERVICE_ID, TS_ID, NETWORK_ID, PROGRAM_NUMBER, \
        NAME, LANGUAGE) VALUES (\"%s\", %d, %d, %d, %d, %d, %d, \"%s\", \"%s\");", lcn.c_str(), (int)frequency, (int)modulation
        , (int)serviceId, (int)tsId, (int)networkId, (int)programNo, name, language.size() ? language.c_str() : "und");

    TRACE(Trace::Information, (_T("QUERY = %s"), sqlQuery));
    return ExecuteSQLQuery(sqlQuery);
}

bool EPGDataBase::InsertProgramInfo(uint16_t sourceId, uint16_t eventId, time_t startTime, time_t duration, const char* eventName, const std::string& rating, const std::string& subtitleLanguage, const std::string& genre, const std::string& audioLanguage)
{
    char sqlQuery[1024];
    snprintf(sqlQuery, 1024, "INSERT OR REPLACE INTO PROGRAM (SOURCE_ID, EVENT_ID, START_TIME, \
        DURATION, EVENT_NAME, SUBTITLE_LANG, RATING, GENRE, AUDIO_LANG) VALUES (%d, %d, %d, %d, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\");"
        , (int)sourceId, (int)eventId, (int)startTime, (int)duration, eventName, subtitleLanguage.size() ? subtitleLanguage.c_str() : "und"
        , rating.size() ? rating.c_str() : "Not Available", genre.size() ? genre.c_str() : "Not Available"
        , audioLanguage.size() ? audioLanguage.c_str() : "und");
    return ExecuteSQLQuery(sqlQuery);
}

bool EPGDataBase::IsTableEmpty(const std::string& table)
{
    char sqlDelete[1024];
    DBLock();
    snprintf(sqlDelete, 1024, "SELECT Count(*) FROM %s;", table.c_str());
    sqlite3_prepare_v2(_dataBase, sqlDelete, -1, &_stmt, nullptr);
    if ((sqlite3_step(_stmt) == SQLITE_ROW) && !sqlite3_column_int(_stmt, 0)) {
        TRACE(Trace::Information, (_T("%s table empty"), table.c_str()));
        sqlite3_finalize(_stmt);
        DBUnlock();
        return true;
    }
    TRACE(Trace::Information, (_T("%s table has content"), table.c_str()));
    sqlite3_finalize(_stmt);
    DBUnlock();
    return false;

}

bool EPGDataBase::GetFrequencyListFromNit(std::vector<uint32_t>& frequencyList)
{
    DBLock();
    sqlite3_prepare_v2(_dataBase, "SELECT FREQUENCY FROM NIT", -1, &_stmt, nullptr);
    while (sqlite3_step(_stmt) == SQLITE_ROW)
        frequencyList.push_back((uint32_t)sqlite3_column_int(_stmt, 0));
    sqlite3_finalize(_stmt);
    DBUnlock();
    return true;
}

void EPGDataBase::ClearSIData()
{
    time_t cTime;
    time(&cTime);
    uint32_t currTime = (uint32_t)cTime;
    TRACE(Trace::Information, (_T("current time = %u"), currTime));
    char sqlQuery[1024];
    snprintf(sqlQuery, 1024, "DELETE FROM PROGRAM WHERE (START_TIME + DURATION) < %d;", currTime);

    ExecuteSQLQuery(sqlQuery);
}

bool EPGDataBase::ReadFrequency(std::vector<uint32_t>& frequencyList)
{
    bool ret = false;
    DBLock();
    uint32_t frequency;
    sqlite3_prepare_v2(_dataBase, "SELECT * FROM FREQUENCY;", -1, &_stmt, nullptr);
    frequencyList.clear();
    while (sqlite3_step(_stmt) == SQLITE_ROW) {
        frequency = (uint32_t)sqlite3_column_int(_stmt, 0);
        if (frequency) {
            frequencyList.push_back(frequency);
            ret = true;
        }
    }
    sqlite3_finalize(_stmt);
    DBUnlock();
    return ret;
}

bool EPGDataBase::ReadChannels(WPEFramework::Core::JSON::ArrayType<WPEFramework::Channel>& channelVector)
{
    bool ret = false;
    DBLock();
    std::string table("CHANNEL");
    if (!IsTableEmpty(table)) {
        sqlite3_prepare_v2(_dataBase, "SELECT * FROM CHANNEL;", -1, &_stmt, nullptr);
        while (sqlite3_step(_stmt) == SQLITE_ROW) {
            WPEFramework::Channel channel;
            channel.number = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 0));
            channel.frequency = (uint32_t)sqlite3_column_int(_stmt, 1);
            channel.serviceId = (uint16_t)sqlite3_column_int(_stmt, 3);
            channel.transportSId = (uint16_t)sqlite3_column_int(_stmt, 4);
            channel.networkId = (uint16_t)sqlite3_column_int(_stmt, 5);
            channel.programNo = (uint16_t)sqlite3_column_int(_stmt, 6);
            channel.name = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 7));
            channelVector.Add(channel);
            ret = true;
        }
        sqlite3_finalize(_stmt);
        DBUnlock();
    }
    return ret;
}

bool EPGDataBase::ReadChannel(const string& channelNum, WPEFramework::Channel& channel)
{
    bool ret = false;
    char sqlQuery[1024];
    std::string table("CHANNEL");
    if (!IsTableEmpty(table)) {
        DBLock();
        snprintf(sqlQuery, 1024, "SELECT * FROM CHANNEL WHERE LCN=%s;", channelNum.c_str());
        sqlite3_prepare_v2(_dataBase, sqlQuery, -1, &_stmt, nullptr);
        while (sqlite3_step(_stmt) == SQLITE_ROW) {
            channel.number = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 0));
            channel.frequency = (uint32_t)sqlite3_column_int(_stmt, 1);
            channel.networkId = 0;
            channel.serviceId = (uint16_t)sqlite3_column_int(_stmt, 3);
            channel.transportSId = (uint16_t)sqlite3_column_int(_stmt, 4);
            channel.networkId = (uint16_t)sqlite3_column_int(_stmt, 5);
            channel.programNo = (uint16_t)sqlite3_column_int(_stmt, 6);
            channel.name = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 7));
            ret = true;
        }
        sqlite3_finalize(_stmt);
        DBUnlock();
    }
    return ret;
}

bool EPGDataBase::ReadPrograms(WPEFramework::Core::JSON::ArrayType<WPEFramework::Program>& programVector)
{
    bool ret = false;
    time_t currTime;
    time(&currTime);
    TRACE(Trace::Information, (_T("current time = %d"), currTime));
    DBLock();
    sqlite3_prepare_v2(_dataBase, "SELECT * FROM PROGRAM;", -1, &_stmt, nullptr);
    while (sqlite3_step(_stmt) == SQLITE_ROW) {
        time_t startTime = (time_t)sqlite3_column_int(_stmt, 2);
        if ((startTime < (currTime + EPG_DURATION)) && ((startTime + sqlite3_column_int(_stmt, 3)) > currTime)) {
            WPEFramework::Program program;
            program.eventId = (uint16_t)sqlite3_column_int(_stmt, 1);
            program.startTime = (uint32_t)sqlite3_column_int(_stmt, 2);
            program.duration = (uint32_t)sqlite3_column_int(_stmt, 3);
            program.title = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 4));
            program.rating = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 6));
            program.serviceId = (uint16_t)sqlite3_column_int(_stmt, 0);
            program.audio = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 8));
            program.subtitle = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 5));
            program.genre = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 7));
            programVector.Add(program);
            ret = true;
        }
    }
    sqlite3_finalize(_stmt);
    DBUnlock();
    return ret;
}

bool EPGDataBase::ReadProgram(uint16_t serviceId, WPEFramework::Program& program)
{
    bool ret = false;
    std::string table("PROGRAM");
    if (!IsTableEmpty(table)) {
        time_t currTime;
        time(&currTime);
        TRACE(Trace::Information, (_T("current time = %d"), currTime));
        DBLock();
        sqlite3_prepare_v2(_dataBase, "SELECT * FROM PROGRAM WHERE SOURCE_ID = ? ;", -1, &_stmt, nullptr);
        sqlite3_bind_int(_stmt, 1, (int)serviceId);
        while (sqlite3_step(_stmt) == SQLITE_ROW) {
            time_t startTime = (time_t)sqlite3_column_int(_stmt, 2);
            TRACE(Trace::Information, (_T("startTime = %d"), startTime));
            if (currTime >= startTime && currTime < (startTime + sqlite3_column_int(_stmt, 3))) {
                program.eventId = (uint16_t)sqlite3_column_int(_stmt, 1);
                program.startTime = (uint32_t)sqlite3_column_int(_stmt, 2);
                program.duration = (uint32_t)sqlite3_column_int(_stmt, 3);
                program.title = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 4));
                program.rating = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 6));
                program.serviceId = (uint16_t)sqlite3_column_int(_stmt, 0);
                program.audio = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 8));
                program.subtitle = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 5));
                program.genre = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 7));
                ret = true;
                break;
            }
        }
        sqlite3_finalize(_stmt);
        DBUnlock();
    }
    return ret;
}

bool EPGDataBase::IsParentalLocked(const string& lcn)
{
    bool ret = false;
    std::string table("CHANNEL");
    char sqlQuery[1024];
    if (!IsTableEmpty(table)) {
        struct stat* buf = (struct stat *)malloc(sizeof(struct stat));
        if (!stat("/root/DVB.db", buf)) {
            snprintf(sqlQuery, 1024, "SELECT PARENTAL_LOCK FROM CHANNEL WHERE LCN=%s;", lcn.c_str());
            DBLock();
            sqlite3_prepare_v2(_dataBase, sqlQuery, -1, &_stmt, nullptr);
            if (sqlite3_step(_stmt) == SQLITE_ROW) {
                if (sqlite3_column_int(_stmt, 0))
                    ret = true;
            }
            sqlite3_finalize(_stmt);
            DBUnlock();
        } else {
            TRACE(Trace::Information, (_T("DataBase not yet created \n")));
            ret = false;
        }
    }
    return ret;
}

bool EPGDataBase::SetParentalLock(const bool lockValue, const string& lcn)
{
    bool ret = true;
    std::string table("CHANNEL");
    if (!IsTableEmpty(table)) {
        char sqlQuery[1024];
        snprintf(sqlQuery, 1024, "UPDATE CHANNEL SET PARENTAL_LOCK = %d WHERE LCN = %s;", (int)lockValue, lcn.c_str());
        if (!ExecuteSQLQuery(sqlQuery)) {
            ret = false;
        }
    }
    return ret;
}

bool EPGDataBase::ReadSubtitleLanguages(uint64_t eventId, std::vector<std::string>& languages)
{
    bool ret = false;
    DBLock();
    sqlite3_prepare_v2(_dataBase, "SELECT * FROM PROGRAM WHERE EVENT_ID = ? ;", -1, &_stmt, nullptr);
    sqlite3_bind_int(_stmt, 1, (int)eventId);
    while (sqlite3_step(_stmt) == SQLITE_ROW) {
        std::string subtitles = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 5));
        std::istringstream subtitleStream(subtitles);
        std::string token;
        while (std::getline(subtitleStream, token, ','))
            languages.push_back(token);
        ret = true;
        break;
    }
    sqlite3_finalize(_stmt);
    DBUnlock();
    return ret;
}

bool EPGDataBase::ReadAudioLanguages(uint64_t eventId, std::vector<std::string>& languages)
{
    bool ret = false;
    DBLock();
    sqlite3_prepare_v2(_dataBase, "SELECT * FROM PROGRAM WHERE EVENT_ID = ? ;", -1, &_stmt, nullptr);
    sqlite3_bind_int(_stmt, 1, (int)eventId);
    while (sqlite3_step(_stmt) == SQLITE_ROW) {
        std::string audios = reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 8));
        std::istringstream subtitleStream(audios);
        std::string token;
        while (std::getline(subtitleStream, token, ','))
            languages.push_back(token);
        ret = true;
        break;
    }
    sqlite3_finalize(_stmt);
    DBUnlock();
    return ret;
}

bool EPGDataBase::InsertTSInfo(TSInfoList& TSInfoList)
{
    char const* sql = "DELETE FROM TSINFO";
    DBLock();
    if (sqlite3_exec(_dataBase, sql, Callback, 0, &_errMsg) != SQLITE_OK) {
        TRACE(Trace::Error, (_T("Error = %s"), _errMsg));
        sqlite3_free(_errMsg);
        DBUnlock();
        return false;
    }
    DBUnlock();

    char sqlQuery[1024];
    for (auto& tsInfo : TSInfoList) {
        snprintf(sqlQuery, 1024, "INSERT OR IGNORE INTO TSINFO (FREQUENCY, PROGRAM_NUMBER, VIDEO_PID, VIDEO_CODEC, VIDEO_PCR_PID, AUDIO_PID, AUDIO_CODEC, AUDIO_PCR_PID, PMT_PID) VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d);", (int)tsInfo.frequency, tsInfo.programNumber, tsInfo.videoPid, tsInfo.videoCodec, tsInfo.videoPcrPid, tsInfo.audioPid, tsInfo.audioCodec, tsInfo.audioPcrPid, tsInfo.pmtPid);
        TRACE(Trace::Information, (_T("QUERY = %s"), sqlQuery));
        if (!ExecuteSQLQuery(sqlQuery)) {
            return false;
        }
    }
    return true;
}

bool EPGDataBase::ReadTSInfo(TSInfo& tsInfo)
{
    bool ret = false;
    std::string table("TSINFO");
    if (!IsTableEmpty(table)) {
        DBLock();
        sqlite3_prepare_v2(_dataBase, "SELECT * FROM TSINFO WHERE FREQUENCY = ? AND PROGRAM_NUMBER = ? ;", -1, &_stmt, nullptr);
        sqlite3_bind_int(_stmt, 1, (int)tsInfo.frequency);
        sqlite3_bind_int(_stmt, 2, (int)tsInfo.programNumber);

        if (sqlite3_step(_stmt) == SQLITE_ROW) {
            tsInfo.videoPid = (uint16_t)sqlite3_column_int(_stmt, 2);
            tsInfo.videoCodec = (uint32_t)sqlite3_column_int(_stmt, 3);
            tsInfo.videoPcrPid = (uint16_t)sqlite3_column_int(_stmt, 4);
            tsInfo.audioPid = (uint16_t)sqlite3_column_int(_stmt, 5);
            tsInfo.audioCodec = (uint32_t)sqlite3_column_int(_stmt, 6);
            tsInfo.audioPcrPid = (uint16_t)sqlite3_column_int(_stmt, 7);
            tsInfo.pmtPid  = (uint16_t)sqlite3_column_int(_stmt, 8);
            ret = true;
            TRACE(Trace::Information, (_T("Data read with freq = %u program num = %d videoPid = %d, \
                videocodec=%d, videoPCRpid = %d, audioPid = %d , audiocodec=%d, \
                AudioPCRPid = %d pmtpid =%d"), tsInfo.frequency, tsInfo.programNumber,
                tsInfo.videoPid, tsInfo.videoCodec, tsInfo.videoPcrPid, tsInfo.audioPid, tsInfo.audioCodec, tsInfo.audioPcrPid, tsInfo.pmtPid));
        }
        sqlite3_finalize(_stmt);
        DBUnlock();
    }
    return ret;
}

bool EPGDataBase::IsServicePresentInTSInfo(int32_t programNumber)
{
    bool ret = false;
    std::string table("TSINFO");
    if (!IsTableEmpty(table)) {
        char sqlQuery[1024];
        snprintf(sqlQuery, 1024, "SELECT * FROM TSINFO WHERE PROGRAM_NUMBER=%d;", programNumber);
        DBLock();
        sqlite3_prepare_v2(_dataBase, sqlQuery, -1, &_stmt, nullptr);
        while (sqlite3_step(_stmt) == SQLITE_ROW) {
            ret = true;
            break;
        }
        sqlite3_finalize(_stmt);
        DBUnlock();
    }
    return ret;

}

bool EPGDataBase::UnlockChannels()
{
    std::string table("CHANNEL");
    if (!IsTableEmpty(table)) {
        char sqlQuery[1024];
        snprintf(sqlQuery, 1024, "UPDATE CHANNEL SET PARENTAL_LOCK = 0;");
        DBLock();
        if (sqlite3_exec(_dataBase, sqlQuery, Callback, 0, &_errMsg) != SQLITE_OK) {
            TRACE(Trace::Information, (_T("FAILURE..@ %s,%s,%d,error = %s"), __FILE__, __func__, __LINE__, _errMsg));
            sqlite3_free(_errMsg);
        } else {
            TRACE(Trace::Information, (_T("Success unlocking channels\n%s,%s,%d"), __FILE__, __func__, __LINE__));
            return true;
        }
        DBUnlock();
    }
    return false;
}

bool EPGDataBase::IsServicePresentInChannelTable(uint16_t serviceId, uint16_t tsId, uint16_t& lcn)
{
    bool ret = false;
    std::string table("CHANNEL");
    if (!IsTableEmpty(table)) {
        DBLock();
        sqlite3_prepare_v2(_dataBase, "SELECT LCN FROM CHANNEL WHERE SERVICE_ID = ? AND TS_ID = ?;", -1, &_stmt, nullptr);
        sqlite3_bind_int(_stmt, 1, (int)serviceId);
        sqlite3_bind_int(_stmt, 2, (int)tsId);
        if (sqlite3_step(_stmt) == SQLITE_ROW) {
            lcn = (uint16_t)std::atoi(reinterpret_cast<const char*>(sqlite3_column_text(_stmt, 0)));
            ret = true;
        }
        sqlite3_finalize(_stmt);
        DBUnlock();
    }
    return ret;
}
