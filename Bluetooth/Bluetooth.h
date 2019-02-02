#pragma once

#include "Module.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/mgmt.h>

namespace WPEFramework {

namespace Core {

    struct IOutbound {

        struct ICallback{
            virtual ~ICallback() {}

            virtual void Updated (const Core::IOutbound& data, const uint32_t error_code) = 0;
        };

        virtual ~IOutbound() {};

        virtual uint16_t Id() const = 0;
        virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const = 0;
    };

    struct IInbound {

        virtual ~IInbound() {};

        virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t length) = 0;
        virtual bool IsCompleted() const = 0;
    };

    template <typename CHANNEL>
    class SynchronousChannelType : public CHANNEL {
    private:
        SynchronousChannelType() = delete;
        SynchronousChannelType(const SynchronousChannelType<CHANNEL>&) = delete;
        SynchronousChannelType<CHANNEL>& operator=(const SynchronousChannelType<CHANNEL>&) = delete;

        class Frame {
        private:
            Frame() = delete;
            Frame(const Frame&) = delete;
            Frame& operator= (Frame&) = delete;

            enum state : uint8_t {
                IDLE      = 0x00,
                INBOUND   = 0x01,
                COMPLETE  = 0x02
            };

        public:
            Frame(const IOutbound& message, IInbound* response) 
                : _message(message)
                , _response(response)
                , _state(IDLE)
                , _expired(0)
                , _callback(nullptr) {
            }
            Frame(const IOutbound& message, IInbound* response, IOutbound::ICallback* callback, const uint32_t waitTime) 
                : _message(message)
                , _response(response)
                , _state(IDLE)
                , _expired(Core::Time::Now().Add(waitTime).Ticks())
                , _callback(callback) {
            }
            ~Frame() {
            }

        public:
            IOutbound::ICallback* Callback() const {
                return (_callback);
            }
            const IOutbound& Outbound() const {
                return (_message);
            }
            const IInbound* Inbound() const {
                return (_response);
            }
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) {
                uint16_t result = _message.Serialize(dataFrame, maxSendSize);

                if (result < maxSendSize) {
                    _state = (_response == nullptr ?  COMPLETE : INBOUND);
                }
                return (result);
            }
            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData) {
                uint16_t result = 0;

                if (_response != nullptr) {
                    result = _response->Deserialize(dataFrame, availableData);
                    if (_response->IsCompleted() == true) {
                        _state = COMPLETE;
                    }
                }
                return (result);
            }
            bool operator== (const IOutbound& message) const {
                return (&_message == &message);
            }
            bool operator!= (const IOutbound& message) const {
                return (&_message != &message);
            }
            bool IsSend() const {
                return (_state != IDLE);
            }
            bool CanBeRemoved () const {
                return ((_state == COMPLETE) && (_expired != 0));
            }
            bool IsComplete() const {
                return (_state == COMPLETE);
            }
            bool IsExpired() const {
                return ((_expired != 0) && (_expired < Core::Time::Now().Ticks()));
            }

