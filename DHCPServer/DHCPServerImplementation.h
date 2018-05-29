#ifndef __DHCPSERVERIMPLEMENTATION_H__
#define __DHCPSERVERIMPLEMENTATION_H__

#include "Module.h"

namespace WPEFramework {

	namespace Plugin {

		class DHCPServerImplementation : public Core::SocketDatagram {
		private:
			DHCPServerImplementation() = delete;
			DHCPServerImplementation(const DHCPServerImplementation&) = delete;
			DHCPServerImplementation& operator= (const DHCPServerImplementation&) = delete;

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
                OPTION_RENEWALTIME = 58,
                OPTION_REBINDINGTIME = 59,
				OPTION_CLIENTIDENTIFIER = 61,
				OPTION_END = 255,
			};

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

			// Max UDP datagram size (RFC 768)
			static constexpr uint32_t MaxUDPFrameSize = 65536 - 8;
            static constexpr uint8_t  MaxHWLength          = 16;
            static constexpr uint8_t  MaxServerNameLength  = 64;
            static constexpr uint8_t  MaxFileLength        = 128;

			// DHCP constants (see RFC 2131 section 4.1)
			static constexpr uint16_t DefaultDHCPServerPort = 67;
			static constexpr uint16_t DefaultDHCPClientPort = 68;

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
                            uint8_t pbMagicCookie[4];
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
			class Identifier
			{
			public:
				Identifier() : _length(0) {
					_id._allocation = nullptr;
				}
				Identifier(const uint8_t id[], const uint8_t length) : _length(length) {

					if (_length > sizeof(_id._buffer)) {
						_id._allocation = new uint8_t[_length];
						::memcpy(_id._allocation, id, _length);
					}
					else {
						::memcpy(_id._buffer, id, _length);
					}
				}
				Identifier(const Identifier& copy) : _length(copy._length) {
					printf("Identifier copy %d\n", __LINE__);
					if (_length > sizeof(_id._buffer)) {
						_id._allocation = new uint8_t[_length];
						::memcpy(_id._allocation, copy._id._allocation, _length);
					}
					else {
						printf("Identifier copy %d\n", __LINE__);
						::memcpy(_id._buffer, copy._id._buffer, _length);
					}
				}
				Identifier(const Identifier&& copy) : _length(copy._length) {

					if (_length > sizeof(_id._buffer)) {
						_id._allocation = copy._id._allocation;
					}
					else {
						::memcpy(_id._buffer, copy._id._buffer, _length);
					}
				}
				~Identifier() {
					Clear();
				}

				Identifier& operator= (const Identifier& rhs) {
					if (_length > sizeof(_id._buffer)) {
						if (rhs._length > _length) {
							// Reallocation required.
							delete[] _id._allocation;
							_length = rhs._length;
							_id._allocation = new uint8_t[_length];
							::memcpy(_id._allocation, rhs._id._allocation, _length);
						}
						else if (rhs._length > sizeof(_id._buffer)) {
							// Reuse the buffer, it wll fit..
							_length = rhs._length;
							::memcpy(_id._allocation, rhs._id._allocation, _length);
						}
						else {
							// Buffer is nolonger needed.
							delete[] _id._allocation;
							_length = rhs._length;
							::memcpy(_id._buffer, rhs._id._buffer, _length);
						}
					}
					else if (rhs._length > sizeof(_id._buffer)) {
						_length = rhs._length;
						_id._allocation = new uint8_t[_length];
						::memcpy(_id._allocation, rhs._id._allocation, _length);
					}
					else {
						_length = rhs._length;
						::memcpy(_id._buffer, rhs._id._buffer, _length);
					}

					return (*this);
				}

			public:
				inline void Clear() {
					if (_length > sizeof(_id._buffer)) {
						delete[] _id._allocation;
						_id._allocation = nullptr;
					}
					_length = 0;
				}
				inline uint8_t Length() const {
					return (_length);
				}
				inline const uint8_t* Id() const {
					return (_length <= sizeof(_id._buffer) ? _id._buffer : _id._allocation);
				}
				inline bool operator== (const Identifier& rhs) const {
					return ((_length == rhs._length) && (::memcmp(Id(), rhs.Id(), _length) == 0));
				}
				inline bool operator!= (const Identifier& rhs) const {
					return(!operator==(rhs));
				}
				inline string Text() const {
					return (Core::ToString(string(reinterpret_cast<const char*>(Id()), _length)));
				}

