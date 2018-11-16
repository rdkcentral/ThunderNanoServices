#ifndef RTSPPARSER_H
#define RTSPPARSER_H

#include <string>
#include <map>

#include "RtspCommon.h"
#include "RtspSessionInfo.h"

namespace WPEFramework {
namespace Plugin {

typedef std::map<std::string, std::string> NAMED_ARRAY;

class RtspParser
{
    public:
        RtspParser(RtspSessionInfo& sessionInfo);
        RtspMessagePtr BuildSetupRequest(const std::string &server, const std::string &assetId);
        RtspMessagePtr BuildPlayRequest(float scale = 1.0, uint32_t position = 0);
        RtspMessagePtr BuildGetParamRequest(bool bSRM);
        RtspMessagePtr BuildTeardownRequest(int reason);
        RtspMessagePtr BuildResponse(int seq, bool bSRM);

        int ProcessSetupResponse(const std::string &response);
        int ProcessPlayResponse(const std::string &response);
        int ProcessGetParamResponse(const std::string &response);
        int ProcessTeardownResponse(const std::string &response);

        void Parse(const std::string &str,  NAMED_ARRAY &contents, const string &sep1, const string &sep2);
        RtspMessagePtr ParseResponse(const std::string str);
        RtspMessagePtr ParseAnnouncement(const std::string &response, bool bSRM);

        static void HexDump(const char* label, const std::string& msg, uint16_t charsPerLine = 32);

    private:
        void UpdateNPT(NAMED_ARRAY &playMap);
        int Split(const string& str, const string& delim,  std::vector<string>& tokens);

    public:
        RtspSessionInfo& _sessionInfo;

    private:
        static constexpr const char* const RtspLineTerminator = "\r\n";
        static unsigned int _sequence;
};

}} // WPEFramework::Plugin

#endif
