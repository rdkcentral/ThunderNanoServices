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

namespace WPEFramework {

namespace Plugin {

    /* static */ constexpr uint8_t DHCPClientImplementation::MagicCookie[];

    static Core::NodeId RemoteAddress()
    {
        struct sockaddr_in sockaddr_broadcast;

        /* send the DHCPDISCOVER packet to broadcast address */
        sockaddr_broadcast.sin_family = AF_INET;
        sockaddr_broadcast.sin_port = htons(DHCPClientImplementation::DefaultDHCPServerPort);
        sockaddr_broadcast.sin_addr.s_addr = INADDR_BROADCAST;

        return (Core::NodeId(sockaddr_broadcast));
    }

    DHCPClientImplementation::DHCPClientImplementation(const string& interfaceName, DiscoverCallback discoverCallback, RequestCallback claimCallback, LeaseExpiredCallback leaseExpiredCallback)
        : Core::SocketDatagram(false, Core::NodeId(_T("0.0.0.0"), DefaultDHCPClientPort, Core::NodeId::TYPE_IPV4), RemoteAddress(), 1024, 2048)
        , _adminLock()
        , _interfaceName(interfaceName)
        , _state(IDLE)
        , _serverIdentifier(0)
        , _xid(0)
        , _preferred()
        , _discoverCallback(discoverCallback)
        , _claimCallback(claimCallback)
        , _leaseExpiredCallback(leaseExpiredCallback)
        , _interval(leaseCheckInterval)
        , _leasedOffer()
        , _unleasedOffers()
        , _activity(*this)
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
        _activity.Revoke();
        SocketDatagram::Close(Core::infinite);
    }

    // Methods to extract and insert data into the socket buffers
    /* virtual */ uint16_t DHCPClientImplementation::SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
    {
        uint16_t result = 0;

        if (_state == SENDING) {
            _state = RECEIVING;
            TRACE_L1("Sending DHCP message type: %d for interface: %s", _modus, _interfaceName.c_str());
            result = Message(dataFrame, maxSendSize);
        }

        return (result);
    }

    /* virtual */ uint16_t DHCPClientImplementation::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
    {
        ProcessMessage(SocketDatagram::ReceivedNode(), dataFrame, receivedSize);
        return (receivedSize);
    }

    // Signal a state change, Opened, Closed or Accepted
    /* virtual */ void DHCPClientImplementation::StateChange()
    {
    }
}
} // namespace WPEFramework::Plugin
