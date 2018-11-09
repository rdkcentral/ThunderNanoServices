#include <netdb.h>

#include "Module.h"
#include "RtspSession.h"

#define ANNOUNCEMENT_CHECK_INTERVAL     10
#define RTSP_RESPONSE_WAIT_TIME         3000
#define RTSP_THREAD_SLEEP_MS            1000

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
 , _srmSocket(nullptr)
 , _controlSocket(nullptr)
{
    Core::Thread::Run();
}

RtspSession::~RtspSession()
{
}

RtspReturnCode
RtspSession::Initialize(const string& hostname, uint16_t port)
{
    RtspReturnCode rc = ERR_OK;

    _adminLock.Lock();
    if (!(_srmSocket && _srmSocket->IsOpen())) {
        _isSessionActive = false;
        _nextSRMHeartbeatMS = 0;
        _nextPumpHeartbearMS = 0;

        // Core::Thread::Run();
        _sessionInfo.srm.name = hostname;
        _sessionInfo.srm.port = port;
        remote = Core::NodeId(_sessionInfo.srm.name.c_str(), _sessionInfo.srm.port);
        _srmSocket = new RtspSession::Socket(local, remote, *this);
        if (_srmSocket->State() == 0) {
            TRACE_L1( "%s: SRM Socket failed. State=%x", __FUNCTION__, _srmSocket->State());
            rc = ERR_SESSION_FAILED;
        } else {
            TRACE_L1( "%s: srmSock->State=%x", __FUNCTION__, _srmSocket->State());
        }
    } else {
        TRACE_L1( "%s: Initialize failed, session is active", __FUNCTION__);
        rc = ERR_ACTIVE;
    }
    _adminLock.Unlock();

    return rc;
}

RtspReturnCode
RtspSession::Terminate()
{
    _adminLock.Lock();
    //Core::Thread::Stop();
    //Core::Thread::Wait (Core::Thread::STOPPED, Core::infinite);
    // XXX: After Stop can't restart, try suspend

    TRACE_L1( "%s: closing SRM socket", __FUNCTION__);
    delete _srmSocket;
    _srmSocket = nullptr;
    if (!IsSrmRtspProxy()) {
        TRACE_L4( "%s: closing control socket", __FUNCTION__);
        delete _controlSocket;
        _controlSocket = nullptr;
    }
    _adminLock.Unlock();
}

RtspReturnCode
RtspSession::Open(const string assetId, uint32_t position, const string &reqCpeId, const string &remoteIp)
{
    RtspReturnCode rc = ERR_OK;
    RtspMessage request;
    RtspMessage response;

    if (!_isSessionActive) {
        _sessionInfo.reset();
        // new session, flush any remaining message in the queue
        while (!_responseQueue.IsEmpty()){
            TRACE_L1( "%s: Removing old message", __FUNCTION__);
            _responseQueue.Extract(response, 0);
        }

        _isSessionActive = true;
        request.message = _parser.BuildSetupRequest(_sessionInfo.srm.name, assetId);
        _requestQueue.Post(request);

        if (_responseQueue.Extract(response, RTSP_RESPONSE_WAIT_TIME)) {
            _adminLock.Lock();
            _parser.ProcessSetupResponse(response.message);

            if (!IsSrmRtspProxy()) {
                TRACE_L1( "%s: NOT in rtsp proxy mode, connecting control socket (%s:%d)",
                    __FUNCTION__, _sessionInfo.pump.address.c_str(), _sessionInfo.pump.port);
                _controlSocket = new RtspSession::Socket(Core::NodeId(), Core::NodeId(_sessionInfo.pump.address.c_str(), _sessionInfo.pump.port), *this);
                if (_controlSocket->State() == 0) {
                    TRACE_L1( "%s: Control Socket failed. State=%x", __FUNCTION__, _controlSocket->State());
                    rc = ERR_SESSION_FAILED;
                } else {
                    TRACE_L1( "%s: _controlSocket->State=%x", __FUNCTION__, _controlSocket->State());
                }
            }
            _adminLock.Unlock();
        } else {
            TRACE_L1( "%s: Failed to get Response", __FUNCTION__);
            rc = ERR_TIMED_OUT;
        }

        if (rc == ERR_OK) {
            _nextSRMHeartbeatMS  = _sessionInfo.sessionTimeout;
            _nextPumpHeartbearMS = _sessionInfo.ctrlSessionTimeout;

            // implicit play
            Play(1.0, (position == 0) ? _sessionInfo.bookmark : position);
        }
    } else {
        TRACE_L1( "%s: Open failed, session is active", __FUNCTION__);
        rc = ERR_ACTIVE;
    }
    return rc;
}


