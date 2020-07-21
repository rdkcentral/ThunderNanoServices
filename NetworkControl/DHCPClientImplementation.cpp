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

    class DHCPIPPacket {
        /*
         * DHCP Protocol (rfc2131) defines, that every message prior to acquiring IP 
         * should be send from 0.0.0.0 source. To reliably force that we need to 
         * implement IP header on raw socket level. Otherwise network driver can 
         * interpret 0.0.0.0 as IPADDR_ANY and reassign it if it has any.
        */
    public:
        static constexpr uint16_t minimumFrameSize = sizeof(iphdr) + sizeof(udphdr);

    public:
        DHCPIPPacket() = delete;
        DHCPIPPacket(const DHCPIPPacket&) = delete;
        DHCPIPPacket& operator=(const DHCPIPPacket&) = delete;

        DHCPIPPacket(uint8_t* buffer, const uint16_t bufferSize, const bool initialize) 
            : _buffer(buffer)
            , _bufferSize(bufferSize)
        {
            if (initialize == true) {
                InitIpHeader();
                InitUDPHeader();
            }
        }

        uint8_t* DHCPFrame() 
        {
            return _buffer + dhcpHeaderOffset;   
        }

        const uint16_t MaxDHCPFrameSize()
        {
            return _bufferSize - sizeof(iphdr) - sizeof(udphdr);
        } 

        const uint16_t TotalSize()
        {
            return _dhcpFrameSize + sizeof(iphdr) + sizeof(udphdr);
        } 

        void DHCPFrameSize(const uint16_t size)
        {
            _dhcpFrameSize = size;

            RecalcHeadersSizes();
            RecalcIpHeaderChecksum();
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

        const bool IsIncomingMessage()
        {
            const iphdr* ipHeader = reinterpret_cast<iphdr*>(_buffer);
            const udphdr* udpHeader = reinterpret_cast<udphdr*>(_buffer + udpHeaderOffset);


            return (_bufferSize > minimumFrameSize) 
                    && (ipHeader->protocol == PROTOCOL_UDP)
                    && (udpHeader->dest == htons(DHCPClientImplementation::DefaultDHCPClientPort));
        }
    private:
        void InitIpHeader() 
        {
            iphdr* ipHeader = reinterpret_cast<iphdr*>(_buffer);

            memset(ipHeader, 0, sizeof(iphdr));
            ipHeader->version = 4; // Use IPv4
            ipHeader->ihl = 5; // Standard IP header length
            ipHeader->ttl = 64; // Standard TTL
            ipHeader->protocol = PROTOCOL_UDP;
            memset(&(ipHeader->daddr), 0xff, sizeof(ipHeader->daddr));
        }

        void InitUDPHeader() 
        {
            udphdr* udpHeader = reinterpret_cast<udphdr*>(_buffer + udpHeaderOffset);

            memset(udpHeader, 0, sizeof(udphdr));
            udpHeader->source = htons(DHCPClientImplementation::DefaultDHCPClientPort);
            udpHeader->dest = htons(DHCPClientImplementation::DefaultDHCPServerPort);
        }

        void RecalcHeadersSizes() 
        {
            iphdr* ipHeader = reinterpret_cast<iphdr*>(_buffer);
            udphdr* udpHeader = reinterpret_cast<udphdr*>(_buffer + udpHeaderOffset);

            ipHeader->tot_len = htons(sizeof(iphdr) + sizeof(udphdr) + _dhcpFrameSize);
            udpHeader->len = htons(sizeof(udphdr) + _dhcpFrameSize);
        }

        void RecalcIpHeaderChecksum() 
        {
            // src: https://gist.github.com/david-hoze/0c7021434796997a4ca42d7731a7073a
            iphdr* ipHeader = reinterpret_cast<iphdr*>(_buffer);
            uint16_t* data = reinterpret_cast<uint16_t*>(ipHeader);
            uint32_t count = (ipHeader->ihl) << 2;

            // Checksum should be calculated on itself set to 0
            ipHeader->check = 0;

            uint32_t sum = 0;
            while (count > 1) {
                sum += * data++;
                count -= 2;
            }

            if(count > 0) {
                sum += ((*data)&htons(0xFF00));
            }
            
            while (sum>>16) {
                sum = (sum & 0xffff) + (sum >> 16);
            }

            sum = ~sum;
            ipHeader->check = static_cast<uint16_t>(sum);
        }
    private:
        uint8_t* _buffer;
        const uint16_t _bufferSize;
        uint16_t _dhcpFrameSize;
    private:
        static constexpr uint16_t udpHeaderOffset = sizeof(iphdr);
        static constexpr uint16_t dhcpHeaderOffset = udpHeaderOffset + sizeof(udphdr);
        static constexpr uint16_t PROTOCOL_UDP = 17;
    };

    DHCPClientImplementation::DHCPClientImplementation(const string& interfaceName, DiscoverCallback discoverCallback, RequestCallback claimCallback)
        : Core::SocketDatagram(false, DHCPIPPacket::BroadcastNode(interfaceName), DHCPIPPacket::BroadcastNode(interfaceName), 512, 1024)
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

            ASSERT(maxSendSize > DHCPIPPacket::minimumFrameSize);

            DHCPIPPacket frame(dataFrame, maxSendSize, true);
            result = Message(frame.DHCPFrame(), frame.MaxDHCPFrameSize());

            if (result > 0) {

                frame.DHCPFrameSize(result);
                result = frame.TotalSize();
            }
        }

        return (result);
    }

    /* virtual */ uint16_t DHCPClientImplementation::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
    {
        DHCPIPPacket frame(dataFrame, receivedSize, false);
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
