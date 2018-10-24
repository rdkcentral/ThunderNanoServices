#include "RtspParser.h"

#include <sstream>
#include <vector>
#include <iomanip>

#define ANNOUNCEMENT_CHECK_INTERVAL     10
#define RTSP_THREAD_SLEEP_MS            1000
#define SEC2MS(s)                       (s*1000)
#define MS2SEC(ms)                      (ms/1000)
#define RTSP_RESPONSE_WAIT_TIME         3000
#define RTSP_TERMINATOR                 "\r\n"

unsigned int RtspParser::seq = 0;

RtspParser::RtspParser()
{
}

std::string
RtspParser::buildSetupRequest(const std::string &server, const std::string &assetId)
{
    std::stringstream ss;

    ss << "SETUP rtsp://" << server << "/" << assetId << "?";
    ss << "VODServingAreaId=1099" << "&";
    ss << "StbId=943BB162A323&";
    ss << "CADeviceId=943BB162A323";
    ss << " RTSP/1.0\r\n";
    ss << "CSeq:" << ++seq << "\r\n";
    ss << "User-Agent: Metro\r\n";
    ss << "Transport: MP2T/DVBC/QAM;unicast;\r\n";
    ss << "\r\n";

    hexDump("SETUP", ss.str());

    return ss.str();
}

std::string
RtspParser::buildPlayRequest(float scale, int offset)
{
    std::stringstream ss;
    string sessionId;
    string cmd = (scale == 0) ? "PAUSE" : "PLAY";

    if (sessionInfo.bSrmIsRtspProxy)
        sessionId = sessionInfo.sessionId;
    else
        sessionId = sessionInfo.ctrlSessionId;

    ss << cmd << " * RTSP/1.0\r\n";
    ss << "CSeq:" << ++seq << "\r\n";
    ss << "Session:" << sessionId << "\r\n";
    if (offset) {
        int pos = (offset == INT_MAX) ? 0 : MS2SEC(sessionInfo.npt) + offset;
        ss << "Range: npt=" << pos << "\r\n";
    }
    ss << "Scale: " << scale << "\r\n";
    ss << "\r\n";

    hexDump("PLAY", ss.str());

    return ss.str();
}

std::string
RtspParser::buildGetParamRequest(bool bSRM)
{
    string strParams;
    string sessId;

    if (bSRM) {
        sessId = sessionInfo.sessionId;
    } else {
        sessId = sessionInfo.ctrlSessionId;
        std::stringstream ssParams;
        ssParams << "Position\r\n" ;
        ssParams << "Scale\r\n";
        ssParams << "stream_state\r\n";
        strParams = ssParams.str();
    }

    std::stringstream ss;
    ss << "GET_PARAMETER * RTSP/1.0\r\n";
    ss << "CSeq:" << ++seq << "\r\n";
    ss << "Session:" << sessId << "\r\n";
    ss << "Content-Type: text/parameters\r\n";
    ss << "Content-Length: " << strParams.length() << "\r\n";
    if (strParams.length()) {
        ss << "\r\n";
        ss << strParams;
    }
    ss << "\r\n";

    hexDump("GETPARAM", ss.str());

    TRACE_L2( "%s: %d. '%s'", __FUNCTION__, bSRM, ss.str().c_str());
    return ss.str();
}

std::string
RtspParser::buildTeardownRequest(int reason)
{
    std::stringstream ss;
    string strReason = "Cleint Intiated";

    ss << "TEARDOWN * RTSP/1.0\r\n";
    ss << "CSeq:" << ++seq << "\r\n";
    ss << "Session:" << sessionInfo.sessionId << "\r\n";
    ss << "Reason:" << reason << " " << strReason << "\r\n";
    ss << "\r\n";

    hexDump("TEARDOWN", ss.str());

    return ss.str();
}

std::string
RtspParser::buildResponse(int respSeq, bool bSRM)
{
    string sessId = (bSRM) ? sessionInfo.sessionId : sessionInfo.ctrlSessionId;

    std::stringstream ss;
    ss << "RTSP/1.0 200 OK\r\n";
    ss << "CSeq:" << respSeq << "\r\n";
    ss << "Session:" << sessionInfo.sessionId << "\r\n";
    ss << "\r\n";
    ss << "\r\n";
    return ss.str();
}