        private:
            const IOutbound& _message;
            IInbound* _response;
            state _state;
            uint64_t _expired;
            IOutbound::ICallback* _callback;
        };

    public:
        template <typename... Args>
        SynchronousChannelType(Args... args)
            : CHANNEL(args ...)
            , _adminLock()
            , _queue()
            , _reevaluate (false, true)
            , _waitCount(0) {
        }
        virtual ~SynchronousChannelType() {
            CHANNEL::Close(Core::infinite);
        }

        uint32_t Exchange(const IOutbound& message, const uint32_t waitTime) {

            _adminLock.Lock();

            _queue.emplace_back(message, nullptr);

            uint32_t result = Completed(_queue.back(), waitTime);

            Revoke (message);

            _adminLock.Unlock();

            return (result);
        }

        uint32_t Exchange(const IOutbound& message, IInbound& response, const uint32_t waitTime) {

            _adminLock.Lock();

            _queue.emplace_back(message, response);

            uint32_t result = Completed(_queue.back(), waitTime);

            Revoke (message);

            _adminLock.Unlock();

            return (result);
        }

        void Send(const uint32_t waitTime, IOutbound::ICallback* callback, const IOutbound& message, IInbound* response = nullptr) {

            _adminLock.Lock();

            _queue.emplace_back(message, response, callback, waitTime);
            bool trigger = (_queue.size() == 1);

            _adminLock.Unlock();

            if (trigger == true) {
                CHANNEL::Trigger();
            }
        }

        void Revoke(const IOutbound& message) {
            bool trigger = false;

            _adminLock.Lock();

            typename std::list<Frame>::iterator index = std::find(_queue.begin(), _queue.end(), message);
            if (index != _queue.end()) {
                trigger = (_queue.size() > 1) && (index == _queue.begin());
                _queue.erase (index);
            }

            _adminLock.Unlock();

            if (trigger == true) {
                CHANNEL::Trigger();
            }
        }

    protected:        
        int Handle() const {
            return (static_cast<const Core::IResource&>(*this).Descriptor());
        }
        void Lock() {
            _adminLock.Lock();
        }
        void Unlock() {
            _adminLock.Unlock();
        }
        virtual void StateChange () override
	{
            Reevaluate();
        }
        virtual uint16_t Deserialize (const uint8_t* dataFrame, const uint16_t availableData) = 0;

    private:        
        void Cleanup() {
            while ( (_queue.size() > 0) && (_queue.front().IsExpired() == true) ) {
                Frame& frame = _queue.front();
                if (frame.Callback() != nullptr) {
                    frame.Callback()->Updated(frame.Outbound(), Core::ERROR_TIMEDOUT);
                }
                _queue.pop_front();
            }
        }
        void Reevaluate() {
            _reevaluate.SetEvent();
 
            while (_waitCount != 0) {
                SleepMs(0);
            }
 
            _reevaluate.ResetEvent();
        }
        uint32_t Completed (const Frame& request, const uint32_t allowedTime) {

            Core::Time now = Core::Time::Now();
            Core::Time endTime = Core::Time(now).Add(allowedTime);
            uint32_t result = Core::ERROR_NONE;

            if (_queue.size() == 1) {

                _adminLock.Unlock();

                CHANNEL::Trigger();

                _adminLock.Lock();
            }

            while ( (CHANNEL::IsOpen() == true) && (request.IsComplete() == false) && (endTime > now) ) {
                uint32_t remainingTime = (endTime.Ticks() - now.Ticks()) / Core::Time::TicksPerMillisecond;

                _waitCount++;

                _adminLock.Unlock();

                result == _reevaluate.Lock(remainingTime);

                _waitCount--;

                _adminLock.Lock();
                
                now = Core::Time::Now();
            }

            return (request.IsComplete() ? Core::ERROR_NONE : Core::ERROR_ASYNC_ABORTED);
        }

        // Methods to extract and insert data into the socket buffers
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
        {
            uint16_t result = 0;

            _adminLock.Lock();

            // First clear all potentially expired objects...
            Cleanup();

            if (_queue.size() > 0) {
                Frame& frame = _queue.front();
            
                if (frame.IsSend() == false) {
                    result = frame.SendData(dataFrame, maxSendSize);
                    if (frame.CanBeRemoved() == true) {
                        _queue.pop_front();
                        if (frame.Callback() != nullptr) {
                            frame.Callback()->Updated(frame.Outbound(), Core::ERROR_NONE);
                        }
                        Reevaluate();
                    }
                    else if (frame.IsSend() == true) {
                        Reevaluate();
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData) override
        {
            uint16_t result = 0;

            _adminLock.Lock();

            // First clear all potentially expired objects...
            Cleanup();

            if (_queue.size() > 0) {
                Frame& frame = _queue.front();

                result = frame.ReceiveData(dataFrame, availableData);

                if (frame.CanBeRemoved() == true) {
                    ASSERT(frame.Inbound() != nullptr);
                    _queue.pop_front();
                    if (frame.Callback() != nullptr) {
                        frame.Callback()->Updated(frame.Outbound(), Core::ERROR_NONE);
                    }
                    CHANNEL::Trigger();
                    Reevaluate();
                } 
                else if (frame.IsComplete()) {
                    Reevaluate();
                }
            }
            if (result < availableData) { 
                result += Deserialize(&(dataFrame[result]), availableData - result);
            }

            _adminLock.Unlock();

            return (result);
        }

    private:
        Core::CriticalSection _adminLock;
        std::list<Frame> _queue;
        Core::Event _reevaluate;
        std::atomic<uint32_t> _waitCount;
    };

} // namespace Core


namespace Bluetooth {

    class Address {
    public:
        Address() : _length(0) {
        }
        Address(const int handle) : _length(0) {
            if (handle > 0) {
                if (hci_devba(handle, &_address) >= 0) {
                    _length = sizeof(_address);
                }
            }
        }
        Address(const bdaddr_t& address) : _length(sizeof(_address)) {
            ::memcpy(&_address, &address, sizeof(_address));
        }
        Address(const TCHAR address[]) : _length(sizeof(_address)) {
            ::memset(&_address, 0, sizeof(_address));
            str2ba(address, &_address);
        }
        Address(const Address& address) : _length(address._length) {
            ::memcpy (&_address, &(address._address), sizeof(_address));
        }
        ~Address() {
        }

    public:
        Address& operator= (const Address& rhs) {
            _length = rhs._length;
            ::memcpy (&_address, &(rhs._address), sizeof(_address));
            return (*this);
        }
        bool IsValid() const {
            return (_length == sizeof(_address));
        }
        bool Default() {
            _length = 0;
            int deviceId = hci_get_route(nullptr);
            if ((deviceId >= 0) && (hci_devba(deviceId, &_address) >= 0)) {
                _length = sizeof(_address);
            }
            return (IsValid());
        }
        const bdaddr_t* Data () const {
            return (IsValid() ? &_address : nullptr);
        }
        uint8_t Length () const {
            return (_length);
        }
        Core::NodeId NodeId(const uint16_t channelType) const {
            Core::NodeId result;
            int deviceId = hci_get_route(const_cast<bdaddr_t*>(Data()));

            if (deviceId >= 0) {
                result = Core::NodeId(static_cast<uint16_t>(deviceId), channelType);
            }

            return (result);
        }
        Core::NodeId NodeId(const uint16_t cid, const uint8_t addressType) const {
            return (Core::NodeId (_address, cid, addressType)); 
        }
        bool operator== (const Address& rhs) const {
            return ((_length == rhs._length) && (memcmp(rhs._address.b, _address.b, _length) == 0));
        }
        bool operator!= (const Address& rhs) const {
            return(!operator==(rhs));
        }
        void OUI(char oui[9]) const {
            ba2oui(Data(), oui);
        }
        string ToString() const {
            static constexpr TCHAR _hexArray[] = "0123456789ABCDEF";
            string result;

            if (IsValid() == true) {
                for (uint8_t index = 0; index < _length; index++) {
                    if (result.empty() == false) {
                        result += ':';
                    }
                    result += _hexArray[(_address.b[(_length - 1) - index] >> 4) & 0x0F];
                    result += _hexArray[_address.b[(_length - 1) - index] & 0x0F];
                }
            }

            return (result);
        }

    private:
        bdaddr_t _address;
        uint8_t _length;
    };

    class HCISocket : public Core::SynchronousChannelType<Core::SocketPort> {
    private:
        HCISocket& operator=(const HCISocket&) = delete;

        static constexpr int      SCAN_TIMEOUT           = 1000;
        static constexpr uint8_t  SCAN_TYPE              = 0x01;
        static constexpr uint8_t  SCAN_FILTER_POLICY     = 0x00;
        static constexpr uint8_t  SCAN_FILTER_DUPLICATES = 0x01;
        static constexpr uint8_t  EIR_NAME_SHORT         = 0x08;
        static constexpr uint8_t  EIR_NAME_COMPLETE      = 0x09;

    public:
        struct IScanning {
            virtual ~IScanning () {}

            virtual void DiscoveredDevice (const bool lowEnergy, const Address&, const string& name) = 0;
        };

        template<typename OUTBOUND>
        class ManagementType : public Core::IOutbound{
        private: 
            ManagementType() = delete;
            ManagementType(const ManagementType<OUTBOUND>&) = delete;
            ManagementType<OUTBOUND>& operator= (const ManagementType<OUTBOUND>&) = delete;

        public:
            ManagementType(const uint16_t adapterIndex) 
                : _offset(sizeof(_buffer)) {
                _buffer[2] = (adapterIndex & 0xFF);
                _buffer[3] = ((adapterIndex >> 8) & 0xFF);
                _buffer[4] = static_cast<uint8_t>(sizeof(OUTBOUND) & 0xFF);
                _buffer[5] = static_cast<uint8_t>((sizeof(OUTBOUND) >> 8) & 0xFF);
            }
            virtual ~ManagementType() {
            }

        public:
            void Clear () {
                _offset = 0;
                ::memset(&(_buffer[6]), 0, sizeof(_buffer) - 6);
            }
            OUTBOUND* operator->() {
                return (reinterpret_cast<OUTBOUND*>(&(_buffer[6])));
            }
            uint32_t Send (HCISocket& socket, const uint32_t waitTime, const uint16_t opCode) {
                _offset = 0;
                _buffer[0] = (opCode & 0xFF);
                _buffer[1] = ((opCode >> 8) & 0xFF);

                uint32_t result = socket.Exchange(*this, waitTime);
            }

        private:
            virtual uint16_t Id() const {
                return ((_buffer[0] << 8) | _buffer[1]);
            }
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const {
                uint16_t result = std::min(static_cast<uint16_t>(sizeof(_buffer) - _offset), length);
                if (result > 0) {

                    ::memcpy (stream, &(_buffer[_offset]), result);
                    // for (uint8_t index = 0; index < result; index++) { printf("%02X:", stream[index]); } printf("\n");
                    _offset += result;
                }
                return (result);
            }

        private:
            mutable uint16_t _offset;
            uint8_t _buffer[6 + sizeof(OUTBOUND)];
        };

        template<const uint16_t OPCODE, typename OUTBOUND>
        class CommandType : public Core::IOutbound{
        private: 
            CommandType(const CommandType<OPCODE,OUTBOUND>&) = delete;
            CommandType<OPCODE,OUTBOUND>& operator= (const CommandType<OPCODE,OUTBOUND>&) = delete;

        public:
            enum : uint16_t { ID = OPCODE };

        public:
            CommandType() 
                : _parent(nullptr) 
                , _offset(sizeof(_buffer)) {
            }
            virtual ~CommandType() {
            }

        public:
            OUTBOUND* operator->() {
                return (reinterpret_cast<OUTBOUND*>(&(_buffer[4])));
            }
            void Clear () {
                _offset = 0;
                _buffer[0] = HCI_COMMAND_PKT;
                _buffer[1] = (OPCODE & 0xFF);
                _buffer[2] = ((OPCODE >> 8) & 0xFF);
                _buffer[3] = static_cast<uint8_t>(sizeof(OUTBOUND));
                ::memset(&(_buffer[4]), 0, sizeof(_buffer) - 4);
            }
            void Send (HCISocket& socket, const uint32_t waitTime, IOutbound::ICallback* callback) {
                _parent = &socket;
                socket.Send(waitTime, callback, *this, nullptr);
            }

        protected:
            void Parent(HCISocket& socket) {
                _parent = &socket;
            }

        private:
            virtual uint16_t Id() const {
                return (ID);
            }
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const {
                uint16_t result = std::min(static_cast<uint16_t>(sizeof(_buffer) - _offset), length);
                if (result > 0) {

                    // Seems we need to push "our" message, register it for whatever godsake reason...
                    _parent->SetOpcode(OPCODE);
                    
                    ::memcpy (stream, &(_buffer[_offset]), result);
                    _offset += result;
                }
                return (result);
            }

        private:
            HCISocket* _parent;
            mutable uint16_t _offset;
            uint8_t _buffer[1 + 3 + sizeof(OUTBOUND)];
        };

        template<const uint16_t OPCODE, typename OUTBOUND, typename INBOUND, const uint16_t RESPONSECODE>
        class ExchangeType : public CommandType<OPCODE, OUTBOUND>, public Core::IInbound {
        private: 
            ExchangeType(const ExchangeType<OPCODE, OUTBOUND, INBOUND, RESPONSECODE>&) = delete;
            ExchangeType<OPCODE, OUTBOUND, INBOUND, RESPONSECODE>& operator= (const ExchangeType<OPCODE, OUTBOUND, INBOUND, RESPONSECODE>&) = delete;

        public:
            ExchangeType() 
                : CommandType<OPCODE, OUTBOUND>()
                , _error(~0) {
            }
            virtual ~ExchangeType() {
            }
           
        public:
            uint32_t Error() const {
                return (_error);
            }
            const INBOUND& Response() const {
                return(_response);
            }
            void Send (HCISocket& socket, const uint32_t waitTime, Core::IOutbound::ICallback* callback) {
                CommandType<OPCODE, OUTBOUND>::Parent(socket);
                ::memset (&_response, 0, sizeof(INBOUND));
                socket.Send(waitTime, callback, *this, this);
            }

        private:
            virtual bool IsCompleted() const override {
                return (_error != ~0);
            }
            virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override {
                uint16_t result = 0;
                if (length >= (HCI_EVENT_HDR_SIZE + 1)) {
                    const hci_event_hdr* hdr = reinterpret_cast<const hci_event_hdr*>(&(stream[1]));
                    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&(stream[1 + HCI_EVENT_HDR_SIZE]));
                    uint16_t len = (length - (1 + HCI_EVENT_HDR_SIZE));
  
                    switch (hdr->evt) {
                    case EVT_CMD_STATUS: 
                    {
                         const evt_cmd_status* cs = reinterpret_cast<const evt_cmd_status*>(ptr);
                         if (cs->opcode == RESPONSECODE) {
                             if (cs->status != 0) {
                                 _error = cs->status;
                             }
                         }
                         break;
                    }
                    case EVT_CMD_COMPLETE:
                    {
                         const evt_cmd_complete* cc = reinterpret_cast<const evt_cmd_complete*>(ptr);;
                         if ( (cc->opcode == RESPONSECODE) && (len > EVT_CMD_COMPLETE_SIZE) ) {
                             uint16_t length = std::min(static_cast<uint16_t>(sizeof(INBOUND)), static_cast<uint16_t>(len - EVT_CMD_COMPLETE_SIZE));

                             ::memcpy(reinterpret_cast<uint8_t*>(_response), &(ptr[EVT_CMD_COMPLETE_SIZE]), length);
                             _error = Core::ERROR_NONE;
                         }
                         else {
                             _error = Core::ERROR_GENERAL;
                         }
                         break;
                    }
                    default:
                         break;
                    }
                }
                return (result);
            }

        private:
            INBOUND _response;
            uint16_t _error;
        };

        
        static constexpr uint32_t MAX_ACTION_TIMEOUT = 1000; /* 2 Seconds for commands to complete ? */
        static constexpr uint16_t ACTION_MASK = 0x3FFF;

    public:
        enum state : uint16_t {
            IDLE          = 0x0000,
            SCANNING      = 0x0001,
            CONNECTING    = 0x0002,
            DISCONNECTING = 0x0004,
            METADATA      = 0x0008,
            ABORT         = 0x4000,
            LOW_ENERGY    = 0x8000
        };

        // ------------------------------------------------------------------------
        // Create definitions for the HCI commands
        // ------------------------------------------------------------------------
        struct Command {
        typedef ExchangeType<cmd_opcode_pack(OGF_LINK_CTL,OCF_CREATE_CONN), create_conn_cp, evt_conn_complete, EVT_CONN_COMPLETE>
                Connect;

        typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL,OCF_LE_CREATE_CONN), le_create_connection_cp, evt_le_connection_complete, EVT_LE_CONN_COMPLETE>  
                ConnectLE;

        typedef ExchangeType<cmd_opcode_pack(OGF_LINK_CTL,OCF_DISCONNECT), disconnect_cp, evt_disconn_complete, EVT_DISCONN_COMPLETE>
                Disconnect;

        typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL,OCF_REMOTE_NAME_REQ), remote_name_req_cp, evt_remote_name_req_complete, EVT_REMOTE_NAME_REQ_COMPLETE>
                RemoteName;
        };
 
        // ------------------------------------------------------------------------
        // Create definitions for the Management commands
        // ------------------------------------------------------------------------
        typedef ManagementType<mgmt_mode> ManagementMode;

    public:
        HCISocket()
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::RAW, Core::NodeId(), Core::NodeId(), 256, 256)
            , _state(IDLE) {
        }
        HCISocket(const Core::NodeId& sourceNode)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::RAW, sourceNode, Core::NodeId(), 256, 256)
            , _state(IDLE) {

       }
        virtual ~HCISocket() {
            Close(Core::infinite);
        }

    public:
        bool IsScanning() const {
            return ((_state & SCANNING) != 0);
        }
        void Scan(IScanning* callback, const uint16_t scanTime, const uint32_t type, const uint8_t flags) {

            ASSERT (scanTime <= 326);

            Lock();

            if ((_state & ACTION_MASK) == 0) {
                int descriptor = Handle();

                _state.SetState(static_cast<state>(_state.GetState() | SCANNING));

                Unlock();

                ASSERT (descriptor >= 0);

                void* buf = ALLOCA(sizeof(struct hci_inquiry_req) + (sizeof(inquiry_info) * 128));
                struct hci_inquiry_req* ir = reinterpret_cast<struct hci_inquiry_req*> (buf);
                std::list<Address> reported;

                ir->dev_id  = hci_get_route(nullptr);
                ir->num_rsp = 128;
                ir->length  = (((scanTime * 100) + 50) / 128);
                ir->flags   = flags|IREQ_CACHE_FLUSH;
                ir->lap[0]  = (type >> 16) & 0xFF; // 0x33;
                ir->lap[1]  = (type >> 8)  & 0xFF; // 0x8b;
                ir->lap[2]  = type & 0xFF;         // 0x9e;
                // Core::Time endTime = Core::Time::Now().Add(scanTime * 1000);

                // while ((ir->length != 0) && (ioctl(descriptor, HCIINQUIRY, reinterpret_cast<unsigned long>(buf)) >= 0)) {
                if (ioctl(descriptor, HCIINQUIRY, reinterpret_cast<unsigned long>(buf)) >= 0) {

                    for (uint8_t index = 0; index < (ir->num_rsp); index++) {
                        inquiry_info* info = reinterpret_cast<inquiry_info*>(&(reinterpret_cast<uint8_t*>(buf)[sizeof(hci_inquiry_req)]));

                        bdaddr_t* address = &(info[index].bdaddr);
                        Address newSource (*address);

                        std::list<Address>::const_iterator finder(std::find(reported.begin(), reported.end(), newSource));

                        if (finder == reported.end()) {
                            char name[250];
                            name[0] = '\0';
                            uint8_t pos = 0;

                            if (hci_read_remote_name_with_clock_offset(descriptor,
                                        address,
                                        info[index].pscan_rep_mode,
                                        info[index].clock_offset | 0x8000,
                                        sizeof(name), name, 100000) >= 0) {

                                while ((pos < sizeof(name)) && (name[pos] != '\0')) {
                                    if (::isprint(name[pos]) == 0) name[pos] = '.';
                                    pos++;
                                }
                            }
                            if (pos == 0) {
                                printf("Entry[%s] has no name. Do not report it.\n", Address(*address).ToString().c_str());
                            }
                            else {
                                reported.push_back(newSource);
                                callback->DiscoveredDevice(false, newSource, string(name, pos));
                            }
                        }
                    }

                    // Reset go for the next round !!
                    // ir->length  = (endTime <= Core::Time::Now() ? 0 : 1);
                    // ir->num_rsp = 128;
                    // ir->flags  &= ~IREQ_CACHE_FLUSH;
                }

                Lock();

                _state.SetState(static_cast<state>(_state.GetState() & (~(ABORT|SCANNING))));
            }

            Unlock();
        }
        void Scan (IScanning* callback, const uint16_t scanTime, const bool limited, const bool passive) {
            Lock();

            if ((_state & ACTION_MASK) == 0) {
                int descriptor = Handle();

                ASSERT (descriptor >= 0);

                const uint16_t interval = (limited ? 0x12 : 0x10);
                const uint16_t window   = (limited ? 0x12 : 0x10);
                const uint8_t  scanType = (passive ? 0x00 : 0x01);

                if (hci_le_set_scan_parameters(descriptor, scanType, htobs(interval), htobs(window), LE_PUBLIC_ADDRESS, SCAN_FILTER_POLICY, SCAN_TIMEOUT) >= 0) {
                    _callback = callback;

                    if (hci_le_set_scan_enable(descriptor, 1, SCAN_FILTER_DUPLICATES, SCAN_TIMEOUT) >= 0) {
                        _state.SetState(static_cast<state>(_state.GetState() | (SCANNING|LOW_ENERGY)));

                        // Now lets wait for the scanning period..
                        Unlock();

                        printf ("Entering scan wait...\n");
                        _state.WaitState(ABORT, scanTime*1000);
                        printf ("Leaving scan wait...\n");
                        
                        Lock();

                        _callback = nullptr;
                        
                        printf ("Disable scan...\n");
                        hci_le_set_scan_enable(descriptor, 0, SCAN_FILTER_DUPLICATES, SCAN_TIMEOUT);
                        printf ("Disabled scan...\n");

                        _state.SetState(static_cast<state>(_state.GetState() & (~(ABORT|SCANNING|LOW_ENERGY))));
                    }
                }
            }

            Unlock();
        }
        void Abort() {

            Lock();

            if ((_state & ACTION_MASK) != 0) {
                // TODO: Find if we can actually abort a IOCTL:HCIINQUIRY !!
                _state.SetState (static_cast<state>(_state.GetState() | ABORT));
            }

            Unlock();
        }
        void Connect (Core::IOutbound::ICallback* callback, const bool lowEnergy, const Address& remote) {

            Lock();

            if ((_state & ACTION_MASK) == 0) {
                if (lowEnergy == false) {

                    _cmds.connect.Clear();
                    _cmds.connect->bdaddr         = *remote.Data();
                    _cmds.connect->pkt_type       = htobs(HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5);
                    _cmds.connect->pscan_rep_mode = 0x02;
                    _cmds.connect->clock_offset   = 0x0000;
                    _cmds.connect->role_switch    = 0x01;
                    _cmds.connect.Send(*this, MAX_ACTION_TIMEOUT, callback);

                    _state.SetState(static_cast<state>(_state.GetState() | CONNECTING));
                }
                else {

                    _cmds.connectLE.Clear();
                    _cmds.connectLE->interval = htobs(0x0004);
                    _cmds.connectLE->window = htobs(0x0004);
                    _cmds.connectLE->initiator_filter = 0;
                    _cmds.connectLE->peer_bdaddr_type = LE_PUBLIC_ADDRESS;
                    _cmds.connectLE->peer_bdaddr = *remote.Data();
                    _cmds.connectLE->own_bdaddr_type = LE_PUBLIC_ADDRESS;
                    _cmds.connectLE->min_interval = htobs(0x000F);
                    _cmds.connectLE->max_interval = htobs(0x000F);
                    _cmds.connectLE->latency = htobs(0x0000);
                    _cmds.connectLE->supervision_timeout = htobs(0x0C80);
                    _cmds.connectLE->min_ce_length = htobs(0x0001);
                    _cmds.connectLE->max_ce_length = htobs(0x0001);
                    _cmds.connectLE.Send(*this, MAX_ACTION_TIMEOUT, callback);

                    _state.SetState(static_cast<state>(_state.GetState() | (CONNECTING|LOW_ENERGY)));
                }
            }

            Unlock();
        }
        void Disconnect (Core::IOutbound::ICallback* callback, const uint16_t handle, const uint8_t reason) {

            Lock();

            if ((_state & ACTION_MASK) == 0) {

                _cmds.disconnect->handle = htobs(handle);
                _cmds.disconnect->reason = reason;
                _cmds.disconnect.Send(*this, MAX_ACTION_TIMEOUT, callback);
                _state.SetState(static_cast<state>(_state.GetState() | DISCONNECTING));
            }

            Unlock();
        }
        void RemoteName(Core::IOutbound::ICallback* callback, const Address& remote) {

            Lock();

            if ((_state & ACTION_MASK) == 0) {

                _cmds.name.Clear();
                _cmds.name->bdaddr         = *remote.Data();
                _cmds.name->pscan_mode     = 0x00;
                _cmds.name->pscan_rep_mode = 0x02;
                _cmds.name->clock_offset   = 0x0000;
                _cmds.name.Send(*this, MAX_ACTION_TIMEOUT, callback);
                _state.SetState(static_cast<state>(_state.GetState() | METADATA));
            }

            Unlock();
        }

    private:
        void SetOpcode(const uint16_t opcode) {
            hci_filter_set_opcode(opcode, &_filter);
            setsockopt(Handle(), SOL_HCI, HCI_FILTER, &_filter, sizeof(_filter));
        }
        virtual uint16_t Deserialize (const uint8_t* dataFrame, const uint16_t availableData) override
        {
            const hci_event_hdr* hdr = reinterpret_cast<const hci_event_hdr*>(&(dataFrame[1]));
            const uint8_t* ptr = reinterpret_cast<const uint8_t*>(&(dataFrame[1 + HCI_EVENT_HDR_SIZE]));

            printf("Received Event...\n");
  
            switch (hdr->evt) {
            case EVT_LE_META_EVENT:
                 const evt_le_meta_event* eventMetaData = reinterpret_cast<const evt_le_meta_event*>(ptr);

                 if (eventMetaData->subevent == 0x02) {
                     string shortName;
                     string longName;
                     const le_advertising_info* advertisingInfo = reinterpret_cast<const le_advertising_info*>(&(eventMetaData->data[1]));
                     uint16_t offset = 0;
                     uint16_t length = advertisingInfo->length;
                     const uint8_t* buffer = advertisingInfo->data;
                     const char* name = nullptr;
                     uint8_t pos = 0;

                     while (((offset + buffer[offset]) <= advertisingInfo->length) && (buffer[offset] != 0)) {

                         if ( ((buffer[offset+1] == EIR_NAME_SHORT) && (name == nullptr)) ||
                              (buffer[offset+1] == EIR_NAME_COMPLETE) ) {
                             name = reinterpret_cast<const char*>(&(buffer[offset+2]));
                             pos  = buffer[offset] - 1;
                         }
                         offset += (buffer[offset] + 1);
                     }

                     if ( (name == nullptr) || (pos == 0) ) {
                         TRACE_L1("Entry[%s] has no name. Do not report it.", Address(advertisingInfo->bdaddr).ToString());
                     }
                     else {
                         _state.Lock();
                         if (_callback != nullptr) {
                             _callback->DiscoveredDevice (true, Address(advertisingInfo->bdaddr), string (name, pos));
                         }
                         _state.Unlock();
                     }
                 }
                 break;
 
            }

            return (availableData);
        }
        virtual void StateChange() override {
            Core::SynchronousChannelType<Core::SocketPort>::StateChange();
            if (IsOpen() == true) {
                hci_filter_clear(&_filter);
                hci_filter_set_ptype(HCI_EVENT_PKT,  &_filter);
                hci_filter_set_event(EVT_CMD_STATUS, &_filter);
                hci_filter_set_event(EVT_CMD_COMPLETE, &_filter);
                hci_filter_set_event(EVT_LE_META_EVENT, &_filter);

                // Interesting why this needs to be set.... I hope not!!
                // hci_filter_set_opcode(0, &_filter);

                setsockopt(Handle(), SOL_HCI, HCI_FILTER, &_filter, sizeof(_filter));
            }
        }

    private:
        Core::StateTrigger<state> _state;
        IScanning* _callback;
        struct hci_filter _filter;
        union Commands {
           Commands() {}
           ~Commands() {}

           Command::Connect    connect;
           Command::ConnectLE  connectLE;
           Command::Disconnect disconnect;
           Command::RemoteName name;
        } _cmds;
    };

    class L2Socket : public Core::SocketPort {
    private:
        L2Socket(const L2Socket&) = delete;
        L2Socket& operator=(const L2Socket&) = delete;

        enum state {
            DETECTING,
            METADATA_ID,
            METADATA_NAME_HANDLE,
            METADATA_NAME,
            METADATA_DESCRIPTORS_HANDLE,
            METADATA_DESCRIPTORS,
            OPERATIONAL
        };

        class CommandFrame {
        private: 
            CommandFrame(const CommandFrame&) = delete;
            CommandFrame& operator= (const CommandFrame&) = delete;

        public:
            CommandFrame()
                : _offset(0)
                , _maxSize(64)
                , _buffer(reinterpret_cast<uint8_t*>(::malloc(_maxSize)))
                , _size(0) {
            }
            ~CommandFrame() {
                ::free (_buffer);
            }
           
        public:
            uint16_t Serialize(uint8_t stream[], const uint16_t length) const {
                uint16_t copyLength = std::min(static_cast<uint16_t>(_size - _offset), length);
                if (copyLength > 0) {
                    ::memcpy (stream, &(_buffer[_offset]), copyLength);
                    _offset += copyLength;
                }
                return (copyLength);
            }
            void Clear() {
                _offset = 0;
                _size = 0;
            }
            template<typename VALUE>
            inline CommandFrame& Add(const VALUE& value) {

                if ((sizeof(VALUE) + _size) > _maxSize) {
                    _maxSize = _maxSize + (8 * sizeof(VALUE));
                    uint8_t* newBuffer = reinterpret_cast<uint8_t*>(::malloc(_maxSize));
                    ::memcpy(newBuffer, _buffer, _size);
                    ::free(_buffer);
                }
                ::memcpy (&(_buffer[_size]), &value, sizeof(VALUE));
                _size += sizeof(VALUE);

                return (*this);
            }
        private:
            mutable uint16_t _offset;
            uint16_t _maxSize;
            uint8_t* _buffer;
            uint16_t _size;
        };

    public:
        static constexpr uint8_t LE_PUBLIC_ADDRESS        = 0x00;
        static constexpr uint8_t LE_RANDOM_ADDRESS        = 0x01;
        static constexpr uint8_t LE_ATT_CID               = 4;

        static constexpr uint8_t ATT_OP_ERROR             = 0x01;
        static constexpr uint8_t ATT_OP_FIND_INFO_REQ     = 0x04;
        static constexpr uint8_t ATT_OP_FIND_INFO_RESP    = 0x05;
        static constexpr uint8_t ATT_OP_READ_BY_TYPE_REQ  = 0x08;
        static constexpr uint8_t ATT_OP_READ_BY_TYPE_RESP = 0x09;
        static constexpr uint8_t ATT_OP_READ_REQ          = 0x0A;
        static constexpr uint8_t ATT_OP_READ_BLOB_REQ     = 0x0C;
        static constexpr uint8_t ATT_OP_READ_BLOB_RESP    = 0x0D;
        static constexpr uint8_t ATT_OP_WRITE_REQ         = 0x12;
        static constexpr uint8_t ATT_OP_HANDLE_NOTIFY     = 0x1B;
        static constexpr uint8_t ATT_OP_WRITE_RESP        = 0x13;

        static constexpr uint16_t DEVICE_NAME_UUID        = 0x2a00;
        static constexpr uint16_t REPORT_MAP_UUID         = 0x2a4b;
 
        class Metadata {
        private:
            Metadata (const Metadata&) = delete;
            Metadata& operator= (const Metadata&) = delete;

        public:
            Metadata ()
                : _vendorId(0)
                , _productId(0)
                , _version(0)
                , _name()
                , _blob() {
            }
            ~Metadata() {
            }

        public:
            inline uint16_t VendorId () const {
                    return (_vendorId);
            }
            inline uint16_t ProductId() const {
                    return (_productId);
            }
            inline uint16_t Version() const {
                    return (_version);
            }
            inline const string& Name() const {
                    return (_name);
            }
            inline const uint8_t* Blob() const {
                    return (_blob);
            }
            inline uint16_t Length() const {
                    return (sizeof(_blob));
            }
            inline uint16_t Country() const {
                    return (0);
            }

        private:
            friend class L2Socket;

            uint16_t _vendorId;
            uint16_t _productId;
            uint16_t _version;
            string _name;
            uint8_t _blob[8 * 22];
        }; 

    public:
        L2Socket(const Core::NodeId& localNode, const Core::NodeId& remoteNode, const uint16_t min, const uint16_t max, const uint16_t bufferSize)
            : SocketPort(SocketPort::SEQUENCED, localNode, remoteNode, bufferSize, bufferSize)
            , _state(DETECTING)
            , _metadata()
            , _min(min)
            , _max(max)
            , _handle(0)
            , _offset(0)
            , _frame() {
 
        }
        virtual ~L2Socket() {
            Close(Core::infinite);
        }

    public:
        inline bool IsDetecting() const {
            return (_state == DETECTING);
        }
        inline bool IsOperational() const {
            return (_state == OPERATIONAL);
        }
        inline const Metadata& Info() const {
            return (_metadata);
        }
        bool Security (const uint8_t level, const uint8_t keySize) {
            bool result = true;
            struct bt_security btSecurity;
            memset(&btSecurity, 0, sizeof(btSecurity));
            btSecurity.level = level;
            btSecurity.key_size = keySize;
            if (::setsockopt(Handle(), SOL_BLUETOOTH, BT_SECURITY, &btSecurity, sizeof(btSecurity))) {
                TRACE(Trace::Error, ("Failed to set L2CAP Security level for Device [%s]", RemoteId().c_str()));
                result = false;
            }
            return (result);
        }

        virtual void Received(const uint8_t dataFrame[], const uint16_t availableData) = 0;

    protected:
        void CreateFrame(const uint8_t type, const uint8_t length, const uint16_t entries[]) {
            _frame.Clear();
            _frame.Add<uint8_t>(type);
            for (uint8_t index = 0; index < length; index++) {
                _frame.Add<uint16_t>(htobs(entries[index]));
            }
        }

    private:        
        int Handle() const {
            return (static_cast<const Core::IResource&>(*this).Descriptor());
        }
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override {
            uint16_t length = _frame.Serialize(dataFrame, maxSendSize);
            _frame.Clear();
            return (length);
        }
        virtual void StateChange () override
	{
	    if (IsOpen() == true) {
                printf ("l2Socket reports statchange to OPEN!!!\n");
                socklen_t len = sizeof(_connectionInfo);
                ::getsockopt(Handle(), SOL_L2CAP, L2CAP_CONNINFO, &_connectionInfo, &len);
 
                // Send out a potential prepared frame fro Detection...
                Trigger();
            }
            else {

                printf ("l2Socket reports statchange to CLOSED!!!\n");
            }
        }
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData) override
        {
           printf ("Executing in state: %d\n", _state);
            if (_state == OPERATIONAL) {
                Received (dataFrame, availableData);
            }
            else {
                switch (_state) {
                case DETECTING:
                {
                    break;
                }
                case METADATA_ID:
                {
                    if (dataFrame[0] == ATT_OP_READ_BY_TYPE_RESP) {
                        _metadata._vendorId  = (dataFrame[5] << 8) | dataFrame[6];
                        _metadata._productId = (dataFrame[7] << 8) | dataFrame[8];
                        _metadata._version   = (dataFrame[9] << 8) | dataFrame[10];
                        _state = METADATA_NAME_HANDLE;
                        const uint16_t message[] = { _min, _max, DEVICE_NAME_UUID };
                        CreateFrame(ATT_OP_READ_BY_TYPE_REQ, (sizeof(message)/sizeof(uint16_t)), message);
                    }
                    break;
                }
                case METADATA_NAME_HANDLE:
                {    
                    if (dataFrame[0] == ATT_OP_READ_BY_TYPE_RESP) {
                        _state = METADATA_NAME;
                        _handle = (dataFrame[5] << 8) | dataFrame[6];
                        const uint16_t message[] = { _handle, 0 };
                        CreateFrame(ATT_OP_READ_BLOB_REQ, (sizeof(message)/sizeof(uint16_t)), message);
                    }
                    break;
                }
                case METADATA_NAME:
                {
                    if (dataFrame[0] == ATT_OP_READ_BLOB_RESP) {
                        uint16_t length = 1;
                        while ((length < availableData) && (dataFrame[length] != '\0')) length++;
                        _metadata._name = string(reinterpret_cast<const char*>(dataFrame[1]), length - 1);
                        _state = METADATA_DESCRIPTORS_HANDLE;
                        const uint16_t message[] = { _min, _max, REPORT_MAP_UUID };
                        CreateFrame(ATT_OP_READ_BY_TYPE_REQ, (sizeof(message)/sizeof(uint16_t)), message);
                    }
                    break;
                }
                case METADATA_DESCRIPTORS_HANDLE:
                {
                    if (dataFrame[0] == ATT_OP_READ_BY_TYPE_RESP) {
                        _offset = 0;
                        _handle = ((dataFrame[5] << 8) | dataFrame[6]);
                        _state = METADATA_DESCRIPTORS;
                        const uint16_t message[] = { _handle, _offset };
                        CreateFrame(ATT_OP_READ_BLOB_REQ, (sizeof(message)/sizeof(uint16_t)), message);
                    }
                    break;
                }
                case METADATA_DESCRIPTORS:
                {
                    if (dataFrame[0] == ATT_OP_READ_BLOB_RESP) {
                        ::memcpy (&(_metadata._blob[_offset]), &(dataFrame[1]), availableData-1);
                        _offset += (availableData - 1);

                        if (_offset < sizeof(_metadata._blob)) {
                            const uint16_t message[] = { _handle, _offset };
                            CreateFrame(ATT_OP_READ_BLOB_REQ, (sizeof(message)/sizeof(uint16_t)), message);
                        }
                        else {
                            Received (nullptr, 0);
                            _state = OPERATIONAL;
                        }
                    }
                    break;
                }
                default:
                    ASSERT (false);
                }
            }

            return(availableData);
        }

    private:
        state _state;
        Metadata _metadata;
        uint16_t _min;
        uint16_t _max;
        uint16_t _handle;
        uint16_t _offset;
        CommandFrame _frame;
        struct l2cap_conninfo _connectionInfo;
    };

} // namespace Bluetooth

} // namespace WPEFramework
