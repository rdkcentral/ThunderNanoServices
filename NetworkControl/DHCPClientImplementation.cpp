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

    DHCPClientImplementation::DHCPClientImplementation(const string& interfaceName, ICallback* callback, const string& persistentStoragePath)
        : Core::SocketDatagram(false, Core::NodeId(_T("0.0.0.0"), DefaultDHCPClientPort, Core::NodeId::TYPE_IPV4), RemoteAddress(), 1024, 2048)
        , _adminLock()
        , _interfaceName(interfaceName)
        , _state(IDLE)
        , _xid(0)
        , _preferred()
        , _last()
        , _callback(callback)
        , _offers()
        , _persistentStorage(persistentStoragePath)
    {
         _last = Core::NodeId(ReadLastIP().c_str(), DefaultDHCPClientPort, Core::NodeId::TYPE_IPV4);
        if (!_persistentStorage.empty()) {
            if (!Core::Directory(_persistentStorage.c_str()).CreatePath()) {
                TRACE_L1("Could not create a NetworkControl persistent storage directory");
            }
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

            if (_modus != CLASSIFICATION_DISCOVER) {
                _callback->Dispatch(_interfaceName);
            }
        }

        return (result);
    }

    /* virtual */ uint16_t DHCPClientImplementation::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
    {
        Offering(SocketDatagram::ReceivedNode(), dataFrame, receivedSize);
        return (receivedSize);
    }

    // Signal a state change, Opened, Closed or Accepted
    /* virtual */ void DHCPClientImplementation::StateChange()
    {
    }

    string DHCPClientImplementation::ReadLastIP() 
    {
        string result = _T("0.0.0.0");

        if (!_persistentStorage.empty()) {
            Core::File file(_persistentStorage + lastIPFileName);
            if (file.Exists()) {
                file.Open();
                IPStorage storage;
                storage.FromFile(file);

                if (storage.ip_adress.IsSet()) 
                    result = storage.ip_adress.Value();

                file.Close();
            } 
        }

        return result;
    }

    void DHCPClientImplementation::SaveLastIP(const string& last_ip) 
    {
        if (!_persistentStorage.empty()) {
            Core::File file(_persistentStorage + lastIPFileName);
            if (file.Create()) {
                IPStorage storage;
                storage.ip_adress = last_ip.c_str();
                storage.ToFile(file);
                file.Close();
            } else {
                TRACE_L1("Failed to update last ip information");
            }
        } else {
            TRACE_L1("Persistent path is empty. IP might not be retained over reboots");
        } 
    }

    constexpr TCHAR DHCPClientImplementation::lastIPFileName[];
}
} // namespace WPEFramework::Plugin
