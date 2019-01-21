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

        class HCISocket : public Bluetooth::HCISocket {
        private:
            HCISocket() = delete;
            HCISocket(const HCISocket&) = delete;
            HCISocket& operator= (const HCISocket&) = delete;

        public:
            HCISocket(BluetoothControl& parent) 
                : Bluetooth::HCISocket()
                , _parent(parent) {
            }
            virtual ~HCISocket() {
            }

        public:
            virtual void DiscoveredDevice (const Bluetooth::Address& address, const string& shortName, const string& longName) {
                _parent.DiscoveredDevice (address, shortName, longName);
            }
            
        private:
            BluetoothControl& _parent;
        };

        class HIDSocket : public Bluetooth::L2Socket {
        private:
            HIDSocket() = delete;
            HIDSocket(const HCISocket&) = delete;
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
            }
            virtual ~HIDSocket() {
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
            DeviceImpl& operator=(const DeviceImpl&) = delete;

            enum state : uint8_t {
                DISCOVERED = 0x01,
                CONNECTED  = 0x02
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
                {
                    Add(_T("address"), &Address);
                    Add(_T("name"), &Name);
                }
                JSON(const JSON& copy)
                    : Core::JSON::Container()
                    , Address()
                    , Name()
                {
                    Add(_T("address"), &Address);
                    Add(_T("name"), &Name);
                    Address = copy.Address;
                    Name = copy.Name;
                }
                virtual ~JSON() {
                }

            public:
                JSON& Set (const DeviceImpl* source) {
                    if (source != nullptr) {
                        Address = source->Address();
                        Name = source->Name();
                    }
                    else {
                        Address.Clear();
                        Name.Clear();
                    }
                    return (*this);
                }
                Core::JSON::String Address;
                Core::JSON::String Name;
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
            DeviceImpl() 
                : _address()
                , _name()
                , _longName()
                , _state(0) {
            }
            DeviceImpl(const Bluetooth::Address& address, const string& shortName, const string& longName) 
                : _address(address)
                , _name(shortName)
                , _longName(longName)
                , _state(DISCOVERED) {
            }
            DeviceImpl(const DeviceImpl& copy) 
                : _address(copy._address)
                , _name(copy._name) 
                , _longName(copy._longName)
                , _state(copy._state) {
            }
            ~DeviceImpl() {
            }

        public:
            virtual uint32_t Pair(const string& source) override;
            virtual uint32_t Unpair() override;
            virtual string Address() const override {
                return (_address.ToString());
            }
            virtual string Name() const override {
                return (_name);
            }
            virtual bool IsDiscovered () const override {
                return ((_state & DISCOVERED) != 0);
            }
            virtual bool IsPaired() const override {
                return (_channel != nullptr);
            }
            virtual bool IsConnected() const override {
                return ((_state & CONNECTED) != 0);
            }
            inline void Clear() { 
                _state &= (~DISCOVERED);
            }
            inline bool operator== (const Bluetooth::Address& rhs) const {
                return (_address == rhs);
            }
            inline bool operator!= (const Bluetooth::Address& rhs) const {
                return (_address != rhs);
            }
            inline void Update (const string& shortName, const string& longName) {
                _name = shortName;
                _longName = longName;
                _state |= DISCOVERED;
            }

            BEGIN_INTERFACE_MAP(DeviceImpl)
                INTERFACE_ENTRY(IBluetooth::IDevice)
            END_INTERFACE_MAP

        private:
            Bluetooth::Address _address;
            string _name;
            string _longName;
            uint8_t _state;
            Bluetooth::L2Socket* _channel;
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
            , _hciSocket(*this)
            , _btAddress()
            , _interface()
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
        void DiscoveredDevice(const Bluetooth::Address& address, const string& shortName, const string& longName);
        DeviceImpl* Find(const string&);

    private:
        uint8_t _skipURL;
        Core::CriticalSection _adminLock;
        PluginHost::IShell* _service;
        Bluetooth::Driver* _driver;
        HCISocket _hciSocket;
        Bluetooth::Address _btAddress;
        Bluetooth::Driver::Interface _interface;
        std::list<DeviceImpl*> _devices;
        std::list<IBluetooth::INotification*> _observers;
        static string _HIDPath;
    };
} //namespace Plugin

} //namespace Solution
