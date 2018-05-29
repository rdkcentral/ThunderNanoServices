#ifndef DHCPCLIENTIMPLEMENTATION__H
#define DHCPCLIENTIMPLEMENTATION__H

#include "Module.h"

namespace WPEFramework {

namespace Plugin {

class DHCPClientImplementation : public Core::SocketDatagram {
public:
    enum classifications
    {
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
    DHCPClientImplementation& operator= (const DHCPClientImplementation&) = delete;

    enum state {
        IDLE,
        SENDING,
        RECEIVING
    };

    // RFC 2131 section 2
    enum operations
    {
        OPERATION_BOOTREQUEST = 1,
        OPERATION_BOOTREPLY = 2,
    };

    // RFC 2132 section 9.6
    enum enumOptions
    {
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
    static constexpr uint32_t MaxUDPFrameSize      = 65536 - 8;
    static constexpr uint8_t  MaxHWLength          = 16;
    static constexpr uint8_t  MaxServerNameLength  = 64;
    static constexpr uint8_t  MaxFileLength        = 128;


    // Broadcast bit for flags field (RFC 2131 section 2)
    static constexpr uint16_t BroadcastValue = 0x8000;

    // For display of host name information
    static constexpr uint16_t MaxHostNameSize = 256;

    // DHCP magic cookie values
    static constexpr uint8_t  MagicCookie[] = { 99, 130, 83, 99 };

    // RFC 2131 section 2
#pragma pack(push, 1)
    struct CoreMessage
    {
        uint8_t  operation;                   /* packet type */
        uint8_t  htype;                       /* type of hardware address for this machine (Ethernet, etc) */
        uint8_t  hlen;                        /* length of hardware address (of this machine) */
        uint8_t  hops;                        /* hops */
        uint32_t xid;                         /* random transaction id number - chosen by this machine */
        uint16_t secs;                        /* seconds used in timing */
        uint16_t flags;                       /* flags */
        struct in_addr ciaddr;              /* IP address of this machine (if we already have one) */
        struct in_addr yiaddr;              /* IP address of this machine (offered by the DHCP server) */
        struct in_addr siaddr;              /* IP address of DHCP server */
        struct in_addr giaddr;              /* IP address of DHCP relay */
        unsigned char chaddr [MaxHWLength]; /* hardware address of this machine */
        char sname [MaxServerNameLength];   /* name of DHCP server */
        char file [MaxFileLength];          /* boot file name (used for diskless booting?) */
        uint8_t magicCookie[4];               /* fixed cookie to validate the frame */
    };
#pragma pack(pop)

    class EXTERNAL Flow {
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

    class Offer {
    public:
        typedef Core::IteratorType<const std::list<Core::NodeId>, const Core::NodeId&,  std::list<Core::NodeId>::const_iterator> DnsIterator;