			private:
				uint8_t _length;
				union {
					uint8_t* _allocation;
					uint8_t  _buffer[16];
				} _id;
			};
			class Lease
			{
			private:
				Lease& operator= (const Lease&) = delete;

			public:
				inline Lease() : _id(), _expiration(0), _address(0) {
				}
				inline Lease(const Identifier& id, const uint32_t address) : _id(id), _expiration(0), _address(address) {
				}
				inline Lease(const Lease& copy) : _id(copy._id), _expiration(copy._expiration), _address(copy._address) {
				}
				Lease(const Lease&& copy) : _id(copy._id), _expiration(copy._expiration), _address(copy._address) {
				}
				~Lease() {
				}

			public:
				inline bool IsExpired() const {
					return (_expiration < (Core::Time::Now().Ticks()));
				}
				inline const Identifier& Id() const {
					return (_id);
				}
				uint32_t Raw () const {
                                	return (_address);
				}
				Core::NodeId Address() const {
					struct in_addr info;
                                	info.s_addr = htonl(_address);
                                	return (Core::NodeId(info));
				}
				const uint64_t& Expiration() const {
					return (_expiration);
				}
				void Expiration(const uint64_t& time) {
					_expiration = time;
				}
				void Update(const Identifier& id) {
					_id = id;
				}

			private:
				Identifier _id;
				uint64_t _expiration;
				const uint32_t _address;
			};
		private:
			class ScratchPad {
			private:
				ScratchPad() = delete;
				ScratchPad(const ScratchPad&) = delete;
				ScratchPad& operator= (const ScratchPad&) = delete;

			public:
				ScratchPad(const uint8_t dataFrame[], const uint32_t length)
					: _id()
					, _optionData(&(dataFrame[sizeof(CoreMessage)]))
					, _optionSize(length - sizeof(CoreMessage)) {

					const CoreMessage* const message = reinterpret_cast<const CoreMessage*>(dataFrame);

					// Determine the classification
					const uint8_t* locator = GetOption(OPTION_DHCPMESSAGETYPE);
					_classification = (((locator != nullptr) && (locator[0] == 1) && (locator[1] >= 1) && (locator[1] <= 8)) ?
						static_cast<classifications>(locator[1]) : CLASSIFICATION_INVALID);

					// Determine client identifier in proper RFC 2131 order (client identifier option then chaddr)
					locator = ScratchPad::GetOption(OPTION_CLIENTIDENTIFIER);
					_id = Identifier((locator == nullptr ? message->chaddr : &locator[1]), (locator == nullptr ? sizeof(message->chaddr) : locator[0]));

					// Determine preferred IP address
					locator = GetOption(OPTION_REQUESTEDIPADDRESS);
					_preferred = ((locator != nullptr) && (locator[0] == sizeof(uint32_t)) ? ((locator[1] << 24) | (locator[2] << 16) | (locator[3] << 8) | locator[4]) : 0);
				}
				~ScratchPad() {
				}

			public:
				inline const Identifier& Id() const {
					return (_id);
				}
				uint32_t RequestedIP() const {
					return (_preferred);
				}
				inline bool HasClassification() const {
					return (_classification != CLASSIFICATION_INVALID);
				}
				inline classifications Classification() const {
					return (_classification);
				}
				inline uint32_t ServerIdentifier() const {

					const uint8_t* locator = GetOption(OPTION_SERVERIDENTIFIER);
					return ((locator != nullptr) && (locator[0] == sizeof(uint32_t)) ? ((locator[1] << 24) | (locator[2] << 16) | (locator[3] << 8) | locator[4]) : 0);
				}

			private:
				const uint8_t* GetOption(const uint8_t option) const {
					const uint8_t* index = _optionData;

					while (((index - _optionData) < _optionSize) && (*index != OPTION_END) && (*index != option)) {

						if (*index == OPTION_PAD) {
							index++;
						}
						else
						{
							++index;
							// Jump the type..
							if ((index - _optionData) < _optionSize) {
								index += *(index) + 1;
							}
						}
					}
					// If we found the option and the size finds the length, pass it on !!!
					return ((*index == option) && (((index - _optionData) + 1 + index[1]) <= _optionSize) ? &(index[1]) : nullptr);
				}
				inline bool GetValue(const uint8_t option, Core::TextFragment& text) const
				{
					const uint8_t* result = GetOption(option);

					if (result != nullptr)
					{
						text = Core::TextFragment(reinterpret_cast<const char*>(&result[1]), result[0]);

						return(true);
					}
					return (false);
				}

