/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "Module.h"
#include "DHCPClient.h"
#include <interfaces/IConfiguration.h>
#include <interfaces/INetworkControl.h>

namespace Thunder {

namespace Plugin
{
    class NetworkControlImplementation : public Exchange::INetworkControl,
                                         public Exchange::IConfiguration {

    public:
        using NameServers = DHCPClient::NameServers;

        class Entry : public Core::JSON::Container {
        public:
            Entry& operator=(Entry&&) = delete;
            Entry& operator=(const Entry&) = delete;

            Entry()
                : Core::JSON::Container()
                , Interface()
                , MAC()
                , Source()
                , Mode(Exchange::INetworkControl::ModeType::DYNAMIC)
                , Address()
                , Mask(32)
                , Gateway()
                , LeaseTime(0)
                , RenewalTime(0)
                , RebindingTime(0)
                , Xid(0)
                , DNS()
                , TimeOut(10)
                , Retries(3)
                , _broadcast()
            {
                Add(_T("interface"), &Interface);
                Add(_T("mac"), &MAC);
                Add(_T("source"), &Source);
                Add(_T("mode"), &Mode);
                Add(_T("address"), &Address);
                Add(_T("mask"), &Mask);
                Add(_T("gateway"), &Gateway);
                Add(_T("broadcast"), &_broadcast);
                Add(_T("leaseTime"), &LeaseTime);
                Add(_T("renewalTime"), &RenewalTime);
                Add(_T("rebindingTime"), &RebindingTime);
                Add(_T("xid"), &Xid);
                Add(_T("dns"), &DNS);
                Add(_T("timeout"), &TimeOut);
                Add(_T("retries"), &Retries);
            }
            Entry(Entry&& move)
                : Core::JSON::Container()
                , Interface(move.Interface)
                , MAC(move.MAC)
                , Source(move.Source)
                , Mode(move.Mode)
                , Address(move.Address)
                , Mask(move.Mask)
                , Gateway(move.Gateway)
                , LeaseTime(move.LeaseTime)
                , RenewalTime(move.RenewalTime)
                , RebindingTime(move.RebindingTime)
                , Xid(move.Xid)
                , DNS(move.DNS)
                , TimeOut(move.TimeOut)
                , Retries(move.Retries)
                , _broadcast(move._broadcast)
            {
                Add(_T("interface"), &Interface);
                Add(_T("mac"), &MAC);
                Add(_T("source"), &Source);
                Add(_T("mode"), &Mode);
                Add(_T("address"), &Address);
                Add(_T("mask"), &Mask);
                Add(_T("gateway"), &Gateway);
                Add(_T("broadcast"), &_broadcast);
                Add(_T("leaseTime"), &LeaseTime);
                Add(_T("renewalTime"), &RenewalTime);
                Add(_T("rebindingTime"), &RebindingTime);
                Add(_T("xid"), &Xid);
                Add(_T("dns"), &DNS);
                Add(_T("timeout"), &TimeOut);
                Add(_T("retries"), &Retries);
            }
 
            Entry(const Entry& copy)
                : Core::JSON::Container()
                , Interface(copy.Interface)
                , MAC(copy.MAC)
                , Source(copy.Source)
                , Mode(copy.Mode)
                , Address(copy.Address)
                , Mask(copy.Mask)
                , Gateway(copy.Gateway)
                , LeaseTime(copy.LeaseTime)
                , RenewalTime(copy.RenewalTime)
                , RebindingTime(copy.RebindingTime)
                , Xid(copy.Xid)
                , DNS(copy.DNS)
                , TimeOut(copy.TimeOut)
                , Retries(copy.Retries)
                , _broadcast(copy._broadcast)
            {
                Add(_T("interface"), &Interface);
                Add(_T("mac"), &MAC);
                Add(_T("source"), &Source);
                Add(_T("mode"), &Mode);
                Add(_T("address"), &Address);
                Add(_T("mask"), &Mask);
                Add(_T("gateway"), &Gateway);
                Add(_T("broadcast"), &_broadcast);
                Add(_T("leaseTime"), &LeaseTime);
                Add(_T("renewalTime"), &RenewalTime);
                Add(_T("rebindingTime"), &RebindingTime);
                Add(_T("xid"), &Xid);
                Add(_T("dns"), &DNS);
                Add(_T("timeout"), &TimeOut);
                Add(_T("retries"), &Retries);
            }
            ~Entry() override = default;

        public:
            Core::JSON::String Interface;
            Core::JSON::String MAC;
            Core::JSON::String Source;
            Core::JSON::EnumType<Exchange::INetworkControl::ModeType> Mode;
            Core::JSON::String Address;
            Core::JSON::DecUInt8 Mask;
            Core::JSON::String Gateway;
            Core::JSON::DecUInt32 LeaseTime;
            Core::JSON::DecUInt32 RenewalTime;
            Core::JSON::DecUInt32 RebindingTime;
            Core::JSON::DecUInt32 Xid;
            Core::JSON::ArrayType<Core::JSON::String> DNS;
            Core::JSON::DecUInt8 TimeOut;
            Core::JSON::DecUInt8 Retries;

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
        class Settings {
        public:
            Settings& operator= (const Settings& rhs) = delete;

            Settings()
                : _xid(0)
                , _mode(Exchange::INetworkControl::ModeType::DYNAMIC)
                , _address()
                , _gateway()
                , _broadcast()
                , _dns()
            {
            }
            Settings(const Entry& info)
                : _xid(info.Xid.Value())
                , _mode(info.Mode.Value())
                , _address(Core::IPNode(Core::NodeId(info.Address.Value().c_str()), info.Mask.Value()))
                , _gateway(Core::NodeId(info.Gateway.Value().c_str()))
                , _broadcast(info.Broadcast())
                , _dns()
            {
            }
            Settings(const Settings& copy)
                : _xid(copy._xid)
                , _mode(copy._mode)
                , _address(copy._address)
                , _gateway(copy._gateway)
                , _broadcast(copy._broadcast)
                , _dns()
            {
            }
            ~Settings() = default;
       public:
            inline Exchange::INetworkControl::ModeType Mode() const
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
            inline uint32_t Xid() const
            {
                return (_xid);
            }
            bool Store(const Entry& info) {

                bool updated = false;

                if (_mode != info.Mode.Value()) {
                    _mode = info.Mode.Value();
                    updated = true;
                }
                Core::NodeId address(info.Address.Value().c_str());
                if ((address.IsValid() == true) && ((_address != address) || (_address.Mask() != info.Mask.Value()))) {
                    _address = Core::IPNode(address, info.Mask.Value());

                    updated = true;
                }
                Core::NodeId gateway(info.Gateway.Value().c_str());
                if ((gateway.IsValid() == true) && (_gateway != gateway)) {
                    _gateway = gateway;
                    updated = true;
                }
                if ((_broadcast.IsValid() == true) && (_broadcast != info.Broadcast())) {
                    _broadcast = info.Broadcast();
                    updated = true;

                }
                return updated;
            }
            bool Store(const DHCPClient::Offer& offer) {
                bool updated  = false;
                if (_xid != offer.Xid()) {
                    _xid = offer.Xid();
                }

                if ( (_address != offer.Address()) || (_address.Mask() != offer.Netmask()) ) {
                    updated = true;
                    _address = Core::IPNode(offer.Address(), offer.Netmask());
                }
                if (_gateway != offer.Gateway()) {
                    updated = true;
                    _gateway = offer.Gateway();
                }
                if (_broadcast != offer.Broadcast()) {
                    updated = true;
                    _broadcast = offer.Broadcast();
                }
                if (_dns.size() == offer.DNS().size()) {
                    for (auto& dns:offer.DNS()) {
                        if (std::find(_dns.begin(), _dns.end(), dns) == _dns.end()) {
                            updated = true;
                            _dns = offer.DNS();
                            break;
                        }
                    }
                } else {
                    updated = true;
                    _dns = offer.DNS();
                }

                return(updated);
            }

            void Clear() {
                _address = Core::IPNode();
                _gateway = Core::NodeId();
                _broadcast = Core::NodeId();
            }

        private:
            uint32_t _xid;
            Exchange::INetworkControl::ModeType _mode;
            Core::IPNode _address;
            Core::NodeId _gateway;
            Core::NodeId _broadcast;
            NameServers _dns;
        };

        class AdapterObserver : public Thunder::Core::AdapterObserver::INotification {
        public:
            AdapterObserver() = delete;
            AdapterObserver(AdapterObserver&&) = delete;
            AdapterObserver(const AdapterObserver&) = delete;
            AdapterObserver& operator=(AdapterObserver&&) = delete;
            AdapterObserver& operator=(const AdapterObserver&) = delete;

            PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            AdapterObserver(NetworkControlImplementation& parent)
                : _parent(parent)
                , _adminLock()
                , _observer(this)
                , _reporting()
                , _job(*this)
            {
            }
            POP_WARNING()
            ~AdapterObserver() override = default;

        public:
            void Open()
            {
                _observer.Open();
            }
            void Close()
            {
                _observer.Close();

                _adminLock.Lock();

                _reporting.clear();

                _adminLock.Unlock();

                _job.Revoke();
            }
 
            void Event(const string& interface) override
            {
                _adminLock.Lock();

                if (std::find(_reporting.begin(), _reporting.end(), interface) == _reporting.end()) {
                    // We need to add this interface, it is currently not present.
                    _reporting.push_back(interface);

                    _job.Submit();
                }

                _adminLock.Unlock();
            }
            void Dispatch()
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
            NetworkControlImplementation& _parent;
            Core::CriticalSection _adminLock;
            Core::AdapterObserver _observer;
            std::list<string> _reporting;
            Core::WorkerPool::JobType<AdapterObserver&> _job;
        };

        class Config : public Core::JSON::Container {
        public:
            Config(Config&&) = delete;
            Config(const Config&) = delete;
            Config& operator=(Config&&) = delete;
            Config& operator=(const Config&) = delete;

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
                Add(_T("required"), &Set);
            }
            ~Config() override = default;

        public:
            Core::JSON::String DNSFile;
            Core::JSON::ArrayType<Entry> Interfaces;
            Core::JSON::ArrayType<Core::JSON::String> DNS;
            Core::JSON::ArrayType<Core::JSON::String> Set;
            Core::JSON::DecUInt8 TimeOut;
            Core::JSON::DecUInt8 Retries;
            Core::JSON::Boolean Open;
        };

        class DHCPEngine : private DHCPClient::ICallback {
        private:
            static constexpr uint32_t AckWaitTimeout = 1000; // 1 second is a life time for a server to respond!

        public:
            DHCPEngine() = delete;
            DHCPEngine(DHCPEngine&&) = delete;
            DHCPEngine(const DHCPEngine&) = delete;
            DHCPEngine& operator=(DHCPEngine&&) = delete;
            DHCPEngine& operator=(const DHCPEngine&) = delete;

            PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            DHCPEngine(NetworkControlImplementation& parent, const string& interfaceName, const uint8_t waitTimeSeconds, const uint8_t maxRetries, const Entry& info)
                : _parent(parent)
                , _retries(0)
                , _maxRetries(maxRetries)
                , _handleTime(1000 * waitTimeSeconds)
                , _client(interfaceName, this)
                , _offers()
                , _job(*this)
                , _settings(info)
            {
                if ( (_settings.Address().IsValid() == true) && (info.Source.IsSet() == true) ) {
                    // We can start with an Request, i.s.o. an ack...?
                    Core::NodeId source(info.Source.Value().c_str());

                    if (source.IsValid() == true) {
                        // Extract the DNS that where associated with this DHCP..
                        NameServers dns;

                        Core::JSON::ArrayType<Core::JSON::String>::ConstIterator entries(info.DNS.Elements());

                        while (entries.Next() == true) {
                            Core::NodeId entry(entries.Current().Value().c_str());

                            if (entry.IsValid() == true) {
                                dns.push_back(entry);
                            }
                        }

                        _offers.emplace_back(source, static_cast<const Core::NodeId&>(_settings.Address()), _settings.Address().Mask(), _settings.Gateway(), _settings.Broadcast(), _settings.Xid(), std::move(dns));
                    }
                }
            }
            POP_WARNING()
           ~DHCPEngine() = default;

        public:
            inline uint32_t Discover(const Core::NodeId& preferred)
            {
                uint32_t result;
                _retries = 0;
                _job.Revoke();
                if ( (_offers.size() > 0) && (_offers.front().Address() == preferred) ) {
                    _job.Reschedule(Core::Time::Now().Add(AckWaitTimeout));
                    result = _client.Request(_offers.front());
                }
                else {
                    ClearLease();
                    _job.Reschedule(Core::Time::Now().Add(_handleTime));
                    result = _client.Discover();
                }

                return (result);
            }
            inline void UpdateMAC(const uint8_t buffer[], const uint8_t size)
            {
                _client.UpdateMAC(buffer, size);
            }
            void Dispatch()
            {
                Core::AdapterIterator hardware(_client.Interface());

                if ((hardware.IsValid() == false) || (hardware.IsRunning() == false)) {
                    // If the interface is nolonger available, no need to reschedule , just report Failed!
                    _client.Close();
                    _parent.Failed(_client.Interface());
                }
                else if (_client.HasActiveLease() == true) {
                    // See if the lease time is over...
                    if (_client.Expired() <= Core::Time::Now()) {
                        if (_retries++ >= _maxRetries){
                            // Tried extending the lease but we did not get a response. Rediscover
                            _retries = 0;
                            ClearLease();
                            _client.Discover();
                            _job.Reschedule(Core::Time::Now().Add(_handleTime));
                        }
                        else {
                            // Start refreshing the Lease..
                            _client.Request(_client.Lease());
                            _job.Reschedule(Core::Time::Now().Add(AckWaitTimeout));
                        }
                    }
                    else {
                        TRACE(Trace::Information, ("Installing the lease, Rechecking in %d seconds from now", _client.Lease().LeaseTime()));

                        // We are good to go report success!, if this is a different set..
                        if (_settings.Store(_client.Lease()) == true) {
                            _parent.Accepted(_client.Interface(), _client.Lease());
                            _client.Close();
                        }
                        _retries = 0;
                        _job.Reschedule(_client.Expired());
                    }
                }
                else if (_offers.size() == 0) {
                    // Looks like the Discovers did not discover anything, should we retry ?
                    if (_retries++ < _maxRetries) {
                        ClearLease();
                        _client.Discover();
                        _job.Reschedule(Core::Time::Now().Add(_handleTime));
                    }
                    else {
                        _client.Close();
                        _parent.Failed(_client.Interface());
                    }
                }
                else if ( (_client.Lease() == _offers.front()) && (_retries++ < _maxRetries) ) {
                    // Looks like the acknwledge did not get a reply, should we retry ?
                    _client.Request(_client.Lease());
                    _job.Reschedule(Core::Time::Now().Add(AckWaitTimeout));
                }
                else {
                    // Request retries expired or Request rejected
                    if (_retries >= _maxRetries) {
                        hardware.Delete(Core::IPNode(_offers.front().Address(), _offers.front().Netmask()));
                        _offers.pop_front();
                    }

                    if (_offers.size() == 0) {
                        // There is no valid offer, so request for new offer
                        _retries = 0;
                        _job.Reschedule(Core::Time::Now().Add(_handleTime));
                    }
                    else {
                        DHCPClient::Offer& node (_offers.front());
                        hardware.Add(Core::IPNode(node.Address(), node.Netmask()));

                        TRACE(Trace::Information, ("Requesting a lease, for [%s]", node.Address().HostAddress().c_str()));

                        // Seems we have some offers pending and we are not Active yet, request an ACK
                        _client.Request(node);
                        _retries = 0;
                        _job.Reschedule(Core::Time::Now().Add(AckWaitTimeout));
                    }
                }
            }
            const Settings& Info() const {
                return (_settings);
            }
            const NameServers& DNS() const {
                return (_client.Lease().DNS());
            }
            bool HasActiveLease() const {
                return (_client.HasActiveLease());
            }
            void Get(Entry& info) const
            {
                uint8_t mac[DHCPClient::MACSize];
               
                info.Mode = _settings.Mode();
                info.Interface = _client.Interface();
                if (_client.MAC(mac, sizeof(mac)) == DHCPClient::MACSize) {
                    string macString;
                    Core::ToHexString(mac, DHCPClient::MACSize, macString, ':');
                    info.MAC = macString;
                }

                if (_client.HasActiveLease() == true) {
                    if (_settings.Address().IsValid() == true) {
                        info.Address = _settings.Address().HostAddress();
                        info.Mask = _settings.Address().Mask();
                        if (_settings.Gateway().IsValid() == true) {
                            info.Gateway = _settings.Gateway().HostAddress();
                        }
                        if (_settings.Broadcast().IsValid() == true) {
                            info.Broadcast(_settings.Broadcast());
                        }
                    }
                    DHCPClient::Offer offer(_client.Lease());
                    info.Source = offer.Source().HostAddress();
                    info.Xid = _settings.Xid();
                    for (const Core::NodeId& value : offer.DNS()) {
                        Core::JSON::String& entry = info.DNS.Add();
                        entry = value.HostAddress();
                    }
                }
            }
            inline void ClearLease() {
                _offers.clear();
                _settings.Clear();
            }

        private:
            // Offered, Approved and Rejected all run on the communication thread, so be carefull !!
            void Offered(const DHCPClient::Offer& offer) override {
                _adminLock.Lock();
                bool reschedule = (_offers.size() == 0);
                _offers.emplace_back(offer);
                _adminLock.Unlock();

                TRACE(Trace::Information, ("Received an Offer from: %s for %s", offer.Source().HostAddress().c_str(), offer.Address().HostAddress().c_str()));
                if (reschedule == true) {
                    _job.Reschedule(Core::Time::Now());
                }
            }
            void Approved(const DHCPClient::Offer& offer) override {
                TRACE(Trace::Information, ("Acknowledged an Offer from: %s for %s", offer.Source().HostAddress().c_str(), offer.Address().HostAddress().c_str()));
                _job.Reschedule(Core::Time::Now());
            }
            void Rejected(const DHCPClient::Offer& offer) override {
                _retries = _maxRetries;
                TRACE(Trace::Information, ("Rejected an Offer from: %s for %s", offer.Source().HostAddress().c_str(), offer.Address().HostAddress().c_str()));
                _job.Reschedule(Core::Time::Now());
            }

        private:
            NetworkControlImplementation& _parent;
            Core::CriticalSection _adminLock;
            uint8_t _retries;
            uint8_t _maxRetries;
            uint32_t _handleTime;
            DHCPClient _client;
            std::list<DHCPClient::Offer> _offers;
            Core::WorkerPool::JobType<DHCPEngine&> _job;
            Settings _settings;
        };

        using Store = Core::JSON::ArrayType<Entry>;
        using NetworkConfigs = std::unordered_map<string, Entry>;
        using RequiredSets = std::vector<string>;
        using Networks = std::unordered_map<string, DHCPEngine>;
        using Notifications = std::vector<Exchange::INetworkControl::INotification*>;
        using StringList = RPC::IteratorType<Exchange::INetworkControl::IStringIterator>;
        using NetworkInfoIteratorImplementation = RPC::IteratorType<Exchange::INetworkControl::INetworkInfoIterator>;

    public:
        NetworkControlImplementation(NetworkControlImplementation&&) = delete;
        NetworkControlImplementation(const NetworkControlImplementation&) = delete;
        NetworkControlImplementation& operator= (NetworkControlImplementation&&) = delete;
        NetworkControlImplementation& operator= (const NetworkControlImplementation&) = delete;

        NetworkControlImplementation()
            : _adminLock()
            , _config()
            , _info()
            , _dnsFile()
            , _persistentStoragePath()
            , _responseTime(0)
            , _retries(0)
            , _dns()
            , _requiredSet()
            , _dhcpInterfaces()
            , _observer(*this)
            , _open(false)
            , _service(nullptr)
        {
        }
        ~NetworkControlImplementation() override
        {
            // Stop observing.
            _observer.Close();
            _dns.clear();
            _dhcpInterfaces.clear();
            if (_service) {
                _service->Release();
                _service = nullptr;
            }
        }

    public:
        uint32_t Register(Exchange::INetworkControl::INotification* notification) override
        {
            ASSERT(notification);

            _adminLock.Lock();
            auto item = std::find(_notifications.begin(), _notifications.end(), notification);
            ASSERT(item == _notifications.end());
            if (item == _notifications.end()) {
                notification->AddRef();
                _notifications.push_back(notification);
            }
            _adminLock.Unlock();

            return Core::ERROR_NONE;
        }

        uint32_t Unregister(Exchange::INetworkControl::INotification* notification) override
        {
            ASSERT(notification);

            _adminLock.Lock();
            auto item = std::find(_notifications.begin(), _notifications.end(), notification);
            ASSERT(item != _notifications.end());
            _notifications.erase(item);
            (*item)->Release();
            _adminLock.Unlock();

            return Core::ERROR_NONE;
        }

        uint32_t Configure(PluginHost::IShell* service) override
        {
            _config.FromString(service->ConfigLine());

            ASSERT(service != nullptr);
            _service = service;
            _service->AddRef();

            _responseTime = _config.TimeOut.Value();
            _retries = _config.Retries.Value();
            _dnsFile = _config.DNSFile.Value();

            // We will only "open" the DNS resolve file, so of ot does not exist yet, create an empty file.
            Core::File dnsFile(_dnsFile);
            if (dnsFile.Exists() == false) {
                if (dnsFile.Create() == false) {
                    SYSLOG(Logging::Startup, (_T("Could not create DNS configuration file [%s]"), _dnsFile.c_str()));
                } else {
                    dnsFile.Close();

                     Core::JSON::ArrayType<Core::JSON::String>::Iterator entries(_config.DNS.Elements());

                     while (entries.Next() == true) {
                        Core::NodeId entry(entries.Current().Value().c_str());

                        if (entry.IsValid() == true) {
                            _dns.push_back(entry);
                        }
                    }

                    RefreshDNS();
                }
            }

            // Try to create persistent storage folder
            _persistentStoragePath = _service->PersistentPath();
            if (_persistentStoragePath.empty() == false) {
                if (Core::Directory(_persistentStoragePath.c_str()).CreatePath() == false) {
                    _persistentStoragePath.clear();
                    SYSLOG(Logging::Startup, (_T("Failed to create storage for network info: %s"), _persistentStoragePath.c_str()));
                }
                _persistentStoragePath += _T("Data.json");
            }

            Core::JSON::ArrayType<Core::JSON::String>::Iterator entries2(_config.Set.Elements());
            while (entries2.Next() == true) {
                _requiredSet.push_back(entries2.Current().Value());
            }

            // Lets download leases for potential DHCP interfaces
            Load(_persistentStoragePath, _info);

            Core::JSON::ArrayType<Entry>::Iterator index(_config.Interfaces.Elements());

            // From now on we observer the states of the give interfaces.
            _observer.Open();

            // Load the configured interface with their settings..
            while (index.Next() == true) {
                if (index.Current().Interface.IsSet() == true) {
                    string interfaceName(index.Current().Interface.Value());

                    Core::AdapterIterator hardware(interfaceName);

                    if (hardware.IsValid() == false) {
                        SYSLOG(Logging::Startup, (_T("Interface [%s], not available"), interfaceName.c_str()));
                    } else {
                        DHCPEngine& engine = AddInterface(hardware, index.Current());
                        if (hardware.IsUp() == false) {
                            hardware.Up(true);
                        } else {
                            Reload(interfaceName, (engine.Info().Mode() == Exchange::INetworkControl::ModeType::DYNAMIC));
                        }
                    }
                }
            }

            // If we are oke for loading all interfaces, load any interface not yet configured.
            if (_config.Open.Value() == true) {
                // Find all adapters that are not listed.
                Core::AdapterIterator notListed;

                _open = true;

                while (notListed.Next() == true) {
                    const string interfaceName(notListed.Name());

                    if (_dhcpInterfaces.find(interfaceName) == _dhcpInterfaces.end()) {
                        _dhcpInterfaces.emplace(std::piecewise_construct,
                            std::forward_as_tuple(interfaceName),
                            std::forward_as_tuple(*this, interfaceName, _responseTime, _retries, Entry()));
                    }
                }
            }

            return Core::ERROR_NONE;
        }

        uint32_t Interfaces(Exchange::INetworkControl::IStringIterator*& interfaces /* @out */) const override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;
            std::list<string> interfaceList;
            Interfaces(interfaceList);

            if (interfaceList.empty() != true) {
                interfaces = Core::ServiceType<StringList>::Create<Exchange::INetworkControl::IStringIterator>(interfaceList);
                result = Core::ERROR_NONE;
            }
             
            return result;
        }

