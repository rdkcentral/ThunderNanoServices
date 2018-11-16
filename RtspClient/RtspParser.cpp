#include <sstream>
#include <vector>
#include <iomanip>

#include <plugins/Logging.h>

#include "RtspParser.h"

#define SEC2MS(s)                       (s*1000)
#define MS2SEC(ms)                      (ms/1000)

namespace WPEFramework {
namespace Plugin {

unsigned int RtspParser::_sequence = 0;

RtspParser::RtspParser(RtspSessionInfo& info)
    : _sessionInfo(info)
{
    TRACE_L2( "%s: %s:%d", __FUNCTION__, __FILE__, __LINE__);
}

RtspMessagePtr RtspParser::BuildSetupRequest(const std::string &server, const std::string &assetId)
{
    RtspMessagePtr request = RtspMessagePtr(new RtspRequst);
    std::stringstream ss;

    ss << "SETUP rtsp://" << server << "/" << assetId << "?";
    ss << "VODServingAreaId=1099" << "&";
    ss << "StbId=943BB162A323&";
    ss << "CADeviceId=943BB162A323";
    ss << " RTSP/1.0" <<  RtspLineTerminator;
    ss << "CSeq:" << ++_sequence << RtspLineTerminator;
    ss << "User-Agent: Metro" <<  RtspLineTerminator;
    ss << "Transport: MP2T/DVBC/QAM;unicast;" <<  RtspLineTerminator;
    ss << RtspLineTerminator;

    HexDump("SETUP", ss.str());

    request->message = ss.str();

    return request;
}

RtspMessagePtr RtspParser::BuildPlayRequest(float scale, uint32_t position)
{
    RtspMessagePtr request = RtspMessagePtr(new RtspRequst);
    std::stringstream ss;
    string sessionId;
    string cmd = (scale == 0) ? "PAUSE" : "PLAY";

    if (_sessionInfo.bSrmIsRtspProxy) {
        sessionId = _sessionInfo.sessionId;
        request->bSRM = true;
    } else {
        sessionId = _sessionInfo.ctrlSessionId;
        request->bSRM = false;
    }
    ss << cmd << " * RTSP/1.0" <<  RtspLineTerminator;
    ss << "CSeq:" << ++_sequence << RtspLineTerminator;
    ss << "Session:" << sessionId << RtspLineTerminator;
    ss << "Range: npt=" << position << RtspLineTerminator;
    ss << "Scale: " << scale << RtspLineTerminator;
    ss << RtspLineTerminator;

    HexDump("PLAY", ss.str());

    request->message = ss.str();

    return request;
}

RtspMessagePtr RtspParser::BuildGetParamRequest(bool bSRM)
{
    RtspMessagePtr request = RtspMessagePtr(new RtspRequst);
    string strParams;
    string sessId;

    if (bSRM) {
        sessId = _sessionInfo.sessionId;
    } else {
        sessId = _sessionInfo.ctrlSessionId;
        std::stringstream ssParams;
        ssParams << "Position" <<  RtspLineTerminator ;
        ssParams << "Scale" <<  RtspLineTerminator;
        ssParams << "stream_state" <<  RtspLineTerminator;
        strParams = ssParams.str();
    }

    std::stringstream ss;
    ss << "GET_PARAMETER * RTSP/1.0" <<  RtspLineTerminator;
    ss << "CSeq:" << ++_sequence << RtspLineTerminator;
    ss << "Session:" << sessId << RtspLineTerminator;
    ss << "Content-Type: text/parameters" <<  RtspLineTerminator;
    ss << "Content-Length: " << strParams.length() << RtspLineTerminator;
    if (strParams.length()) {
        ss << RtspLineTerminator;
        ss << strParams;
    }
    ss << RtspLineTerminator;

    HexDump("GETPARAM", ss.str());

    request->message = ss.str();

    return request;
}

RtspMessagePtr RtspParser::BuildTeardownRequest(int reason)
{
    RtspMessagePtr request = RtspMessagePtr(new RtspRequst);
    std::stringstream ss;
    string strReason = "Cleint Intiated";

    ss << "TEARDOWN * RTSP/1.0" <<  RtspLineTerminator;
    ss << "CSeq:" << ++_sequence << RtspLineTerminator;
    ss << "Session:" << _sessionInfo.sessionId << RtspLineTerminator;
    ss << "Reason:" << reason << " " << strReason << RtspLineTerminator;
    ss << RtspLineTerminator;

    HexDump("TEARDOWN", ss.str());

    request->message = ss.str();

    return request;
}

RtspMessagePtr RtspParser::BuildResponse(int respSeq, bool bSRM)
{
    RtspMessagePtr request = RtspMessagePtr(new RtspRequst);
    string sessId = (bSRM) ? _sessionInfo.sessionId : _sessionInfo.ctrlSessionId;

    std::stringstream ss;
    ss << "RTSP/1.0 200 OK" <<  RtspLineTerminator;
    ss << "CSeq:" << respSeq << RtspLineTerminator;
    ss << "Session:" << _sessionInfo.sessionId << RtspLineTerminator;
    ss << RtspLineTerminator;
    ss << RtspLineTerminator;

    HexDump("ANNOUNCERESP", ss.str());

    request->message = ss.str();

    return request;
}

int RtspParser::ProcessSetupResponse(const std::string &response)
{
    NAMED_ARRAY setupMap;               // entire response
    NAMED_ARRAY params;                 // single line
    Parse(response, setupMap, RtspLineTerminator, ": ");

    string sess = setupMap["Session"];
    TRACE_L2( "%s: session id='%s'", __FUNCTION__, sess.c_str());
    if (sess.find(";") == string::npos) {
        _sessionInfo.sessionId = sess;
        _sessionInfo.sessionTimeout = SEC2MS(_sessionInfo.defaultSessionTimeout);
        TRACE_L2( "%s: using default sessionTimeout %d", __FUNCTION__, _sessionInfo.defaultSessionTimeout);
    } else {                            // contains heartbeat
        Parse(sess, params, ";", "=");
        NAMED_ARRAY::iterator it=params.begin();
        _sessionInfo.sessionId = it->first;

        if (params.size() > 1 )
            _sessionInfo.sessionTimeout = SEC2MS(atoi(params["timeout"].c_str()));
    }

    sess = setupMap["ControlSession"];
    if (sess.size()) {
        if (sess.find(";") == string::npos) {
            _sessionInfo.ctrlSessionId = sess;
            _sessionInfo.ctrlSessionTimeout = SEC2MS(_sessionInfo.defaultCtrlSessionTimeout);
            TRACE_L2( "%s: using default ctrlSessionTimeout %d", __FUNCTION__, _sessionInfo.defaultCtrlSessionTimeout);
        } else {
            Parse(sess, params, ";", "=");
            NAMED_ARRAY::iterator it=params.begin();
            _sessionInfo.ctrlSessionId = it->first;

            if (params.size() > 1 )
                _sessionInfo.ctrlSessionTimeout = SEC2MS(atoi(params["timeout"].c_str()));
        }

        if (_sessionInfo.sessionId.compare(_sessionInfo.ctrlSessionId) == 0)  // XXX: check IP Addr ???
            _sessionInfo.bSrmIsRtspProxy = true;
        else
            _sessionInfo.bSrmIsRtspProxy = false;
    }

    string location = setupMap["Location"];
    string chan = setupMap["Tuning"];
    Parse(chan, params, ";", "=");
    _sessionInfo.frequency  = atoi(params["frequency"].c_str()) * 100;
    _sessionInfo.modulation = atoi(params["modulation"].c_str());
    _sessionInfo.symbolRate = atoi(params["symbol_rate"].c_str());

    string tune = setupMap["Channel"];
    Parse(tune, params, ";", "=");
    _sessionInfo.programNum = atoi(params["Svcid"].c_str());

    _sessionInfo.bookmark = atof(setupMap["Bookmark"].c_str());
    _sessionInfo.duration = atoi(setupMap["Duration"].c_str());

    TRACE_L2( "%s: f=%d p=%d m=%d s=%d bookmark=%f duration=%d",
        __FUNCTION__, _sessionInfo.frequency, _sessionInfo.programNum, _sessionInfo.modulation, _sessionInfo.symbolRate, _sessionInfo.bookmark, _sessionInfo.duration);
}

void RtspParser::UpdateNPT(NAMED_ARRAY &playMap)
{
    float nptStart = 0, nptEnd = 0;
    float oldScale = _sessionInfo.scale;
    float oldNPT = _sessionInfo.npt;

    NAMED_ARRAY::iterator it = playMap.find("Scale");
    if (it != playMap.end())
        _sessionInfo.scale = atof(playMap["Scale"].c_str());

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

        _sessionInfo.npt = SEC2MS(nptStart);
        TRACE_L2( "%s: npt=%6.2f scale=%2.2f oldNPT=%6.2f oldScale=%2.2f", __FUNCTION__,  SEC2MS(_sessionInfo.npt), _sessionInfo.scale, oldNPT, oldScale);
    }
}

int RtspParser::ProcessPlayResponse(const std::string &response)
{
    NAMED_ARRAY playMap;
    Parse(response, playMap, RtspLineTerminator, ": ");
    UpdateNPT(playMap);
}

int RtspParser::ProcessGetParamResponse(const std::string &response)
{
    NAMED_ARRAY playMap;
    Parse(response, playMap, RtspLineTerminator, ": ");
    UpdateNPT(playMap);
}

int RtspParser::ProcessTeardownResponse(const std::string &response)
{
    NAMED_ARRAY playMap;
    Parse(response, playMap, RtspLineTerminator, ": ");
    //
}

void RtspParser::Parse(const std::string &str,  NAMED_ARRAY &contents, const string &sep1, const string &sep2)
{
    TRACE_L4( "%s: size=%d input='%s'", __FUNCTION__, str.size(), str.c_str());
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
        if (pos2 == string::npos) {
            string left = sub.substr(0, pos2);
            contents[left] = "";
        } else {
            string left = sub.substr(0, pos2);
            string right = sub.substr(pos2+sep2.size());
            TRACE_L4("%s:     left=%s right=%s pos=%u", __FUNCTION__, left.c_str(), right.c_str(), pos2);
            contents[left] = right;
        }

        start = pos+sep1.size();
        if (start >= str.size())
            done = true;
    }

