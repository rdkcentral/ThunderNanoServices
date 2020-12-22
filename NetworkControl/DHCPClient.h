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

#pragma once

#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    class DHCPClient : public Core::SocketDatagram {
    public:
        enum classifications : uint8_t {
            CLASSIFICATION_INVALID = 0,
            CLASSIFICATION_DISCOVER = 1,
            CLASSIFICATION_OFFER = 2,
            CLASSIFICATION_REQUEST = 3,
            CLASSIFICATION_DECLINE = 4,
            CLASSIFICATION_ACK = 5,
            CLASSIFICATION_NAK = 6,
            CLASSIFICATION_RELEASE = 7,
            CLASSIFICATION_INFORM = 8,
        };

    private:
        using UDPv4Frame = Core::UDPv4FrameType<1024>;

        enum state : uint8_t {
            IDLE,
            SENDING,
            RECEIVING
        };
        // RFC 2131 section 2
        enum operations : uint8_t {
            OPERATION_BOOTREQUEST = 1,
            OPERATION_BOOTREPLY = 2
        };

        // RFC 2132 section 9.6
        enum enumOptions : uint8_t {
            OPTION_PAD = 0,
            OPTION_SUBNETMASK = 1,
            OPTION_ROUTER = 3,
            OPTION_DNS = 6,
            OPTION_HOSTNAME = 12,
            OPTION_BROADCASTADDRESS = 28,
            OPTION_REQUESTEDIPADDRESS = 50,
            OPTION_IPADDRESSLEASETIME = 51,
            OPTION_DHCPMESSAGETYPE = 53,
            OPTION_SERVERIDENTIFIER = 54,
            OPTION_REQUESTLIST = 55,
            OPTION_RENEWALTIME = 58,
            OPTION_REBINDINGTIME = 59,
            OPTION_CLIENTIDENTIFIER = 61,
            OPTION_END = 255,
        };

        // Max UDP datagram size (RFC 768)
        static constexpr uint32_t MaxUDPFrameSize = 65536 - 8;
        static constexpr uint8_t MaxHWLength = 16;
        static constexpr uint8_t MaxServerNameLength = 64;
        static constexpr uint8_t MaxFileLength = 128;

        // Broadcast bit for flags field (RFC 2131 section 2)
        static constexpr uint16_t BroadcastValue = 0x8000;

        // For display of host name information
        static constexpr uint16_t MaxHostNameSize = 256;

        // DHCP magic cookie values
        static constexpr uint8_t MagicCookie[] = { 99, 130, 83, 99 };

        // RFC 2131 section 2
        #pragma pack(push, 1)
        struct CoreMessage {
            uint8_t operation; /* packet type */
            uint8_t htype; /* type of hardware address for this machine (Ethernet, etc) */
            uint8_t hlen; /* length of hardware address (of this machine) */
            uint8_t hops; /* hops */
            uint32_t xid; /* random transaction id number - chosen by this machine */
            uint16_t secs; /* seconds used in timing */
            uint16_t flags; /* flags */
            struct in_addr ciaddr; /* IP address of this machine (if we already have one) */
            struct in_addr yiaddr; /* IP address of this machine (offered by the DHCP server) */
            struct in_addr siaddr; /* IP address of DHCP server */
            struct in_addr giaddr; /* IP address of DHCP relay */
            unsigned char chaddr[MaxHWLength]; /* hardware address of this machine */
            char sname[MaxServerNameLength]; /* name of DHCP server */
            char file[MaxFileLength]; /* boot file name (used for diskless booting?) */
            uint8_t magicCookie[4]; /* fixed cookie to validate the frame */
        };
        #pragma pack(pop)


    public:
        // DHCP constants (see RFC 2131 section 4.1)
        static constexpr uint16_t DefaultDHCPServerPort = 67;
        static constexpr uint16_t DefaultDHCPClientPort = 68;

        class Options {
        public:
            Options()
                : messageType()
                , gateway()
                , broadcast()
                , dns()
                , netmask()
                , leaseTime()
                , renewalTime()
                , rebindingTime()
            {
            }

            Options(const uint8_t optionsData[], const uint16_t length)
                : messageType()
                , gateway()
                , broadcast()
                , dns()
                , netmask()
                , leaseTime()
                , renewalTime()
                , rebindingTime()
            {
                FromRAW(optionsData, length);    
            }

            bool FromRAW(const uint8_t optionsData[], const uint16_t length) 
            {
              uint32_t used = 0;

                /* process all DHCP options present in the packet */
                while ((used < length) && (optionsData[used] != OPTION_END)) {

                    /* skip the padding bytes */
                    while ((used < length) && (optionsData[used] == OPTION_PAD)) {
                        used++;
                    }

                    /* get option type */
                    uint8_t classification = optionsData[used++];

                    /* get option length */
                    uint8_t size = optionsData[used++];

                    /* get option data */
                    switch (classification) {
                    case OPTION_SUBNETMASK: {
                        uint8_t teller = 0;
                        while ((teller < 4) && (optionsData[used + teller] == 0xFF)) {
                            teller++;
                        }
                        netmask = (teller * 8);
                        if (teller < 4) {
                            uint8_t mask = (optionsData[used + teller] ^ 0xFF);
                            netmask += 8;
                            while (mask != 0) {
                                mask = mask >> 1;
                                netmask--;
                            }
                        }
                        break;
                    }
                    case OPTION_DHCPMESSAGETYPE:
                        messageType = optionsData[used];
                        break;

                    case OPTION_ROUTER: {
                        struct in_addr rInfo;
                        rInfo.s_addr = htonl(optionsData[used] << 24 | optionsData[used + 1] << 16 | optionsData[used + 2] << 8 | optionsData[used + 3]);
                        gateway = rInfo;
                        break;
                    }
                    case OPTION_DNS: {
                        uint16_t index = 0;
                        while ((index + 4) <= size) {
                            struct in_addr rInfo;
                            rInfo.s_addr = htonl(optionsData[used + index] << 24 | optionsData[used + index + 1] << 16 | optionsData[used + index + 2] << 8 | optionsData[used + index + 3]);
                            dns.push_back(rInfo);
                            index += 4;
                        }
                        break;
                    }
                    case OPTION_BROADCASTADDRESS: {
                        struct in_addr rInfo;
                        rInfo.s_addr = htonl(optionsData[used] << 24 | optionsData[used + 1] << 16 | optionsData[used + 2] << 8 | optionsData[used + 3]);
                        broadcast = rInfo;
                        break;
                    }
                    case OPTION_IPADDRESSLEASETIME:
                        ::memcpy(&leaseTime, &optionsData[used], sizeof(leaseTime));
                        leaseTime = ntohl(leaseTime);
                        break;
                    case OPTION_RENEWALTIME:
                        ::memcpy(&renewalTime, &optionsData[used], sizeof(renewalTime));
                        renewalTime = ntohl(renewalTime);
                        break;
                    case OPTION_REBINDINGTIME:
                        ::memcpy(&rebindingTime, &optionsData[used], sizeof(rebindingTime));
                        rebindingTime = ntohl(rebindingTime);
                        break;
                    }

                    /* move on to the next option. */
                    used += size;
                }

                return true;
            }

        public:
            Core::OptionalType<uint8_t> messageType;
            Core::NodeId gateway; /* the IP address that was offered to us */
            Core::NodeId broadcast; /* the IP address that was offered to us */
            std::list<Core::NodeId> dns; /* the IP address that was offered to us */
            Core::OptionalType<uint8_t> netmask;
            Core::OptionalType<uint32_t> leaseTime; /* lease time in seconds */
            Core::OptionalType<uint32_t> renewalTime; /* renewal time in seconds */
            Core::OptionalType<uint32_t> rebindingTime; /* rebinding time in seconds */
        };
        class Offer {
        public:
            Offer()
                : _source()
                , _offer()
                , _gateway()
                , _broadcast()
                , _dns()
                , _netmask(0)
                , _xid(0)
                , _leaseTime(0)
                , _renewalTime(0)
                , _rebindingTime(0)
            {
            }
            Offer(const Core::NodeId& source, const CoreMessage& frame, const Options& options)
                : _source(source)
                , _offer()
                , _gateway()
                , _broadcast()
                , _dns()
                , _netmask(~0)
                , _xid(0)
                , _leaseTime(0)
                , _renewalTime(0)
                , _rebindingTime(0)
            {
                if (frame.siaddr.s_addr != 0) {
                    // There are implementation of DHCP servers out there that do not fill in this 
                    // So only fill it in if it is set!
                    _source = frame.siaddr;
                }
                _offer = frame.yiaddr;
                _xid = ntohl(frame.xid);

                Update(options);
            }
            Offer(const Core::NodeId& source, const Core::NodeId& offer, const uint8_t netmask, const Core::NodeId& gateway, const Core::NodeId& broadcast, const uint32_t xid, std::list<Core::NodeId>&& dns)
                : _source(source)
                , _offer(offer)
                , _gateway(gateway)
                , _broadcast(broadcast)
                , _dns(dns)
                , _netmask(netmask)
                , _xid(xid)
                , _leaseTime(0)
                , _renewalTime(0)
                , _rebindingTime(0)
            {
            }
            Offer(const Offer& copy)
                : _source(copy._source)
                , _offer(copy._offer)
                , _gateway(copy._gateway)
                , _broadcast(copy._broadcast)
                , _dns(copy._dns)
                , _netmask(copy._netmask)
                , _xid(copy._xid)
                , _leaseTime(copy._leaseTime)
                , _renewalTime(copy._renewalTime)
                , _rebindingTime(copy._rebindingTime)
            {
            }
            
            ~Offer() = default;

        public:
            bool operator== (const Offer& rhs) const {
                return ((rhs._source == _source) && (rhs._offer == _offer));
            }
            bool operator!= (const Offer& rhs) const {
                return (!operator==(rhs));
            }
            void Update(const Options& options) {
               if (_offer.IsValid() == true) {
                    
                    _leaseTime = 0;
                    _rebindingTime = 0;
                    _renewalTime = 0;
                    _gateway = options.gateway;
                    _broadcast = options.broadcast;
                    _dns = options.dns;

                    // Get as much info from options as possible...
                    if (options.netmask.IsSet()) 
                        _netmask = options.netmask.Value();
                    else 
                        TRACE(Trace::Warning, (_T("DHCP message without netmask information")));

                    if (options.leaseTime.IsSet()) 
                        _leaseTime = options.leaseTime.Value();

                    if (options.renewalTime.IsSet()) 
                        _renewalTime = options.renewalTime.Value();

                    if (options.rebindingTime.IsSet()) 
                        _rebindingTime = options.rebindingTime.Value();

                    if (_leaseTime != 0) {
                        if (_renewalTime   == 0) _renewalTime   = (_leaseTime / 2);
                        if (_rebindingTime == 0) _rebindingTime = ((_leaseTime * 7) / 8);
                    }
                    else if (_renewalTime != 0) {
                        _leaseTime = 2 * _renewalTime;
                        if (_rebindingTime == 0) _rebindingTime = ((_leaseTime * 7) / 8);
                    }
                    else if (_rebindingTime != 0) {
                        _leaseTime = ((_rebindingTime * 8) / 7);
                        _renewalTime = _leaseTime / 2;
                    }
                    else {
                        TRACE(Trace::Warning, (_T("DHCP message came without any timing information")));
                        _leaseTime =  24 * 60 * 60; // default to 24 Hours!!!
                        _renewalTime = _leaseTime / 2;
                        _rebindingTime = ((_leaseTime * 7) / 8);
                    }

                    if (_netmask == static_cast<uint8_t>(~0)) {
                        _netmask = _offer.DefaultMask();
                        TRACE(Trace::Information, (_T("Set the netmask of the offer: %d"), _netmask));
                    }

                    if (_broadcast.IsValid() == false) {
                        _broadcast = Core::IPNode(_offer, _netmask).Broadcast();
                        TRACE(Trace::Information, (_T("Set the broadcast of the offer: %s"), _broadcast.HostAddress().c_str()));
                    }

                    if (_gateway.IsValid() == false) {
                        _gateway = _source;
                        TRACE(Trace::Information, (_T("Set the gateway to the source: %s"), _gateway.HostAddress().c_str()));
                    }

                    if (_dns.size() == 0) {
                        _dns.push_back(_source);
                        TRACE(Trace::Information, (_T("Set the DNS to the source: %s"), _source.HostAddress().c_str()));
                    }
                }
            }

            Offer& operator=(const Offer& rhs)
            {
                _source = rhs._source;
                _offer = rhs._offer;
                _gateway = rhs._gateway;
                _broadcast = rhs._broadcast;
                _dns = rhs._dns;
                _netmask = rhs._netmask;
                _xid = rhs._xid;
                _leaseTime = rhs._leaseTime;
                _renewalTime = rhs._renewalTime;
                _rebindingTime = rhs._rebindingTime;

                return (*this);
            }
            void Clear()
            {
                Core::NodeId node;
                _source = node;
                _offer = node;
                _gateway = node;
                _broadcast = node;
                _dns.clear();
                _netmask = 0;
                _xid = 0;
                _leaseTime = 0;
                _renewalTime = 0;
                _rebindingTime = 0;
            }

        public:
            bool IsValid() const
            {
                return (_offer.IsValid());
            }
            const Core::NodeId& Source() const
            {
                return (_source);
            }
            const Core::NodeId& Address() const
            {
                return (_offer);
            }
            const Core::NodeId& Gateway() const
            {
                return (_gateway);
            }
            const std::list<Core::NodeId>& DNS() const
            {
                return (_dns);
            }
            const Core::NodeId& Broadcast() const
            {
                return (_broadcast);
            }
            uint8_t Netmask() const
            {
                return (_netmask);
            }
            uint32_t Xid() const
            {
                return (_xid);
            }
            uint32_t LeaseTime() const
            {
                return (_leaseTime);
            }
            void LeaseTime(uint32_t leaseTime)
            {
                _leaseTime = leaseTime;
            }
            uint32_t RenewalTime() const
            {
                return (_renewalTime);
            }
            uint32_t RebindingTime() const
            {
                return (_rebindingTime);
            }

        private:
            Core::NodeId _source; /* address of DHCP server that sent this offer */
            Core::NodeId _offer; /* the IP address that was offered to us */
            Core::NodeId _gateway; /* the IP address that was offered to us */
            Core::NodeId _broadcast; /* the IP address that was offered to us */
            std::list<Core::NodeId> _dns; /* the IP address that was offered to us */
            uint8_t _netmask;
            uint32_t _xid;
            uint32_t _leaseTime; /* lease time in seconds */
            uint32_t _renewalTime; /* renewal time in seconds */
            uint32_t _rebindingTime; /* rebinding time in seconds */
        };

        struct ICallback {
            virtual void Offered(const Offer&) = 0;
            virtual void Approved(const Offer&) = 0;
            virtual void Rejected(const Offer&) = 0;
        };

        typedef Core::IteratorType<std::list<Offer>, Offer&, std::list<Offer>::iterator> Iterator;
        typedef Core::IteratorType<const std::list<Offer>, const Offer&, std::list<Offer>::const_iterator> ConstIterator;

    public:
        DHCPClient() = delete;
        DHCPClient(const DHCPClient&) = delete;
        DHCPClient& operator=(const DHCPClient&) = delete;

        DHCPClient(const string& interfaceName, ICallback* callback);
        virtual ~DHCPClient();

    public:
        bool HasActiveLease() const {
            return (_expired.IsValid() == true);
        }
        const Core::Time& Expired() const {
            return (_expired);
        }
        const Offer& Lease() const {
            return (_offer);
        }
        const string& Interface() const {
            return (_interfaceName);
        }
        void UpdateMAC(const uint8_t buffer[], const uint8_t size) {
            ASSERT(size == _udpFrame.MACSize);
            _udpFrame.SourceMAC(buffer);
        }
        /* Ask DHCP servers for offers. */
        inline uint32_t Discover(const Core::NodeId& preferredAddres)
        {
            uint32_t result = Core::ERROR_OPENING_FAILED;

            if ((SocketDatagram::IsOpen() == true) || (SocketDatagram::Open(Core::infinite, _interfaceName) == Core::ERROR_NONE)) {

                result = Core::ERROR_INPROGRESS;
                
                _adminLock.Lock();

                if ((_state == RECEIVING) || (_state == IDLE)) {

                    _state = SENDING;
                    _offer.Clear();
                    _expired = Core::Time();
                    _adminLock.Unlock();

                    Crypto::Random(_xid);

                    _modus = CLASSIFICATION_DISCOVER;

                    result = Core::ERROR_NONE;

                    Core::SocketDatagram::Broadcast(true);
                    Core::SocketDatagram::Trigger();

                } else {
                    _adminLock.Unlock();
                }
            }

            return (result);
        }
        inline uint32_t Request(const Offer& offer) {

            uint32_t result = Core::ERROR_OPENING_FAILED;

            if ((SocketDatagram::IsOpen() == true) || (SocketDatagram::Open(Core::infinite, _interfaceName) == Core::ERROR_NONE)) {

                result = Core::ERROR_INPROGRESS;
                
                _adminLock.Lock();

                if ((_state == RECEIVING) || (_state == IDLE)) {

                    // Looks like the ACK on which we where waiting is not coming, move on..
                    ASSERT(offer.Source().IsValid() == true);

                    _modus = CLASSIFICATION_REQUEST;
                    _state = SENDING;
                    _offer = offer;
                    _xid = _offer.Xid();
                    _expired = Core::Time();

                    _adminLock.Unlock();
  
                    result = Core::ERROR_NONE;
                    auto addr = reinterpret_cast<const sockaddr_in*>(static_cast<const struct sockaddr*>(offer.Source()));
                        
                    memcpy(&_serverIdentifier, &(addr->sin_addr), 4);

                    Core::SocketDatagram::Broadcast(true);
                    Core::SocketDatagram::Trigger();
                }
                else {
                    _adminLock.Unlock();
                }
            }

            return (result);
        }
        inline uint32_t Release()
        {
            uint32_t result = Core::ERROR_OPENING_FAILED;

            if ( (_offer.Address().IsValid() == true) && (_offer.Source().IsValid() == true) ) {

                result = Core::ERROR_OPENING_FAILED;

                if ((SocketDatagram::IsOpen() == true) || (SocketDatagram::Open(Core::infinite, _interfaceName) == Core::ERROR_NONE)) {

                    result = Core::ERROR_INPROGRESS;
                
                    _adminLock.Lock();

                    if ((_state == RECEIVING) || (_state == IDLE)) {

                        _expired = Core::Time();
                        _state = SENDING;
                        _adminLock.Unlock();

                        Crypto::Random(_xid);

                        _modus = CLASSIFICATION_RELEASE;

                        result = Core::ERROR_NONE;
                        Core::SocketDatagram::Trigger();
                    }
                    else {
                        _adminLock.Unlock();
                    }
                }
            }

            return (result);
        }
        inline void Close()
        {
            _adminLock.Lock();

            _state = IDLE;

            _adminLock.Unlock();

            // Do not wait for UDP closure, this is also called on the Communication Thread !!!
            if (SocketDatagram::IsOpen()) {
                SocketDatagram::Close(0);
            }
        }

    private:
        // Methods to extract and insert data into the socket buffers
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override;

        // Signal a state change, Opened, Closed or Accepted
        virtual void StateChange() override;

        inline bool IsZero(const uint8_t option[], const uint8_t length) const
        {
            uint8_t index = 0;
            while ((index < length) && (option[index] == 0)) {
                index++;
            }
            return (index == length);
        }

        uint16_t Message(uint8_t stream[], const uint16_t length) const
        {

            CoreMessage& frame(*reinterpret_cast<CoreMessage*>(stream));

            /* clear the packet data structure */
            ::memset(&frame, 0, sizeof(CoreMessage));

            /* boot request flag (backward compatible with BOOTP servers) */
            frame.operation = OPERATION_BOOTREQUEST;

            /* hardware address type */
            frame.htype = 1; // ETHERNET_HARDWARE_ADDRESS

            /* length of our hardware address */
            frame.hlen = _udpFrame.MACSize; // ETHERNET_HARDWARE_ADDRESS_LENGTH;

            frame.hops = 0;

            /* sets a transaction identifier */
            frame.xid = htonl(_xid);

            /*discover_packet.secs=htons(65535);*/
            frame.secs = 0xFF;

            /* tell server it should broadcast its response */
            frame.flags = htons(BroadcastValue);

            /* our hardware address */
            ::memcpy(frame.chaddr, _udpFrame.SourceMAC(), frame.hlen);

            /* Close down the header field with a magic cookie (as per RFC 2132) */
            ::memcpy(frame.magicCookie, MagicCookie, sizeof(frame.magicCookie));

            uint8_t* options(&stream[sizeof(CoreMessage)]);
            uint16_t index(3);

            /* DHCP message type is embedded in options field */
            options[0] = OPTION_DHCPMESSAGETYPE; /* DHCP message type option identifier */
            options[1] = 1; /* DHCP message option length in bytes */
            options[2] = _modus;

            /* the IP address we're preferring (DISCOVER) /  REQUESTING (REQUEST) */
            if (_offer.Address().Type() == Core::NodeId::TYPE_IPV4) {
                const struct sockaddr_in* data(reinterpret_cast<const struct sockaddr_in*>(static_cast<const struct sockaddr*>(_offer.Address())));

                options[index++] = OPTION_REQUESTEDIPADDRESS;
                options[index++] = 4;
                ::memcpy(&(options[index]), &(data->sin_addr.s_addr), 4);
                index += 4;
            }

            /* Add identifier so that DHCP server can recognize device */
            options[index++] = OPTION_CLIENTIDENTIFIER;
            options[index++] = frame.hlen + 1; // Identifier length
            options[index++] = 1; // Ethernet hardware
            ::memcpy(&(options[index]), _udpFrame.SourceMAC(), frame.hlen);
            index += frame.hlen;

            /* Ask for extended informations in offer */
            if (_modus == CLASSIFICATION_DISCOVER) {
                options[index++] = OPTION_REQUESTLIST;
                options[index++] = 4;
                options[index++] = OPTION_SUBNETMASK;
                options[index++] = OPTION_ROUTER;
                options[index++] = OPTION_DNS;
                options[index++] = OPTION_BROADCASTADDRESS;
            } else if (_modus == CLASSIFICATION_REQUEST) {
                // required for usage in bridged networks
                if (_serverIdentifier != 0) {
                    options[index++] = OPTION_SERVERIDENTIFIER;
                    options[index++] = sizeof(_serverIdentifier);
                    ::memcpy(&(options[index]), &_serverIdentifier, sizeof(_serverIdentifier));
                    index += sizeof(_serverIdentifier);
                }
            }

            options[index++] = OPTION_END;

            return (sizeof(CoreMessage) + index);
        }

        /* parse a DHCP message and take appropiate action */
        uint16_t ProcessMessage(const Core::NodeId& source, const uint8_t stream[], const uint16_t length)
        {
            uint16_t result = sizeof(CoreMessage);
            const CoreMessage& frame(*reinterpret_cast<const CoreMessage*>(stream));
            const uint32_t xid = ntohl(frame.xid);

            if (::memcmp(frame.chaddr, _udpFrame.SourceMAC(), _udpFrame.MACSize) != 0) {
                TRACE(Trace::Information, (_T("Unknown CHADDR encountered.")));
            } else if ((_udpFrame.MACSize < sizeof(frame.chaddr)) && (IsZero(&(frame.chaddr[_udpFrame.MACSize]), sizeof(frame.chaddr) - _udpFrame.MACSize) == false)) {
                TRACE(Trace::Information, (_T("Unknown CHADDR (clearance) encountered.")));
            } else {
                const uint8_t* optionsRaw = reinterpret_cast<const uint8_t*>(&(stream[result]));
                const uint16_t optionsLen = length - result;

                Options options(optionsRaw, optionsLen);

                _adminLock.Lock();

                switch (options.messageType.Value()) {
                    case CLASSIFICATION_OFFER:
                        {
                            if (xid == _xid) {

                                Offer offer(source, frame, options);

                                _callback->Offered(offer);
                            }
                            else {
                                TRACE(Trace::Information, (_T("Unknown Discover XID encountered: %d"), xid));
                            }
                            break;
                        }
                    case CLASSIFICATION_ACK: 
                        {
                            if (xid == _xid) {

                                _offer.Update(options); // Update if informations changed since offering
                                
                                _expired = Core::Time::Now().Add(_offer.LeaseTime() * 1000);

                                _callback->Approved(_offer);

                                Close();
                            }
                            else {
                                TRACE(Trace::Information, (_T("Unknown Acknowledge XID encountered: %d"), xid));
                            }
                            break;
                        }
                    case CLASSIFICATION_NAK:
                        {
                            if (xid == _xid) {
                                
                                Offer offer(_offer);
                                _offer.Clear();

                                _callback->Rejected(offer);
                            }
                            else {
                                TRACE(Trace::Information, (_T("Unknown Negative Acknowledge XID encountered: %d"), xid));
                            }
                            break;
                        }                   
                }

                _adminLock.Unlock();

                result = length;
            }

            return (result);
        }

    private:
        Core::CriticalSection _adminLock;
        string _interfaceName;
        state _state;
        classifications _modus;
        uint32_t _serverIdentifier;
        uint32_t _xid;
        Offer _offer;
        UDPv4Frame _udpFrame;
        ICallback* _callback;
        Core::Time _expired;
    };
}
} // namespace WPEFramework::Plugin
