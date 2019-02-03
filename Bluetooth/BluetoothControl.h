#pragma once

#include "Module.h"
#include "BlueDriver.h"
#include "Bluetooth.h"

#include <interfaces/IBluetooth.h>
#include <linux/uhid.h>

namespace WPEFramework {
namespace Plugin {

    class BluetoothControl : public PluginHost::IPlugin, public PluginHost::IWeb, public Exchange::IBluetooth {
    private:
        BluetoothControl(const BluetoothControl&) = delete;
        BluetoothControl& operator=(const BluetoothControl&) = delete;

        class Scanner : public Bluetooth::HCISocket::IScanning, public Bluetooth::HCISocket {
        private:
            Scanner() = delete;
            Scanner(const Scanner&) = delete;
            Scanner& operator= (const Scanner&) = delete;

        private:
            // The bluetooth library has some unexpected behaviour. For example, the scan of NON-BLE devices
            // is a blocking call for the duration of the passed in time. Which is, I think, very intrusive 
            // fo any responsive design. If a RESTFull call would start a scan, the call would last the duration
            // of the scan, which is typicall >= 10Secods which is unacceptable, so it needs to be decoupled.
            // This decoupling is done on this internal Worker thread.
            class Job : public Core::IDispatch {
            private:
                Job() = delete;
                Job(const Job&) = delete;
                Job& operator= (const Job&) = delete;

                enum scanMode {
                    LOW_ENERGY = 0x01,
                    REGULAR    = 0x02,
                    PASSIVE    = 0x04,
                    LIMITED    = 0x08
                };

            public:
                Job(Scanner* parent) : _parent(*parent), _mode(0) {
                }    
                virtual ~Job() {
                }

            public:
                void Load (const uint16_t scanTime, const uint32_t type, const uint8_t flags) {
                    if (_mode == 0) {
                        _mode = REGULAR;
                        _scanTime = scanTime;
                        _type = type;
                        _flags = flags;
                        PluginHost::WorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                    }
                }
                void Load (const uint16_t scanTime, const bool limited, const bool passive) {
                    if (_mode == 0) {
                        _mode = LOW_ENERGY | (passive ? PASSIVE : 0) | (limited ? LIMITED : 0);
                        _scanTime = scanTime;
                        PluginHost::WorkerPool::Instance().Submit(Core::ProxyType<Core::IDispatch>(*this));
                    }
                }

            private:
                virtual void Dispatch() {

                    if ((_mode & REGULAR) != 0) {
                        TRACE(Trace::Information, (_T("Start regular scan: %s"), Core::Time::Now().ToRFC1123().c_str()));
                        _parent.Run(_scanTime, _type, _flags);
                    }
                    else {
                        TRACE(Trace::Information, (_T("Start Low Energy scan: %s"), Core::Time::Now().ToRFC1123().c_str()));
                        _parent.Run(_scanTime, ((_mode & LIMITED) != 0), ((_mode & PASSIVE) != 0));
                    }
                    TRACE(Trace::Information, (_T("Scan completed: %s"), Core::Time::Now().ToRFC1123().c_str()));
                    _mode = 0;
                }

            private:
                Scanner& _parent;
                uint16_t _scanTime;
                uint32_t _type;
                uint8_t _flags;
                uint8_t _mode;
            };

        public:
            Scanner(BluetoothControl& parent) 
                : Bluetooth::HCISocket()
                , _parent(parent)
                , _activity(Core::ProxyType<Job>::Create(this)) {
            }
            virtual ~Scanner() {
                PluginHost::WorkerPool::Instance().Revoke(_activity);
            }

