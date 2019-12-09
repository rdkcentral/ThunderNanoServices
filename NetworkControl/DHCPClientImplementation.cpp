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

    DHCPClientImplementation::DHCPClientImplementation(const string& interfaceName, DiscoverCallback discoverCallback, RequestCallback claimCallback)
        : Core::SocketDatagram(false, Core::NodeId(_T("0.0.0.0"), DefaultDHCPClientPort, Core::NodeId::TYPE_IPV4), RemoteAddress(), 1024, 2048)
        , _adminLock()
        , _interfaceName(interfaceName)
        , _state(IDLE)
        , _serverIdentifier(0)
        , _xid(0)
        , _preferred()
        , _discoverCallback(discoverCallback)
        , _claimCallback(claimCallback)
        , _unleasedOffers()
        , _leasedOffers()
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