    TRACE_L4( "%s: contents.size=%d", __FUNCTION__, contents.size());
    for (NAMED_ARRAY::iterator it=contents.begin(); it!=contents.end(); ++it)
        TRACE_L4( "%s: %s => '%s'", __FUNCTION__, it->first.c_str(), it->second.c_str());
}

RtspMessagePtr RtspParser::ParseResponse(const std::string str)
{
    int rtspCode = 0;
    RtspMessagePtr response;

    HexDump("Response: ", str);
    // -------------------------------------------------------------------------
    // RTSP/1.0 200 OK
    // RTSP/1.0 400 Bad Request
    // ANNOUNCE rtsp://x.x.x.x:8060 RTSP/1.0
    // -------------------------------------------------------------------------
    int pos = str.find(RtspLineTerminator);
    if (pos !=std::string::npos) {
        string header = str.substr(0, pos);
        std::vector<string> tokens;
        Split(header, " ", tokens);
        //TRACE_L2( "%s: header.length=%d tokens.size=%d", __FUNCTION__, header.length(), tokens.size());
        //for (int i = 0; i < tokens.size(); i++)
        //    TRACE_L2( "%s: %d. '%s'", __FUNCTION__, i, tokens.at(i).c_str());

        // Parse rest, only if the header is valid
        if (tokens.size() >= 3) {
            string first = tokens.at(0);
            string rtspBody = str.substr(pos+2);       // +2 CRLR

            if (first.compare("ANNOUNCE") == 0) {
                response = ParseAnnouncement(rtspBody, 0);
            } else if (first.compare(0, 5, "RTSP/") == 0) {
                rtspCode = std::stoi(tokens.at(1));
                response = RtspMessagePtr(new RtspResponse(rtspCode));
                response->message = rtspBody;
            }
            //Parse(tokenStr, contents, ":", RtspLineTerminator);
        }
    }

    return response;
}