RtspReturnCode
RtspSession::Close()
{
    RtspReturnCode rc = ERR_OK;
    int reason = 0;
    RtspMessage request;
    RtspMessage response;

    if (_isSessionActive) {
        request.message = _parser.BuildTeardownRequest(reason);
        _requestQueue.Post(request);
        if (_responseQueue.Extract(response, RTSP_RESPONSE_WAIT_TIME)) {
            _parser.ProcessTeardownResponse(response.message);
        } else {
            TRACE_L1( "%s: Failed to get Response", __FUNCTION__);
            rc = ERR_TIMED_OUT;
        }

        _isSessionActive = false;
    } else {
        rc = ERR_NO_ACTIVE_SESSION;
    }


    return rc;
}

RtspReturnCode
RtspSession::Play(float scale, uint32_t position)
{
    RtspReturnCode rc = ERR_OK;

    if (_isSessionActive) {
        TRACE_L2( "%s: scale=%f offset=%d", __FUNCTION__, scale, position);

        RtspMessage request;
        RtspMessage response;

        request.bStreamRequest = true;
        request.message = _parser.BuildPlayRequest(scale, position);
        _requestQueue.Post(request);
        if (_responseQueue.Extract(response, RTSP_RESPONSE_WAIT_TIME)) {
            _parser.ProcessPlayResponse(request.message);
        } else {
            TRACE_L1( "%s: Failed to get Response", __FUNCTION__);
            rc = ERR_TIMED_OUT;
        }
    } else {
        rc = ERR_NO_ACTIVE_SESSION;
    }
    return rc;
}


RtspReturnCode
RtspSession::processResponse(const string &response)
{
    RtspReturnCode rc = ERR_OK;

    if (response.length()) {
        string responseBody;
        RtspParser::MessageType msgType;
        int rtspRc = _parser.ParseResponse(response, responseBody, msgType);
        if (msgType == RtspParser::RTSP_ANNOUNCE) {
            //rc = ProcessAnnouncement(responseBody, bSRM);
        } else if (msgType == RtspParser::RTSP_RESPONSE) {
            RtspMessage rtspResp;
            rtspResp.message = responseBody;
            _responseQueue.Post(rtspResp);
        } else {
            TRACE_L1( "%s: UNKNOWN response '%s'", __FUNCTION__, response.c_str());
        }
    }
    return rc;
}

RtspReturnCode
RtspSession::SendHeartbeat(bool bSRM)
{
    RtspReturnCode rc = ERR_OK;
    RtspMessage request;
    RtspMessage response;

    request.message = _parser.BuildGetParamRequest(bSRM);
    int sockRc;

    _requestQueue.Post(request);

    if (_responseQueue.Extract(response, RTSP_RESPONSE_WAIT_TIME)) {
        _parser.ProcessGetParamResponse(response.message);
    } else {
        TRACE_L1( "%s: Failed to get Response", __FUNCTION__);
        rc = ERR_TIMED_OUT;
    }

    return rc;
}