int RtspParser::processSetupResponse(const std::string &response)
{
    NAMED_ARRAY setupMap;               // entire response
    NAMED_ARRAY params;                 // single line
    parse(response, setupMap, "\r\n", ": ");

    string sess = setupMap["Session"];
    TRACE_L2( "%s: session id='%s'", __FUNCTION__, sess.c_str());
    if (sess.find(";") == string::npos) {
        sessionInfo.sessionId = sess;
        sessionInfo.sessionTimeout = SEC2MS(sessionInfo.defaultSessionTimeout);
        TRACE_L2( "%s: using default sessionTimeout %d", __FUNCTION__, sessionInfo.defaultSessionTimeout);
    } else {                            // contains heartbeat
        parse(sess, params, ";", "=");
        NAMED_ARRAY::iterator it=params.begin();
        sessionInfo.sessionId = it->first;

        if (params.size() > 1 )
            sessionInfo.sessionTimeout = SEC2MS(atoi(params["timeout"].c_str()));
    }

    sess = setupMap["ControlSession"];
    if (sess.find(";") == string::npos) {
        sessionInfo.ctrlSessionId = sess;
        sessionInfo.ctrlSessionTimeout = SEC2MS(sessionInfo.defaultCtrlSessionTimeout);
        TRACE_L2( "%s: using default ctrlSessionTimeout %d", __FUNCTION__, sessionInfo.defaultCtrlSessionTimeout);
    } else {
        parse(sess, params, ";", "=");
        NAMED_ARRAY::iterator it=params.begin();
        sessionInfo.ctrlSessionId = it->first;

        if (params.size() > 1 )
            sessionInfo.ctrlSessionTimeout = SEC2MS(atoi(params["timeout"].c_str()));
    }

    if (sessionInfo.sessionId.compare(sessionInfo.ctrlSessionId) == 0)  // XXX: check IP Addr ???
        sessionInfo.bSrmIsRtspProxy = true;
    else
        sessionInfo.bSrmIsRtspProxy = false;

    string location = setupMap["Location"];
    string chan = setupMap["Tuning"];
    parse(chan, params, ";", "=");
    sessionInfo.frequency  = atoi(params["frequency"].c_str()) * 100;
    sessionInfo.modulation = atoi(params["modulation"].c_str());
    sessionInfo.symbolRate = atoi(params["symbol_rate"].c_str());

    string tune = setupMap["Channel"];
    parse(tune, params, ";", "=");
    sessionInfo.programNum = atoi(params["Svcid"].c_str());

    sessionInfo.bookmark = atof(setupMap["Bookmark"].c_str());
    sessionInfo.duration = atoi(setupMap["Duration"].c_str());

    TRACE_L2( "%s: f=%d p=%d m=%d s=%d bookmark=%f duration=%d",
        __FUNCTION__, sessionInfo.frequency, sessionInfo.programNum, sessionInfo.modulation, sessionInfo.symbolRate, sessionInfo.bookmark, sessionInfo.duration);
}

void RtspParser::updateNPT(NAMED_ARRAY &playMap)
{
    float nptStart = 0, nptEnd = 0;
    float oldScale = sessionInfo.scale;
    float oldNPT = sessionInfo.npt;

    NAMED_ARRAY::iterator it = playMap.find("Scale");
    if (it != playMap.end())
        sessionInfo.scale = atof(playMap["Scale"].c_str());

    it = playMap.find("Range");
    if (it != playMap.end()) {
        string range = playMap["Range"];
        size_t posEq = range.find('=');
        size_t posHyphen = range.find('-');
        if (posEq != string::npos) {
            if (posHyphen != string::npos) {
                nptStart = atof(range.substr(posEq+1, posHyphen-1).c_str());
                nptEnd = atoi(range.substr(posHyphen+1).c_str());
            } else {
                nptStart = atof(range.substr(posEq+1).c_str());
            }

        }

        sessionInfo.npt = SEC2MS(nptStart);
        TRACE_L2( "%s: NPT=%6.2f scale=%2.2f oldNPT=%6.2f oldScale=%2.2f", __FUNCTION__,  SEC2MS(sessionInfo.npt), sessionInfo.scale, oldNPT, oldScale);
    }
}