        public:
            virtual void DiscoveredDevice (const bool lowEnergy, const Bluetooth::Address& address, const string& name) override {
		TRACE(Trace::Information, (_T("Discoverd Device: %s: %s"), address.ToString().c_str(), name.c_str()));

                _parent.DiscoveredDevice (lowEnergy, address, name);
            }
            void Scan(const uint16_t scanTime, const uint32_t type, const uint8_t flags) {
                _activity->Load(scanTime, type, flags);
            }
            void Scan(const uint16_t scanTime, const bool limited, const bool passive) {
                _activity->Load(scanTime, limited, passive);
            }
            uint32_t Open (const Bluetooth::Address& address) {
                uint32_t result = Core::ERROR_NONE;
 
                if (IsOpen() == false) {
                    Bluetooth::HCISocket::LocalNode(address.NodeId(HCI_CHANNEL_RAW));
                    result = Bluetooth::HCISocket::Open(Core::infinite);
                }

                return (result);
            }
            uint32_t Close() {
                return (Bluetooth::HCISocket::Close(Core::infinite));
            }

        private:
            void Run (const uint16_t scanTime, const uint32_t type, const uint8_t flags) {
	        Bluetooth::HCISocket::Scan(this, scanTime, type, flags);
	    }
	    void Run(const uint16_t scanTime, const bool limited, const bool passive) {
	        Bluetooth::HCISocket::Scan(this, scanTime, limited, passive);
	    }
	 
        private:
            BluetoothControl& _parent;
            Core::ProxyType<Job> _activity;
        };

        class HIDSocket : public Bluetooth::L2Socket {
        private:
            HIDSocket() = delete;
            HIDSocket(const HIDSocket&) = delete;
            HIDSocket& operator= (const HIDSocket&) = delete;

            static constexpr uint8_t ATT_OP_FIND_BY_TYPE_REQ  = 0x06;
            static constexpr uint8_t ATT_OP_FIND_BY_TYPE_RESP = 0x07;

            // UUID
            static constexpr uint16_t PRIMARY_SERVICE_UUID    = 0x2800;
            static constexpr uint16_t HID_UUID                = 0x1812;
            static constexpr uint16_t PNP_UUID                = 0x2a50;
            static constexpr uint16_t REPORT_UUID             = 0x2a4d;
 
            class InputDevice {
            private:
                InputDevice(const InputDevice&) = delete;
                InputDevice& operator= (const InputDevice&) = delete;

            public:
                InputDevice () : _descriptor (-1) {
                }
                ~InputDevice() {
                    Close();
                }

            public:
                bool IsOpen() const {
                    return (_descriptor != -1);
                }
                bool Send (const uint8_t length, const uint8_t data[]) {
                    struct uhid_event uhidEvent;
                    memset(&uhidEvent, 0, sizeof(uhidEvent));
                    uhidEvent.type = UHID_INPUT;
                    uhidEvent.u.input.size = length;
                    uhidEvent.u.input.data[0] = 0x01;
                    uint16_t count = 0;
                    uint16_t index = 0;
                    while (index < length) {
                        // This is an inherited construction. I do not trust it as I would expect that the
                        // uhidEvent.u.input.size should be updated as well. Till we tested it we leave it
                        // like this.
                        if (data[index] != 0x00) {
                            uhidEvent.u.input.data[count++] = data[index];
                        }
                        index++;
                   }

                   return (::write(_descriptor, &uhidEvent, sizeof(uhidEvent)) >= 0);
               } 
               uint32_t Open(const string& path, const Bluetooth::Address& source, const Bluetooth::Address& destination, const Metadata& info) {
                   uint32_t result = Core::ERROR_ALREADY_CONNECTED;

                   if (_descriptor == -1) {
                       result = Core::ERROR_OPENING_FAILED;
                       _descriptor = ::open(path.c_str(), O_RDWR | O_CLOEXEC);

                       if (_descriptor >= 0) {
                           struct uhid_event uhidEvent;
                           result = Core::ERROR_NONE;

                           // Creating UHID Node.
                           memset(&uhidEvent, 0, sizeof(uhidEvent));
                           uhidEvent.type = UHID_CREATE;
                           uhidEvent.u.create.bus = BUS_BLUETOOTH;
                           uhidEvent.u.create.vendor = info.VendorId();
                           uhidEvent.u.create.product = info.ProductId();
                           uhidEvent.u.create.version = info.Version();
                           uhidEvent.u.create.country = info.Country();
                           strncpy(reinterpret_cast<char*>(uhidEvent.u.create.name), info.Name().c_str(), sizeof(uhidEvent.u.create.name));
                           uhidEvent.u.create.rd_data = const_cast<uint8_t*>(info.Blob());
                           uhidEvent.u.create.rd_size = info.Length();
                           strncpy(reinterpret_cast<char*>(uhidEvent.u.create.phys), source.ToString().c_str(), sizeof(uhidEvent.u.create.phys));
                           strncpy(reinterpret_cast<char*>(uhidEvent.u.create.uniq), destination.ToString().c_str(), sizeof(uhidEvent.u.create.uniq));
                            
                           if (::write(_descriptor, &uhidEvent, sizeof(uhidEvent)) < 0) {
                               ::close(_descriptor);
                               _descriptor = -1;
                               result = Core::ERROR_WRITE_ERROR;
                           }
                       }
                   }
                   return (result);
                }
                uint32_t Close() {
                    if (_descriptor != -1) {
                        ::close(_descriptor);
                        _descriptor = -1;
                    }
                    return (Core::ERROR_NONE);
                }