RtspReturnCode
RtspSession::SendHeartbeats()
{
    RtspReturnCode rc = ERR_OK;
    int sessionTimeoutMS     = _sessionInfo.sessionTimeout;
    int ctrlSessionTimeoutMS = _sessionInfo.ctrlSessionTimeout;

    // SRM Heartbeat
    if ( !_sessionInfo.sessionId.empty() && sessionTimeoutMS > 0) {
        _nextSRMHeartbeatMS -= RTSP_THREAD_SLEEP_MS;
        if (_nextSRMHeartbeatMS <= 0) {
            rc = SendHeartbeat(true);
            _nextSRMHeartbeatMS = sessionTimeoutMS;
        }
    }

    // VideoServer/pump Heartbeat
    if ( !_sessionInfo.ctrlSessionId.empty() && ctrlSessionTimeoutMS > 0) {
        _nextPumpHeartbearMS -= RTSP_THREAD_SLEEP_MS;
        if (_nextPumpHeartbearMS <= 0) {
            rc = SendHeartbeat(false);
            _nextPumpHeartbearMS = ctrlSessionTimeoutMS;
        }
    }

    return rc;
}


uint32_t RtspSession::Worker ()
{
    TRACE_L2( "%s: Entering ", __FUNCTION__);
    RtspReturnCode rc = ERR_OK;
    int sockRc = 0;

    while (IsRunning() == true) {
        if (_isSessionActive) {
            _sessionInfo.npt += RTSP_THREAD_SLEEP_MS * _sessionInfo.scale;
            TRACE(Trace::Information, ("npt=%.3f_nextSRMHeartbeat=%d _nextPumpHeartbear=%d sessionTimeout=%d ctrlSessionTimeout=%d",
                _sessionInfo.npt, _nextSRMHeartbeatMS, _nextPumpHeartbearMS, _sessionInfo.sessionTimeout, _sessionInfo.ctrlSessionTimeout));

            SendHeartbeats();

            if (!_requestQueue.IsEmpty()) {
                TRACE_L2( "%s: Message received ", __FUNCTION__);

                RtspMessage request;
                _requestQueue.Extract(request, 0);
                bool bSRM = (request.bStreamRequest) ? false : true;
                sockRc = GetSocket(bSRM).Send(request.message);
            } else {
                usleep(RTSP_THREAD_SLEEP_MS * 1000);
            }
        } else {
            usleep(RTSP_THREAD_SLEEP_MS * 1000);
        }
    }
    TRACE_L2( "%s: Exiting IsRunning=%d", __FUNCTION__, IsRunning());
}


RtspSession::Socket::Socket(const Core::NodeId &local, const Core::NodeId &remote, RtspSession& rtspSession)
    : Core::SocketStream(false, local, remote, 4096, 4096)
    , _rtspSession(rtspSession)
{
    Open(1000, "");
};

RtspSession::Socket::~Socket()
{
    Close(1000);
};

uint16_t RtspSession::Socket::Send(string message)
{
    data = message;
    Trigger();
    return 0;
}

uint16_t RtspSession::Socket::SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
{
    uint16_t len = data.size();
    TRACE(Trace::Information, ("%s: maxSendSize=%d bytesToSend=%d", __FUNCTION__, maxSendSize, len));
    memcpy(dataFrame, data.c_str(), len);
    data = string();
    return len;
}

uint16_t RtspSession::Socket::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
{
    TRACE(Trace::Information, ("%s: receivedSize=%d", __FUNCTION__, receivedSize));
    string response((char *)dataFrame, receivedSize);
    _rtspSession.processResponse(response);
    return receivedSize;
}

void RtspSession::Socket::StateChange()
{
    if (State() == 0) {
        TRACE_L1( "%s: Lost connection", __FUNCTION__, State());
        // XXX: Not doing anything if connection is lost, playback might still work but trickplay wont work.
    } else {
        TRACE_L1( "%s: State=%d", __FUNCTION__, State());
    }
}

}} // WPEFramework::Plugin
