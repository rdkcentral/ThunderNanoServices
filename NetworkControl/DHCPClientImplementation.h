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

#ifndef DHCPCLIENTIMPLEMENTATION__H
#define DHCPCLIENTIMPLEMENTATION__H

#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    class DHCPClientImplementation : public Core::SocketDatagram {
    public:
        enum classifications {
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
        DHCPClientImplementation() = delete;
        DHCPClientImplementation(const DHCPClientImplementation&) = delete;
        DHCPClientImplementation& operator=(const DHCPClientImplementation&) = delete;

        enum state {
            IDLE,
            SENDING,
            RECEIVING
        };

        // Name of file containing last ip
        static constexpr TCHAR lastIPFileName[] = _T("dhcpclient.json");

        // RFC 2131 section 2
        enum operations {
            OPERATION_BOOTREQUEST = 1,
            OPERATION_BOOTREPLY = 2,
        };

        // RFC 2132 section 9.6
        enum enumOptions {
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

        class Flow {
        private:
            Flow(const Flow& a_Copy) = delete;
            Flow& operator=(const Flow& a_RHS) = delete;

        public:
            inline Flow()
            {
            }
            inline Flow(const TCHAR formatter[], ...)
            {
                va_list ap;
                va_start(ap, formatter);
                Trace::Format(_text, formatter, ap);
                va_end(ap);
            }
            inline Flow(const std::string& text)
                : _text(Core::ToString(text.c_str()))
            {
            }
            inline Flow(const char text[])
                : _text(Core::ToString(text))
            {
            }
            ~Flow()
            {
            }

        public:
            inline const char* Data() const
            {
                return (_text.c_str());
            }
            inline uint16_t Length() const
            {
                return (static_cast<uint16_t>(_text.length()));
            }

        private:
            std::string _text;
        };

    public:
        // DHCP constants (see RFC 2131 section 4.1)
        static constexpr uint16_t DefaultDHCPServerPort = 67;
        static constexpr uint16_t DefaultDHCPClientPort = 68;

        class DHCPMessageOptions {
        public:
            DHCPMessageOptions()
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

            DHCPMessageOptions(const uint8_t optionsData[], const uint16_t length)
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
            typedef Core::IteratorType<const std::list<Core::NodeId>, const Core::NodeId&, std::list<Core::NodeId>::const_iterator> DnsIterator;

            class JSON : public Core::JSON::Container {
            private:
                JSON& operator=(const JSON&) = delete;
            public:
                JSON() 
                    : source()
                    , offer()
                    , gateway()
                    , broadcast()
                    , dns()
                    , netmask()
                    , leaseTime()
                    , renewalTime()
                    , rebindingTime()
                {
                    Add("source", &source);
                    Add("offer", &offer);
                    Add("gateway", &gateway);
                    Add("broadcast", &broadcast);
                    Add("dns", &dns);
                    Add("netmask", &netmask);
                    Add("leaseTime", &leaseTime);
                    Add("renewalTime", &renewalTime);
                    Add("rebindingTime", &rebindingTime);
                }

                JSON(Offer& object) 
                {
                    Add("source", &source);
                    Add("offer", &offer);
                    Add("gateway", &gateway);
                    Add("broadcast", &broadcast);
                    Add("dns", &dns);
                    Add("netmask", &netmask);
                    Add("leaseTime", &leaseTime);
                    Add("renewalTime", &renewalTime);
                    Add("rebindingTime", &rebindingTime);

                    Set(object);
                }

                JSON(const JSON& copy) 
                    : source(copy.source)
                    , offer(copy.offer)
                    , gateway(copy.gateway)
                    , broadcast(copy.broadcast)
                    , dns(copy.dns)
                    , netmask(copy.netmask)
                    , leaseTime(copy.leaseTime)
                    , renewalTime(copy.renewalTime)
                    , rebindingTime(copy.rebindingTime)
                {
                    Add("source", &source);
                    Add("offer", &offer);
                    Add("gateway", &gateway);
                    Add("broadcast", &broadcast);
                    Add("dns", &dns);
                    Add("netmask", &netmask);
                    Add("leaseTime", &leaseTime);
                    Add("renewalTime", &renewalTime);
                    Add("rebindingTime", &rebindingTime);
                }

                void Set(Offer& object) {
                    source = object._source.HostAddress();
                    offer = object._offer.HostAddress();
                    gateway = object._gateway.HostAddress();
                    broadcast = object._broadcast.HostAddress();

                    for (auto entry : object._dns) {
                        Core::JSON::String dnsAddress;
                        dnsAddress = entry.HostAddress();
                        dns.Add(dnsAddress);
                    }

                    netmask = object._netmask;
                    leaseTime = object._leaseTime;
                    renewalTime = object._renewalTime;
                    rebindingTime = object._rebindingTime;
                }

                Offer Get() {
                    Offer result;
                    result._source = Core::NodeId(source.Value().c_str());
                    result._offer = Core::NodeId(offer.Value().c_str());
                    result._gateway = Core::NodeId(gateway.Value().c_str());
                    result._broadcast = Core::NodeId(broadcast.Value().c_str());

                    auto entry = dns.Elements();
                    while (entry.Next()) {
                        result._dns.push_back(Core::NodeId(entry.Current().Value().c_str()));
                    }

                    result._netmask = netmask.Value();
                    result._leaseTime = leaseTime.Value();
                    result._renewalTime = renewalTime.Value();
                    result._rebindingTime = rebindingTime.Value();

                    return result;
                }
            private:
                Core::JSON::String source;
                Core::JSON::String offer;
                Core::JSON::String gateway;
                Core::JSON::String broadcast;

                Core::JSON::ArrayType<Core::JSON::String> dns;

                Core::JSON::DecSInt8 netmask;
                Core::JSON::DecUInt32 leaseTime;
                Core::JSON::DecUInt32 renewalTime;
                Core::JSON::DecUInt32 rebindingTime;
            };
        public:
            Offer()
                : _source()
                , _offer()
                , _gateway()
                , _broadcast()
                , _dns()
                , _netmask(0)
                , _leaseTime(0)
                , _renewalTime(0)
                , _rebindingTime(0)
            {
                Crypto::Random(_id);
            }
            Offer(const Core::NodeId& source, const CoreMessage& frame, DHCPMessageOptions& options)
                : _source(source)
                , _offer()
                , _gateway()
                , _broadcast()
                , _dns()
                , _netmask(~0)
                , _leaseTime(0)
                , _renewalTime(0)
                , _rebindingTime(0)
            {
                _source = frame.siaddr;
                _offer = frame.yiaddr;
                Crypto::Random(_id);

                Update(options);
            }
            Offer(const Offer& copy)
                : _source(copy._source)
                , _offer(copy._offer)
                , _gateway(copy._gateway)
                , _broadcast(copy._broadcast)
                , _dns(copy._dns)
                , _netmask(copy._netmask)
                , _leaseTime(copy._leaseTime)
                , _renewalTime(copy._renewalTime)
                , _rebindingTime(copy._rebindingTime)
                , _id(copy._id)
            {
            }
            
            ~Offer()
            {
            }

            void Update(DHCPMessageOptions& options) {
               if (_offer.IsValid() == true) {
                    
                    _gateway = options.gateway;
                    _broadcast = options.broadcast;
                    _dns = options.dns;

                    // Get as much info from options as possible...
                    if (options.netmask.IsSet()) 
                        _netmask = options.netmask.Value();
                    else 
                        TRACE_L1("DHCP message without netmask information");

                    if (options.leaseTime.IsSet()) 
                        _leaseTime = options.leaseTime.Value();
                    else 
                        TRACE_L1("DHCP message came without lease time information");

                    if (options.renewalTime.IsSet()) 
                        _renewalTime = options.renewalTime.Value();
                    else 
                        TRACE_L1("DHCP message came without renewal time information");

                    if (options.rebindingTime.IsSet()) 
                        _rebindingTime = options.rebindingTime.Value();
                    else 
                        TRACE_L1("DHCP message came without rebinding time information");

                    if (_netmask == static_cast<uint8_t>(~0)) {
                        _netmask = _offer.DefaultMask();
                        TRACE_L1("Set the netmask of the offer: %d", _netmask);
                    }

                    if (_broadcast.IsValid() == false) {
                        _broadcast = Core::IPNode(_offer, _netmask).Broadcast();
                        TRACE_L1("Set the broadcast of the offer: %s", _broadcast.HostAddress().c_str());
                    }

                    if (_gateway.IsValid() == false) {
                        _gateway = _source;
                        TRACE_L1("Set the gateway to the source: %s", _gateway.HostAddress().c_str());
                    }

                    if (_dns.size() == 0) {
                        _dns.push_back(_source);
                        TRACE_L1("S et the DNS to the source: %s", _source.HostAddress().c_str());
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
                _leaseTime = rhs._leaseTime;
                _renewalTime = rhs._renewalTime;
                _rebindingTime = rhs._rebindingTime;
                _id = rhs._id;

                return (*this);
            }

        public:
            uint32_t Id() const 
            {
                return _id;
            }
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
            DnsIterator DNS() const
            {
                return (DnsIterator(_dns));
            }
            const Core::NodeId& Broadcast() const
            {
                return (_broadcast);
            }
            uint8_t Netmask() const
            {
                return (_netmask);
            }
            uint32_t LeaseTime() const
            {
                return (_leaseTime);
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
            uint32_t _leaseTime; /* lease time in seconds */
            uint32_t _renewalTime; /* renewal time in seconds */
            uint32_t _rebindingTime; /* rebinding time in seconds */
            uint32_t _id; /* unique offer identifier */
        };

        typedef Core::IteratorType<std::list<Offer>, Offer&, std::list<Offer>::iterator> Iterator;
        typedef Core::IteratorType<const std::list<Offer>, const Offer&, std::list<Offer>::const_iterator> ConstIterator;
        typedef std::function<void(Offer&)> DiscoverCallback;
        typedef std::function<void(Offer&, bool)> RequestCallback;

    public:
        DHCPClientImplementation(const string& interfaceName, DiscoverCallback discoverCallback, RequestCallback claimCallback);
        virtual ~DHCPClientImplementation();

    public:
        inline classifications Classification() const
        {
            return (_modus);
        }
        inline const string& Interface() const
        {
            return (_interfaceName);
        }
        inline void UpdateMAC(const uint8_t buffer[], const uint8_t size) {
            ASSERT(size == sizeof(_MAC));

            memcpy(_MAC, buffer, sizeof(_MAC));
        }
        inline void Resend()
        {

            _adminLock.Lock();

            if (_state == RECEIVING) {

                _state = SENDING;
                SocketDatagram::Trigger();
            }

            _adminLock.Unlock();
        }

        /* Ask DHCP servers for offers. */
        inline uint32_t Discover(const Core::NodeId& preferredAddres)
        {
            uint32_t result = Core::ERROR_INPROGRESS;

            _adminLock.Lock();
            if (_state == RECEIVING || _state == IDLE) {
                result = Core::ERROR_OPENING_FAILED;
                
                if (SocketDatagram::IsOpen() == true
                    || SocketDatagram::Open(Core::infinite, _interfaceName) == Core::ERROR_NONE) {
                    _unleasedOffers.clear();

                    SocketDatagram::Broadcast(true);
                    
                    Crypto::Random(_discoverXID);
                    _state = SENDING;
                    _modus = CLASSIFICATION_DISCOVER;
                    _preferred = preferredAddres;
                    _xid = _discoverXID;
                    result = Core::ERROR_NONE;

                    TRACE_L1("Sending a Discover for %s", _interfaceName.c_str());                            

                    SocketDatagram::Trigger();
                } else {
                    TRACE_L1("DatagramSocket for DHCP[%s] could not be opened.", _interfaceName.c_str());
                }
            } else {
                TRACE_L1("Incorrect start to start a new DHCP[%s] send. Current State: %d", _interfaceName.c_str(), _state);
            }
            _adminLock.Unlock();

            return (result);
        }

        uint32_t Request(const Offer& offer) {

            uint32_t result = Core::ERROR_INPROGRESS;

            _adminLock.Lock();
            if (SocketDatagram::IsOpen() == true
                || SocketDatagram::Open(Core::infinite, _interfaceName) == Core::ERROR_NONE) {

                SocketDatagram::Broadcast(true);

                if (_state == RECEIVING || _state == IDLE) {
                    TRACE(Trace::Information, ("Sending REQUEST for %s", offer.Address().HostAddress().c_str()));
                    _state = SENDING;
                    _modus = CLASSIFICATION_REQUEST;
                    _preferred = offer.Address();
                    // Use offer id as transaction id to pair request with correct response
                    _xid = offer.Id(); 

                    if (offer.Source().IsEmpty() == false) {
                        auto addr = reinterpret_cast<const sockaddr_in*>(static_cast<const struct sockaddr*>(offer.Source()));
                        
                        memcpy(&_serverIdentifier, &(addr->sin_addr), 4);
                    }

                    result = Core::ERROR_NONE;
                    SocketDatagram::Trigger();
                }
            } else {
                TRACE_L1("Failed to open socket whilte trying to request ip %s\n", offer.Address().HostAddress().c_str());
            }

            _adminLock.Unlock();

            return (result);
        }

        inline uint32_t Decline(const Core::NodeId& acknowledged)
        {

            uint32_t result = Core::ERROR_INPROGRESS;

            _adminLock.Lock();

            if (_state == RECEIVING) {
                _state = SENDING;
                _modus = CLASSIFICATION_NAK;
                Crypto::Random(_xid);
                _preferred = acknowledged;
                result = Core::ERROR_NONE;
            }

            _adminLock.Unlock();

            return (result);
        }

        inline void AddUnleasedOffer(const Offer& offer) {
            _adminLock.Lock();

            _unleasedOffers.push_back(offer);

            _adminLock.Unlock();
        }

        inline Iterator FindUnleasedOffer(const uint32_t id)
        {
            _adminLock.Lock(); 

            auto foundOffer = std::find_if(_unleasedOffers.begin(), _unleasedOffers.end(), [id](Offer& offer) {return offer.Id() == id;});
            Iterator result = Iterator(_unleasedOffers, foundOffer);
            
            _adminLock.Unlock();

            return result;
        }

        inline Iterator UnleasedOffers()
        {
            return Iterator(_unleasedOffers);
        }

        inline Offer& LeasedOffer()
        {
            return _leasedOffer;
        }

        inline Iterator CurrentlyRequestedOffer() {
            Iterator result(_unleasedOffers);

            if (_modus == CLASSIFICATION_REQUEST) {
                // try to match xid with offer
                result = FindUnleasedOffer(_xid);
            }

            return result;
        }

        inline void RemoveUnleasedOffer(const Offer& offer) 
        {
            _adminLock.Lock();

            _unleasedOffers.remove_if([offer] (Offer& o) {return o.Id() == offer.Id();}); 

            _adminLock.Unlock();
        }

        inline void ClearUnleasedOffers() 
        {
            _adminLock.Lock();

            _unleasedOffers.clear();   

            _adminLock.Unlock();
        }

        inline void Completed()
        {
            _adminLock.Lock();
            TRACE_L1("Closing the DHCP stuff, we are done! State-Modus: [%d-%d]", _state, _modus);
            _state = IDLE;
            // Do not wait for UDP closure, this is also called on the Communication Thread !!!
            if (SocketDatagram::IsOpen())
                SocketDatagram::Close(0);
            _adminLock.Unlock();
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

        Offer& MakeLeased(const Offer& offer) {
            _adminLock.Lock();

            _leasedOffer = offer;
            _unleasedOffers.remove_if([offer] (Offer& o) {return o.Id() == offer.Id();}); 
            
            _adminLock.Unlock();

            return _leasedOffer;
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
            frame.hlen = static_cast<uint8_t>(sizeof(_MAC)); // ETHERNET_HARDWARE_ADDRESS_LENGTH;

            frame.hops = 0;

            /* sets a transaction identifier */
            frame.xid = htonl(_xid);

            /*discover_packet.secs=htons(65535);*/
            frame.secs = 0xFF;

            /* tell server it should broadcast its response */
            frame.flags = htons(BroadcastValue);

            /* our hardware address */
            ::memcpy(frame.chaddr, _MAC, frame.hlen);

            /* Close down the header field with a magic cookie (as per RFC 2132) */
            ::memcpy(frame.magicCookie, MagicCookie, sizeof(frame.magicCookie));

            uint8_t* options(&stream[sizeof(CoreMessage)]);
            uint16_t index(3);

            /* DHCP message type is embedded in options field */
            options[0] = OPTION_DHCPMESSAGETYPE; /* DHCP message type option identifier */
            options[1] = 1; /* DHCP message option length in bytes */
            options[2] = _modus;

            /* the IP address we're preferring (DISCOVER) /  REQUESTING (REQUEST) */
            if ((_preferred.Type() == Core::NodeId::TYPE_IPV4) && (_preferred.IsEmpty() == false)) {
                const struct sockaddr_in* data(reinterpret_cast<const struct sockaddr_in*>(static_cast<const struct sockaddr*>(_preferred)));

                options[index++] = OPTION_REQUESTEDIPADDRESS;
                options[index++] = 4;
                ::memcpy(&(options[index]), &(data->sin_addr.s_addr), 4);
                index += 4;
            }

            /* Add identifier so that DHCP server can recognize device */
            options[index++] = OPTION_CLIENTIDENTIFIER;
            options[index++] = frame.hlen + 1; // Identifier length
            options[index++] = 1; // Ethernet hardware
            ::memcpy(&(options[index]), _MAC, frame.hlen);
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

            if (::memcmp(frame.chaddr, _MAC, sizeof(_MAC)) != 0) {
                TRACE(Trace::Information, (_T("Unknown CHADDR encountered.")));
            } else if ((sizeof(_MAC) < sizeof(frame.chaddr)) && (IsZero(&(frame.chaddr[sizeof(_MAC)]), sizeof(frame.chaddr) - sizeof(_MAC)) == false)) {
                TRACE(Trace::Information, (_T("Unknown CHADDR (clearance) encountered.")));
            } else {
                const uint8_t* optionsRaw = reinterpret_cast<const uint8_t*>(&(stream[result]));
                const uint16_t optionsLen = length - result;

                DHCPMessageOptions options(optionsRaw, optionsLen);

                switch (options.messageType.Value()) {
                    case CLASSIFICATION_OFFER:
                        {
                            if (xid == _discoverXID) {
                                AddUnleasedOffer(Offer(source, frame, options));
                                TRACE(Trace::Information, ("Received an Offer from: %s", source.HostAddress().c_str()));
                                _discoverCallback(_unleasedOffers.back());
                            } else {
                                TRACE_L1("Unknown XID encountered: %d", xid);
                            }
                            break;
                        }
                    case CLASSIFICATION_ACK: 
                        {
                            Iterator offer = FindUnleasedOffer(xid);
                                        
                            if (offer.IsValid()) {
                                offer.Current().Update(options); // Update if informations changed since offering
                                
                                Offer& leased = MakeLeased(offer.Current());
                                _claimCallback(leased, true);
                            }
                            break;
                        }
                    case CLASSIFICATION_NAK:
                        {
                            Iterator offer = FindUnleasedOffer(xid);

                            if (offer.IsValid()) {
                                Offer copy = offer.Current(); // we need a copy because of deletion
                                RemoveUnleasedOffer(offer.Current());

                                _claimCallback(copy, false);
                            }
                            break;
                        }                   
                }

                result = length;
            }

            return (result);
        }

    private:
        Core::CriticalSection _adminLock;
        string _interfaceName;
        state _state;
        classifications _modus;
        uint8_t _MAC[6];
        mutable uint32_t _serverIdentifier;
        mutable uint32_t _xid;
        mutable uint32_t _discoverXID;
        Core::NodeId _preferred;
        DiscoverCallback _discoverCallback;
        RequestCallback _claimCallback;
        std::list<Offer> _unleasedOffers;
        Offer _leasedOffer;
    };
}
} // namespace WPEFramework::Plugin

#endif // DHCPCLIENTIMPLEMENTATION__H
