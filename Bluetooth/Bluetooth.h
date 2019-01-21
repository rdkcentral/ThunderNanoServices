#pragma once

#include "Module.h"
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/mgmt.h>

namespace WPEFramework {

namespace Core {

    class SynchronousSocket : public SocketPort {
    private:
        SynchronousSocket() = delete;
        SynchronousSocket(const SynchronousSocket&) = delete;
        SynchronousSocket& operator=(const SynchronousSocket&) = delete;

    public:
        struct IOutbound {

            virtual ~IOutbound() {};

            virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const = 0;
        };

    public:
        SynchronousSocket(const SocketPort::enumType socketType, const NodeId& localNode, const NodeId& remoteNode, const uint16_t sendBufferSize, const uint16_t receiveBufferSize)
            : SocketPort(socketType, localNode, remoteNode, sendBufferSize, receiveBufferSize)
            , _adminLock()
            , _outbound(nullptr)
            , _reevaluate (false, true)
            , _waitCount(0) {
        }
        virtual ~SynchronousSocket() {
            Close(Core::infinite);
        }

        uint32_t Send(const IOutbound& message, const uint32_t waitTime) {

            _adminLock.Lock();

            uint32_t result = ClaimSlot(message, waitTime);

            if (_outbound == &message) {

                ASSERT (result == Core::ERROR_NONE);

                _adminLock.Unlock();

                Trigger();

                result = CompletedSlot (message, waitTime);

                _adminLock.Lock();

                _outbound = nullptr;

                Reevaluate();
            }

            _adminLock.Unlock();

            return (result);
        }

    protected:        
        int Handle() const {
            return (static_cast<const Core::IResource&>(*this).Descriptor());
        }
        void Lock () {
            _adminLock.Lock();
        }
        void Unlock () {
            _adminLock.Unlock();
        }

    private:        
        void Reevaluate() {
            _reevaluate.SetEvent();

            while (_waitCount != 0) {
                SleepMs(0);
            }

            _reevaluate.ResetEvent();
        }
        uint32_t ClaimSlot (const IOutbound& request, const uint32_t allowedTime) {
            uint32_t result = Core::ERROR_NONE;

            while ( (IsOpen() == true) && (_outbound != nullptr) && (result == Core::ERROR_NONE) ) {

                _waitCount++;

                _adminLock.Unlock();

                result = _reevaluate.Lock(allowedTime);

                _waitCount--;

                _adminLock.Lock();
            }

            if (_outbound == nullptr) {
                if (IsOpen() == true) {
                    _outbound = &request;
                }
                else {
                    result = Core::ERROR_CONNECTION_CLOSED;
                }
            }

            return (result);
        }
        uint32_t CompletedSlot (const IOutbound& request, const uint32_t allowedTime) {

            uint32_t result = Core::ERROR_NONE;

            if (_outbound == &request) {

                _waitCount++;

                _adminLock.Unlock();

                result == _reevaluate.Lock(allowedTime);

                _waitCount--;

                _adminLock.Lock();
            }

            if ((result == Core::ERROR_NONE) && (_outbound == &request)) {
                result = Core::ERROR_ASYNC_ABORTED;
            }

            return (result);
        }