			private:
				Identifier _id;
				const uint8_t* _optionData;
				uint16_t _optionSize;
				uint32_t _preferred;
				classifications _classification;
			};
			class LeaseList : public std::list<Lease> {
			private:
				LeaseList(const LeaseList&) = delete;
				LeaseList& operator= (const LeaseList&) = delete;

			public:
				LeaseList() : std::list<Lease>() {
				}
				~LeaseList() {
				}

			public:
				inline void Lock() {
					_adminLock.Lock();
				}
				inline void Unlock() {
					_adminLock.Unlock();
				}
				inline void ReadLock() const {
					_adminLock.Lock();
				}
				inline void ReadUnlock() const {
					_adminLock.Unlock();
				}

			private:
				mutable Core::CriticalSection	_adminLock;
			};

	

			class Response {
			private:
				Response(const Response&) = delete;
				Response& operator= (const Response&) = delete;

			public:
				Response() {
					Clear();
				}
				~Response() {
				}

			public:
				inline void Clear() {
					::memset(&_dhcpReply, 0, sizeof(_dhcpReply));
					_optionSize = 0;
				}
				inline bool IsValid() const {
					return (_optionData[2] != 0);
				}
				inline uint8_t Option() const {
					return (_optionData[2]);
				}
				inline uint16_t SendData(uint8_t dataFrame[], const uint16_t length) {
					ASSERT (length >= (sizeof(_dhcpReply) + _optionSize + 1));

					::memcpy(dataFrame, &_dhcpReply, sizeof(_dhcpReply));
					if (_optionSize != 0) {
					    ::memcpy(&dataFrame[sizeof(_dhcpReply)], &_optionData, _optionSize);
                                        }
					dataFrame[sizeof(_dhcpReply) + _optionSize] = OPTION_END;

					return (sizeof(_dhcpReply) + _optionSize + 1);
				}
				inline void Base(const std::string& serverName, const uint32_t server, const CoreMessage& message, const uint32_t router, const uint32_t dns) {

					// Server message handling RFC 2131 section 4.3
                    _replyAddress = message.giaddr.s_addr;
					_ciaddr = message.ciaddr.s_addr;
					_yiaddr = message.yiaddr.s_addr;

					_dhcpReply.operation = OPERATION_BOOTREPLY;
					_dhcpReply.htype = message.htype;
					_dhcpReply.hlen = message.hlen;
					// dhcpReply->hops = 0;
					_dhcpReply.xid = message.xid;
					// dhcpReply->ciaddr = 0;
					// dhcpReply->yiaddr = 0;  Or changed below
					_dhcpReply.siaddr.s_addr = server;
					_dhcpReply.flags = (message.giaddr.s_addr != 0 ? htons(BroadcastValue) : 0) | message.flags;
					_dhcpReply.giaddr = message.giaddr;
					::memcpy(_dhcpReply.chaddr, message.chaddr, sizeof(_dhcpReply.chaddr));
					::memcpy(_dhcpReply.sname, serverName.c_str(), std::min(sizeof(_dhcpReply.sname), serverName.length()));
					// dhcpReply->file = 0;

					::memcpy(_dhcpReply.pbMagicCookie, MagicCookie, sizeof(_dhcpReply.pbMagicCookie));

					// DHCP Message Type - RFC 2132 section 9.6
					_optionData[0] = OPTION_DHCPMESSAGETYPE;
					_optionData[1] = 1;
					_optionData[2] = 0;

					// Server Identifier - RFC 2132 section 9.7
					_optionData[3] = OPTION_SERVERIDENTIFIER;
					_optionData[4] = 4;
					::memcpy(&(_optionData[5]), &server, 4);

                    _optionSize = 9;
					if (router != static_cast<uint32_t>(~0)) {
						Router(router);
					}
					if (dns != static_cast<uint32_t>(~0)) {
						DNS(dns);
					}

				}
				inline Core::NodeId Reply() const {
					uint32_t result = _replyAddress;

					// Determine how to send the reply  RFC 2131 section 4.1
                    if (result == 0) {
						switch (_optionData[2]) {
						case CLASSIFICATION_OFFER:
							// Fall-through
						    result = INADDR_BROADCAST;
						    break;
						case CLASSIFICATION_ACK:
							if (_ciaddr == 0) {
								printf("Stiekem altijd 255 \n");
								result = (((htons(BroadcastValue) & _dhcpReply.flags) != 0) ? INADDR_BROADCAST : INADDR_BROADCAST);
							}
							else {
								result = _ciaddr;  // Already in network order
							}
							break;
						case CLASSIFICATION_NAK:
							result = INADDR_BROADCAST;
							break;
						default:
							TRACE_L1("Unknown classification: %d \n", _optionData[2]);
							break;
						}
	
                                        }
					sockaddr_in saClientAddress;
					saClientAddress.sin_family = AF_INET;
					saClientAddress.sin_addr.s_addr = result;
					saClientAddress.sin_port = htons(DefaultDHCPClientPort);

					return (Core::NodeId(saClientAddress));
				}
				inline void Offer(const uint32_t address) {

					_dhcpReply.yiaddr.s_addr = htonl(address);
					_optionData[2] = CLASSIFICATION_OFFER;
				}
				inline void Acknowledge(const bool positive, const uint32_t address) {

					if (positive == true) {

						_optionData[2] = CLASSIFICATION_ACK;
						_dhcpReply.ciaddr.s_addr = htonl(address);
						_dhcpReply.yiaddr.s_addr = _dhcpReply.ciaddr.s_addr;
					}
					else {
						_optionData[2] = CLASSIFICATION_NAK;
					}
				}
				inline void LeaseTime(const uint16_t hours) {

                                        uint32_t value = hours * 60 * 60;

					// IP Address Lease Time - RFC 2132 section 9.2
					_optionData[_optionSize]     = OPTION_IPADDRESSLEASETIME;
					_optionData[_optionSize + 1] = 4;
					_optionData[_optionSize + 2] = (value >> 24) & 0xFF;
					_optionData[_optionSize + 3] = (value >> 16) & 0xFF;
					_optionData[_optionSize + 4] = (value >> 8) & 0xFF;
					_optionData[_optionSize + 5] = value & 0xFF;

                                        _optionSize += 6;
				}
				inline void SubnetMask(const uint8_t bits) {

					const uint32_t mask(static_cast<uint32_t>(~0) << (32 - bits));

					// Subnet Mask - RFC 2132 section 3.3
					_optionData[_optionSize]     = OPTION_SUBNETMASK;
					_optionData[_optionSize + 1] = 4;
					_optionData[_optionSize + 2] = (mask >> 24) & 0xFF;
					_optionData[_optionSize + 3] = (mask >> 16) & 0xFF;
					_optionData[_optionSize + 4] = (mask >> 8) & 0xFF;
					_optionData[_optionSize + 5] = mask & 0xFF;

                                        _optionSize += 6;
				}
				inline void Router(const uint32_t address) {

                                        const uint32_t value = htonl(address);
					_optionData[_optionSize]     = OPTION_ROUTER;
					_optionData[_optionSize + 1] = 4;
					::memcpy(&(_optionData[_optionSize + 2]), &value, 4);
                                        _optionSize += 6;
				}
				inline void DNS (const uint32_t address) {

                                        const uint32_t value = htonl(address);
					_optionData[_optionSize]     = OPTION_DNS;
					_optionData[_optionSize + 1] = 4;
					::memcpy(&(_optionData[_optionSize + 2]), &value, 4);
                                        _optionSize += 6;
				}
	