            private:
                int _descriptor;
            };

        public:
            HIDSocket(const string& HIDPath, const Bluetooth::Address& localNode, const Bluetooth::Address& remoteNode) 
                : Bluetooth::L2Socket(
                      localNode.NodeId(Bluetooth::L2Socket::LE_ATT_CID, BDADDR_LE_PUBLIC),
                      remoteNode.NodeId(Bluetooth::L2Socket::LE_ATT_CID, BDADDR_LE_PUBLIC),
                      0x0001, 0xFFFF, 256)
                , _hidPath(HIDPath)
                , _localNode(localNode)
                , _remoteNode(remoteNode)
                , _device()
                , _inputHandler(nullptr) {
                const uint16_t message[] = { 0x0001, 0xFFFF, PRIMARY_SERVICE_UUID, HID_UUID }; 
                CreateFrame(ATT_OP_FIND_BY_TYPE_REQ, (sizeof(message)/sizeof(uint16_t)), message);
                printf ("Connecting from: [%s] to: [%s]\n", LocalNode().HostName().c_str(), RemoteNode().HostName().c_str());
            }
            virtual ~HIDSocket() {
            }

        protected:
            virtual bool Initialize() override {
                printf("Setting the security.\n");
                Security(BT_SECURITY_MEDIUM, 0);
                printf("Done.\n");
                return (Bluetooth::L2Socket::Initialize());
            }
            
        private:
            void EnableInputNotification()
            {
                const uint16_t message[] = { 0x0001, 0xFFFF, REPORT_UUID }; 
                CreateFrame(ATT_OP_READ_BY_TYPE_REQ, (sizeof(message)/sizeof(uint16_t)), message);
            }
            virtual void Received(const uint8_t dataFrame[], const uint16_t availableData) {
                if (availableData == 0) {
                    if (_hidPath.empty() == false) {
                        _device.Open(_hidPath, _localNode, _remoteNode, Info());
                        if (_device.IsOpen() == true) {
                            EnableInputNotification();
                        }
                    }
                    else {
                        _inputHandler = PluginHost::InputHandler::KeyHandler();
                        if (_inputHandler  != nullptr) {
                            EnableInputNotification();
                        }
                    }
                }
                else if (dataFrame[0] == ATT_OP_HANDLE_NOTIFY) {
                    // We got a key press.. where to ?
                    if (_device.IsOpen() == true) {
                        _device.Send(availableData - 2, &(dataFrame[2]));
                    }
                }
                else if (dataFrame[0] == ATT_OP_READ_BY_TYPE_RESP) {
                    // We have the notification responses.
                    _index = 0;
                    _reportHandles[0] = dataFrame[6] + 1;
                    _reportHandles[1] = dataFrame[10] + 1;
                    const uint16_t message[] = { static_cast<uint16_t>(dataFrame[2] + 1), 1 };
                    CreateFrame(ATT_OP_WRITE_REQ, (sizeof(message)/sizeof(uint16_t)), message);
                }
                else if (dataFrame[0] == ATT_OP_WRITE_RESP) {
                    if (_index < 2) {
                        const uint16_t message[] = { _reportHandles[_index++], 1 };
                        CreateFrame(ATT_OP_WRITE_REQ, (sizeof(message) / sizeof(uint16_t)), message );
                    }
                    else {
                        TRACE(Trace::Information, (_T("We have reached an observing mode!!!")));
                    }
                }
            }

