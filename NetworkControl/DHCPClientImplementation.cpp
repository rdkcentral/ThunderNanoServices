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

#include "DHCPClientImplementation.h"
#include "net/if_arp.h"
#include<net/ethernet.h>
#include<netinet/udp.h>
#include<netinet/ip.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<net/if.h>

namespace WPEFramework {

namespace Plugin {

    /* static */ constexpr uint8_t DHCPClientImplementation::MagicCookie[];

    class DHCPPacket : public Core::IPUDPPacket {
        /*
         * DHCP Protocol (rfc2131) defines, that every message prior to acquiring IP 
         * should be send from 0.0.0.0 source. To reliably force that we need to 
         * implement IP header on raw socket level. Otherwise network driver can 
         * interpret 0.0.0.0 as IPADDR_ANY and reassign it if it has any.
        */
    public:
        DHCPPacket() = delete;
        DHCPPacket& operator=(const DHCPPacket&) = delete;

        DHCPPacket(uint8_t* buffer, const uint16_t bufferSize, const Core::NodeId& source, const Core::NodeId& target)
            : IPUDPPacket(buffer, bufferSize, source, target)
        {
        }

        DHCPPacket(uint8_t* buffer, const uint16_t bufferSize)
            : IPUDPPacket(buffer, bufferSize)
        {
        }

        uint8_t* DHCPFrame() 
        {
            return IPUDPPacket::Payload();   
        }

        const uint16_t MaxDHCPFrameSize()
        {
            return TotalBufferSize() - TotalHeaderLength();
        } 

        static Core::NodeId BroadcastNode(const string& interfaceName)
        {
            Core::NodeId result;

            uint16_t index = if_nametoindex(interfaceName.c_str());

            struct sockaddr_ll target;
            memset(&target, 0, sizeof(target));
            target.sll_family   = PF_PACKET;
            target.sll_protocol = htons(ETH_P_IP);
            target.sll_ifindex  = index;
            target.sll_hatype   = ARPHRD_ETHER;
            target.sll_pkttype  = PACKET_BROADCAST;
            target.sll_halen    = ETH_ALEN;

            // Fill with broadcast addr
            memset(target.sll_addr, 0xff, ETH_ALEN);

            return Core::NodeId(target);
        }

        bool IsIncomingMessage() const override
        {
            return (Core::IPUDPPacket::IsIncomingMessage()  && (UDPHeader()->dest == htons(DHCPClientImplementation::DefaultDHCPClientPort)) 
                                                            && (IPHeader()->protocol == PROTOCOL_UDP) );
        }

    private:
        static constexpr uint16_t udpHeaderOffset = sizeof(iphdr);
        static constexpr uint16_t dhcpHeaderOffset = udpHeaderOffset + sizeof(udphdr);
        static constexpr uint16_t PROTOCOL_UDP = 17;
    };

    DHCPClientImplementation::DHCPClientImplementation(const string& interfaceName, DiscoverCallback discoverCallback, RequestCallback claimCallback)
        : Core::SocketDatagram(false, DHCPPacket::BroadcastNode(interfaceName), DHCPPacket::BroadcastNode(interfaceName), 512, 1024)
        , _adminLock()
        , _interfaceName(interfaceName)
        , _state(IDLE)
        , _serverIdentifier(0)
        , _xid(0)
        , _preferred()
        , _discoverCallback(discoverCallback)
        , _claimCallback(claimCallback)
        , _unleasedOffers()
        , _leasedOffer()
    {
        Core::AdapterIterator adapters(_interfaceName);

        if (adapters.IsValid() == true) {
            adapters.MACAddress(_MAC, sizeof(_MAC));
        } else {
            TRACE_L1("Could not read mac address of %s\n", _interfaceName.c_str());
        }
    }

    /* virtual */ DHCPClientImplementation::~DHCPClientImplementation()
    {
        SocketDatagram::Close(Core::infinite);
    }

    // Methods to extract and insert data into the socket buffers
    /* virtual */ uint16_t DHCPClientImplementation::SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
    {
        uint16_t result = 0;

        if (_state == SENDING) {
            _state = RECEIVING;
            TRACE_L1("Sending DHCP message type: %d for interface: %s", _modus, _interfaceName.c_str());

            DHCPPacket frame(dataFrame, maxSendSize, Core::NodeId("0.0.0.0", DHCPClientImplementation::DefaultDHCPClientPort), Core::NodeId("255.255.255.255", DHCPClientImplementation::DefaultDHCPServerPort));
            result = Message(frame.DHCPFrame(), frame.MaxDHCPFrameSize());

            if (result > 0) {

                frame.PayloadSet(result);
                result = frame.PacketLength();
            }
        }

        return (result);
    }

    /* virtual */ uint16_t DHCPClientImplementation::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
    {
        DHCPPacket frame(dataFrame, receivedSize);
        if (frame.IsIncomingMessage()) {
            ProcessMessage(SocketDatagram::ReceivedNode(), frame.DHCPFrame(), frame.MaxDHCPFrameSize());
        }

        //ProcessMessage(SocketDatagram::ReceivedNode(), dataFrame, receivedSize);
        return (receivedSize);
    }

    // Signal a state change, Opened, Closed or Accepted
    /* virtual */ void DHCPClientImplementation::StateChange()
    {
    }
}
} // namespace WPEFramework::Plugin