RtspMessagePtr RtspParser::ParseAnnouncement(const std::string &response, bool bSRM)
{
    /*
        contents.size=3
        CSeq => '6'
        Notice => '2104 "Start-of-Stream Reached" event-date=20160623T231007Z'
        Session => '2709130937-52547519'
    */
    int code = 0;
    string reason;
    NAMED_ARRAY announceMap;
    RtspMessage::Type msgType;
    Parse(response, announceMap, RtspLineTerminator, ": ");
    if (announceMap.size()) {
        int respSeq = atoi(announceMap["CSeq"].c_str());
        TRACE_L2( "%s: respSeq=%d", __FUNCTION__, respSeq);

        string notice = announceMap["Notice"];
        size_t pos = notice.find(' ');
        if (pos != string::npos) {
            size_t pos2;

            string strCode = notice.substr(0, pos);
            code = atoi(strCode.c_str());

            pos = notice.find('"');
            if (pos != string::npos) {
                pos2 = notice.find('"', pos+1);
                if (pos2 != string::npos) {
                    reason = notice.substr(pos+1, pos2-pos);
                }
            }
        }
    } else {
        TRACE_L1( "%s: ANNOUNCEMENT without body", __FUNCTION__, response.c_str());
    }

    return RtspMessagePtr(new RtspAnnounce(code, reason));
}


int RtspParser::Split(const string& str, const string& delim,  std::vector<string>& tokens)
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

void RtspParser::HexDump(const char* label, const std::string& msg, uint16_t charsPerLine)
{
    std::stringstream ssHex, ss;
    for (int32_t i = 0; i < msg.length(); i++) {
        int byte = (uint8_t)msg.at(i);
        ssHex << std::setfill('0') << std::setw(2) << std::hex <<  byte << " ";
        ss << char((byte < 32) ? '.' : byte);

        if (!((i+1) % charsPerLine)) {
            TRACE_L2("%s: %s %s", label, ssHex.str().c_str(), ss.str().c_str());
            ss.str(std::string());
            ssHex.str(std::string());
        }
    }
    TRACE_L2("%s: %s %s", label, ssHex.str().c_str(), ss.str().c_str());
}

}} // WPEFramework::Plugin