        // Methods to extract and insert data into the socket buffers
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
        {
            uint16_t result = 0;

            _adminLock.Lock();

            if ( (_outbound != nullptr) && (_outbound != reinterpret_cast<const IOutbound*>(~0)) ) {
                result = _outbound->Serialize(dataFrame, maxSendSize);

                if (result == 0) {
                    _outbound = reinterpret_cast<const IOutbound*>(~0);
                    Reevaluate();
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData) override
        {
            return (availableData);
        }
        virtual void StateChange () override
	{
	    if (IsOpen() == false) {
                _reevaluate.SetEvent();
            }
            else {
                _reevaluate.ResetEvent();
            }
        }

    private:
        Core::CriticalSection _adminLock;
        const IOutbound* _outbound;
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
        Core::NodeId NodeId() const {
            Core::NodeId result;
            int deviceId = hci_get_route(const_cast<bdaddr_t*>(Data()));

            if (deviceId >= 0) {
                result = Core::NodeId(static_cast<uint16_t>(deviceId), 0);
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
 
        string ToString() const {
            static constexpr TCHAR _hexArray[] = "0123456789ABCDEF";
            string result;

            if (IsValid() == true) {
                for (uint8_t index = 1; index < _length; index++) {
                    if (result.empty() == false) {
                        result += ':';
                    }
                    result += _hexArray[(_address.b[index] >> 4) & 0x0F];
                    result += _hexArray[_address.b[index] & 0x0F];
                }
            }

            return (result);
        }

    private:
        bdaddr_t _address;
        uint8_t _length;
    };

    class ManagementFrame : public Core::SynchronousSocket::IOutbound {
    private: 
        ManagementFrame() = delete;
        ManagementFrame(const ManagementFrame&) = delete;
        ManagementFrame& operator= (const ManagementFrame&) = delete;

    public:
        ManagementFrame(const uint16_t index)
            : _offset(0)
            , _maxSize(64)
            , _buffer(reinterpret_cast<uint8_t*>(::malloc(_maxSize))) {
            _mgmtHeader.index = htobs(index);
            _mgmtHeader.opcode = 0xDEAD;
            _mgmtHeader.len = 0;
        }
        virtual ~ManagementFrame() {
            ::free (_buffer);
        }
           
    public:
        virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const {
            uint16_t result = 0;
            if (_offset < sizeof(_mgmtHeader)) {
                uint8_t copyLength = std::min(static_cast<uint8_t>(sizeof(_mgmtHeader) - _offset), static_cast<uint8_t>(length));
                ::memcpy (stream, &(reinterpret_cast<const uint8_t*>(&_mgmtHeader)[_offset]), copyLength);
                result   = copyLength;
                _offset += copyLength;
            }
            if (result < length) {
                uint8_t copyLength = std::min(static_cast<uint8_t>(_mgmtHeader.len - (_offset - sizeof(_mgmtHeader))), static_cast<uint8_t>(length - result));
                if (copyLength > 0) {
                    ::memcpy (&(stream[result]), &(_buffer[_offset - sizeof(_mgmtHeader)]), copyLength);
                    result  += copyLength;
                    _offset += copyLength;
                }
            }
            return (result);
        }
        template<typename VALUE>
        inline ManagementFrame& Set (const uint16_t opcode, const VALUE& value) {
            _offset = 0;

            if (sizeof(VALUE) > _maxSize) {
                ::free(_buffer);
                _maxSize = sizeof(VALUE);
                _buffer = reinterpret_cast<uint8_t*>(::malloc(_maxSize));
            }
            ::memcpy (_buffer, &value, sizeof(VALUE));
            _mgmtHeader.len = htobs(sizeof(VALUE));
            _mgmtHeader.opcode = htobs(opcode);

            return (*this);
        }
    private:
        mutable uint16_t _offset;
        uint16_t _maxSize;
        struct mgmt_hdr _mgmtHeader;
        uint8_t* _buffer;
    };

    class HCISocket : public Core::SynchronousSocket {
    private:
        HCISocket(const HCISocket&) = delete;
        HCISocket& operator=(const HCISocket&) = delete;

        static constexpr int      SCAN_TIMEOUT           = 1000;
        static constexpr uint8_t  SCAN_TYPE              = 0x01;
        static constexpr uint8_t  OWN_TYPE               = 0x00;
        static constexpr uint8_t  SCAN_FILTER_POLICY     = 0x00;
        static constexpr uint8_t  SCAN_FILTER_DUPLICATES = 0x01;
        static constexpr uint8_t  EIR_NAME_SHORT         = 0x08;
        static constexpr uint8_t  EIR_NAME_COMPLETE      = 0x09;

    public:
        class Filter {
        private: 
            Filter() = delete;

        public: 
            Filter(const uint32_t type, const uint32_t events) {
                hci_filter_clear(&_filter);
                hci_filter_set_ptype(type, &_filter);
                hci_filter_set_event(events, &_filter);
            }
            Filter(const struct hci_filter& filter) {
                ::memcpy(&_filter, &filter, sizeof(_filter));
            }
            Filter(const Filter& copy) {
                ::memcpy(&_filter, &(copy._filter), sizeof(_filter));
            }
            ~Filter() {
            }

            Filter& operator=(const Filter& rhs) {
                ::memcpy(&_filter, &(rhs._filter), sizeof(_filter));
                return (*this);
            }

        private:
            friend class HCISocket;

            struct hci_filter& Data() {
                return (_filter);
            } 
            const struct hci_filter& Data() const {
                return (_filter);
            } 


        private:
            struct hci_filter _filter;
        };

    public:
        enum state : uint8_t {
            SCANNING = 0x0001
        };

    public:
        HCISocket()
            : Core::SynchronousSocket(SocketPort::RAW, Core::NodeId(HCI_DEV_NONE, HCI_CHANNEL_CONTROL), Core::NodeId(), 256, 256)
            , _state(0) {

        }
        virtual ~HCISocket() {
            Close(Core::infinite);
        }

    public:
        bool IsScanning() const {
            return ((_state & SCANNING) != 0);
        }
        void StartScan(const uint16_t interval, const uint16_t window) {

            Lock();

            if ((_state & SCANNING) == 0) {
                int descriptor = Handle();

                ASSERT (descriptor >= 0);

                Set (Filter(HCI_EVENT_PKT, EVT_LE_META_EVENT));

                if (hci_le_set_scan_parameters(descriptor, SCAN_TYPE, htobs(interval), htobs(window), OWN_TYPE, SCAN_FILTER_POLICY, SCAN_TIMEOUT) >= 0) {
                    if (hci_le_set_scan_enable(descriptor, 1, SCAN_FILTER_DUPLICATES, SCAN_TIMEOUT) >= 0) {
                        _state |= SCANNING;
                    }
                }
            }

            Unlock();
        }
        void StopScan() {

            Lock();

            if ((_state & SCANNING) != 0) {
                int descriptor = Handle();

                ASSERT (descriptor >= 0);
 
                if (hci_le_set_scan_enable(descriptor, 0, SCAN_FILTER_DUPLICATES, SCAN_TIMEOUT) >= 0) {
                    _state ^= SCANNING;
                }
            }

            Unlock();
        }
        void Get (Filter& filter) const {
            socklen_t filterLen = sizeof(struct hci_filter);
            ::getsockopt(Handle(), SOL_HCI, HCI_FILTER, const_cast<struct hci_filter*>(&(filter.Data())), &filterLen);
        }
        void Set (const Filter& filter) {
            ::setsockopt(Handle(), SOL_HCI, HCI_FILTER, &(filter.Data()), sizeof(struct hci_filter));
        }

        virtual void DiscoveredDevice (const Address&, const string& shortAddress, const string& longName) = 0;

    private:        
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData) override
        {
            const evt_le_meta_event* eventMetaData = reinterpret_cast<const evt_le_meta_event*>(&(dataFrame[1 + HCI_EVENT_HDR_SIZE]));

            if (eventMetaData->subevent == 0x02) {
                string shortName;
                string longName;
                const le_advertising_info* advertisingInfo = reinterpret_cast<const le_advertising_info*>(&(eventMetaData->data[1]));
                uint16_t offset = 0;
                uint16_t length = advertisingInfo->length;
                const uint8_t* buffer = advertisingInfo->data;

                while (((offset + buffer[offset]) <= advertisingInfo->length) && (buffer[offset] != 0)) {

                    if (buffer[offset+1] != EIR_NAME_SHORT) {
                        shortName = string(reinterpret_cast<const char*>(&(buffer[offset])), buffer[offset]);
                    }
                    else if (buffer[offset+1] != EIR_NAME_COMPLETE) {
                        longName = string(reinterpret_cast<const char*>(&(buffer[offset])), buffer[offset]);
                    }
                    offset += (buffer[offset] + 1);
                }

                DiscoveredDevice (Address(advertisingInfo->bdaddr), shortName, longName);
            }

            return (availableData);
        }

    private:
        uint8_t _state;
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
                socklen_t len = sizeof(_connectionInfo);
                ::getsockopt(Handle(), SOL_L2CAP, L2CAP_CONNINFO, &_connectionInfo, &len);
 
                // Send out a potential prepared frame fro Detection...
                Trigger();
            }
        }
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData) override
        {
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