        private:
            const string _hidPath;
            Bluetooth::Address _localNode;
            Bluetooth::Address _remoteNode;
            InputDevice _device; 
            PluginHost::VirtualInput* _inputHandler;
            uint8_t _index;
            uint16_t _reportHandles[2];
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&);
            Config& operator=(const Config&);

        public:
            Config()
                : Core::JSON::Container()
                , Interface(0)
                , HIDPath()
            {
                Add(_T("interface"), &Interface);
                Add(_T("hidpath"), &HIDPath);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::DecUInt8 Interface;
            Core::JSON::String HIDPath;
        };

    public:
        class EXTERNAL DeviceImpl : public IBluetooth::IDevice {
        private: 
            DeviceImpl() = delete;
            DeviceImpl(const DeviceImpl&) = delete;
            DeviceImpl& operator=(const DeviceImpl&) = delete;

            static constexpr uint32_t MAX_ACTION_TIMEOUT = 2000; /* 2S to setup a connection ? */
            static constexpr uint16_t ACTION_MASK = 0x0FFF;

            enum state : uint16_t {
                DECOUPLED     = 0x0001,
                METADATA      = 0x0002,
                CONNECTING    = 0x0004,
                DISCONNECTING = 0x0008,
                PAIRED        = 0x4000,
                LOWENERGY     = 0x8000
            };

            class Sink : public Core::IOutbound::ICallback {
            private:
                Sink() = delete;
                Sink(const Sink&) = delete;
                Sink& operator= (const Sink&) = delete;

            public:
                Sink (DeviceImpl& parent) : _parent(parent) {
                }
                virtual ~Sink() {
                }

            public:
                virtual void Updated (const Core::IOutbound& data, const uint32_t error_code) override {
                    _parent.Updated(data, error_code);
                }

            private:
                DeviceImpl& _parent;
            };

        public:
            class JSON : public Core::JSON::Container {
            private:
                JSON& operator= (const JSON&);

            public:
                JSON()
                    : Core::JSON::Container()
                    , Address()
                    , Name()
                    , LowEnergy(false)
                {
                    Add(_T("address"), &Address);
                    Add(_T("name"), &Name);
                    Add(_T("le"), &LowEnergy);
                }
                JSON(const JSON& copy)
                    : Core::JSON::Container()
                    , Address()
                    , Name()
                    , LowEnergy(false)
                {
                    Add(_T("address"), &Address);
                    Add(_T("name"), &Name);
                    Add(_T("le"), &LowEnergy);
                    Address = copy.Address;
                    Name = copy.Name;
                    LowEnergy= copy.LowEnergy;
                }
                virtual ~JSON() {
                }

            public:
                JSON& Set (const DeviceImpl* source) {
                    if (source != nullptr) {
                        Address = source->Address();
                        Name = source->Name();
                        LowEnergy = source->LowEnergy();
                    }
                    else {
                        Address.Clear();
                        Name.Clear();
                        LowEnergy.Clear();
                    }
                    return (*this);
                }
                Core::JSON::String Address;
                Core::JSON::String Name;
                Core::JSON::Boolean LowEnergy;
            };

            class IteratorImpl : public IBluetooth::IDevice::IIterator {
            private:
                IteratorImpl() = delete;
                IteratorImpl(const IteratorImpl&) = delete;
                IteratorImpl& operator= (const IteratorImpl&) = delete;