int RtspParser::processPlayResponse(const std::string &response)
{
    NAMED_ARRAY playMap;
    parse(response, playMap, "\r\n", ": ");
    updateNPT(playMap);
}

int RtspParser::processGetParamResponse(const std::string &response)
{
    NAMED_ARRAY playMap;
    parse(response, playMap, "\r\n", ": ");
    updateNPT(playMap);
}

int RtspParser::processTeardownResponse(const std::string &response)
{
    NAMED_ARRAY playMap;
    parse(response, playMap, "\r\n", ": ");
    //
}

void RtspParser::parse(const std::string &str,  NAMED_ARRAY &contents, const string &sep1, const string &sep2)
{
    //TRACE_L2( "%s: size=%d input='%s'\n", __FUNCTION__, str.size(), str.c_str());
    contents.clear();

    bool done = false;
    size_t pos = 0;
    size_t start = 0;
    while (!done) {
        pos = str.find (sep1, start);

        //TRACE_L2("%s: pos=%u start=%u size=%u", __FUNCTION__, pos, start, str.size());
        if ((pos == string::npos) || (pos >= str.size()))
            done = true;

        string sub = str.substr(start, pos-start);
        size_t pos2 = sub.find(sep2);
        if (pos2 != string::npos) {
            string left = sub.substr(0, pos2);
            string right = sub.substr(pos2+sep2.size());
            //TRACE_L2("%s: \tleft=%s right=%s pos=%u", __FUNCTION__, left.c_str(), right.c_str(), pos2);
            contents[left] = right;
        }
//        cout << sub << endl;

        start = pos+sep1.size();
        if (start >= str.size())
            done = true;
    }

    TRACE_L2( "%s: contents.size=%d", __FUNCTION__, contents.size());
    for (NAMED_ARRAY::iterator it=contents.begin(); it!=contents.end(); ++it)
        TRACE_L4( "%s: %s => '%s'", __FUNCTION__, it->first.c_str(), it->second.c_str());
}

int
RtspParser::parseResponse(const std::string str,  std::string &rtspBody, RtspParser::MessageType &msgType)
{
    int rtspCode = 0;
    msgType = RTSP_UNKNOWN;
    // -------------------------------------------------------------------------
    // RTSP/1.0 200 OK
    // RTSP/1.0 400 Bad Request
    // ANNOUNCE rtsp://x.x.x.x:8060 RTSP/1.0
    // -------------------------------------------------------------------------
    int pos = str.find(RTSP_TERMINATOR);
    if (pos !=std::string::npos) {
        string header = str.substr(0, pos);
        std::vector<string> tokens;
        split(header, " ", tokens);
        //TRACE_L2( "%s: header.length=%d tokens.size=%d", __FUNCTION__, header.length(), tokens.size());
        //for (int i = 0; i < tokens.size(); i++)
        //    TRACE_L2( "%s: %d. '%s'", __FUNCTION__, i, tokens.at(i).c_str());

        if (tokens.size() >= 3) {
            string first = tokens.at(0);

            if (first.compare("ANNOUNCE") == 0) {
                msgType = RTSP_ANNOUNCE;
            } else if (first.compare(0, 5, "RTSP/") == 0) {
                msgType = RTSP_RESPONSE;
                rtspCode = std::stoi(tokens.at(1));
            }
            // parse rest, only if header is valid
            rtspBody = str.substr(pos+2);       // +2 CRLR
            //parse(tokenStr, contents, ":", "\r\n");
        }
    }

    return msgType;
}

int RtspParser::split(const string& str, const string& delim,  std::vector<string>& tokens)
{
    size_t prev = 0, pos = 0;
    do
    {
        pos = str.find(delim, prev);
        if (pos == string::npos)
            pos = str.length();
        string token = str.substr(prev, pos-prev);
        if (!token.empty())
            tokens.push_back(token);
        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());

    return tokens.size();
}

void RtspParser::hexDump(const char* label, const std::string& msg)
{
    std::stringstream ss;
    int32_t max = (msg.length() > 32) ? 32 : msg.length();
    for (int32_t i = 0; i < max; i++) {
        int byte = (uint8_t)msg.at(i);
        ss << std::setfill('0') << std::setw(2) << std::hex <<  byte << " ";
    }
    TRACE_L2("%s: %s", label, ss.str().c_str());
}