			private:
				CoreMessage _dhcpReply;
				uint8_t _optionSize;
				uint8_t _optionData[256];
                                uint32_t _replyAddress;
                                uint32_t _ciaddr;
                                uint32_t _yiaddr;
			};

		public:
			typedef Core::LockableIteratorType<const LeaseList, const Lease&, LeaseList::const_iterator> Iterator;

		public:
			DHCPServerImplementation(const string& serverName, const string& interfaceName, const uint32_t poolStart, const uint32_t poolSize, const uint32_t router, const Core::NodeId& DNS)
				: Core::SocketDatagram(false, Core::NodeId("255.255.255.255",DefaultDHCPServerPort), Core::NodeId("255.255.255.255", DefaultDHCPClientPort), 1024, 16384)
				, _serverName(Core::ToString(serverName))
                                , _interfaceName(interfaceName)
                                , _poolStart(poolStart)
                                , _poolSize(poolSize)
				, _minAddress(0)
				, _maxAddress(0)
				, _lastIssued(0)
				, _server(0)
				, _router(router)
				, _dns(~0)
				, _leases()
				, _responses() {
				static_assert(sizeof(uint32_t) == 4, "Incorrect architecture chosen. uint32_t must by 4 bytes");

				if (DNS.IsValid() == true) {
                	_dns = ntohl(static_cast<const Core::NodeId::SocketInfo&>(DNS).IPV4Socket.sin_addr.s_addr);
				} 

				
			}
			virtual ~DHCPServerImplementation() {
			}