            public:
                IteratorImpl(const std::list<DeviceImpl*>& container) {
                    std::list<DeviceImpl*>::const_iterator index = container.begin();
                    while (index != container.end()) {
                        IBluetooth::IDevice* element = (*index);
                        element->AddRef();
                        _list.push_back(element);
                        index++;
                    }
                }
                virtual ~IteratorImpl() {
                    while (_list.size() != 0) {
                        _list.front()->Release();
                        _list.pop_front();
                    }
                }
 
            public: 
                virtual void Reset() override {
                    _index = 0;
                }
                virtual bool IsValid() const override {
                    return ((_index != 0) && (_index <= _list.size()));
                }
                virtual bool Next() override {
                    if (_index == 0) {
                        _index = 1;
                        _iterator = _list.begin();
                    }
                    else if (_index <= _list.size()) {
                        _index++;
                        _iterator++;
                    }
                    return (IsValid());
                }
                virtual IBluetooth::IDevice* Current() {
                    ASSERT (IsValid() == true);
                    IBluetooth::IDevice* result = nullptr;
                    result = (*_iterator);
                    ASSERT (result != nullptr);
                    result->AddRef();
                    return (result);
                }

                BEGIN_INTERFACE_MAP(IteratorImpl)
                    INTERFACE_ENTRY(IBluetooth::IDevice::IIterator)
                END_INTERFACE_MAP

            private:
                uint32_t _index;
                std::list<IBluetooth::IDevice*> _list;
                std::list<IBluetooth::IDevice*>::iterator _iterator;
            };

        public:
            DeviceImpl(Bluetooth::HCISocket* application, const bool lowEnergy, const Bluetooth::Address& address, const string& name) 
                : _application(application)
                , _address(address)
                , _name(name)
                , _fullName()
                , _state( static_cast<state>((_application != nullptr ? METADATA : DECOUPLED) | (lowEnergy ? LOWENERGY : 0)) )
                , _handle(~0)
                , _channel(nullptr)
                , _sink (*this) {
                RemoteName();
            }
            ~DeviceImpl() {
            }

        public:
            virtual uint32_t Pair(const string&) override;
            virtual uint32_t Unpair() override;

            virtual uint32_t Connect () {
                uint32_t result = Core::ERROR_INPROGRESS;

                _state.Lock();

                if ((_state & ACTION_MASK) == 0) {

                    result = Core::ERROR_NONE;

                    _state.SetState(static_cast<state>(_state.GetState() | CONNECTING));

                    if ((_state & LOWENERGY) == 0) {

                        connect.Clear();
                        connect->bdaddr         = *_address.Data();
                        connect->pkt_type       = htobs(HCI_DM1 | HCI_DM3 | HCI_DM5 | HCI_DH1 | HCI_DH3 | HCI_DH5);
                        connect->pscan_rep_mode = 0x02;
                        connect->clock_offset   = 0x0000;
                        connect->role_switch    = 0x01;
                        connect.Send(*_application, MAX_ACTION_TIMEOUT, &_sink);
                    }
                    else {

                        connectLE.Clear();
                        connectLE->interval = htobs(0x0004);
                        connectLE->window = htobs(0x0004);
                        connectLE->initiator_filter = 0;
                        connectLE->peer_bdaddr_type = LE_PUBLIC_ADDRESS;
                        connectLE->peer_bdaddr = *_address.Data();
                        connectLE->own_bdaddr_type = LE_PUBLIC_ADDRESS;
                        connectLE->min_interval = htobs(0x000F);
                        connectLE->max_interval = htobs(0x000F);
                        connectLE->latency = htobs(0x0000);
                        connectLE->supervision_timeout = htobs(0x0C80);
                        connectLE->min_ce_length = htobs(0x0001);
                        connectLE->max_ce_length = htobs(0x0001);
                        connectLE.Send(*_application, MAX_ACTION_TIMEOUT, &_sink);
                    }
                }

                _state.Unlock();
            }
            virtual void Disconnect (const uint8_t reason) {

                uint32_t result = Core::ERROR_INPROGRESS;

                _state.Lock();

                if ((_state & ACTION_MASK) == 0) {

                    result = Core::ERROR_NONE;

                    _state.SetState(static_cast<state>(_state.GetState() | DISCONNECTING));

                    disconnect->handle = htobs(_handle);
                    disconnect->reason = reason;
                    disconnect.Send(*_application, MAX_ACTION_TIMEOUT, &_sink);
                }

                _state.Unlock();
            }
            virtual string Address() const override {
                return (_address.ToString());
            }
            virtual string Name() const override {
                return (_name);
            }
            virtual bool IsDiscovered () const override {
                return ((_state & (METADATA|DECOUPLED)) == 0);
            }
            virtual bool IsPaired() const override {
                return ((_state & PAIRED) != 0);
            }
            virtual bool IsConnected() const override {
                return (_handle != static_cast<uint16_t>(~0));
            }
            inline bool LowEnergy() const {
                return ((_state & LOWENERGY) != 0);
            }
            inline void Clear() { 
                _state.Lock();
                if ( (IsConnected() == false) && (IsPaired() == false) && ((_state & ACTION_MASK) == 0) ) {
                    _state.SetState(static_cast<state>(_state.GetState() | DECOUPLED));
                    _fullName.clear();
                }
                _state.Unlock();
            }
            inline void Discovered() { 
                _state.Lock();
                if ( ((_state & ACTION_MASK) == DECOUPLED) && (_application != nullptr) ) {
                    _state.SetState(static_cast<state>(_state.GetState() | METADATA));
                    RemoteName();
                }
                _state.Unlock();
            }
            inline bool operator== (const Bluetooth::Address& rhs) const {
                return (_address == rhs);
            }
            inline bool operator!= (const Bluetooth::Address& rhs) const {
                return (_address != rhs);
            }

