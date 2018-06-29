#ifndef WIFICONTROL_H
#define WIFICONTROL_H

#include "Module.h"
#include "Network.h"
#ifdef USE_WIFI_HAL
#include "WifiHAL.h"
#else
#include "Controller.h"
#endif

namespace WPEFramework {
namespace Plugin {

    class WifiControl : public PluginHost::IPlugin, public PluginHost::IWeb {
    private:
        class Sink : public Core::IDispatchType<const WPASupplicant::Controller::events> {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator= (const Sink&) = delete;

        public:
            Sink(WifiControl& parent) : _parent(parent) {
            }
            virtual ~Sink() {
            }

        private:
            virtual void Dispatch (const WPASupplicant::Controller::events& event) override {
                _parent.WifiEvent(event);
            }

        private:
            WifiControl& _parent;
        };

        class WifiDriver {
        private:
            WifiDriver (const WifiDriver&) = delete;
            WifiDriver& operator= (const WifiDriver&) = delete;

       public:
            WifiDriver ()
                : _interfaceName()
                , _connector()
                , _process(true) {
            }
            ~WifiDriver () {
            }

        public:
            uint32_t Lauch (const string& connector, const string& interfaceName, const uint16_t waitTime) {
                _interfaceName = interfaceName;
                _connector = connector;

                Core::Process::Options options(_T("/usr/sbin/wpa_supplicant"));
                /* interface name *mandatory */
                options.Set(_T("i") + _interfaceName);

                /* ctrl_interface parameter *mandatory */
                options.Set(_T("C") + _connector);

                /* driver name (can be multiple drivers: nl80211,wext) *optional */
                options.Set(_T("Dnl80211"));

                /* global ctrl_interface group */
                options.Set(_T("G0"));

                #ifdef __DEBUG__
                options.Set(_T("dd"));
                options.Set(_T("f/tmp/wpasupplicant.log"));
                #endif

                TRACE_L1("Launching %s.", options.Command().c_str());
                uint32_t result = _process.Launch(options, &_pid);

        if (result == Core::ERROR_NONE) {
                    string remoteName(Core::Directory::Normalize(_connector) + _interfaceName);
                    uint32_t slices = (waitTime * 10);

                    // Wait till we see the socket being available.
                while ((slices != 0) && (_process.IsActive() == true) && (Core::File(remoteName).Exists() == false)) {
                slices--;
                        ::SleepMs(100);
                    }

                if ((slices == 0) || (_process.IsActive() == false)) {
                        _process.Kill(false);
                        result = Core::ERROR_TIMEDOUT;
                    }
                }

                return (result);
            }
            inline void Terminate() {
                _process.Kill(false);
            }

        private:
            string _interfaceName;
            string _connector;
            Core::Process _process;
            uint32_t _pid;
        };

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Connector(_T("/var/run/wpa_supplicant"))
                , Interface(_T("wlan0"))
        , Application(_T("/usr/sbin/wpa_supplicant"))
            {
                Add(_T("connector"), &Connector);
                Add(_T("interface"), &Interface);
                Add(_T("application"), &Application);
            }
            virtual ~Config()
            {
            }

        public:
            Core::JSON::String Connector;
            Core::JSON::String Interface;
            Core::JSON::String Application;
        };

        class Status : public Core::JSON::Container {
        private:
            Status(const Status&) = delete;
            Status& operator=(const Status&) = delete;

        public:
            Status()
                : Scanning()
                , Connected()
            {
                Add(_T("scanning"), &Scanning);
                Add(_T("connected"), &Connected);
            }
            virtual ~Status()
            {
            }

        public:
            Core::JSON::Boolean Scanning;
            Core::JSON::String Connected;
        };

        class NetworkList : public Core::JSON::Container {
        public:
            class Network : public Core::JSON::Container {
            private:
                Network& operator= (const Network&) = delete;

            public:
                class Pairing : public Core::JSON::Container {
                private:
                    Pairing& operator= (const Pairing&) = delete;

