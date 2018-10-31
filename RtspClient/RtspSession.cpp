#include <netdb.h>

#include "RtspSession.h"

#define ANNOUNCEMENT_CHECK_INTERVAL     10
#define RTSP_RESPONSE_WAIT_TIME         3000
#define RTSP_THREAD_SLEEP_MS            1000
#define TR TRACE_L4( "%s: %s:%d", __FUNCTION__, __FILE__, __LINE__);

namespace WPEFramework {
namespace Plugin {

RtspSession::RtspSession()
 : Core::Thread(KB(64), _T("RtspClient"))
 , _parser(_sessionInfo)
 , _requestQueue(64)
 , _responseQueue(64)
 , _isSessionActive(false)
 , _nextSRMHeartbeatMS(0)
 , _nextPumpHeartbearMS(0)
 , _playDelay(2000)
{ TR;
}

RtspSession::~RtspSession()
{ TR;
}

RtspReturnCode
RtspSession::Initialize(const string& hostname, int port)
{   TR;
    RtspReturnCode rc = ERR_OK;

    _isSessionActive = false;
    _nextSRMHeartbeatMS = 0;
    _nextPumpHeartbearMS = 0;

    if (!_srmSocket.isValid()) {
        string hostaddress;
        struct hostent *he = gethostbyname (hostname.c_str());
        if (he == NULL) {
            TRACE_L1 ( "%s: Unable to resolve hostname (%s)", __FUNCTION__, hostaddress.c_str());
        } else {
            hostaddress = inet_ntoa (*((struct in_addr *) he->h_addr_list[0]));
            TRACE_L2( "%s: host=%s srmAddr=%s port=%d", __FUNCTION__, hostname.c_str(), hostaddress.c_str(), port);
        }

        if (_srmSocket.Create() == 0) {
            if (_srmSocket.Connect(hostaddress, port) == 0) {
                TRACE_L1( "%s: Connected, starting messaging thread", __FUNCTION__);
                Core::Thread::Run();
            } else {
                rc = ERR_SESSION_FAILED;
                TRACE_L1( "%s: Failed to connect to SRM socket", __FUNCTION__);
            }
        } else {
            rc = ERR_SESSION_FAILED;
            TRACE_L1( "%s: Failed to create SRM socket", __FUNCTION__);
        }
    } else {
        TRACE_L1( "%s: Initialize failed, session is active", __FUNCTION__);
        rc = ERR_ACTIVE;
    }

    return rc;
}

RtspReturnCode
RtspSession::Terminate()
{
    //Core::Thread::Stop();
    //Core::Thread::Wait (Core::Thread::STOPPED, Core::infinite);
    // XXX: After Stop can't restart, try suspend

    TRACE_L4( "%s: closing SRM socket", __FUNCTION__);
    _srmSocket.Disconnect();

    if (!IsSrmRtspProxy()) {
        TRACE_L4( "%s: closing control socket", __FUNCTION__);
        _controlSocket.Disconnect();
    }
}

RtspReturnCode
RtspSession::open(const string assetId, int start, const string &reqCpeId, const string &remoteIp)
{   TR;
    RtspReturnCode rc = ERR_OK;
    RtspMessage request;
    RtspMessage response;

    if (!_isSessionActive) {
        _sessionInfo.reset();
        // new session, flush any remaining message in the que
        while (!_responseQueue.IsEmpty()){
            TRACE_L1( "%s: Removing old message", __FUNCTION__);
            _responseQueue.Extract(response, 0);
        }

        _isSessionActive = true;
        request.message = _parser.buildSetupRequest(_sessionInfo.srm.address, assetId);
        _requestQueue.Post(request);

        if (_responseQueue.Extract(response, RTSP_RESPONSE_WAIT_TIME)) {
            _parser.processSetupResponse(response.message);

            if (!IsSrmRtspProxy()) {
                _sessionInfo.pump.address = "192.168.2.9";  // XXX: use pump address from Session setup response
                _sessionInfo.pump.port = 8060;
                TRACE_L1( "%s: NOT in rtsp proxy mode, connecting control socket", __FUNCTION__);
                if (_controlSocket.Create() == 0) {
                    if (_controlSocket.Connect(_sessionInfo.pump.address, _sessionInfo.pump.port) == -1) {
                        rc = ERR_CONNECT_FAILED;
                        TRACE_L1( "%s: Failed to connect to socket", __FUNCTION__);
                    }
                } else {
                    rc = ERR_CONNECT_FAILED;
                    TRACE_L1( "%s: Failed to create control socket", __FUNCTION__);
                }
            }
        } else {
            TRACE_L1( "%s: Failed to get Response", __FUNCTION__);
            rc = ERR_TIMED_OUT;
        }

        if (rc == ERR_OK) {
            // -1 - bookmark, 0 - beginning, >0 actual
            int offset;
            if (start == -1)
                offset = _sessionInfo.bookmark;
            else if (start == 0)
                offset = INT_MAX;
            else
                offset = start;
//            play(offset);               // implicit play

            _nextSRMHeartbeatMS  = _sessionInfo.sessionTimeout;
            _nextPumpHeartbearMS = _sessionInfo.ctrlSessionTimeout;
        }
    } else {
        TRACE_L1( "%s: Open failed, session is active", __FUNCTION__);
        rc = ERR_ACTIVE;
    }
    return rc;
}


RtspReturnCode
RtspSession::close()
{   TR;
    RtspReturnCode rc = ERR_OK;
    int reason = 0;
    RtspMessage request;
    RtspMessage response;

    request.message = _parser.buildTeardownRequest(reason);
    _requestQueue.Post(request);
    if (_responseQueue.Extract(response, RTSP_RESPONSE_WAIT_TIME)) {
        _parser.processTeardownResponse(response.message);
        TRACE_L1( "%s: after processTeardownResponse", __FUNCTION__);
    } else {
        TRACE_L1( "%s: Failed to get Response", __FUNCTION__);
        rc = ERR_TIMED_OUT;
    }

    _isSessionActive = false;
    TRACE_L1( "%s: Exiting", __FUNCTION__);

    return rc;
}

RtspReturnCode
RtspSession::play(float scale, int offset)
{   TR;
    RtspReturnCode rc = ERR_OK;

    TRACE_L2( "%s: scale=%f offset=%d", __FUNCTION__, scale, offset);

    RtspMessage request;
    RtspMessage response;

    request.bStreamRequest = true;
    request.message = _parser.buildPlayRequest(scale, offset);
    _requestQueue.Post(request);
    if (_responseQueue.Extract(response, RTSP_RESPONSE_WAIT_TIME)) {
        _parser.processPlayResponse(request.message);
    } else {
        TRACE_L1( "%s: Failed to get Response", __FUNCTION__);
        rc = ERR_TIMED_OUT;
    }

    return rc;
}

RtspReturnCode
RtspSession::check(bool bSRM, int timeout, string *pStr)
{   TR;
    RtspReturnCode rc = ERR_OK;
    int sockRc = 0;
    string response;
    sockRc = getSocket(bSRM).Receive(response, timeout);

    if (response.length()) {
        //hexDump("Response: ", response);
        string responseBody;
        RtspParser::MessageType msgType;
        int rtspRc = _parser.parseResponse(response, responseBody, msgType);
        if (msgType == RtspParser::RTSP_ANNOUNCE) {
            //rc = processAnnouncement(responseBody, bSRM);
        } else if (msgType == RtspParser::RTSP_RESPONSE) {
            if (pStr) {
                *pStr = responseBody;    // Heartbeat response
            } else {
                RtspMessage rtspResp;
                rtspResp.message = responseBody;
                _responseQueue.Post(rtspResp);
            }
        } else {
            TRACE_L1( "%s: UNKNOWN response '%s'", __FUNCTION__, response.c_str());
        }
    }
    return rc;
}

RtspReturnCode
RtspSession::checkResponse(bool bStream)
{ TR
    // Session messages goes to SRM, Stream messages goes to VideoServer
    bool bSRM = (bStream) ? false : true;
    return check(bSRM, RTSP_RESPONSE_WAIT_TIME);
}

RtspReturnCode
RtspSession::checkAnnouncement()
{
    RtspReturnCode rc = ERR_OK;
    if (!IsSrmRtspProxy())
        rc = check(false, ANNOUNCEMENT_CHECK_INTERVAL); // from VOD pump

    rc = check(true, ANNOUNCEMENT_CHECK_INTERVAL);      // from SRM

    return rc;
}

RtspReturnCode
RtspSession::sendHeartbeat(bool bSRM)
{
    RtspReturnCode rc = ERR_OK;
    RtspMessage request;
    RtspMessage response;

    TRACE_L1( "%s: bSRM=%d", __FUNCTION__, bSRM);
    request.message = _parser.buildGetParamRequest(bSRM);
    int sockRc;

    //hexDump(request.message);
    sockRc = getSocket(bSRM).Send(request.message);

    if (sockRc == 0) {
        check(bSRM, RTSP_RESPONSE_WAIT_TIME, &response.message);
        _parser.processGetParamResponse(response.message);
    } else {
       TRACE_L1( "%s: send failed rc=%d", __FUNCTION__, sockRc);
    }
    return rc;
}

RtspReturnCode
RtspSession::sendHeartbeats()
{
    RtspReturnCode rc = ERR_OK;
    int sessionTimeoutMS     = _sessionInfo.sessionTimeout;
    int ctrlSessionTimeoutMS = _sessionInfo.ctrlSessionTimeout;

    TRACE_L4( "%s: _nextSRMHeartbeatMS=%d _nextPumpHeartbearMS=%d sessionTimeoutMS=%d ctrlSessionTimeoutMS=%d",
            __FUNCTION__, _nextSRMHeartbeatMS, _nextPumpHeartbearMS, sessionTimeoutMS, ctrlSessionTimeoutMS);

    // SRM Heartbeat
    if ( !_sessionInfo.sessionId.empty() && sessionTimeoutMS > 0) {
        _nextSRMHeartbeatMS -= RTSP_THREAD_SLEEP_MS;
        if (_nextSRMHeartbeatMS <= 0) {
            rc = sendHeartbeat(true);
            _nextSRMHeartbeatMS = sessionTimeoutMS;
        }
    }

    // VideoServer/pump Heartbeat
    if ( !_sessionInfo.ctrlSessionId.empty() && ctrlSessionTimeoutMS > 0) {
        _nextPumpHeartbearMS -= RTSP_THREAD_SLEEP_MS;
        if (_nextPumpHeartbearMS <= 0) {
            rc = sendHeartbeat(false);
            _nextPumpHeartbearMS = ctrlSessionTimeoutMS;
        }
    }

    return rc;
}


uint32_t RtspSession::Worker ()
{
    TRACE_L1( "%s: Entering ", __FUNCTION__);
    RtspReturnCode rc = ERR_OK;
    int sockRc = 0;

    while (IsRunning() == true) {
        if (_isSessionActive) {
            checkAnnouncement();
            sendHeartbeats();
        }

        if (!_requestQueue.IsEmpty()) {
            TRACE_L2( "%s: Message received ", __FUNCTION__);

            RtspMessage request;
            _requestQueue.Extract(request, 0);
            //_requestQueue.Remove(request);
            bool bSRM = (request.bStreamRequest) ? false : true;

            sockRc = getSocket(bSRM).Send(request.message);
            check(bSRM, RTSP_RESPONSE_WAIT_TIME);
        } else {
            TRACE_L4( "%s: Waiting for request", __FUNCTION__);
            usleep(RTSP_THREAD_SLEEP_MS * 1000);
        }
    }
    TRACE_L1( "%s: Exiting IsRunning=%d", __FUNCTION__, IsRunning());
}

}} // WPEFramework::Plugin