    public:
        Offer ()
            : _source()
            , _offer()
            , _gateway()
            , _broadcast()
            , _dns()
            , _netmask(0)
            , _leaseTime(0)
            , _renewalTime(0)
            , _rebindingTime(0) {
        }
        Offer(const Core::NodeId& source, const CoreMessage& frame, const uint8_t options[], const uint16_t length)
            : _source(source)
            , _offer()
            , _gateway()
            , _broadcast()
            , _dns()
            , _netmask(~0)
            , _leaseTime(0)
            , _renewalTime(0)
            , _rebindingTime(0) {

	    //_source = frame.ciaddr;
	    _offer = frame.yiaddr;

            uint32_t used = 0;

	    /* process all DHCP options present in the packet */
	    while ( (used < length) && (options[used] != OPTION_END) ) {

                /* skip the padding bytes */
                while ( (used < length) && (options[used] == 0)) { used++; }

		/* get option type */
		uint8_t classification = options[used++];

		/* get option length */
		uint8_t size = options[used++];

		/* get option data */
                switch (classification) {
                case OPTION_SUBNETMASK:
                {
                    uint8_t teller = 0;
                    while ((teller < 4) && (options[used + teller] == 0xFF)) { teller++; }
                    _netmask = (teller * 8);
                    if (teller < 4) {
                        uint8_t mask = (options[used + teller] ^ 0xFF);
                        _netmask += 8;
                        while (mask != 0) {
                            mask = mask >> 1;
                            _netmask--;
                        }
                    }
                    break;
                }
                case OPTION_ROUTER:
                {
        	    struct in_addr rInfo;
    		    rInfo.s_addr = htonl(options[used] << 24 | options[used+1] << 16 | options[used+2] << 8 | options[used+3]); 
                    _gateway = rInfo;
                    break;
                }
                case OPTION_DNS:
                {
                    uint16_t index = 0;
                    while ((index + 4) <= size) {
        	        struct in_addr rInfo;
    		        rInfo.s_addr = htonl(options[used+index] << 24 | options[used+index+1] << 16 | options[used+index+2] << 8 | options[used+index+3]); 
                        _dns.push_back(rInfo);
                        index += 4;
                    }
                    break;
                }
                case OPTION_BROADCASTADDRESS:
                {
        	    struct in_addr rInfo;
    		    rInfo.s_addr = htonl(options[used] << 24 | options[used+1] << 16 | options[used+2] << 8 | options[used+3]); 
                    _broadcast = rInfo;
                    break;
                }
                case OPTION_IPADDRESSLEASETIME: 
                    ::memcpy(&_leaseTime, &options[used], sizeof(_leaseTime));
                    _leaseTime = ntohl(_leaseTime);
                    break;
                case OPTION_RENEWALTIME: 
                    ::memcpy(&_renewalTime, &options[used], sizeof(_renewalTime));
                    _renewalTime = ntohl(_renewalTime);
                    break;
                case OPTION_REBINDINGTIME: 
                    ::memcpy(&_rebindingTime, &options[used], sizeof(_rebindingTime));
                    _rebindingTime = ntohl(_rebindingTime);
                    break;
                }

                /* move on to the next option. */
                used += size;
            }

            if (_offer.IsValid() == true) {
                if (_netmask == static_cast<uint8_t>(~0)) {
                    _netmask = _offer.DefaultMask();
                    TRACE_L1("Set the netmask of the offer: %d", _netmask);
                }

		if (_broadcast.IsValid() == false) {
                    _broadcast = Core::IPNode(_offer, _netmask).Broadcast();
                    TRACE_L1("Set the broadcast of the offer: %s", _broadcast.HostAddress().c_str());
                }

                if (_gateway.IsValid() == false) {
                    _gateway = source;
                    TRACE_L1("Set the gateway to the source: %s", _gateway.HostAddress().c_str());
                }

                if (_dns.size() == 0) {
                    _dns.push_back(source);
                    TRACE_L1("Set the DNS to the source: %s", _source.HostAddress().c_str());
                }
            }
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
            , _rebindingTime(copy._rebindingTime) {
        }
        ~Offer() {
        }

        Offer& operator= (const Offer& rhs) {
            _source = rhs._source;
            _offer = rhs._offer;
            _gateway = rhs._gateway;
            _broadcast = rhs._broadcast;
            _dns = rhs._dns;
            _netmask = rhs._netmask;
            _leaseTime = rhs._leaseTime;
            _renewalTime = rhs._renewalTime;
            _rebindingTime = rhs._rebindingTime;

            return (*this);
        }
	public:
                bool IsValid() const {
                    return (_offer.IsValid());
                }
		const Core::NodeId& Source() const {
			return (_source);
		}
		const Core::NodeId& Address() const {
			return (_offer);
		}
		const Core::NodeId& Gateway() const {
			return (_gateway);
		}
		DnsIterator DNS() const {
			return (DnsIterator(_dns));
		}
		const Core::NodeId& Broadcast() const {
			return (_broadcast);
		}
                uint8_t Netmask () const {
                    return (_netmask);
                }
                uint32_t LeaseTime() const {
                    return (_leaseTime);
                }
                uint32_t RenewalTime() const {
                    return (_renewalTime);
                }
                uint32_t RebindingTime() const {
                    return (_rebindingTime);
                }

	private:
		Core::NodeId _source;    /* address of DHCP server that sent this offer */
		Core::NodeId _offer;     /* the IP address that was offered to us */
		Core::NodeId _gateway;   /* the IP address that was offered to us */
		Core::NodeId _broadcast; /* the IP address that was offered to us */
		std::list<Core::NodeId> _dns;       /* the IP address that was offered to us */
                uint8_t _netmask;
		uint32_t _leaseTime;       /* lease time in seconds */
		uint32_t _renewalTime;     /* renewal time in seconds */
		uint32_t _rebindingTime;   /* rebinding time in seconds */
    };