                public:
                    Pairing()
                        : Core::JSON::Container()
                        , Method()
                        , Keys() {
                        Add(_T("method"), &Method);
                        Add(_T("keys"), &Keys);
                    }
                    Pairing(const enum WPASupplicant::Network::pair method, const uint8_t keys)
                        : Core::JSON::Container()
                        , Method()
                        , Keys() {
                        Core::JSON::String textKey;

                        Add(_T("method"), &Method);
                        Add(_T("keys"), &Keys);

                        Method = string(Core::EnumerateType<WPASupplicant::Network::pair>(method).Data());

                        uint8_t bit = 0x1;
                        uint8_t value = keys;
                        while (value != 0) {
                            if ((bit & value) != 0) {
                                textKey = string(Core::EnumerateType<WPASupplicant::Network::key>(static_cast<WPASupplicant::Network::key>(bit)).Data());
                                Keys.Add(textKey);
                                value &= ~bit;
                            }
                            bit = (bit << 1);
                        }
                    }
                    Pairing(const Pairing& copy)
                        : Core::JSON::Container()
                        , Method(copy.Method)
                        , Keys(copy.Keys) {
                        Add(_T("method"), &Method);
                        Add(_T("keys"), &Keys);
                    }
                    ~Pairing() {
                    }

                public:
                    Core::JSON::String Method;
                    Core::JSON::ArrayType<Core::JSON::String> Keys;
                };

                Network ()
                    : Core::JSON::Container()
                    , BSSID()
                    , Frequency()
                    , Signal()
                    , Pairs()
                    , SSID()
                {
                    Add(_T("bssid"), &BSSID);
                    Add(_T("frequency"), &Frequency);
                    Add(_T("signal"), &Signal);
                    Add(_T("pairs"), &Pairs);
                    Add(_T("ssid"), &SSID);
                }
                Network (const WPASupplicant::Network& info)
                    : Core::JSON::Container()
                    , BSSID()
                    , Frequency()
                    , Signal()
                    , Pairs()
                    , SSID()
                {
                    Add(_T("bssid"), &BSSID);
                    Add(_T("frequency"), &Frequency);
                    Add(_T("signal"), &Signal);
                    Add(_T("pairs"), &Pairs);
                    Add(_T("ssid"), &SSID);

                    BSSID = info.BSSID();
                    Frequency = info.Frequency();
                    Signal = info.Signal();
                    SSID = info.SSID();

                    uint16_t bit = 0x1;
                    uint16_t pairs = info.Pair();
                    while (pairs != 0) {
                        if ((bit & pairs) != 0) {
                            WPASupplicant::Network::pair method (static_cast<WPASupplicant::Network::pair>(bit));

                            Pairs.Add(Pairing(method, info.Key(method)));
                            pairs &= ~bit;
                        }
                        bit = (bit << 1);
                    }
                }
                Network(const Network& copy)
                    : Core::JSON::Container()
                    , BSSID(copy.BSSID)
                    , Frequency(copy.Frequency)
                    , Signal(copy.Signal)
                    , Pairs(copy.Pairs)
                    , SSID(copy.SSID)
                {
                    Add(_T("bssid"), &BSSID);
                    Add(_T("frequency"), &Frequency);
                    Add(_T("signal"), &Signal);
                    Add(_T("pairs"), &Pairs);
                    Add(_T("ssid"), &SSID);
                }

/*
                Network& operator=(const Network& RHS)
                {
                    BSSID = RHS.BSSID;
                    Frequency = RHS.Frequency;
                    Signal = RHS.Signal;
                    Pairs = RHS.Pairs;
                    SSID = RHS.SSID;

                    return *this;
                }
*/

                virtual ~Network()
                {
                }

            public:
                Core::JSON::String BSSID;
                Core::JSON::DecUInt32 Frequency;
                Core::JSON::DecSInt32 Signal;
                Core::JSON::ArrayType<Pairing> Pairs;
                Core::JSON::String SSID;
            };

        private:
            NetworkList(const NetworkList&) = delete;
            NetworkList& operator=(const NetworkList&) = delete;

        public:
            NetworkList()
                : Core::JSON::Container()
                , Networks()
            {
                Add(_T("networks"), &Networks);
            }

            virtual ~NetworkList()
            {
            }

        public:
            void Set (WPASupplicant::Network::Iterator& list) {
                list.Reset();
                while (list.Next() == true) {
                    Networks.Add(Network(list.Current()));
                }
            }

            Core::JSON::ArrayType<Network> Networks;
        };

        class ConfigList : public Core::JSON::Container {
        public:
            class Config : public Core::JSON::Container {
            public:
                enum keyType {
                    UNKNOWN,
                    UNSECURE,
                    WPA,
                    ENTERPRISE
                };

