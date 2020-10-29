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

#include "DHCPClient.h"

namespace WPEFramework {

namespace Plugin {

    /* static */ constexpr uint8_t DHCPClient::MagicCookie[];
    static Core::NodeId BroadcastClientNode (_T("0.0.0.0"), DHCPClient::DefaultDHCPClientPort, Core::NodeId::TYPE_IPV4);
    static Core::NodeId BroadcastServerNode (_T("255.255.255.255"), DHCPClient::DefaultDHCPServerPort, Core::NodeId::TYPE_IPV4);

    DHCPClient::RawSocket::RawSocket(DHCPClient& parent, const string& interfaceName) 
        : Core::SocketPort(Core::SocketPort::RAW, Core::NodeId(interfaceName.c_str(), 0, 0, 0, nullptr), Core::NodeId(), 512, 0)
        , _parent(parent) 
        , _udpFrame(BroadcastClientNode, BroadcastServerNode) {

        SocketPort::Open(1000);
    }

    DHCPClient::RawSocket::~RawSocket() /* override */ {
        Close(Core::infinite);
    }

    // Methods to extract and insert data into the socket buffers
    uint16_t DHCPClient::RawSocket::SendData(uint8_t* dataFrame, const uint16_t maxSendSize) /* override */ {
        uint16_t result = 0;

        if (_parent.CanSend() == true) {

            result = _parent.Message(_udpFrame.Frame(), 1024 - _udpFrame.HeaderSize());

            if (result > 0) {
                _udpFrame.Length(result);
              
                result = std::min(maxSendSize, static_cast<uint16_t>(_udpFrame.Size()));

                ::memcpy(dataFrame, _udpFrame.Data(), result);
            }
        }

        return (result);
    }

    uint16_t DHCPClient::RawSocket::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) /* override */ {
        return (0);
    }

    // Signal a state change, Opened, Closed or Accepted
    void DHCPClient::RawSocket::StateChange() /* override */ {
    }

    DHCPClient::DHCPClient(const string& interfaceName, ICallback* callback)
        : Core::SocketDatagram(false, BroadcastClientNode, BroadcastServerNode, 512, 1024)
        , _adminLock()
        , _interfaceName(interfaceName)
        , _state(IDLE)
        , _modus(CLASSIFICATION_INVALID)
        , _serverIdentifier(0)
        , _xid(0)
        , _offer()
        , _rawSocket(*this, interfaceName)
        , _callback(callback)
    {
    }

    /* virtual */ DHCPClient::~DHCPClient()
    {
        SocketDatagram::Close(Core::infinite);
    }

    // Methods to extract and insert data into the socket buffers
    uint16_t DHCPClient::SendData(uint8_t* dataFrame, const uint16_t maxSendSize) /* override */
    {
        return (0);
    }

    uint16_t DHCPClient::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) /* override */
    {
        ProcessMessage(ReceivedNode(), dataFrame, receivedSize);

        return (receivedSize);
    }

    // Signal a state change, Opened, Closed or Accepted
    /* virtual */ void DHCPClient::StateChange()
    {
    }
}
} // namespace WPEFramework::Plugin