            BEGIN_INTERFACE_MAP(DeviceImpl)
                INTERFACE_ENTRY(IBluetooth::IDevice)
            END_INTERFACE_MAP

        private:
            void RemoteName() {

                if ((_state & ACTION_MASK) == METADATA) {

                    name.Clear();
                    name->bdaddr         = *_address.Data();
                    name->pscan_mode     = 0x00;
                    name->pscan_rep_mode = 0x02;
                    name->clock_offset   = 0x0000;
                    name.Send(*_application, MAX_ACTION_TIMEOUT, &_sink);
                }
            }
            void Updated (const Core::IOutbound& data, const uint32_t error_code) {
                if (data.Id() == Bluetooth::HCISocket::Command::RemoteName::ID) {
                    // Metadata is flowing in, handle it..
                    // _cmds.name.Response().bdaddr;
                    const char* longName = reinterpret_cast<const char*>(name.Response().name);
                    uint8_t index = 0; 
                    while (index < HCI_MAX_NAME_LENGTH) { printf("%c", ::isprint(longName[index]) ? longName[index] : '.');  index++; }
                    index = 0;
                    while ((index < HCI_MAX_NAME_LENGTH) && (::isprint(longName[index]) != 0)) { index++; }
                     
                    _fullName = std::string(longName, index);
                    printf ("Loaded Long Device Name: %s\n", _fullName.c_str());
                    _state.SetState(static_cast<state>(_state.GetState() & (~METADATA)));
                }
                else if (data.Id() == Bluetooth::HCISocket::Command::Connect::ID) {
                    // looks like we are connected..
                    _handle = connect.Response().handle;
                    printf ("Connected using handle: %d\n", _handle);
                    _state.SetState(static_cast<state>(_state.GetState() & (~CONNECTING)));
                }
                else if (data.Id() == Bluetooth::HCISocket::Command::ConnectLE::ID) {
                    _handle = connect.Response().handle;
                    printf ("Connected using handle: %d\n", _handle);
                    _state.SetState(static_cast<state>(_state.GetState() & (~CONNECTING)));
                }
                else if (data.Id() == Bluetooth::HCISocket::Command::Disconnect::ID) {
                    if (error_code == Core::ERROR_NONE) {
                        printf ("Disconnected\n");
                        _handle = ~0;
                    }
                    else {
                        printf ("Disconnected Failed!\n");
                    }
                    _state.SetState(static_cast<state>(_state.GetState() & (~DISCONNECTING)));
                }
            }

