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
    static const uint8_t BroadcastMAC[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

    DHCPClient::DHCPClient(const string& interfaceName, ICallback* callback)
        : Core::SocketDatagram(
            false,
#ifdef __WINDOWS__
            BroadcastClientNode,
            BroadcastServerNode,
#else
            Core::NodeId(interfaceName.c_str(), ETH_P_IP, PACKET_BROADCAST, 0, sizeof(BroadcastMAC), BroadcastMAC),
            Core::NodeId(interfaceName.c_str(), ETH_P_IP, PACKET_BROADCAST, 0, sizeof(BroadcastMAC), BroadcastMAC),
#endif
            512,
            1024)
        , _adminLock()
        , _interfaceName(interfaceName)
        , _state(IDLE)
        , _modus(CLASSIFICATION_INVALID)
        , _serverIdentifier(0)
        , _xid(0)
        , _offer()
        , _udpFrame(BroadcastClientNode, BroadcastServerNode)
        , _callback(callback)
        , _expired(Core::Time())
    {
    }

    /* virtual */ DHCPClient::~DHCPClient()
    {
        SocketDatagram::Close(Core::infinite);
    }

    // Methods to extract and insert data into the socket buffers
    uint16_t DHCPClient::SendData(uint8_t* dataFrame, const uint16_t maxSendSize) /* override */
    {
        uint16_t result = 0;

        _adminLock.Lock();

        if (_state == SENDING) {
            _state = RECEIVING;

            TRACE(Trace::Information, (_T("Sending for mode :%d"), _modus));

#ifdef __WINDOWS__
            result = Message(dataFrame, maxSendSize);
#else
            result = Message(_udpFrame.Frame(), _udpFrame.FrameSize);

            if (result > 0) {
                _udpFrame.Length(result);
             
                // We are not running on a RAW socket but on an IP_PACKET socket, so we need to do all from 
                // the IP layer up. The kernel will add the Ethernet part... 
                result = std::min(maxSendSize, static_cast<uint16_t>(_udpFrame.Size() - Core::EthernetFrameSize));

                ::memcpy(dataFrame, &(_udpFrame.Data()[Core::EthernetFrameSize]), result);
            }
#endif
        }

        _adminLock.Unlock();

        return (result);
    }

    uint16_t DHCPClient::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) /* override */
    {
#ifdef __WINDOWS__
        ProcessMessage(ReceivedNode(), dataFrame, receivedSize);
#else
        UDPv4Frame udpFrame;

        // The load should take place at the IPFrame level, so at the base of the UDP4Frame...
        uint16_t result = static_cast<UDPv4Frame::Base&>(udpFrame).Load(dataFrame, receivedSize);

        // Make sure the package fits...       
        ASSERT(result == receivedSize);

        if ((udpFrame.IsValid() == true) &&
            (udpFrame.SourcePort() == DHCPClient::DefaultDHCPServerPort) &&
            (udpFrame.DestinationPort() == DHCPClient::DefaultDHCPClientPort)) {
            ProcessMessage(udpFrame.Source(), udpFrame.Frame(), udpFrame.Length());
        }
#endif
        return (receivedSize);
    }

    // Signal a state change, Opened, Closed or Accepted
    /* virtual */ void DHCPClient::StateChange()
    {
    }
}
} // namespace WPEFramework::Plugin
