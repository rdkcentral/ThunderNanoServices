#include <netdb.h>

#include "Module.h"
#include "RtspSession.h"


namespace WPEFramework {
namespace Plugin {

RtspSession::RtspSession(RtspSession::AnnouncementHandler& handler)
 : _announcementHandler(handler)
 , _parser(_sessionInfo)
 , _requestQueue(64)
 , _responseQueue(64)
 , _isSessionActive(false)
 , _nextSRMHeartbeatMS(0)
 , _nextPumpHeartbeatMS(0)
 , _playDelay(2000)
 , _srmSocket(nullptr)
 , _controlSocket(nullptr)
 , _heartbeatTimer(Core::Thread::DefaultStackSize(), _T("RtspHeartbeatTimer"))
{
}

RtspSession::~RtspSession()
{
}

RtspReturnCode RtspSession::Initialize(const string& hostname, uint16_t port)
{
    RtspReturnCode rc = ERR_OK;

    _adminLock.Lock();
    if (!(_srmSocket && _srmSocket->IsOpen())) {
        _isSessionActive = false;
        _nextSRMHeartbeatMS = 0;
        _nextPumpHeartbeatMS = 0;

        _sessionInfo.srm.name = hostname;
        _sessionInfo.srm.port = port;
        _remote = Core::NodeId(_sessionInfo.srm.name.c_str(), _sessionInfo.srm.port);
        _srmSocket = new RtspSession::Socket(_local, _remote, *this);
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

RtspReturnCode RtspSession::Terminate()
{
    _adminLock.Lock();

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

RtspReturnCode RtspSession::Send(const RtspMessagePtr& request)
{
    _requestQueue.Post(request);
    GetSocket(request->bSRM).Trigger();;
}

uint64_t RtspSession::Timed(const uint64_t scheduledTime)
{
    if (_isSessionActive) {
        _sessionInfo.npt += NptUpdateInterwal * _sessionInfo.scale;
        TRACE(Trace::Information, ("npt=%.3f_nextSRMHeartbeat=%d _nextPumpHeartbeat=%d sessionTimeout=%d ctrlSessionTimeout=%d",
        _sessionInfo.npt, _nextSRMHeartbeatMS, _nextPumpHeartbeatMS, _sessionInfo.sessionTimeout, _sessionInfo.ctrlSessionTimeout));

        SendHeartbeats();

        Core::Time NextTick = Core::Time::Now();
        NextTick.Add(NptUpdateInterwal);
        _heartbeatTimer.Schedule(NextTick.Ticks(), HeartbeatTimer(*this));
    }
}

RtspReturnCode RtspSession::Open(const string assetId, uint32_t position, const string &reqCpeId, const string &remoteIp)
{
    RtspReturnCode rc = ERR_OK;
    RtspMessagePtr response;

    if (!_isSessionActive) {
        _sessionInfo.reset();
        // new session, flush any remaining message in the queue
        while (!_responseQueue.IsEmpty()){
            TRACE_L1( "%s: Removing old message", __FUNCTION__);
            _responseQueue.Extract(response, 0);
        }

        _isSessionActive = true;
        RtspMessagePtr request = _parser.BuildSetupRequest(_sessionInfo.srm.name, assetId);
        Send(request);

        if (_responseQueue.Extract(response, ResponseWaitTime)) {
            _adminLock.Lock();
            _parser.ProcessSetupResponse(response->message);

            Core::Time NextTick = Core::Time::Now();
            NextTick.Add(NptUpdateInterwal);
            _heartbeatTimer.Schedule(NextTick.Ticks(), HeartbeatTimer(*this));

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
            _nextPumpHeartbeatMS = _sessionInfo.ctrlSessionTimeout;

            // implicit play
            Play(1.0, (position == 0) ? _sessionInfo.bookmark : position);
        }
    } else {
        TRACE_L1( "%s: Open failed, session is active", __FUNCTION__);
        rc = ERR_ACTIVE;
    }
    return rc;
}


RtspReturnCode RtspSession::Close()
{
    RtspReturnCode rc = ERR_OK;
    int reason = 0;
    RtspMessagePtr response;

    if (_isSessionActive) {
        RtspMessagePtr request = _parser.BuildTeardownRequest(reason);
        Send(request);
        if (_responseQueue.Extract(response, ResponseWaitTime)) {
            _parser.ProcessTeardownResponse(response->message);
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

RtspReturnCode RtspSession::Play(float scale, uint32_t position)
{
    RtspReturnCode rc = ERR_OK;

    if (_isSessionActive) {
        TRACE_L2( "%s: scale=%f offset=%d", __FUNCTION__, scale, position);

        RtspMessagePtr response;

        RtspMessagePtr request = _parser.BuildPlayRequest(scale, position);
        Send(request);
        if (_responseQueue.Extract(response, ResponseWaitTime)) {
            _parser.ProcessPlayResponse(response->message);
        } else {
            TRACE_L1( "%s: Failed to get Response", __FUNCTION__);
            rc = ERR_TIMED_OUT;
        }
    } else {
        rc = ERR_NO_ACTIVE_SESSION;
    }
    return rc;
}

RtspReturnCode RtspSession::Get(const string name, string &value) const
{
    RtspReturnCode rc = ERR_OK;

    return rc;
}

RtspReturnCode RtspSession::Set(const string& name, const string& value)
{
    RtspReturnCode rc = ERR_OK;

    return rc;
}


RtspReturnCode RtspSession::ProcessResponse(const string &responseStr, bool bSRM)
{
    RtspReturnCode rc = ERR_OK;

    if (responseStr.length()) {
        RtspMessagePtr response = _parser.ParseResponse(responseStr);
        if (dynamic_cast<RtspAnnounce*>(response.get()) != nullptr) {
            RtspAnnounce& announcement = *dynamic_cast<RtspAnnounce*>(response.get());
            // rc = sendResponse(respSeq, bSRM);

            // reset scale & npt
            if (announcement.GetCode() == RtspAnnounce::EosReached) {
                _sessionInfo.scale = 1;
                _sessionInfo.npt = 0;
            }
            _announcementHandler.announce(announcement);
        } else if (dynamic_cast<RtspResponse*>(response.get()) != nullptr) {
            _responseQueue.Post(response);
        } else {
            TRACE_L1( "%s: UNKNOWN response '%s'", __FUNCTION__, responseStr.c_str());
        }
    }
    return rc;
}

RtspReturnCode RtspSession::SendResponse(int respSeq, bool bSRM)
{
    RtspReturnCode rc = ERR_OK;
    RtspMessagePtr  request = _parser.BuildResponse(respSeq, bSRM);

    TRACE_L1( "%s: Sending Announcement Response", __FUNCTION__);
    Send(request);

    return rc;
}

RtspReturnCode RtspSession::SendHeartbeat(bool bSRM)
{
    RtspReturnCode rc = ERR_OK;
    RtspMessagePtr response;

    RtspMessagePtr request = _parser.BuildGetParamRequest(bSRM);
    Send(request);

    if (_responseQueue.Extract(response, ResponseWaitTime)) {
        _parser.ProcessGetParamResponse(response->message);
    } else {
        TRACE_L1( "%s: Failed to get Response", __FUNCTION__);
        rc = ERR_TIMED_OUT;
    }

    return rc;
}

RtspReturnCode RtspSession::SendHeartbeats()
{
    RtspReturnCode rc = ERR_OK;
    int sessionTimeoutMS     = _sessionInfo.sessionTimeout;
    int ctrlSessionTimeoutMS = _sessionInfo.ctrlSessionTimeout;

    // SRM Heartbeat
    if ( !_sessionInfo.sessionId.empty() && sessionTimeoutMS > 0) {
        _nextSRMHeartbeatMS -= NptUpdateInterwal;
        if (_nextSRMHeartbeatMS <= 0) {
            rc = SendHeartbeat(true);
            _nextSRMHeartbeatMS = sessionTimeoutMS;
        }
    }

    // VideoServer/pump Heartbeat
    if ( !_sessionInfo.ctrlSessionId.empty() && ctrlSessionTimeoutMS > 0) {
        _nextPumpHeartbeatMS -= NptUpdateInterwal;
        if (_nextPumpHeartbeatMS <= 0) {
            rc = SendHeartbeat(false);
            _nextPumpHeartbeatMS = ctrlSessionTimeoutMS;
        }
    }

    return rc;
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

uint16_t RtspSession::Socket::SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
{
    TRACE_L4( "%s: _requestQueue.IsEmpty=%d ", __FUNCTION__, _rtspSession._requestQueue.IsEmpty());

    uint16_t len = 0;
    if (!_rtspSession._requestQueue.IsEmpty()) {
        RtspMessagePtr request;
        _rtspSession._requestQueue.Extract(request, 0);
        len = request->message.size();
        memcpy(dataFrame, request->message.c_str(), len);
        TRACE(Trace::Information, ("%s: maxSendSize=%d bytesToSend=%d", __FUNCTION__, maxSendSize, len));
    }

    return len;
}

uint16_t RtspSession::Socket::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
{
    TRACE(Trace::Information, ("%s: receivedSize=%d", __FUNCTION__, receivedSize));
    string response((char *)dataFrame, receivedSize);
    bool bSRM = (_rtspSession._srmSocket == this);
    _rtspSession.ProcessResponse(response, bSRM);
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