        private:
            Bluetooth::HCISocket* _application;
            Bluetooth::Address _address;
            string _name;
            string _fullName;
            Core::StateTrigger<state> _state;
            uint16_t _handle;
            Bluetooth::L2Socket* _channel;
            Sink _sink;

               Bluetooth::HCISocket::Command::Connect    connect;
               Bluetooth::HCISocket::Command::ConnectLE  connectLE;
               Bluetooth::HCISocket::Command::Disconnect disconnect;
               Bluetooth::HCISocket::Command::RemoteName name;
        };

        class EXTERNAL Status : public Core::JSON::Container {
        private:
            Status(const Status&) = delete;
            Status& operator=(const Status&) = delete;

        public:
            Status()
                : Scanning()
                , Devices() {
                Add(_T("scanning"), &Scanning);
                Add(_T("devices"), &Devices);
            }
            virtual ~Status() {
            }

        public:
            Core::JSON::Boolean Scanning;
            Core::JSON::ArrayType<DeviceImpl::JSON> Devices;
        };

    public:
        BluetoothControl()
            : _skipURL(0)
            , _adminLock()
            , _service(nullptr)
            , _driver(nullptr)
            , _administrator(Core::NodeId (HCI_DEV_NONE, HCI_CHANNEL_CONTROL))
            , _application(*this)
            , _btAddress()
            , _interface()
            , _devices()
            , _observers()
        {
        }
        virtual ~BluetoothControl()
        {
        }

    public:
        BEGIN_INTERFACE_MAP(BluetoothControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_ENTRY(Exchange::IBluetooth)
        END_INTERFACE_MAP

    public:
        //  IPlugin methods
        // -------------------------------------------------------------------------------------------------------

        // First time initialization. Whenever a plugin is loaded, it is offered a Service object with relevant
        // information and services for this particular plugin. The Service object contains configuration information that
        // can be used to initialize the plugin correctly. If Initialization succeeds, return nothing (empty string)
        // If there is an error, return a string describing the issue why the initialisation failed.
        // The Service object is *NOT* reference counted, lifetime ends if the plugin is deactivated.
        // The lifetime of the Service object is guaranteed till the deinitialize method is called.
        virtual const string Initialize(PluginHost::IShell* service);

        // The plugin is unloaded from the webbridge. This is call allows the module to notify clients
        // or to persist information if needed. After this call the plugin will unlink from the service path
        // and be deactivated. The Service object is the same as passed in during the Initialize.
        // After this call, the lifetime of the Service object ends.
        virtual void Deinitialize(PluginHost::IShell* service);

        // Returns an interface to a JSON struct that can be used to return specific metadata information with respect
        // to this plugin. This Metadata can be used by the MetaData plugin to publish this information to the outside world.
        virtual string Information() const;

        //  IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request);
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request);

        //  IBluetooth methods
        // -------------------------------------------------------------------------------------------------------
        virtual bool IsScanning() const override;
        virtual uint32_t Register(IBluetooth::INotification* notification) override;
        virtual uint32_t Unregister(IBluetooth::INotification* notification) override;

        virtual bool Scan(const bool enable) override;
        virtual IBluetooth::IDevice* Device (const string&) override;
        virtual IBluetooth::IDevice::IIterator* Devices () override;

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        void RemoveDevices(std::function<bool(DeviceImpl*)> filter);
        void DiscoveredDevice(const bool lowEnergy, const Bluetooth::Address& address, const string& name);
        DeviceImpl* Find(const string&);

    private:
        uint8_t _skipURL;
        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        Bluetooth::Driver* _driver;
        Bluetooth::HCISocket _administrator;
        Scanner _application;
        Bluetooth::Address _btAddress;
        Bluetooth::Driver::Interface _interface;
        std::list<DeviceImpl*> _devices;
        std::list<IBluetooth::INotification*> _observers;
        static string _HIDPath;
    };
} //namespace Plugin

} //namespace Solution