		public:
			inline bool IsActive() const {
				return (IsOpen());
			}
			inline const string& Interface() const {
				return(_interfaceName);
                        }
			inline Core::NodeId BeginPool() const {
				struct in_addr info;
                                info.s_addr = htonl(_minAddress);
                                return (Core::NodeId(info));
                        }
			inline Core::NodeId Router() const {
				struct in_addr info;
                                info.s_addr = htonl(_router);
                                return (Core::NodeId(info));
                        }
			inline Core::NodeId EndPool() const {
				struct in_addr info;
                                info.s_addr = htonl(_maxAddress);
                                return (Core::NodeId(info));
                        }
			// IMPORTANT NOTE !!!!
			// The Leases() method will lock the lease list. Lifetime  
			// of the returned Iterator object must be deterministic 
			// and short (<10ms). The Server is "blocked" as long as 
			// the Iterator object is not destructed !!!!
			inline Iterator Leases() const {
				return (Iterator(_leases));
                        }
			uint32_t Open();
			uint32_t Close();

		private:
			// NOTE:
			// The next three methods, Find,Find and Create need to be executed within the lock.
			inline Lease* Find(const uint32_t address) {

				LeaseList::iterator index(_leases.begin());
				while ((index != _leases.end()) && (index->Raw() != address)) {
					index++;
				}

				return (index != _leases.end() ? &(*index) : nullptr);
			}
			inline Lease* Find(const Identifier& id) {
				LeaseList::iterator index(_leases.begin());
				while ((index != _leases.end()) && (index->Id() != id)) {
					index++;
				}

				return (index != _leases.end() ? &(*index) : nullptr);
			}
			inline Lease* Create(const Identifier& id, const uint32_t address) {
				_leases.push_back(Lease(id, address));
				return &(_leases.back());
			}
			void Discover(Response& response, const ScratchPad& scratchPad) {

				_leases.Lock();
				Lease* result = Find(scratchPad.Id());

				// RFC 2131 section 4.3.1
				if ((result == nullptr) && (scratchPad.RequestedIP() != 0)) {
					result = Find(scratchPad.RequestedIP());

					if (result == nullptr) {
						// Ip address has not been taken yet, time to "assign" it to this client.
						result = Create(scratchPad.Id(), scratchPad.RequestedIP());
					}
					else if (result->IsExpired() == true) {
						result->Update(scratchPad.Id());
					}
				}

				if (result == nullptr) {
					uint32_t poolSize(_maxAddress - _minAddress);

					// Seems we have no record for this client, create a new one..
					while ((--poolSize != 0) && (Find(++_lastIssued) != nullptr)) /* INTENTIONALLY LEFT EMPTY */;

					if (poolSize != 0) {
						result = Create(scratchPad.Id(), _lastIssued);
					}
				}

				if (result == nullptr) {
					TRACE(Flow, (string(_T("Looks like we ran out of IP addresses!!"))));
				}
				else {
					response.Offer(result->Raw());
					response.SubnetMask(24);

				}

				_leases.Unlock();
			}
			void Request(Response& response, const ScratchPad& scratchPad) {

				_leases.Lock();

				// RFC 2131 section 4.3.2 Determine requested IP address
				Lease* result = Find(scratchPad.Id());
				uint32_t serverId = scratchPad.ServerIdentifier();
				uint32_t requested = scratchPad.RequestedIP();

				response.Acknowledge((serverId != 0) && (result != nullptr), requested);							
				response.LeaseTime(24);
				_leases.Unlock();
			}
			void Submit(const Core::ProxyType< Response > entry) {
				_responses.push_back(entry);

				if (_responses.size() == 1) {
					SocketDatagram::Trigger();
				}
			}
			// Signal a state change, Opened, Closed or Accepted
			virtual void StateChange() {
			}
			virtual uint16_t SendData(uint8_t dataFrame[], const uint16_t length) {
				uint16_t result = 0;

				if (_responses.size() != 0) {

					Core::ProxyType<Response> entry(_responses.front());

					// Pop the front.
					_responses.pop_front();

					if (entry->IsValid() == false) {
						TRACE_L1("Dropped a response frame as it is invalid. [%d]", __LINE__);
					}
					else {
						SocketDatagram::RemoteNode(entry->Reply());
						TRACE_L1("Sending %d to %s:%d", entry->Option(), RemoteNode().HostAddress().c_str(), RemoteNode().PortNumber());
						result = entry->SendData(dataFrame, length);
					}
				}

				return(result);
			}
			virtual uint16_t ReceiveData(uint8_t dataFrame[], const uint16_t length) {
				const CoreMessage* const message = reinterpret_cast<const CoreMessage*>(dataFrame);
				// Check the size of the message, certain elments need to be in there (RFC 2131 section 3)
				if (sizeof(CoreMessage) > length) {
					TRACE(Trace::Information, (_T("Message is too short for a DHCP request. Size %d"), length));
				}
				else if (message->operation != OPERATION_BOOTREQUEST) {
					TRACE(Trace::Information, (string(_T("Currently, only the OPERATION_BOOTREQUEST is supported!!"))));
				}
				else if (memcmp(MagicCookie, &(dataFrame[sizeof(CoreMessage) - sizeof(MagicCookie)]), sizeof(MagicCookie)) != 0) {
					TRACE(Trace::Information, (string(_T("Magic cookie does not comply."))));
				}
				else if (SocketDatagram::RemoteNode() == SocketDatagram::LocalNode()) {
					TRACE(Trace::Information, (string(_T("Receiving a request from the our-selves, will not respond."))));
				}
				else {
					// We got a legitimate DHCP request. Continue with it.
					ScratchPad scratchPad(dataFrame, length);

					if (scratchPad.HasClassification() == false) {
						TRACE(Trace::Information, (string(_T("Request type unknown or unspecified."))));
					}
					else {
						// Create our selves a response we can reply.
						Core::ProxyType<Response> response(_responseFactory.Element());
						response->Base(_serverName, _server, *message, _router, _dns);

						switch (scratchPad.Classification()) {
						case CLASSIFICATION_DISCOVER:
							Discover(*response, scratchPad);

							break;
						case CLASSIFICATION_REQUEST:
							Request(*response, scratchPad);
							break;
						case CLASSIFICATION_DECLINE:
							// Fall-through
						case CLASSIFICATION_RELEASE:
							// UNSUPPORTED: Mark address as unused
							break;
						case CLASSIFICATION_INFORM:
							// Unsupported DHCP message type - fail silently
							break;
						case CLASSIFICATION_OFFER:
						case CLASSIFICATION_ACK:
						case CLASSIFICATION_NAK:
							TRACE(Trace::Information, (_T("Unexpected DHCP message. Type: %d."), scratchPad.Classification()));
							break;
						default:
							ASSERT(false);
							break;
						}

						if (response.IsValid() == true) {
							Submit(response);
						}
					}
				}
				return (length);
			}

		private:
			std::string _serverName;
                        string _interfaceName;
                        uint32_t _poolStart;
                        uint32_t _poolSize;
			uint32_t _minAddress;
			uint32_t _maxAddress;
			uint32_t _lastIssued;
                        uint32_t _server;
			uint32_t _router;
			uint32_t _dns;
			LeaseList _leases;
			std::list< Core::ProxyType<Response> > _responses;

			static Core::ProxyPoolType<Response> _responseFactory;
		};
	}
} // Namespace WPEFramework::plugin

#endif // __DHCPSERVERIMPLEMENTATION_H__
