/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RTSPSESSION_H
#define RTSPSESSION_H

#include <linux/netlink.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <core/NodeId.h>
#include <core/Queue.h>
#include <core/SocketPort.h>
#include <core/Timer.h>

#include "RtspCommon.h"
#include "RtspParser.h"

namespace WPEFramework {
namespace Plugin {

    typedef Core::QueueType<RtspMessagePtr> RequestQueue;
    typedef Core::QueueType<RtspMessagePtr> ResponseQueue;

    class RtspSession {
    public:
        class Socket : public Core::SocketStream {
        public:
            Socket(const Core::NodeId& local, const Core::NodeId& remote, RtspSession& rtspSession);
            virtual ~Socket();
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize);
            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize);
            void StateChange();

        private:
            RtspSession& _rtspSession;
        };

        class AnnouncementHandler {
        public:
            virtual void announce(const RtspAnnounce& announcement) = 0;
        };

        class HeartbeatTimer {
        public:
            HeartbeatTimer(RtspSession& parent)
                : _parent(&parent)
            {
            }
            HeartbeatTimer(const HeartbeatTimer& copy)
                : _parent(copy._parent)
            {
            }
            ~HeartbeatTimer()
            {
            }

            HeartbeatTimer& operator=(const HeartbeatTimer& RHS)
            {
                _parent = RHS._parent;
                return (*this);
            }

        public:
            uint64_t Timed(const uint64_t scheduledTime)
            {
                ASSERT(_parent != nullptr);
                return (_parent->Timed(scheduledTime));
            }

        private:
            RtspSession* _parent;
        };

    public:
        RtspSession(RtspSession::AnnouncementHandler& handler);
        ~RtspSession();
        RtspReturnCode Initialize(const string& hostname, uint16_t port);
        RtspReturnCode Terminate();
        RtspReturnCode Open(const string assetId, uint32_t position = 0, const string& reqCpeId = "", const string& remoteIp = "");
        RtspReturnCode Close();
        RtspReturnCode Play(float scale, uint32_t position = 0);
        RtspReturnCode Get(const string name, string& value) const;
        RtspReturnCode Set(const string& name, const string& value);

        RtspReturnCode Send(const RtspMessagePtr& request);
        RtspReturnCode SendHeartbeat(bool bSRM);
        RtspReturnCode SendHeartbeats();

        RtspReturnCode ProcessResponse(const string& response, bool bSRM);
        RtspReturnCode ProcessAnnouncement(const std::string& response, bool bSRM);
        RtspReturnCode SendResponse(int respSeq, bool bSRM);
        RtspReturnCode SendAnnouncement(int code, const string& reason);

        uint64_t Timed(const uint64_t scheduledTime);

    private:
        inline RtspSession::Socket& GetSocket(bool bSRM)
        {
            return (bSRM || _sessionInfo.bSrmIsRtspProxy) ? *_srmSocket : *_controlSocket;
        }

        inline bool IsSrmRtspProxy()
        {
            return _sessionInfo.bSrmIsRtspProxy;
        }

    private:
        static constexpr uint16_t ResponseWaitTime = 3000;
        static constexpr uint16_t NptUpdateInterwal = 1000;

        RtspSession::AnnouncementHandler& _announcementHandler;

        Core::NodeId _remote;
        Core::NodeId _local;
        RtspSession::Socket* _srmSocket;
        RtspSession::Socket* _controlSocket;

        RtspParser _parser;
        RtspSessionInfo _sessionInfo;
        Core::CriticalSection _adminLock;
        RequestQueue _requestQueue;
        ResponseQueue _responseQueue;
        Core::TimerType<HeartbeatTimer> _heartbeatTimer;

        bool _isSessionActive;
        int _nextSRMHeartbeatMS;
        int _nextPumpHeartbeatMS;
        int _playDelay;
    };
}
} // WPEFramework::Plugin

#endif