        uint32_t Status(const string& interface, StatusType& status) const override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (interface.empty() != true) {
                auto entry = _dhcpInterfaces.find(interface);
                if (entry != _dhcpInterfaces.end()) {
                    Core::AdapterIterator adapter(entry->first);
                    if (adapter.IsValid() == true) {
                        if (adapter.IsRunning() == true) {
                            status = Exchange::INetworkControl::AVAILABLE;
                        } else {
                            status = Exchange::INetworkControl::UNAVAILABLE;
                        }
                        result = Core::ERROR_NONE;
                    }
                }
            }

            return result;
        }

        uint32_t Up(const string& interface, bool& up) const override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (interface.empty() != true) {
                auto entry = _dhcpInterfaces.find(interface);
                if (entry != _dhcpInterfaces.end()) {
                    Core::AdapterIterator adapter(entry->first);
                    if (adapter.IsValid() == true) {
                        up = adapter.IsUp();
                        result = Core::ERROR_NONE;
                    }
                }
            }
            return result;
        }

        uint32_t Up(const string& interface, const bool up) override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            _adminLock.Lock();
            if (interface.empty() != true) {

                Networks::iterator entry(_dhcpInterfaces.find(interface));
                if (entry != _dhcpInterfaces.end()) {
                    Core::AdapterIterator adapter(entry->first);
                    if (adapter.IsValid() == true) {
                        adapter.Up(up);
                        result = Core::ERROR_NONE;
                    }
                }
            }

            _adminLock.Unlock();

            return result;
        }

        uint32_t Flush(const string& interface) override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            _adminLock.Lock();

            if (interface.empty() != true) {
                Networks::iterator entry(_dhcpInterfaces.find(interface));
                if (entry != _dhcpInterfaces.end()) {
                    if (entry->second.Info().Mode() == Exchange::INetworkControl::ModeType::STATIC) {
                        result = Reload(entry->first, false);
                    } else {
                        result = Reload(entry->first, true);
                    }
                }
            }

            _adminLock.Unlock();
            return result;
        }

        uint32_t Network(const string& interface, Exchange::INetworkControl::INetworkInfoIterator*& networks) const override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            _adminLock.Lock();
            std::list<Exchange::INetworkControl::NetworkInfo> networksInfo;
            if (interface.empty() != true) {
                const auto entry = _dhcpInterfaces.find(interface);
                if (entry != _dhcpInterfaces.end()) {
                   result = NetworkInfo(entry, networksInfo);
                }
            }

            if (networksInfo.empty() != true) {
                networks = Core::ServiceType<NetworkInfoIteratorImplementation>::Create<NetworkInfoIteratorImplementation>(networksInfo);
            }
            _adminLock.Unlock();

            return result;
        }

        uint32_t Network(const string& interface, Exchange::INetworkControl::INetworkInfoIterator* const& networks) override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            Exchange::INetworkControl::NetworkInfo networkInfo;
            if (interface.empty() != true) {
                if (networks->Next(networkInfo) == true) {
                    result = NetworkInfo(interface, networkInfo);
                }
            }

            return result;
        }

        uint32_t DNS(Exchange::INetworkControl::IStringIterator*& dns) const override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;
            NameServers servers;
            DNS(servers);

            std::list<string> dnsList;
            for (const Core::NodeId& entry : servers) {

                dnsList.push_back(entry.HostAddress());
            }

            if (servers.empty() != true) {
                dns = Core::ServiceType<StringList>::Create<Exchange::INetworkControl::IStringIterator>(dnsList);
                result = Core::ERROR_NONE;
            }

            return result;
        }

        uint32_t DNS(Exchange::INetworkControl::IStringIterator* const& dns) override
        {
            _adminLock.Lock();

            string address;
            while (dns->Next(address) == true) {

                Core::NodeId entry(address.c_str());

                if ((entry.IsValid() == true) &&
                    (std::find(_dns.begin(), _dns.end(), entry) == _dns.end())) {
                    _dns.push_back(entry);
                }
            }
            if (dns->Count() > 0) {
                RefreshDNS();
            }
            _adminLock.Unlock();

            return Core::ERROR_NONE;
        }

        BEGIN_INTERFACE_MAP(NetworkControlImplementation)
        INTERFACE_ENTRY(Exchange::INetworkControl)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

    private:
        DHCPEngine& AddInterface(Core::AdapterIterator& hardware, const Entry& entry)
        {
            string interfaceName(hardware.Name());
            NetworkConfigs::const_iterator more(_info.find(interfaceName));
            const Entry& info = (more != _info.end()? more->second : entry);

            if ( (entry.MAC.IsSet() == true) && ((!entry.MAC.IsNull()) || (info.MAC.IsSet() && (!info.MAC.IsNull()))) ) {
                uint8_t buffer[DHCPClient::MACSize];
                Core::FromHexString(entry.MAC.IsNull() ? info.MAC.Value() : entry.MAC.Value(), buffer, sizeof(buffer), ':');
                if (hardware.MACAddress(buffer) != Core::ERROR_NONE) {
                    SYSLOG(Logging::Startup, (_T("Interface [%s], could not set requested MAC address"), interfaceName.c_str()));
                }
            }

            auto element = _dhcpInterfaces.emplace(std::piecewise_construct,
                std::forward_as_tuple(interfaceName),
                std::forward_as_tuple(*this, interfaceName, _responseTime, _retries, info));

            return (element.first->second);
        }

        uint32_t Reload(const string& interfaceName, const bool dynamic)
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            Networks::iterator index(_dhcpInterfaces.find(interfaceName));

            if (index != _dhcpInterfaces.end()) {

                Core::AdapterIterator adapter(interfaceName);

                if ((adapter.IsValid() == false) || (adapter.IsRunning() == false) || (adapter.HasMAC() == false) ) {
                    SYSLOG(Logging::Notification, (_T("Adapter [%s] not available or in the wrong state."), interfaceName.c_str()));
                }
                else {
                    uint8_t mac[6];
                    adapter.MACAddress(mac, sizeof(mac));
                    index->second.UpdateMAC(mac, sizeof(mac));

                    if (index->second.Info().Address().IsValid() == true) {
                        result = SetIP(adapter, index->second.Info().Address(), index->second.Info().Gateway(), index->second.Info().Broadcast(), true);
                    }
                    else if (dynamic == false) {
                        SYSLOG(Logging::Notification, (_T("Invalid Static IP address: %s, for interface: %s"), index->second.Info().Address().HostAddress().c_str(), interfaceName.c_str()));
                    }
                    if (dynamic == true) {
                        index->second.Discover(index->second.Info().Address());
                        result = Core::ERROR_NONE;
                    }
                }
            }

            return (result);
        }

        void ClearIP(Core::AdapterIterator& adapter)
        {

            Core::IPV4AddressIterator checker4(adapter.IPV4Addresses());
            while (checker4.Next() == true) {
                adapter.Delete(checker4.Address());
            }

            Core::IPV6AddressIterator checker6(adapter.IPV6Addresses());
            while (checker6.Next() == true) {
                adapter.Delete(checker6.Address());
            }

            SubSystemValidation();
        }

        uint32_t SetIP(Core::AdapterIterator& adapter, const Core::IPNode& ipAddress, const Core::NodeId& gateway, const Core::NodeId& broadcast, bool clearOld)
        {
            if (adapter.IsValid() == true) {
                bool addIt = true;

                if (ipAddress.Type() == Core::NodeId::TYPE_IPV4) {
                    Core::IPV4AddressIterator checker(adapter.IPV4Addresses());
                    while ((checker.Next() == true) && ((addIt == true) || (clearOld == true))) {
                        if (checker.Address() == ipAddress) {
                            addIt = false;
                        }
                        else if (clearOld == true) {
                            adapter.Delete(checker.Address());
                        }
                        TRACE(Trace::Information, (_T("Validating IP: %s - %s [%s,%s]"), checker.Address().HostAddress().c_str(), ipAddress.HostAddress().c_str(), addIt ? _T("true") : _T("false"), clearOld ? _T("true") : _T("false")));
                    }
                } else if (ipAddress.Type() == Core::NodeId::TYPE_IPV6) {
                    Core::IPV6AddressIterator checker(adapter.IPV6Addresses());
                    while ((checker.Next() == true) && ((addIt == true) || (clearOld == true))) {
                        if (checker.Address() == ipAddress) {
                            addIt = false;
                        }
                        else if (clearOld == true) {
                            adapter.Delete(checker.Address());
                        }
                    }
                }

                if (addIt == true) {
                    TRACE(Trace::Information, (_T("Setting IP: %s"), ipAddress.HostAddress().c_str()));
                    adapter.Add(ipAddress);
                }
                if (broadcast.IsValid() == true) {
                    adapter.Broadcast(broadcast);
                }
                if (gateway.IsValid() == true) {
                    adapter.Gateway(Core::IPNode(Core::NodeId("0.0.0.0"), 0), gateway);
                }

                SubSystemValidation();

                string message(string("{ \"interface\": \"") + adapter.Name() + string("\", \"status\":0, \"ip\":\"" + ipAddress.HostAddress() + "\" }"));
                TRACE(Trace::Information, (_T("Interface: [%s] set IP: [%s]"), adapter.Name().c_str(), ipAddress.HostAddress().c_str()));

                NotifyNetworkUpdate(adapter.Name());
                _service->Notify(message);
            }

            return (Core::ERROR_NONE);
        }

        /* Saves leased offer to file */
        void Save(const string& filename)
        {
            if (filename.empty() == false) {
                Core::File leaseFile(filename);

                if (leaseFile.Create() == true) {
                    Store storage;

                    for (std::pair<const string, DHCPEngine>& entry : _dhcpInterfaces) {
                        Entry& result = storage.Add();
                        entry.second.Get(result);
                    }

                    if (storage.IElement::ToFile(leaseFile) == false) {
                        TRACE(Trace::Warning, ("Error occured while trying to save dhcp lease to file!"));
                    }

                    leaseFile.Close();
                } else {
                    TRACE(Trace::Warning, ("Failed to open leases file %s\n for saving", leaseFile.Name().c_str()));
                }
            }
        }

        /*
            Loads list of previously saved offers and adds it to unleased list
            for requesting in future.
        */
        bool Load(const string& filename, NetworkConfigs& info)
        {
            bool result = false;

            if (filename.empty() == false) {

                Core::File leaseFile(filename);

                if (leaseFile.Size() != 0 && leaseFile.Open(true) == true) {

                    Store storage;
                    Core::OptionalType<Core::JSON::Error> error;
                    if (storage.IElement::FromFile(leaseFile, error) == true) {

                        Core::JSON::ArrayType<Entry>::Iterator loop(storage.Elements());

                        while (loop.Next() == true) {
                            info.emplace(std::piecewise_construct,
                                std::forward_as_tuple(loop.Current().Interface.Value()),
                                std::forward_as_tuple(loop.Current()));
                        }
                    }

                    if (error.IsSet() == true) {
                        SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                    } else {
                        result = true;
                    }

                    leaseFile.Close();
                }
            }

            return result;
        }

        bool ExternallyAccessible(const Core::AdapterIterator& index)
        {

            ASSERT(index.IsValid());

            bool accessible = index.IsRunning();

            if (accessible == true) {
                Core::IPV4AddressIterator checker4(index.IPV4Addresses());

                accessible = false;
                while ((accessible == false) && (checker4.Next() == true)) {
                    accessible = (checker4.Address().IsLocalInterface() == false);
                }

                if (accessible == false) {
                    Core::IPV6AddressIterator checker6(index.IPV6Addresses());

                    while ((accessible == false) && (checker4.Next() == true)) {
                        accessible = (checker4.Address().IsLocalInterface() == false);
                    }
                }
            }

            return (accessible);
        }

        void SubSystemValidation()
        {

            uint16_t count = 0;
            bool fullSet = true;
            bool validIP = false;

            Core::AdapterIterator adapter;

            while (adapter.Next() == true) {
                const string name (adapter.Name());
                Networks::iterator index (_dhcpInterfaces.find(name));

                if (index != _dhcpInterfaces.end()) {
                    bool hasValidIP = ExternallyAccessible(adapter);
                    TRACE(Trace::Information, (_T("Interface [%s] has a public IP: [%s]"), name.c_str(), hasValidIP ? _T("true") : _T("false")));

                    if (std::find(_requiredSet.cbegin(), _requiredSet.cend(), name) != _requiredSet.cend()) {
                        count++;
                        fullSet &= hasValidIP;
                    }
                    validIP |= hasValidIP;
                }
            }

            ASSERT(count <= _requiredSet.size());

            fullSet &= ( (count == _requiredSet.size()) && validIP );

            PluginHost::ISubSystem* subSystem = _service->SubSystems();
            ASSERT(subSystem != nullptr);

            if (subSystem != nullptr) {
                if (subSystem->IsActive(PluginHost::ISubSystem::NETWORK) ^ fullSet) {
                    subSystem->Set(fullSet ? PluginHost::ISubSystem::NETWORK : PluginHost::ISubSystem::NOT_NETWORK, nullptr);
                }

                subSystem->Release();
            }
        }

        void Accepted(const string& interfaceName, const DHCPClient::Offer& offer)
        {
            Networks::iterator entry(_dhcpInterfaces.find(interfaceName));

            if (entry != _dhcpInterfaces.end()) {

                Core::AdapterIterator adapter(interfaceName);
                ASSERT(adapter.IsValid() == true);

                if (adapter.IsValid() == true) {

                    TRACE(Trace::Information, ("DHCP Request for interface %s accepted, %s offered IP!\n", interfaceName.c_str(), offer.Address().HostAddress().c_str()));

                    SetIP(adapter, Core::IPNode(offer.Address(), offer.Netmask()), offer.Gateway(), offer.Broadcast(), true);

                    RefreshDNS();

                    TRACE(Trace::Information, (_T("New IP Granted for %s:"), interfaceName.c_str()));
                    TRACE(Trace::Information, (_T("     Source:    %s"), offer.Source().HostAddress().c_str()));
                    TRACE(Trace::Information, (_T("     Address:   %s"), offer.Address().HostAddress().c_str()));
                    TRACE(Trace::Information, (_T("     Broadcast: %s"), offer.Broadcast().HostAddress().c_str()));
                    TRACE(Trace::Information, (_T("     Gateway:   %s"), offer.Gateway().HostAddress().c_str()));
                    TRACE(Trace::Information, (_T("     DNS:       %d"), offer.DNS().size()));
                    TRACE(Trace::Information, (_T("     Netmask:   %d"), offer.Netmask()));

                    Save(_persistentStoragePath);
                }
            } else {
                TRACE(Trace::Information, (_T("Request accepted for nonexisting network interface!")));
            }
        }

        void Failed(const string& interfaceName)
        {
            _adminLock.Lock();

            Networks::iterator index(_dhcpInterfaces.find(interfaceName));

            if (index != _dhcpInterfaces.end()) {

                string message(string("{ \"interface\": \"") + index->first + string("\", \"status\":11 }"));
                TRACE(Trace::Information, (_T("DHCP Request timed out on: %s"), index->first.c_str()));

                _service->Notify(message);
            }

            _adminLock.Unlock();
        }

        void RefreshDNS()
        {
            Core::DataElementFile file(_dnsFile, Core::File::SHAREABLE|Core::File::USER_READ|Core::File::USER_WRITE|Core::File::USER_EXECUTE|Core::File::GROUP_READ|Core::File::GROUP_WRITE, 0);

            if (file.IsValid() == false) {
                SYSLOG(Logging::Notification, (_T("DNS functionality could NOT be updated [%s]"), _dnsFile.c_str()));
            } else {
                uint32_t offset = static_cast<uint32_t>(file.Size());
		string startMarker((_T("#++SECTION: ")) + _service->Callsign() + '\n');
                string endMarker((_T("#--SECTION: ")) + _service->Callsign() + '\n');

                if (offset > 0) {
                    uint32_t start = static_cast<uint32_t>(file.Search(0, reinterpret_cast<const uint8_t*>(startMarker.c_str()), static_cast<uint32_t>(startMarker.length())));

                    if (start < file.Size()) {
                        // We found a start marker, see if we have an end marker.
                        uint32_t end = static_cast<uint32_t>(file.Search(start + startMarker.length(), reinterpret_cast<const uint8_t*>(endMarker.c_str()), static_cast<uint32_t>(endMarker.length())));

                        if (end < file.Size()) {
                            end += static_cast<uint32_t>(endMarker.length());
                            offset -= (end - start);

                            if (offset > start) {
                                // We found the beginning and the end of the section in this file, Wipe it.
                                // move everything after de marker, over the marker sections.
                                ::memcpy(&(file[start]), &file[end], static_cast<uint32_t>(file.Size()) - end);
                            }
                        }
                    }
                }

                NameServers servers;
                DNS(servers);

                for (const Core::NodeId& entry : servers) {
                    startMarker += string("nameserver ") + entry.HostAddress() + '\n';
                }

                startMarker += endMarker;
                file.Size(offset + startMarker.length());
                ::memcpy(&(file[offset]), startMarker.c_str(), startMarker.length());
                file.Sync();

                SYSLOG(Logging::Startup, (_T("DNS functionality updated [%s]"), _dnsFile.c_str()));
            }
        }

        void Activity(const string& interfaceName)
        {
            Core::AdapterIterator adapter(interfaceName);

            if (adapter.IsValid() == true) {

                Networks::iterator index(_dhcpInterfaces.find(interfaceName));

                // Send a message with the state of the adapter.
                string message = string(_T("{ \"interface\": \"")) + interfaceName + string(_T("\", \"running\": \"")) + string(adapter.IsRunning() ? _T("true") : _T("false")) + string(_T("\", \"up\": \"")) + string(adapter.IsUp() ? _T("true") : _T("false")) + _T("\", \"event\": \"");
                TRACE(Trace::Information, (_T("Adapter change report on: %s"), interfaceName.c_str()));

                _adminLock.Lock();

                if (index != _dhcpInterfaces.end()) {
                    message += _T("Update\" }");
                    TRACE(Trace::Information, (_T("Statechange on interface: [%s], running: [%s]"), interfaceName.c_str(), adapter.IsRunning() ? _T("true") : _T("false")));

                    if (adapter.IsRunning() == true) {

                        Exchange::INetworkControl::ModeType how(index->second.Info().Mode());
                        Reload(interfaceName, how == Exchange::INetworkControl::ModeType::DYNAMIC);

                    } else {
                        ClearIP(adapter);
                    }

                    _service->Notify(message);
                    NotifyNetworkUpdate(interfaceName);
                }
                else if (_open == true) {
                    message += _T("Create\" }");

                    TRACE(Trace::Information, (_T("Added interface: %s"), interfaceName.c_str()));

                    _service->Notify(message);
                    NotifyNetworkUpdate(interfaceName);

                    // The interface is first created, let's see if it's on the configuration list...
                    bool found = false;
                    Core::JSON::ArrayType<Entry>::Iterator listIndex(_config.Interfaces.Elements());
                    while (listIndex.Next() == true) {
                        if (listIndex.Current().Interface.IsSet() == true) {
                            string listedName(listIndex.Current().Interface.Value());
                            if (interfaceName == listedName) {
                                // It is, so bring it up if neccessary...
                                Core::AdapterIterator hardware(interfaceName);

                                if (hardware.IsValid() == true) {
                                    DHCPEngine& engine = AddInterface(hardware, listIndex.Current());

                                    if (hardware.IsUp() == false) {
                                        hardware.Up(true);
                                    } else {
                                        Reload(interfaceName, (engine.Info().Mode() == Exchange::INetworkControl::ModeType::DYNAMIC));
                                    }
                                }

                                found = true;
                                break;
                            }
                        }
                    }

                    if (found == false) {
                        _dhcpInterfaces.emplace(std::piecewise_construct,
                            std::forward_as_tuple(interfaceName),
                            std::forward_as_tuple(*this, interfaceName, _responseTime, _retries, Entry()));
                    }
                }

                _adminLock.Unlock();
            }
        }

        void Interfaces(std::list<string>& interfaces) const
        {
            for (const auto& info : _info) {
                 interfaces.push_back(info.first);
            }
        }

        uint32_t NetworkInfo(const Networks::const_iterator& engine, std::list<Exchange::INetworkControl::NetworkInfo>& networksInfo) const
        {
            Exchange::INetworkControl::NetworkInfo networkInfo;
            networkInfo.address = engine->second.Info().Address().HostAddress();
            networkInfo.defaultGateway = engine->second.Info().Gateway().HostAddress();
            networkInfo.mask = engine->second.Info().Address().Mask();
            networkInfo.mode = engine->second.Info().Mode();
            networksInfo.push_back(networkInfo);

            return Core::ERROR_NONE;
        }

        uint32_t NetworkInfo(const string& interface, const Exchange::INetworkControl::NetworkInfo network)
        {
            uint32_t result = Core::ERROR_NONE;

            Networks::const_iterator engine(_dhcpInterfaces.find(interface));
            _adminLock.Lock();
            if (engine != _dhcpInterfaces.end()) {
                _adminLock.Unlock();
                Entry entry;
                engine->second.Get(entry);
                if (entry.Mode.Value() != network.mode) {
                    entry.Mode = network.mode;
                }

                if (entry.Mode == Exchange::INetworkControl::ModeType::STATIC) {
                    entry.Address = network.address;
                    entry.Mask = network.mask;
                    entry.Gateway = network.defaultGateway;

                } else if (entry.Mode == Exchange::INetworkControl::ModeType::DYNAMIC) {
                    //It should be get from the DHCP server, so ignore the data or return error
                }

                if (const_cast<Settings&>(engine->second.Info()).Store(entry) == true) {
                    Reload(interface, (entry.Mode == Exchange::INetworkControl::ModeType::DYNAMIC));
                }

            } else {
                _adminLock.Unlock();
                result = Core::ERROR_UNAVAILABLE;
            }

            return result;
        }
 
        void DNS(NameServers& servers) const
        {
            _adminLock.Lock();

            servers = _dns;

            for (const std::pair<const string, DHCPEngine>& entry : _dhcpInterfaces) {

                if ((entry.second.Info().Mode() == Exchange::INetworkControl::ModeType::DYNAMIC) &&
                    (entry.second.HasActiveLease() == true)) {

                    for (const Core::NodeId& node : entry.second.DNS()) {
                        if (std::find(servers.begin(), servers.end(), node) == servers.end()) {

                            servers.push_back(node);
                        }
                    }
                }
            }

            _adminLock.Unlock();
        }

        void NotifyNetworkUpdate(const string& interface)
        {
            _adminLock.Lock();
            for (auto* notification : _notifications) {
                 notification->Update(interface);
            }
            _adminLock.Unlock();
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Config _config;
        NetworkConfigs _info;
        string _dnsFile;
        string _persistentStoragePath;
        uint8_t _responseTime;
        uint8_t _retries;
        NameServers _dns;
        RequiredSets _requiredSet;
        Networks _dhcpInterfaces;
        AdapterObserver _observer;
        bool _open;
        PluginHost::IShell* _service;
        Notifications _notifications;
    };

    SERVICE_REGISTRATION(NetworkControlImplementation, 1, 0)
} // namespace Plugin
} // namespace Thunder

