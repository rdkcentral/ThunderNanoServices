#ifndef PLUGIN_NETWORKCONTROL_H
#define PLUGIN_NETWORKCONTROL_H

#include "DHCPClientImplementation.h"
#include "Module.h"

#include <interfaces/IIPNetwork.h>
#include <interfaces/json/JsonData_NetworkControl.h> 

namespace WPEFramework {
namespace Plugin {

    class NetworkControl : public Exchange::IIPNetwork,
                           public PluginHost::IPlugin,
                           public PluginHost::IWeb,
                           public PluginHost::JSONRPC {
    public:
        class Entry : public Core::JSON::Container {
        private:
            Entry& operator=(const Entry&) = delete;

        public:
            Entry()
                : Core::JSON::Container()
                , Interface()
                , Mode(JsonData::NetworkControl::NetworkData::ModeType::MANUAL)
                , Address()
                , Mask(32)
                , Gateway()
                , _broadcast()
            {
                Add(_T("interface"), &Interface);
                Add(_T("mode"), &Mode);
                Add(_T("address"), &Address);
                Add(_T("mask"), &Mask);
                Add(_T("gateway"), &Gateway);
                Add(_T("broadcast"), &_broadcast);
            }
            Entry(const Entry& copy)
                : Core::JSON::Container()
                , Interface(copy.Interface)
                , Mode(copy.Mode)
                , Address(copy.Address)
                , Mask(copy.Mask)
                , Gateway(copy.Gateway)
                , _broadcast(copy._broadcast)
            {
                Add(_T("interface"), &Interface);
                Add(_T("mode"), &Mode);
                Add(_T("address"), &Address);
                Add(_T("mask"), &Mask);
                Add(_T("gateway"), &Gateway);
                Add(_T("broadcast"), &_broadcast);
            }
            virtual ~Entry()
            {
            }

        public:
            Core::JSON::String Interface;
            Core::JSON::EnumType<JsonData::NetworkControl::NetworkData::ModeType> Mode;
            Core::JSON::String Address;
            Core::JSON::DecUInt8 Mask;
            Core::JSON::String Gateway;

        public:
            void Broadcast(const Core::NodeId& address)
            {
                _broadcast = address.HostAddress();
            }
            Core::NodeId Broadcast() const
            {
                Core::NodeId result;

                if (_broadcast.IsSet() == false) {

                    Core::IPNode address(Core::NodeId(Address.Value().c_str()), Mask.Value());

                    result = address.Broadcast();

                } else {
                    result = Core::NodeId(_broadcast.Value().c_str());
                }

                return (result);
            }

        private:
            Core::JSON::String _broadcast;
        };

    private:
        class AdapterObserver : public Core::IDispatch,
                                public WPEFramework::Core::AdapterObserver::INotification {
        private:
            AdapterObserver() = delete;
            AdapterObserver(const AdapterObserver&) = delete;
            AdapterObserver& operator=(const AdapterObserver&) = delete;

        public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            AdapterObserver(NetworkControl* parent)
                : _parent(*parent)
                , _adminLock()
                , _observer(this)
                , _reporting()
            {
                ASSERT(parent != nullptr);
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
            virtual ~AdapterObserver()
            {
            }

        public:
            void Open()
            {
                _observer.Open();
            }
            void Close()
            {
                Core::ProxyType<Core::IDispatch> job(*this);
                _observer.Close();

                _adminLock.Lock();

                PluginHost::WorkerPool::Instance().Revoke(job);

                _reporting.clear();

                _adminLock.Unlock();
            }
            virtual void Event(const string& interface) override
            {

                _adminLock.Lock();

                if (std::find(_reporting.begin(), _reporting.end(), interface) == _reporting.end()) {
                    // We need to add this interface, it is currently not present.
                    _reporting.push_back(interface);

                    // If this is the first entry, we need to submit a job for processing
                    if (_reporting.size() == 1) {
                        // These events tend to "dender" a lot. Give it some time
                        Core::Time entry(Core::Time::Now().Add(100));
                        Core::ProxyType<Core::IDispatch> job(*this);

                        PluginHost::WorkerPool::Instance().Schedule(entry, job);
                    }
                }

                _adminLock.Unlock();
            }
            virtual void Dispatch() override
            {
                // Yippie a yee, we have an interface notification:
                _adminLock.Lock();
                while (_reporting.size() != 0) {
                    const string interfaceName(_reporting.front());
                    _reporting.pop_front();
                    _adminLock.Unlock();

                    _parent.Activity(interfaceName);

                    _adminLock.Lock();
                }
                _adminLock.Unlock();
            }

        private:
            NetworkControl& _parent;
            Core::CriticalSection _adminLock;
            Core::AdapterObserver _observer;
            std::list<string> _reporting;
        };

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , DNSFile("/etc/resolv.conf")
                , Interfaces()
                , DNS()
                , TimeOut(5)
                , Retries(4)
                , Open(true)
            {
                Add(_T("dnsfile"), &DNSFile);
                Add(_T("interfaces"), &Interfaces);
                Add(_T("timeout"), &TimeOut);
                Add(_T("retries"), &Retries);
                Add(_T("open"), &Open);
                Add(_T("dns"), &DNS);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String DNSFile;
            Core::JSON::ArrayType<Entry> Interfaces;
            Core::JSON::ArrayType<Core::JSON::String> DNS;
            Core::JSON::DecUInt8 TimeOut;
            Core::JSON::DecUInt8 Retries;
            Core::JSON::Boolean Open;
        };

        class StaticInfo {
        private:
            StaticInfo& operator=(const StaticInfo&) = delete;

        public:
            StaticInfo()
                : _mode(JsonData::NetworkControl::NetworkData::ModeType::MANUAL)
                , _address()
                , _gateway()
                , _broadcast()
            {
            }
            StaticInfo(const Entry& info)
                : _mode(info.Mode.Value())
                , _address(Core::IPNode(Core::NodeId(info.Address.Value().c_str()), info.Mask.Value()))
                , _gateway(Core::NodeId(info.Gateway.Value().c_str()))
                , _broadcast(info.Broadcast())
            {
            }
            StaticInfo(const StaticInfo& copy)
                : _mode(copy._mode)
                , _address(copy._address)
                , _gateway(copy._gateway)
                , _broadcast(copy._broadcast)
            {
            }
            ~StaticInfo()
            {
            }

        public:
            inline JsonData::NetworkControl::NetworkData::ModeType Mode() const
            {
                return (_mode);
            }
            inline const Core::IPNode& Address() const
            {
                return (_address);
            }
            inline const Core::NodeId& Gateway() const
            {
                return (_gateway);
            }
            inline const Core::NodeId& Broadcast() const
            {
                return (_broadcast);
            }
            inline const DHCPClientImplementation::Offer& Offer() const
            {
                return (_offer);
            }
            inline void Offer(const DHCPClientImplementation::Offer& offer)
            {
                _offer = offer;
            }
            inline void Store(Entry& info)
            {
                info.Mode = _mode;
                info.Address = _address.HostAddress();
                info.Mask = _address.Mask();
                info.Gateway = _gateway.HostAddress();
                info.Broadcast(_broadcast);
            }

        private:
            JsonData::NetworkControl::NetworkData::ModeType _mode;
            Core::IPNode _address;
            Core::NodeId _gateway;
            Core::NodeId _broadcast;
            DHCPClientImplementation::Offer _offer;
        };
        class DHCPEngine : public Core::IDispatch {
        private:
            DHCPEngine() = delete;
            DHCPEngine(const DHCPEngine&) = delete;
            DHCPEngine& operator=(const DHCPEngine&) = delete;

        public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            DHCPEngine(NetworkControl* parent, const string& interfaceName, const string& persistentStoragePath)
                : _parent(*parent)
                , _retries(0)
                , _client(interfaceName, std::bind(&DHCPEngine::NewOffer, this, std::placeholders::_1), 
                          std::bind(&DHCPEngine::RequestResult, this, std::placeholders::_1, std::placeholders::_2))
                , _leaseFilePath((persistentStoragePath.empty()) ? "" :  (persistentStoragePath + _client.Interface() + ".json"))
            {

            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
            ~DHCPEngine()
            {
            }
        public:
            // Permanent IP storage
            void SaveLeases();
            bool LoadLeases();

            inline DHCPClientImplementation::classifications Classification() const
            {
                return (_client.Classification());
            }
            inline uint32_t Discover()
            {
                return (Discover(Core::NodeId()));
            }

            inline uint32_t Discover(const Core::NodeId& preferred)
            {
                ResetWatchdog();
                uint32_t result = _client.Discover(preferred);

                return (result);
            }

            void GetIP() 
            {
                return (GetIP(Core::NodeId()));
            }

            void GetIP(const Core::NodeId& preferred)
            {

                auto offerIterator = _client.Offers(false);
                if (offerIterator.Next() == true) {
                    Request(offerIterator.Current());
                } else {
                    Discover(preferred);
                }
            }

            void NewOffer(const DHCPClientImplementation::Offer& offer) {
                if (_parent.NewOffer(_client.Interface(), offer) == true) {
                    Request(offer);
                }
            }

            void RequestResult(const DHCPClientImplementation::Offer& offer, const bool result) {
                StopWatchdog();

                JsonData::NetworkControl::ConnectionchangeParamsData::StatusType status;
                if (result == true) {
                    _parent.RequestAccepted(_client.Interface(), offer);
                    status = JsonData::NetworkControl::ConnectionchangeParamsData::StatusType::CONNECTED;
                } else {
                    _parent.RequestFailed(_client.Interface(), offer);
                    status = JsonData::NetworkControl::ConnectionchangeParamsData::StatusType::CONNECTIONFAILED;
                }

                _parent.event_connectionchange(_client.Interface().c_str(), offer.Address().HostAddress().c_str(), status);
            }

            inline void Request(const DHCPClientImplementation::Offer& offer) {

                ResetWatchdog();
                _client.Request(offer);
            }

            inline void Completed()
            {
                _client.Completed();
            }
            inline DHCPClientImplementation::Iterator Offers(const bool leased)
            {
                return (_client.Offers(leased));
            }
            inline void RemoveOffer(const DHCPClientImplementation::Offer& offer, const bool leased) 
            {
                _client.RemoveOffer(offer, leased);
            }

            void SetupWatchdog() 
            {
                const uint16_t responseMS = _parent.ResponseTime() * 1000;
                Core::Time entry(Core::Time::Now().Add(responseMS));
                _retries = _parent.Retries();

                Core::ProxyType<Core::IDispatch> job(*this);    

                // Submit a job, as watchdog.
                PluginHost::WorkerPool::Instance().Schedule(entry, job);
            }

            inline void StopWatchdog() 
            {
                PluginHost::WorkerPool::Instance().Revoke(Core::ProxyType<Core::IDispatch>(*this));
            }

            inline void ResetWatchdog() 
            {
                StopWatchdog();
                SetupWatchdog();
            }

            void CleanUp() 
            {
                StopWatchdog();
                _client.Completed();
            }

            virtual void Dispatch() override
            {
                if (_retries > 0) {
                    const uint16_t responseMS = _parent.ResponseTime() * 1000;
                    Core::Time entry(Core::Time::Now().Add(responseMS));
                    Core::ProxyType<Core::IDispatch> job(*this);

                    _retries--;
                    _client.Resend();

                    // Schedule next retry
                    PluginHost::WorkerPool::Instance().Schedule(entry, job);
                } else {
                    if (_client.Classification() == DHCPClientImplementation::CLASSIFICATION_DISCOVER) {

                        // No acceptable offer found
                        _parent.NoOffers(_client.Interface());
                    } else if (_client.Classification() == DHCPClientImplementation::CLASSIFICATION_REQUEST) {

                        // Request left without repsonse
                        DHCPClientImplementation::Iterator offer = _client.CurrentlyRequestedOffer();
                        if (offer.IsValid()) {
                            // Remove unresponsive offer from potential candidates
                            DHCPClientImplementation::Offer copy = offer.Current(); 
                            _client.RemoveOffer(offer.Current(), false);
                            
                            // Inform controller that request failed
                            _parent.RequestFailed(_client.Interface(), copy);
                        }
                    }
                }
            }

        private:
            NetworkControl& _parent;
            uint8_t _retries;
            DHCPClientImplementation _client;
            string _leaseFilePath;
        };

    private:
        NetworkControl(const NetworkControl&) = delete;
        NetworkControl& operator=(const NetworkControl&) = delete;

    public:
        NetworkControl();
        virtual ~NetworkControl();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(NetworkControl)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(Exchange::IIPNetwork)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
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

        //   IIPNetwork methods
        // -------------------------------------------------------------------------------------------------------
        virtual uint32_t AddAddress(const string& interfaceName) override;
        virtual uint32_t AddAddress(const string& interfaceName, const string& IPAddress, const string& gateway, const string& broadcast, const uint8_t netmask) override;
        virtual uint32_t RemoveAddress(const string& interfaceName, const string& IPAddress, const string& gateway, const string& broadcast) override;
        virtual uint32_t AddDNS(IIPNetwork::IDNSServers* dnsEntries) override;
        virtual uint32_t RemoveDNS(IIPNetwork::IDNSServers* dnsEntries) override;

    private:
        uint32_t Reload(const string& interfaceName, const bool dynamic);
        uint32_t SetIP(Core::AdapterIterator& adapter, const Core::IPNode& ipAddress, const Core::NodeId& gateway, const Core::NodeId& broadcast);
        bool NewOffer(const string& interfaceName, const DHCPClientImplementation::Offer& offer);
        void RequestAccepted(const string& interfaceName, const DHCPClientImplementation::Offer& offer);
        void RequestFailed(const string& interfaceName, const DHCPClientImplementation::Offer& offer);
        void NoOffers(const string& interfaceName);
        void RefreshDNS();
        void Activity(const string& interface);
        uint16_t DeleteSection(Core::DataElementFile& file, const string& startMarker, const string& endMarker);
        inline uint8_t ResponseTime() const
        {
            return (_responseTime);
        }
        inline uint8_t Retries() const
        {
            return (_retries);
        }

        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_reload(const JsonData::NetworkControl::ReloadParamsInfo& params);
        uint32_t endpoint_request(const JsonData::NetworkControl::ReloadParamsInfo& params);
        uint32_t endpoint_assign(const JsonData::NetworkControl::ReloadParamsInfo& params);
        uint32_t endpoint_flush(const JsonData::NetworkControl::ReloadParamsInfo& params);
        uint32_t get_network(const string& index, Core::JSON::ArrayType<JsonData::NetworkControl::NetworkData>& response) const;
        uint32_t get_up(const string& index, Core::JSON::Boolean& response) const;
        uint32_t set_up(const string& index, const Core::JSON::Boolean& param);
        void event_connectionchange(const string& name, const string& address, const JsonData::NetworkControl::ConnectionchangeParamsData::StatusType& status);

    private:
        Core::CriticalSection _adminLock;
        uint16_t _skipURL;
        PluginHost::IShell* _service;
        uint8_t _responseTime;
        uint8_t _retries;
        string _dnsFile;
        string _persistentStoragePath;
        std::list<std::pair<uint16_t, Core::NodeId>> _dns;
        std::map<const string, StaticInfo> _interfaces;
        std::map<const string, Core::ProxyType<DHCPEngine>> _dhcpInterfaces;
        Core::ProxyType<AdapterObserver> _observer;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // PLUGIN_NETWORKCONTROL_H
