#pragma once
#include "Module.h"
#include "Handler.h"
#include "Utils.h"

#include <tinyxml.h>

namespace WPEFramework {

typedef enum
{
    DB_SUCCESS = 0,
    DB_FAILURE,
    DB_ERR_WILDCARD_NOT_SUPPORTED,
    DB_ERR_INVALID_PARAMETER,
    DB_ERR_TIMEOUT,
    DB_ERR_NOT_EXIST
}
DBStatus;

class WalDB {
private:
    static constexpr const uint32_t  MaxNumParameters = 2048;
    static constexpr const TCHAR* InstanceNumberIndicator = "{i}.";

public:
    WalDB() = delete;
    WalDB(const WalDB&) = delete;
    WalDB& operator= (const WalDB&) = delete;
public:
    WalDB(Handler* handler);
    ~WalDB();

    DBStatus LoadDB(const std::string& filename);
    DBStatus Parameters(const std::string& paramName, std::map<uint32_t, std::pair<std::string, std::string>>& paramList) const;
    bool IsValidParameter(const std::string& paramName, std::string& dataType) const;
    int DBHandle() { return _dbHandle; }

private:
    TiXmlNode* Parameters(TiXmlNode* pParent, const std::string& paramName, std::string& currentParam, std::map<uint32_t, std::pair<std::string, std::string>>& paramList) const;
    void CheckforParameterMatch(TiXmlNode *pParent, const std::string& paramName, bool& pMatch, std::string& dataType) const;
    bool IsParamEndsWithInstance(const std::string& paramName) const;
    void ReplaceWithInstanceNumber(string& paramName, uint16_t instanceNumber) const;
    bool CheckMatchingParameter(const char* attrValue, const char* paramName, uint32_t& ret) const;
    bool ValidateParameterInstance(const std::string& paramName, std::string& dataType) const;
    uint8_t FindInstanceOccurance(const std::string& paramName, std::map<uint8_t, std::pair<std::size_t, std::size_t>>& positions) const;
    int16_t ParameterInstanceCount(const std::string& paramName) const;

private:
    int _dbHandle;
    Handler* _handler;
};
}