            public:
                Config ()
                    : Core::JSON::Container()
                    , SSID()
                    , Identity()
                    , Password()
                    , PSK()
                    , Hash()
                    , KeyType(UNKNOWN)
                    , AccessPoint(false)
                    , Hidden(false)
                {
                    Add(_T("ssid"), &SSID);
                    Add(_T("identity"), &Identity);
                    Add(_T("password"), &Password);
                    Add(_T("psk"), &PSK);
                    Add(_T("hash"), &Hash);
                    Add(_T("type"), &KeyType);
                    Add(_T("accesspoint"), &AccessPoint);
                    Add(_T("hidden"), &Hidden);
                }
                Config (const WPASupplicant::Config& element)
                    : Core::JSON::Container()
                    , SSID()
                    , Identity()
                    , Password()
                    , PSK()
                    , Hash()
                    , KeyType(UNKNOWN)
                    , AccessPoint(false)
                    , Hidden(false)
                {
                    Add(_T("ssid"), &SSID);
                    Add(_T("identity"), &Identity);
                    Add(_T("password"), &Password);
                    Add(_T("psk"), &PSK);
                    Add(_T("hash"), &Hash);
                    Add(_T("type"), &KeyType);
                    Add(_T("accesspoint"), &AccessPoint);
                    Add(_T("hidden"), &Hidden);

                    Set(element);
               }
                Config(const Config& copy)
                    : Core::JSON::Container()
                    , SSID(copy.SSID)
                    , Identity(copy.Identity)
                    , Password(copy.Password)
                    , PSK(copy.PSK)
                    , Hash(copy.Hash)
                    , KeyType(copy.KeyType)
                    , AccessPoint(copy.AccessPoint)
                    , Hidden(copy.Hidden)
                {
                    Add(_T("ssid"), &SSID);
                    Add(_T("identify"), &Identity);
                    Add(_T("password"), &Password);
                    Add(_T("psk"), &PSK);
                    Add(_T("hash"), &Hash);
                    Add(_T("type"), &KeyType);
                    Add(_T("accesspoint"), &AccessPoint);
                    Add(_T("hidden"), &Hidden);
                }
                Config& operator=(const Config& RHS)
                {
                    SSID = RHS.SSID;
                    Identity = RHS.Identity;
                    Password = RHS.Password;
                    PSK = RHS.PSK;
                    Hash = RHS.Hash;
                    KeyType = RHS.KeyType;
                    AccessPoint = RHS.AccessPoint;
                    Hidden = RHS.Hidden;

                    return *this;
                }
                virtual ~Config()
                {
                }

            public:
                void Set (const WPASupplicant::Config& element) {
                    SSID = element.SSID();

                    if (element.IsUnsecure() == true) {
                        KeyType = UNSECURE;
                    }
                    else if (element.IsWPA() == true) {
                        KeyType = WPA;
                        if (element.Hash().empty() == false) {
                            Hash = element.Hash();
                        }
                        else {
                            PSK = element.PresharedKey();
                        }
                    }
                    else if (element.IsEnterprise() == true) {
                        KeyType = ENTERPRISE;
                        Identity = element.Identity();
                        Password = element.Password();
                    }

                    AccessPoint = element.IsAccessPoint();
                    Hidden = element.IsHidden();
                }

            public:
                Core::JSON::String SSID;
                Core::JSON::String Identity;
                Core::JSON::String Password;
                Core::JSON::String PSK;
                Core::JSON::String Hash;
                Core::JSON::EnumType<keyType> KeyType;
                Core::JSON::Boolean AccessPoint;
                Core::JSON::Boolean Hidden;
            };

        private:
            ConfigList(const ConfigList&) = delete;
            ConfigList& operator=(const ConfigList&) = delete;

        public:
            ConfigList()
                : Core::JSON::Container()
                , Configs()
            {
                Add(_T("configs"), &Configs);
            }

            virtual ~ConfigList()
            {
            }

        public:
            void Set (WPASupplicant::Config::Iterator& list) {
                while (list.Next() == true) {
                    Configs.Add(Config(list.Current()));
                }
            }

            Core::JSON::ArrayType<Config> Configs;
        };


    private:
        WifiControl(const WifiControl&) = delete;
        WifiControl& operator=(const WifiControl&) = delete;

    public:
        WifiControl();

        virtual ~WifiControl()
        {
        }

        BEGIN_INTERFACE_MAP(WifiControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index);

        void WifiEvent (const WPASupplicant::Controller::events& event);

    private:
        uint8_t _skipURL;
        PluginHost::IShell* _service;
        string _configurationStore;
        Sink _sink;
        WifiDriver _wpaSupplicant;
        Core::ProxyType<WPASupplicant::Controller> _controller;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // WIFICONTROL_H