    typedef Core::IteratorType<const std::list<Offer>, const Offer&, std::list<Offer>::const_iterator > Iterator;
    typedef Core::IDispatchType<const string&> ICallback;

public:
    DHCPClientImplementation(const string& interfaceName, ICallback* notification);
    virtual ~DHCPClientImplementation();

public:
	inline classifications Classification() const {
		return (_modus);
	}
	inline const string& Interface() const {
		return(_interfaceName);
	}
	inline void Resend() {

		_adminLock.Lock();

		if (_state == RECEIVING) {

			_state = SENDING;
			SocketDatagram::Trigger();
		}

		_adminLock.Unlock();
	}
        inline uint32_t Discover (const Core::NodeId& address) {
        	uint32_t result = Core::ERROR_INPROGRESS;

		_adminLock.Lock();
		if (_state == IDLE) {
			result = Core::ERROR_BAD_REQUEST;
	
			// See if the requested interface exists
			Core::AdapterIterator adapters;

			while ( (adapters.Next() == true) && (adapters.Name() != _interfaceName) ) /* INTENTIONALLY LEFT EMPTY */;

			if (adapters.IsValid() == true) {
				result = Core::ERROR_OPENING_FAILED;

				adapters.MACAddress(_MAC, sizeof(_MAC));
				if (SocketDatagram::Open(Core::infinite, _interfaceName) == Core::ERROR_NONE) {

					_offers.clear();

					SocketDatagram::Broadcast(true);

					_state = SENDING;
					_modus = CLASSIFICATION_DISCOVER;
					_preferred = address;
					result = Core::ERROR_NONE;

                			TRACE_L1("Sending a Discover for %s", _interfaceName.c_str());

					SocketDatagram::Trigger();
				}
				else {
					TRACE_L1("DatagramSocket for DHCP[%s] could not be opened.", _interfaceName.c_str());
				}
			}
			else {
				TRACE_L1("Incorrect interface to start a new DHCP[%s] send", _interfaceName.c_str());
			}
		}
		else {
			TRACE_L1("Incorrect start to start a new DHCP[%s] send. Current State: %d", _interfaceName.c_str(), _state);
		}
		_adminLock.Unlock();

		return (result);
	}
        inline uint32_t Request (const Core::NodeId& acknowledged) {

            uint32_t result = Core::ERROR_INPROGRESS;

	    _adminLock.Lock();

            if (_state == RECEIVING) {
                TRACE_L1("Sending an ACK for %s", acknowledged.HostAddress().c_str());
                _state = SENDING;
                _modus = CLASSIFICATION_ACK;
                _preferred = acknowledged;
                result = Core::ERROR_NONE;
		SocketDatagram::Trigger();
            }

            _adminLock.Unlock();

            return(result);
        }
        inline uint32_t Decline (const Core::NodeId& acknowledged) {

            uint32_t result = Core::ERROR_INPROGRESS;

	    _adminLock.Lock();

            if (_state == RECEIVING) {
                _state = SENDING;
                _modus = CLASSIFICATION_NAK;
                _preferred = acknowledged;
                result = Core::ERROR_NONE;
            }

            _adminLock.Unlock();

            return(result);
        }
 
    inline Iterator Offers() const {
        return (Iterator(_offers));
    }
    inline void Completed() {
        _adminLock.Lock();
	TRACE_L1("Closing the DHCP stuff, we are done! State-Modus: [%d-%d]", _state, _modus);
        _state = IDLE;
	// Do not wait for UDP closure, this is also called on the Communication Thread !!!
        SocketDatagram::Close(0);
        _adminLock.Unlock();
    }

private:
    // Methods to extract and insert data into the socket buffers
    virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override;
    virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override;

    // Signal a state change, Opened, Closed or Accepted
    virtual void StateChange() override;

