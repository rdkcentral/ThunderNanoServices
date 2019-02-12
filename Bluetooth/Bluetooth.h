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
        virtual void Reload() const = 0;
    };

    struct IInbound {

        virtual ~IInbound() {};

        enum state : uint8_t {
            INPROGRESS,
            RESEND,
            COMPLETED
        };

        virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t length) = 0;
        virtual state IsCompleted() const = 0;
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
                _message.Reload();
            }
            Frame(const IOutbound& message, IInbound* response, IOutbound::ICallback* callback, const uint32_t waitTime) 
                : _message(message)
                , _response(response)
                , _state(IDLE)
                , _expired(Core::Time::Now().Add(waitTime).Ticks())
                , _callback(callback) {
                _message.Reload();
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
                    IInbound::state newState = _response->IsCompleted();
                    if (newState == IInbound::COMPLETED) {
                        _state = COMPLETE;
                    }
                    else if (newState == IInbound::RESEND) {
                        _message.Reload();
                        _state = IDLE;
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

        uint32_t Exchange(const uint32_t waitTime, const IOutbound& message) {

            _adminLock.Lock();

            Cleanup();

            _queue.emplace_back(message, nullptr);

            uint32_t result = Completed(_queue.back(), waitTime);

            _adminLock.Unlock();

            return (result);
        }

        uint32_t Exchange(const uint32_t waitTime, const IOutbound& message, IInbound& response) {

            bool trigger = false;

            _adminLock.Lock();

            Cleanup();

            _queue.emplace_back(message, &response);

            uint32_t result = Completed(_queue.back(), waitTime);

            _adminLock.Unlock();

            return (result);
        }

        void Send(const uint32_t waitTime, const IOutbound& message, IOutbound::ICallback* callback, IInbound* response) {

            _adminLock.Lock();

            Cleanup();

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

            Cleanup();

            typename std::list<Frame>::iterator index = std::find(_queue.begin(), _queue.end(), message);
            if (index != _queue.end()) {
                trigger = (_queue.size() > 1) && (index == _queue.begin());

                if (index->Callback() != nullptr) {
                    index->Callback()->Updated(index->Outbound(), Core::ERROR_ASYNC_ABORTED);
                }
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
        virtual void StateChange () override
	{
            _adminLock.Lock();
            Reevaluate();
            _adminLock.Unlock();
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

            while (_waitCount.load() != 0) {
                SleepMs(0);
            }
 
            _reevaluate.ResetEvent();
        }
        uint32_t Completed (const Frame& request, const uint32_t allowedTime) {

            Core::Time now = Core::Time::Now();
            Core::Time endTime = Core::Time(now).Add(allowedTime);
            uint32_t result = Core::ERROR_ASYNC_ABORTED;

            if (&(_queue.front()) == &request) {

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

            typename std::list<Frame>::iterator index = std::find(_queue.begin(), _queue.end(), request.Outbound());
            if (index != _queue.end()) {
                if (request.IsComplete() == true) {
                    result = Core::ERROR_NONE;
                }
                _queue.erase (index);
            }

            return (result);
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
                    if (_queue.size() > 0) {
                        CHANNEL::Trigger();
                    }
                } 
                else if (frame.IsComplete()) {
                    Reevaluate();
                }
                else if (frame.IsSend() == false) {
                    CHANNEL::Trigger();
                }
            }

            _adminLock.Unlock();

            if (result < availableData) { 
                result += Deserialize(&(dataFrame[result]), availableData - result);
            }

            return (result);
        }

    private:
        Core::CriticalSection _adminLock;
        std::list<Frame> _queue;
        Core::Event _reevaluate;
        volatile std::atomic<uint32_t> _waitCount;
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

        static constexpr uint8_t LE_PUBLIC_ADDRESS = 0x00;
        static constexpr uint8_t LE_RANDOM_ADDRESS = 0x01;

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
        Address AnyInterface() const {
            static bdaddr_t g_anyAddress;
            return (Address (g_anyAddress));
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
        Core::NodeId NodeId(const uint8_t addressType, const uint16_t cid, const uint16_t psm) const {
            return (Core::NodeId (_address, addressType, cid, psm)); 
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
        class ManagementType : public Core::IOutbound {
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
                ::memset(&(_buffer[6]), 0, sizeof(_buffer) - 6);
            }
            OUTBOUND* operator->() {
                return (reinterpret_cast<OUTBOUND*>(&(_buffer[6])));
            }
            uint32_t Send (HCISocket& socket, const uint32_t waitTime, const uint16_t opCode) {
                _offset = 0;
                _buffer[0] = (opCode & 0xFF);
                _buffer[1] = ((opCode >> 8) & 0xFF);

                uint32_t result = socket.Exchange(waitTime, *this);
            }

        private:
            virtual uint16_t Id() const override {
                return ((_buffer[0] << 8) | _buffer[1]);
            }
            virtual void Reload() const override {
                _offset = 0;
            }
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const override {
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
                : _offset(sizeof(_buffer)) {
                _buffer[0] = HCI_COMMAND_PKT;
                _buffer[1] = (OPCODE & 0xFF);
                _buffer[2] = ((OPCODE >> 8) & 0xFF);
                _buffer[3] = static_cast<uint8_t>(sizeof(OUTBOUND));
            }
            virtual ~CommandType() {
            }

        public:
            OUTBOUND* operator->() {
                return (reinterpret_cast<OUTBOUND*>(&(_buffer[4])));
            }
            void Clear () {
               ::memset(&(_buffer[4]), 0, sizeof(_buffer) - 4);
            }

        private:
            virtual uint16_t Id() const override {
                return (ID);
            }
            virtual void Reload() const override {
                _offset = 0;
            }
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const override {
                uint16_t result = std::min(static_cast<uint16_t>(sizeof(_buffer) - _offset), length);
                if (result > 0) {
                    
                    ::memcpy (stream, &(_buffer[_offset]), result);
                    _offset += result;
                }
                return (result);
            }

        private:
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

        private:
            virtual Core::IInbound::state IsCompleted() const override {
                return (_error != static_cast<uint16_t>(~0) ? Core::IInbound::COMPLETED : Core::IInbound::INPROGRESS);
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
                         if (cs->opcode == CommandType<OPCODE, OUTBOUND>::ID) {
                             
                             if (RESPONSECODE == EVT_CMD_STATUS) {
                                 _error = (cs->status != 0 ? Core::ERROR_GENERAL : Core::ERROR_NONE);
                             }
                             else if (cs->status != 0) {
                                 _error = cs->status;
                             }
                         }
                         else {
                             printf("Received Command STATUS: %04X len %d, but not what we expected.\n", cs->opcode, len);
                         }
                         break;
                    }
                    case EVT_CMD_COMPLETE:
                    {
                         const evt_cmd_complete* cc = reinterpret_cast<const evt_cmd_complete*>(ptr);;
                         if (cc->opcode == CommandType<OPCODE, OUTBOUND>::ID) {
                             if (len <= EVT_CMD_COMPLETE_SIZE) {
                                 _error = Core::ERROR_GENERAL;
                             }
                             else {
                                 uint16_t toCopy = std::min(static_cast<uint16_t>(sizeof(INBOUND)), static_cast<uint16_t>(len - EVT_CMD_COMPLETE_SIZE));
                                 ::memcpy(reinterpret_cast<uint8_t*>(&_response), &(ptr[EVT_CMD_COMPLETE_SIZE]), toCopy);
                                 _error = Core::ERROR_NONE;
                             }
                         }
                         else {
                             printf("Received Command COMPLETED: %04X len %d, but not what we expected.\n", cc->opcode, len);
                         }
                         break;
                    }
                    default:
                         printf("Received EVENT: %X no clue what to do with it.\n", hdr->evt);
                         break;
                    }
                }
                return (result);
            }

        private:
            INBOUND _response;
            uint16_t _error;
        };

        static constexpr uint32_t MAX_ACTION_TIMEOUT = 2000; /* 2 Seconds for commands to complete ? */
        static constexpr uint16_t ACTION_MASK = 0x3FFF;

    public:
        enum state : uint16_t {
            IDLE          = 0x0000,
            SCANNING      = 0x0001,
            ABORT         = 0x8000
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

        typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL,OCF_LE_SET_SCAN_PARAMETERS), le_set_scan_parameters_cp, uint8_t, EVT_CMD_STATUS>
                ScanParametersLE;

        typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL,OCF_LE_SET_SCAN_ENABLE), le_set_scan_enable_cp, uint8_t, EVT_CMD_STATUS>
                ScanEnableLE;

        typedef ExchangeType<cmd_opcode_pack(OGF_LE_CTL,OCF_LE_READ_REMOTE_USED_FEATURES), le_read_remote_used_features_cp, evt_le_read_remote_used_features_complete, EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE>
                RemoteFeaturesLE;
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

            _state.Lock();

            if ((_state & ACTION_MASK) == 0) {
                int descriptor = Handle();

                _state.SetState(static_cast<state>(_state.GetState() | SCANNING));

                _state.Unlock();

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
                            reported.push_back(newSource);
                            callback->DiscoveredDevice(false, newSource, _T("[Unknown]"));
                        }
                    }

                    // Reset go for the next round !!
                    // ir->length  = (endTime <= Core::Time::Now() ? 0 : 1);
                    // ir->num_rsp = 128;
                    // ir->flags  &= ~IREQ_CACHE_FLUSH;
                }

                _state.Lock();

                _state.SetState(static_cast<state>(_state.GetState() & (~(ABORT|SCANNING))));
            }

            _state.Unlock();
        }
        void Scan (IScanning* callback, const uint16_t scanTime, const bool limited, const bool passive) {
            _state.Lock();

            if ((_state & ACTION_MASK) == 0) {
                Command::ScanParametersLE parameters;

                parameters.Clear();
                parameters->type = (passive ? 0x00 : 0x01);
                parameters->interval = htobs((limited ? 0x12 : 0x10));
                parameters->window = htobs((limited ? 0x12 : 0x10));
                parameters->own_bdaddr_type = LE_PUBLIC_ADDRESS;
                parameters->filter = SCAN_FILTER_POLICY;

                if ( (Exchange(MAX_ACTION_TIMEOUT, parameters, parameters) == Core::ERROR_NONE) && (parameters.Response() == 0) ) {

                    Command::ScanEnableLE scanner;
                    _callback = callback;

                    scanner.Clear();
                    scanner->enable = 1;
                    scanner->filter_dup = SCAN_FILTER_DUPLICATES;

                    if ( (Exchange(MAX_ACTION_TIMEOUT, scanner, scanner) == Core::ERROR_NONE) && (scanner.Response() == 0) ) {

                        _state.SetState(static_cast<state>(_state.GetState() | SCANNING));

                        // Now lets wait for the scanning period..
                        _state.Unlock();

                        _state.WaitState(ABORT, scanTime*1000);
                        
                        _state.Lock();

                        _callback = nullptr;
                        
                        scanner->enable = 0;
                        Exchange(MAX_ACTION_TIMEOUT, scanner, scanner);

                        _state.SetState(static_cast<state>(_state.GetState() & (~(ABORT|SCANNING))));
                    }
                }
            }

            _state.Unlock();
        }
        void Abort() {

            _state.Lock();

            if ((_state & ACTION_MASK) != 0) {
                // TODO: Find if we can actually abort a IOCTL:HCIINQUIRY !!
                _state.SetState (static_cast<state>(_state.GetState() | ABORT));
            }

            _state.Unlock();
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

            switch (hdr->evt) {
            case EVT_CMD_STATUS: 
            {
                const evt_cmd_status* cs = reinterpret_cast<const evt_cmd_status*>(ptr);
                printf("EVT_CMD_STATUS:   %X-%03X : %d.%d\n", (cs->opcode >> 10) & 0xF, (cs->opcode & 0x3FF), cs->ncmd, cs->status);
                break;
            }
            case EVT_CMD_COMPLETE:
            {
                const evt_cmd_complete* cc = reinterpret_cast<const evt_cmd_complete*>(ptr);;
                printf("EVT_CMD_COMPLETE: %X-%03X : %d\n", (cc->opcode >> 10) & 0xF, (cc->opcode & 0x3FF), cc->ncmd);
                break;
            }
            case EVT_LE_META_EVENT:
            {
                 const evt_le_meta_event* eventMetaData = reinterpret_cast<const evt_le_meta_event*>(ptr);

                 if (eventMetaData->subevent != 0x02) {
                     printf("Received SUB EVENT: %X\n", eventMetaData->subevent);
                 }
                 else {
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
            default:
                 printf("Received an EVENT: %X, on the application socket. No clue what to do with it..\n", hdr->evt);
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
    };

    class GATTSocket : public Core::SynchronousChannelType<Core::SocketPort> {
    private:
        GATTSocket(const GATTSocket&) = delete;
        GATTSocket& operator=(const GATTSocket&) = delete;

    public:
        class UUID {
        public:
            // const uint8_t BASE[] = { 00000000-0000-1000-8000-00805F9B34FB };

        public:
            UUID(const uint16_t uuid) {
                _uuid[0] = 2;
                _uuid[1] = (uuid & 0xFF);
                _uuid[2] = (uuid >> 8) & 0xFF;
            }
            UUID(const uint8_t uuid[16]) {
                _uuid[0] = 16;
                ::memcpy(&(_uuid[1]), uuid, 16);
            }
            UUID(const UUID& copy) {
                ::memcpy(_uuid, copy._uuid, copy._uuid[0] + 1);
            }
            ~UUID() {
            }

            UUID& operator= (const UUID& rhs) {
                ::memcpy(_uuid, rhs._uuid, rhs._uuid[0] + 1);
                return (*this);
            }

        public:
            uint16_t Short() const {
                ASSERT (_uuid[0] == 2);
                return ((_uuid[2] << 8) | _uuid[1]);
            }
            bool operator== (const UUID& rhs) const {
                return (::memcmp(_uuid, rhs._uuid, _uuid[0] + 1) == 0);
            }
            bool operator!= (const UUID& rhs) const {
                return !(operator==(rhs));
            }
            uint8_t Length() const {
                return (_uuid[0]);
            }
            const uint8_t& Data() const {
                return (_uuid[1]);
            }
            
        private:
            uint8_t _uuid[17];
        };

        class Attribute {
        public:
            enum type {
                NIL              = 0x00,
                INTEGER_UNSIGNED = 0x08,
                INTEGER_SIGNED   = 0x10,
                UUID             = 0x18,
                TEXT             = 0x20,
                BOOLEAN          = 0x28,
                SEQUENCE         = 0x30,
                ALTERNATIVE      = 0x38,
                URL              = 0x40
            };

            Attribute () 
                : _type(~0)
                , _offset(0)
                , _length(~0)
                , _bufferSize(64)
                , _buffer(reinterpret_cast<uint8_t*>(::malloc (_bufferSize))) {
            }
            ~Attribute() {
                if (_buffer != nullptr) { 
                    ::free(_buffer);
                }
            }

        public:
            void Clear() {
                _type = ~0;
                _offset = 0;
                _length = ~0;
            }
            bool IsLoaded() const {
                return (_offset == static_cast<uint32_t>(~0));
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t size) {

                uint16_t result = 0;

                if (size > 0) {
                    if (_offset == 0) {
                        _type = stream[0];
                        if ((_type & 0x7) <= 4) {
                            if (_type  == 0) {
                                result = 1;

                                // It's a nil, nothing more required
                                _offset = static_cast<uint32_t>(~0);
                                _type   = stream[0];
                            }
                            else {
                                uint8_t length = (1 << (_type & 0x07));
                                uint8_t copyLength = std::min(length, static_cast<uint8_t>(size - 1));
                                ::memcpy (_buffer, &stream[1], copyLength);
                                result = 1 + copyLength;
                                _offset = (copyLength == length ? ~0 : copyLength);
                            }
                        }
                        else {
                            uint8_t length = (1 << ((_type & 0x07) - 5) );
                            uint8_t copyLength = std::min(length, static_cast<uint8_t>(size - 1));
                            ::memcpy (_buffer, &stream[1], copyLength);
                            result = 1 + copyLength;
                            _offset = copyLength;
                        }
                    }

                    if ((result < size) && (IsLoaded() == false)) {
                        // See if we need to set the length
                        if ( ((_type & 0x7) > 4) && (_length == static_cast<uint32_t>(~0)) ) {
                            uint8_t length = (1 << ((_type & 0x07) - 5));
                            uint8_t copyLength = std::min(static_cast<uint8_t>(length - _offset), static_cast<uint8_t>(size - result));
                            if (copyLength > 0) {
                                ::memcpy (&(_buffer[_offset]), &stream[result], copyLength);
                                result += copyLength;
                                _offset += copyLength;
                            }

                            if (_offset == length) {
                                _length = 0;
                                for (uint8_t index = 0; index < length; index++) {
                                    _length = (length << 8) | _buffer[index];
                                }
                                _offset = (_length == 0 ? ~0 : 0);
                            }
                        }

                        if (result < size) {
                            // Normal payload loading...
                            uint32_t copyLength = std::min(Length() - _offset, static_cast<uint32_t>(size - result));
                            // TODO: This one might potentially get very big. Check if the buffer still fits..
                            ::memcpy (&(_buffer[_offset]), &stream[result], copyLength);
                            result += copyLength;
                            _offset = (_offset + copyLength == Length() ? ~0 : _offset + copyLength);
                        }
                    }
                }
                return(result);            
            }
            type Type() const {
                return (static_cast<type>(_type & 0xF8));
            }
            uint32_t Length() const {
                return ((_type & 0x3) <= 4 ? (_type == 0 ? 0 : 1 << (_type & 0x03)) : _length);
            }
            template <typename TYPE>
            void Load(TYPE& value) const {
                value = 0;
                for (uint8_t index = 0; index < sizeof(TYPE); index++) {
                    value = (value << 8) | _buffer[index];
                }
            }
            void Load(bool& value) const {
                value = (_buffer[0] != 0);
            }
            void Load(string& value) const {
                value = string(reinterpret_cast<const char*>(_buffer), _length);
            }

        private:
            uint8_t _type;
            uint32_t _offset;
            uint32_t _length;
            uint32_t _bufferSize;
            uint8_t* _buffer;
        };

        class Command : public Core::IOutbound::ICallback, public Core::IOutbound, public Core::IInbound {
        private:
            Command (const Command&) = delete;
            Command& operator= (const Command&) = delete;

            static constexpr uint8_t ATT_OP_ERROR              = 0x01;
            static constexpr uint8_t ATT_OP_MTU_REQ            = 0x02;
            static constexpr uint8_t ATT_OP_MTU_RESP           = 0x03;
            static constexpr uint8_t ATT_OP_FIND_INFO_REQ      = 0x04;
            static constexpr uint8_t ATT_OP_FIND_INFO_RESP     = 0x05;
            static constexpr uint8_t ATT_OP_FIND_BY_TYPE_REQ   = 0x06;
            static constexpr uint8_t ATT_OP_FIND_BY_TYPE_RESP  = 0x07;
            static constexpr uint8_t ATT_OP_READ_BY_TYPE_REQ   = 0x08;
            static constexpr uint8_t ATT_OP_READ_BY_TYPE_RESP  = 0x09;
            static constexpr uint8_t ATT_OP_READ_REQ           = 0x0A;
            static constexpr uint8_t ATT_OP_READ_RESP          = 0x0B;
            static constexpr uint8_t ATT_OP_READ_BLOB_REQ      = 0x0C;
            static constexpr uint8_t ATT_OP_READ_BLOB_RESP     = 0x0D;
            static constexpr uint8_t ATT_OP_READ_MULTI_REQ     = 0x0E;
            static constexpr uint8_t ATT_OP_READ_MULTI_RESP    = 0x0F;
            static constexpr uint8_t ATT_OP_READ_BY_GROUP_REQ  = 0x10;
            static constexpr uint8_t ATT_OP_READ_BY_GROUP_RESP = 0x11;
            static constexpr uint8_t ATT_OP_WRITE_REQ          = 0x12;
            static constexpr uint8_t ATT_OP_WRITE_RESP         = 0x13;

            class Exchange {
            private: 
                Exchange(const Exchange&) = delete;
                Exchange& operator= (const Exchange&) = delete;

            public:
                Exchange() 
                    : _offset(~0)
                    , _size(0) {
                }
                ~Exchange() {
                }

            public:
                bool IsSend() const {
                    return (_offset != 0);
                }
                void Reload () const {
                    _offset = 0;
                }
                uint8_t GetMTU (const uint16_t mySize) {
                    _buffer[0] = ATT_OP_MTU_REQ;
                    _buffer[1] = (mySize & 0xFF);
                    _buffer[2] = (mySize >> 8) & 0xFF;
                    _size = 3;
                    return(ATT_OP_MTU_RESP);
                }
                uint8_t ReadByGroupType (const uint16_t start, const uint16_t end, const UUID& id) {
                    _buffer[0] = ATT_OP_READ_BY_GROUP_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    ::memcpy(&(_buffer[5]), &id.Data(), id.Length()); 
                    _size = id.Length() + 5;
                    return(ATT_OP_READ_BY_GROUP_RESP);
                }
                uint8_t FindByType (const uint16_t start, const uint16_t end, const UUID& id, const uint8_t length, const uint8_t data[]) {
                    _buffer[0] = ATT_OP_FIND_BY_TYPE_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    _buffer[5] = (id.Short() & 0xFF);
                    _buffer[6] = (id.Short() >> 8) & 0xFF;
                    ::memcpy(&(_buffer[7]), data, length); 
                    _size = 7 + length;
                    return(ATT_OP_FIND_BY_TYPE_RESP);
                }
                uint8_t Write (const UUID& id, const uint8_t length, const uint8_t data[]) {
                    _buffer[0] = ATT_OP_WRITE_REQ;
                    _buffer[1] = (id.Short() & 0xFF);
                    _buffer[2] = (id.Short() >> 8) & 0xFF;
                    ::memcpy(&(_buffer[3]), data, length); 
                    _size = 3 + length;
                    return(ATT_OP_WRITE_RESP);
                }
                uint8_t ReadByType (const uint16_t start, const uint16_t end, const UUID& id) {
                    _buffer[0] = ATT_OP_READ_BY_TYPE_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    ::memcpy(&(_buffer[5]), &id.Data(), id.Length()); 
                    _size = id.Length() + 5;
                    return(ATT_OP_READ_BY_TYPE_RESP);
                }
                uint8_t FindInformation (const uint16_t start, const uint16_t end) {
                    _buffer[0] = ATT_OP_FIND_INFO_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    _size = 5;
                    return(ATT_OP_FIND_INFO_RESP);
                }
                uint8_t Read(const uint16_t handle) {
                    _buffer[0] = ATT_OP_READ_REQ;
                    _buffer[1] = (handle & 0xFF);
                    _buffer[2] = (handle >> 8) & 0xFF;
                    _size = 3;
                    return(ATT_OP_READ_RESP);
                }
                uint8_t ReadBlob(const uint16_t handle, const uint16_t offset) {
                    _buffer[0] = ATT_OP_READ_BLOB_REQ;
                    _buffer[1] = (handle & 0xFF);
                    _buffer[2] = (handle >> 8) & 0xFF;
                    _buffer[3] = (offset & 0xFF);
                    _buffer[4] = (offset >> 8) & 0xFF;
                    _size = 5;
                    return (ATT_OP_READ_BLOB_RESP);
                }
                uint16_t Serialize(uint8_t stream[], const uint16_t length) const {
                    uint16_t result = std::min(static_cast<uint16_t>(_size - _offset), length);
                    if (result > 0) {
                        ::memcpy (stream, &(_buffer[_offset]), result);
                        _offset += result;

                        printf("L2CAP send [%d]: ", result);
                        for (uint8_t index = 0; index < (result-1); index++) { printf("%02X:", stream[index]); } printf("%02X\n", stream[result-1]);
                    }

                    return (result);
                }
                uint16_t Handle() const {
                    return (((_buffer[0] == ATT_OP_READ_BLOB_REQ) || (_buffer[0] == ATT_OP_READ_REQ)) ? ((_buffer[2] << 8) | _buffer[1]) : 0);
                }
                uint16_t Offset() const {
                    return ((_buffer[0] == ATT_OP_READ_BLOB_REQ) ? ((_buffer[4] << 8) | _buffer[3]) : 0);
                }
 
            private:
                mutable uint16_t _offset;
                uint8_t _size;
                uint8_t _buffer[64];
            };

        public:
            struct ICallback {
                virtual ~ICallback() {}

                virtual void Completed(const uint32_t result) = 0;
            };

            class Response {
            private:
                Response(const Response&) = delete;
                Response& operator= (const Response&) = delete;

                static constexpr uint16_t BLOCKSIZE = 255;
                typedef std::pair<uint16_t, uint16_t> Entry;

            public:
                Response() 
                    : _maxSize(BLOCKSIZE)
                    , _loaded(0)
                    , _result()
                    , _iterator()
                    , _storage(reinterpret_cast<uint8_t*>(::malloc(_maxSize))) 
                    , _preHead(true)
                    , _min(0x0001)
                    , _max(0xFFFF) {
                }
                ~Response() {
                    if (_storage != nullptr) {
                        ::free(_storage);
                    }
                }
                
            public:
                void Clear() {
                    Reset();
                    _result.clear();
                    _loaded = 0;
                    _min = 0xFFFF;
                    _max = 0x0001;
                }
                void Reset() {
                    _preHead = true;
                }
                bool IsValid() const {
                    return ((_preHead == false) && (_iterator != _result.end()));
                }
                bool Next() {
                    if (_preHead == true) {
                        _preHead = false;
                        _iterator =_result.begin();
                    }
                    else {
                        _iterator++;
                    }
                    return (_iterator != _result.end());
                }
                uint16_t Handle () const {
                    return (_iterator->first);
                }
                uint16_t MTU () const {
                    return ((_storage[0] << 8) | _storage[1]);
                }
                uint16_t Group() const {
                    return (_iterator->second);
                }
                uint16_t Count() const {
                    return (_result.size());
                }
                bool Empty () const {
                    return (_result.empty());
                }
                uint16_t Length() const {
                    uint16_t result = _loaded;
                    if (IsValid() == true) {
                        std::list<Entry>::iterator next (_iterator);
                        result = (++next == _result.end() ? (_loaded - _iterator->second) : (next->second - _iterator->second));
                    }
                    return (result);
                }
                const uint8_t* Data() const {
                    return (IsValid() == true ? &(_storage[_iterator->second]) : (((_result.size() <= 1) && (_loaded > 0)) ? _storage : nullptr));
                }

            private:
                friend class Command;
                void SetMTU (const uint16_t MTU) {
                    _storage[0] =  (MTU >> 8) & 0xFF;
                    _storage[1] =  MTU & 0xFF;
                }
                void Add(const uint16_t handle, const uint16_t group) {
                    if (_min > handle) _min = handle;
                    if (_max < handle) _max = handle;
                    _result.emplace_back(Entry(handle, group));
                }
                void Add(const uint16_t handle, const uint8_t length, const uint8_t buffer[]) {
                    if (_min > handle) _min = handle;
                    if (_max < handle) _max = handle;

                    _result.emplace_back(Entry(handle, _loaded));
                    Extend (length, buffer);
                }
                void Extend(const uint8_t length, const uint8_t buffer[]) {
                    if ((_loaded + length) > _maxSize) {
                        _maxSize = ((((_loaded + length) / BLOCKSIZE) + 1) * BLOCKSIZE);
                        _storage = reinterpret_cast<uint8_t*>(::realloc(_storage, _maxSize));
                    }
 
                    ::memcpy(&(_storage[_loaded]), buffer, length);
                    _loaded += length;
                }
                uint16_t Offset() const {
                    return (_result.size() != 0 ? _loaded : _loaded - _result.back().second);
                }

            private:
                uint16_t _maxSize;
                uint16_t _loaded;
                std::list<Entry> _result;
                std::list<Entry>::iterator _iterator;
                uint8_t* _storage;
                bool _preHead;
                uint16_t _min;
                uint16_t _max;
            };

        public:
            Command(ICallback* completed, const uint16_t bufferSize) 
                : _frame()
                , _callback(completed)
                , _id(~0)
                , _error(~0)
                , _bufferSize(bufferSize)
                , _response() {
            }
            virtual ~Command() {
            }

        public:
            void SetMTU(const uint16_t bufferSize) {
                _bufferSize = bufferSize;
            }
            void GetMTU() {
                _error = ~0;
                _id = _frame.GetMTU(_bufferSize);
            }
            void ReadByGroupType (const uint16_t min, const uint16_t max, const UUID& uuid) {
                _response.Clear();
                _error = ~0;
                _id = _frame.ReadByGroupType(min, max, uuid);
            }
            void ReadByType (const uint16_t min, const uint16_t max, const UUID& uuid) {
                _response.Clear();
                _error = ~0;
                _id = _frame.ReadByType(min, max, uuid);
            }
            void ReadBlob (const uint16_t handle) {
                _response.Clear();
                _error = ~0;
                _id = _frame.ReadBlob(handle, 0);
            }
            void WriteByType (const uint16_t min, const uint16_t max, const UUID& uuid, const uint8_t length, const uint8_t data[]) {
                _response.Clear();
                _response.Extend(length, data);
                _error = ~0;
                _id = _frame.ReadByType(min, max, uuid);
            }
            void FindByType (const uint16_t min, const uint16_t max, const UUID& uuid, const uint8_t length, const uint8_t data[]) {
                ASSERT (uuid.HasShort() == true);
                _response.Clear();
                _error = ~0;
                _id = _frame.FindByType(min, max, uuid, length, data);
            }
            Response& Result() {
                return (_response);
            }

        private:
            virtual void Updated (const Core::IOutbound& data, const uint32_t error_code) {
                ASSERT (&data == this);

                _callback->Completed(error_code);
            }
            virtual uint16_t Id() const override {
                return (_id);
            }
            virtual void Reload() const override {
                _frame.Reload();
            }
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const override {
                return(_frame.Serialize(stream, length));
            }
            virtual Core::IInbound::state IsCompleted() const override {
                return (_error != static_cast<uint16_t>(~0) ? Core::IInbound::COMPLETED : (_frame.IsSend() ? Core::IInbound::INPROGRESS : Core::IInbound::RESEND));
            }
            virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override {
                uint16_t result = 0;

                // See if we need to retrigger..
                if ( (stream[0] != _id) && ((stream[0] != ATT_OP_ERROR) && (stream[1] == _id)) ) {
                    printf("Unexpected L2CapSocket message. Expected: %d, got %d [%d]\n", _id, stream[0], stream[1]);
                }
                else {
                    result = length;

                    printf("L2CapSocket Receive [%d], Type: %02X\n", length, stream[0]);

                    // This is what we are expecting, so process it...
                    switch (stream[0]) {
                    case ATT_OP_ERROR:
                    {
                        printf("Houston we got an error... %d \n", stream[4]);
                        _error = stream[4];
                        break;
                    }
                    case ATT_OP_MTU_RESP: 
                    {
                        _bufferSize = ((stream[2] << 8) | stream[1]);
                        _response.SetMTU (_bufferSize);
                        _error = Core::ERROR_NONE;
                        break;
                    }
                    case ATT_OP_READ_BY_GROUP_RESP:
                    {
                        printf("L2CapSocket Read By Group Type\n");
                        _error = Core::ERROR_NONE;
                        break;
                    }
                    case ATT_OP_FIND_BY_TYPE_RESP:
                    {
                        /* PDU must contain at least:
                         * - Attribute Opcode (1 octet)
                         * - Length (1 octet)
                         * - Attribute Data List (at least one entry):
                         *   - Attribute Handle (2 octets)
                         *   - Attribute Value (at least 1 octet) */
                        /* Minimum Attribute Data List size */
                        if (stream[1] >= 3) {
                            uint8_t entries = (length - 1) / 4;
                            for (uint8_t index = 0; index < entries; index++) {
                                uint16_t offset = 2 + (index * stream[1]);
                                uint16_t foundHandle = (stream[offset+1] << 8) | stream[offset+0];
                                uint16_t groupHandle = (stream[offset+3] << 8) | stream[offset+2];
                                _response.Add(foundHandle, groupHandle);
                            }
                        }
                        _error = Core::ERROR_NONE;
                        break;
                    }
                    case ATT_OP_READ_BY_TYPE_RESP:
                    {
                        /* PDU must contain at least:
                         * - Attribute Opcode (1 octet)
                         * - Length (1 octet)
                         * - Attribute Data List (at least one entry):
                         *   - Attribute Handle (2 octets)
                         *   - Attribute Value (at least 1 octet) */
                        /* Minimum Attribute Data List size */
                        if (stream[1] >= 3) {
                            uint8_t entries = ((length - 2) / stream[1]);
                            for (uint8_t index = 0; index < entries; index++) {
                                uint16_t offset = 2 + (index * stream[1]);
                                uint16_t handle = (stream[offset+1] << 8) | stream[offset+0];

                                _response.Add(handle, stream[1] - 2, &(stream[offset + 2]));
                            }
                        }
                        _error = Core::ERROR_NONE;
                        break;
                    }
                    case ATT_OP_WRITE_RESP: 
                    {
                        printf ("We have written: %d\n", length);
                        _error = Core::ERROR_NONE;
                        break;
                    }
                    case ATT_OP_FIND_INFO_RESP:
                    {
                        _error = Core::ERROR_NONE;
                        break;
                    }
                    case ATT_OP_READ_RESP:
                    {
                        _error = Core::ERROR_NONE;
                        break;
                    }
                    case ATT_OP_READ_BLOB_RESP:
                    {
                        if (_response.Offset() == 0) {
                            _response.Add(_frame.Handle(), length - 1, &(stream[1]));
                        }
                        else {
                            _response.Extend (length - 1, &(stream[1]));
                        }
                        printf ("Received a blob of length %d, BufferSize %d\n", length, _bufferSize);
                        if (length == _bufferSize) {
                            _id = _frame.ReadBlob(_frame.Handle(), _response.Offset());
                            printf ("Now we need to see a send....\n");
                        }
                        else {
                            ASSERT (length < _bufferSize);
                            _error = Core::ERROR_NONE;
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
            Exchange _frame;
            ICallback* _callback;
            uint16_t _id;
            uint16_t _error;
            uint16_t _bufferSize;
            Response _response;
        };

        static constexpr uint8_t LE_ATT_CID                = 4;
        static constexpr uint8_t ATT_OP_HANDLE_NOTIFY      = 0x1B;

    private:
        class LocalCommand : public Command, public Command::ICallback {
        private:
            LocalCommand () = delete;
            LocalCommand (const LocalCommand&) = delete;
            LocalCommand& operator= (const LocalCommand&) = delete;

        public:
            LocalCommand(GATTSocket& parent, const uint16_t bufferSize) 
                : Command(this, bufferSize)
                , _parent(parent)
                , _callback(reinterpret_cast<Command::ICallback*>(~0)) {
            }
            virtual ~LocalCommand() {
            }

            void SetCallback(Command::ICallback* callback) {
                ASSERT ((callback == nullptr) ^ (_callback == nullptr));
                _callback = callback;
            }

        private:
            virtual void Completed(const uint32_t result) override {
                if (_callback == reinterpret_cast<Command::ICallback*>(~0)) {
                    if (result != Core::ERROR_NONE) {
                        TRACE_L1("Could not receive the MTU size correctly. Can not reach Operationl.");
                    }
                    else {
                        _callback = nullptr;
                        _parent.Operational();
                    }
                }
                else if (_callback != nullptr) {
                    Command::ICallback* callback = _callback;
                    _callback = nullptr;
                    callback->Completed(result);
                }
            }

        private:
            GATTSocket& _parent;
            Command::ICallback* _callback;
        };
 
    public:
        GATTSocket(const Core::NodeId& localNode, const Core::NodeId& remoteNode, const uint16_t bufferSize)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::SEQUENCED, localNode, remoteNode, bufferSize, bufferSize)
            , _command(*this, bufferSize) {
        }
        virtual ~GATTSocket() {
        }

    public:
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
        void ReadByGroupType (const uint32_t waitTime, const uint16_t min, const uint16_t max, const UUID& uuid, Command::ICallback* completed) {
            _command.ReadByGroupType(min, max, uuid);
            _command.SetCallback(completed);
            Send(waitTime, _command, &_command, &_command);
        }
        void ReadByType (const uint32_t waitTime, const uint16_t min, const uint16_t max, const UUID& uuid, Command::ICallback* completed) {
            _command.ReadByType(min, max, uuid);
            _command.SetCallback(completed);
            Send(waitTime, _command, &_command, &_command);
        }
        void ReadBlob (const uint32_t waitTime, const uint16_t& handle, Command::ICallback* completed) {
            _command.ReadBlob(handle);
            _command.SetCallback(completed);
            Send(waitTime, _command, &_command, &_command);
        }
        void FindByType (const uint32_t waitTime, const uint16_t min, const uint16_t max, const UUID& uuid, const UUID& type, Command::ICallback* completed) {
            FindByType(waitTime, min, max, uuid, type.Length(), &(type.Data()), completed);
        }
        void FindByType (const uint32_t waitTime, const uint16_t min, const uint16_t max, const UUID& uuid, const uint8_t length, const uint8_t data[], Command::ICallback* completed) {
            _command.FindByType(min, max, uuid, length, data);
            _command.SetCallback(completed);
            Send(waitTime, _command, &_command, &_command);
        }
        void WriteByType (const uint32_t waitTime, const uint16_t min, const uint16_t max, const UUID& uuid, const UUID& type, Command::ICallback* completed) {
            WriteByType(waitTime, min, max, uuid, type.Length(), &(type.Data()), completed);
        }
        void WriteByType (const uint32_t waitTime, const uint16_t min, const uint16_t max, const UUID& uuid, const uint8_t length, const uint8_t data[], Command::ICallback* completed) {
            _command.WriteByType(min, max, uuid, length, data);
            _command.SetCallback(completed);
            Send(waitTime, _command, &_command, &_command);
        }
        void Abort() {
            Revoke(_command);
        }
        Command::Response& Result() {
            return (_command.Result());
        }

        virtual void Operational() = 0;
        virtual uint16_t Deserialize (const uint8_t* dataFrame, const uint16_t availableData) = 0;

    private:        
        virtual void StateChange () override
	{
            Core::SynchronousChannelType<Core::SocketPort>::StateChange();

	    if (IsOpen() == true) {
                socklen_t len = sizeof(_connectionInfo);
                ::getsockopt(Handle(), SOL_L2CAP, L2CAP_CONNINFO, &_connectionInfo, &len);
                _command.GetMTU();
                // Within 1S we should get a response on the MTU.
                Send(1000, _command, &_command, &_command);
            }
        }

    private:
        struct l2cap_conninfo _connectionInfo;
        LocalCommand _command;
    };

} // namespace Bluetooth

} // namespace WPEFramework
