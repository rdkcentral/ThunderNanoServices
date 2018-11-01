#ifndef RTSPPARSER_H
#define RTSPPARSER_H

#include <string>
#include <map>

#include "RtspSessionInfo.h"

namespace WPEFramework {
namespace Plugin {

typedef std::map<std::string, std::string> NAMED_ARRAY;

class RtspParser
{
    public:
        enum MessageType {
            RTSP_UNKNOWN,
            RTSP_RESPONSE,
            RTSP_ANNOUNCE
        };

        RtspParser(RtspSessionInfo& sessionInfo);
        std::string BuildSetupRequest(const std::string &server, const std::string &assetId);
        std::string BuildPlayRequest(float scale = 1.0, uint32_t position = 0);
        std::string BuildGetParamRequest(bool bSRM);
        std::string BuildTeardownRequest(int reason);
        std::string BuildResponse(int seq, bool bSRM);

        int ProcessSetupResponse(const std::string &response);
        int ProcessPlayResponse(const std::string &response);
        int ProcessGetParamResponse(const std::string &response);
        int ProcessTeardownResponse(const std::string &response);
        int ProcessAnnouncement(const std::string &response, bool bSRM);

        void Parse(const std::string &str,  NAMED_ARRAY &contents, const string &sep1, const string &sep2);
        int ParseResponse(const std::string str,  std::string &rtspBody, RtspParser::MessageType &msgType);

    private:
        void UpdateNPT(NAMED_ARRAY &playMap);

        int Split(const string& str, const string& delim,  std::vector<string>& tokens);
        static void HexDump(const char* label, const std::string& msg, uint16_t charsPerLine = 32);

    public:
        RtspSessionInfo& _sessionInfo;

    private:
        static unsigned int _sequence;
};

}} // WPEFramework::Plugin

#endif