    inline bool IsZero(const uint8_t option[], const uint8_t length) const {
        uint8_t index = 0;
        while ((index < length) && (option[index] == 0)) { index++; }
        return (index == length);
    }
    uint16_t Message (uint8_t stream[], const uint16_t length) const {

        CoreMessage& frame (*reinterpret_cast<CoreMessage*>(stream));

	/* clear the packet data structure */
	::memset(&frame, 0, sizeof(CoreMessage));

	/* boot request flag (backward compatible with BOOTP servers) */
	frame.operation = OPERATION_BOOTREQUEST;

	/* hardware address type */
	frame.htype = 1; // ETHERNET_HARDWARE_ADDRESS

	/* length of our hardware address */
	frame.hlen = static_cast<uint8_t>(sizeof(_MAC)); // ETHERNET_HARDWARE_ADDRESS_LENGTH;

	frame.hops = 0;

	/* transaction id is supposed to be random */
	if (_modus == CLASSIFICATION_DISCOVER) Crypto::Random(_xid);
	frame.xid=htonl(_xid);

	/*discover_packet.secs=htons(65535);*/
	frame.secs=0xFF;

	/* tell server it should broadcast its response */ 
	frame.flags=htons(BroadcastValue);

	/* our hardware address */
	::memcpy(frame.chaddr, _MAC, frame.hlen);

	/* Close down the header field with a magic cookie (as per RFC 2132) */
	::memcpy(frame.magicCookie, MagicCookie, sizeof(frame.magicCookie));

        uint8_t* options (&stream[sizeof(CoreMessage)]);
        uint16_t index (3);

	/* DHCP message type is embedded in options field */
        options[0] = OPTION_DHCPMESSAGETYPE;    /* DHCP message type option identifier */
	options[1] = 1;                         /* DHCP message option length in bytes */
	options[2] = _modus;

	/* the IP address we're requesting */
	if (_preferred.Type() == Core::NodeId::TYPE_IPV4) {
	    const struct sockaddr_in* data (reinterpret_cast<const struct sockaddr_in*>(static_cast<const struct sockaddr*>(_preferred)));

            options[index++] = OPTION_REQUESTEDIPADDRESS;
            options[index++] = 4;
            ::memcpy(&(options[index]), &(data->sin_addr.s_addr), 4);
            index += 4;
        }
        if (_modus == CLASSIFICATION_DISCOVER) {
            options[index++] = OPTION_REQUESTLIST;
            options[index++] = 4;
            options[index++] = OPTION_SUBNETMASK;
            options[index++] = OPTION_ROUTER;
            options[index++] = OPTION_DNS;
            options[index++] = OPTION_BROADCASTADDRESS;
        }

        options[index++] = OPTION_END;

        return (sizeof(CoreMessage) + index);
    }
	
    /* parse a DHCPOFFER message from one or more DHCP servers */
    uint16_t Offering (const Core::NodeId& source, const uint8_t stream[], const uint16_t length) {

        uint16_t result = sizeof(CoreMessage);
        const CoreMessage& frame (*reinterpret_cast<const CoreMessage*>(stream));

        /* check packet xid to see if its the same as the one we used in the discover packet */
        if(ntohl(frame.xid) != _xid) {
            TRACE_L1("Unknown XID encountered: %d", __LINE__);
            TRACE(Flow, (string(_T("Unknown XID encountered."))));
        }
        else if (::memcmp(frame.chaddr, _MAC, sizeof(_MAC)) != 0) {
            TRACE_L1("Unknown CHADDR  encountered: %d", __LINE__);
            TRACE(Flow, (string(_T("Unknown CHADDR encountered."))));
        }
        else if ((sizeof(_MAC) < sizeof(frame.chaddr)) && (IsZero(&(frame.chaddr[sizeof(_MAC)]), sizeof(frame.chaddr) - sizeof(_MAC)) == false)) {
            TRACE_L1("Unknown CHADDR  (clearance) encountered: %d", __LINE__);
            TRACE(Flow, (string(_T("Unknown CHADDR (clearance) encountered."))));
        }
        else {
            const uint8_t* options = reinterpret_cast<const uint8_t*>(&(stream[result]));
            const uint16_t optionlen = length - result;
            _offers.push_back(Offer(source, frame, options, optionlen));

            TRACE_L1("Received an Offer from: %s", source.HostAddress().c_str());
	    _callback->Dispatch(_interfaceName);

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
    mutable uint32_t _xid;
    Core::NodeId _preferred;
    ICallback* _callback;
    std::list<Offer> _offers;
};

} } // namespace WPEFramework::Plugin

#endif // DHCPCLIENTIMPLEMENTATION__H
