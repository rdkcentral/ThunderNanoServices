/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#ifndef PLUGIN_NETWORKCONTROL_H
#define PLUGIN_NETWORKCONTROL_H

#include "Module.h"
#include "DHCPClient.h"

#include <interfaces/IIPNetwork.h>
#include <interfaces/json/JsonData_NetworkControl.h> 

namespace WPEFramework {
namespace Plugin {

    class NetworkControl : public PluginHost::IPlugin,
                           public PluginHost::IWeb,
                           public PluginHost::JSONRPC {
    public:
        class Entry : public Core::JSON::Container {
        public:
            Entry& operator=(const Entry&) = delete;

            Entry()
                : Core::JSON::Container()
                , Interface()
                , Source()
                , Mode(JsonData::NetworkControl::NetworkData::ModeType::MANUAL)
                , Address()
                , Mask(32)
                , Gateway()
                , LeaseTime(0)
                , RenewalTime(0)
                , RebindingTime(0)
                , TimeOut(10)
                , Retries(3)
                , _broadcast()
            {
                Add(_T("interface"), &Interface);
                Add(_T("source"), &Source);
                Add(_T("mode"), &Mode);
                Add(_T("address"), &Address);
                Add(_T("mask"), &Mask);
                Add(_T("gateway"), &Gateway);
                Add(_T("broadcast"), &_broadcast);
                Add(_T("leaseTime"), &LeaseTime);
                Add(_T("renewalTime"), &RenewalTime);
                Add(_T("rebindingTime"), &RebindingTime);
                Add(_T("timeout"), &TimeOut);
                Add(_T("retries"), &Retries);
            }
            Entry(const Entry& copy)
                : Core::JSON::Container()
                , Interface(copy.Interface)
                , Source(copy.Source)
                , Mode(copy.Mode)
                , Address(copy.Address)
                , Mask(copy.Mask)
                , Gateway(copy.Gateway)
                , LeaseTime(copy.LeaseTime)
                , RenewalTime(copy.RenewalTime)
                , RebindingTime(copy.RebindingTime)
                , TimeOut(copy.TimeOut)
                , Retries(copy.Retries)
                , _broadcast(copy._broadcast)
            {
                Add(_T("interface"), &Interface);
                Add(_T("source"), &Source);
                Add(_T("mode"), &Mode);
                Add(_T("address"), &Address);
                Add(_T("mask"), &Mask);
                Add(_T("gateway"), &Gateway);
                Add(_T("broadcast"), &_broadcast);
                Add(_T("leaseTime"), &LeaseTime);
                Add(_T("renewalTime"), &RenewalTime);
                Add(_T("rebindingTime"), &RebindingTime);
                Add(_T("timeout"), &TimeOut);
                Add(_T("retries"), &Retries);
            }
            ~Entry() override = default;

        public:
            Core::JSON::String Interface;
            Core::JSON::String Source;
            Core::JSON::EnumType<JsonData::NetworkControl::NetworkData::ModeType> Mode;
            Core::JSON::String Address;
            Core::JSON::DecUInt8 Mask;
            Core::JSON::String Gateway;
            Core::JSON::DecUInt32 LeaseTime;
            Core::JSON::DecUInt32 RenewalTime;
            Core::JSON::DecUInt32 RebindingTime;
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
        using Store = Core::JSON::ArrayType<Entry>;

        class Settings {
        public:
            Settings& operator= (const Settings& rhs) = delete;

            Settings()
                : _mode(JsonData::NetworkControl::NetworkData::ModeType::MANUAL)
                , _address()
                , _gateway()
                , _broadcast()
            {
            }
            Settings(const Entry& info)
                : _mode(info.Mode.Value())
                , _address(Core::IPNode(Core::NodeId(info.Address.Value().c_str()), info.Mask.Value()))
                , _gateway(Core::NodeId(info.Gateway.Value().c_str()))
                , _broadcast(info.Broadcast())
            {
            }
            Settings(const Settings& copy)
                : _mode(copy._mode)
                , _address(copy._address)
                , _gateway(copy._gateway)
                , _broadcast(copy._broadcast)
            {
            }
            ~Settings() = default;

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
            bool Store(const DHCPClient::Offer& offer) {
                bool updated  = false;

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

                return(updated);
            }
            void Clear() {
                _address = Core::IPNode();
                _gateway = Core::NodeId();
                _broadcast = Core::NodeId();
            } 

        private:
            JsonData::NetworkControl::NetworkData::ModeType _mode;
            Core::IPNode _address;
            Core::NodeId _gateway;
            Core::NodeId _broadcast;
        };

        class AdapterObserver : public WPEFramework::Core::AdapterObserver::INotification {
        public:
            AdapterObserver() = delete;
            AdapterObserver(const AdapterObserver&) = delete;
            AdapterObserver& operator=(const AdapterObserver&) = delete;

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            AdapterObserver(NetworkControl& parent)
                : _parent(parent)
                , _adminLock()
                , _observer(this)
                , _reporting()
                , _job(*this)
            {
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
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
            virtual void Event(const string& interface) override
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
            NetworkControl& _parent;
            Core::CriticalSection _adminLock;
            Core::AdapterObserver _observer;
            std::list<string> _reporting;
            Core::WorkerPool::JobType<AdapterObserver&> _job;
        };

        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
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
            DHCPEngine(const DHCPEngine&) = delete;
            DHCPEngine& operator=(const DHCPEngine&) = delete;

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
             DHCPEngine(NetworkControl& parent, const string& interfaceName, const uint8_t waitTimeSeconds, const uint8_t maxRetries, const Entry& info)
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
                        _offers.emplace_back(source, static_cast<const Core::NodeId&>(_settings.Address()));
                    }
                }
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
           ~DHCPEngine() = default; 

        public:
            inline uint32_t Discover(const Core::NodeId& preferred)
            {
                uint32_t result;
                _retries = 0;
                _job.Revoke();
                if ( (_offers.size() > 0) && (_offers.front().Address() == preferred) ) {
                    _job.Schedule(Core::Time::Now().Add(AckWaitTimeout));
                    result = _client.Request(_offers.front());
                }
                else {
                    _offers.clear();
                    _job.Schedule(Core::Time::Now().Add(_handleTime));
                    result = _client.Discover(preferred);
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

                if (hardware.IsValid() == false) {
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
                            _client.Discover(Core::NodeId());
                            _job.Schedule(Core::Time::Now().Add(_handleTime));
                        }
                        else {
                            // Start refreshing the Lease..
                            _client.Request(_client.Lease());
                            _job.Schedule(Core::Time::Now().Add(AckWaitTimeout));
                        }
                    }
                    else {
                        TRACE(Trace::Information, ("Installing the lease, Rechecking in %d seconds from now", _client.Lease().LeaseTime()));

                        // We are good to go report success!, if this is a different set..
                        if (_settings.Store(_client.Lease()) == true) {
                            _parent.Accepted(_client.Interface(), _client.Lease());
                        }
                        _retries = 0;
                        _job.Schedule(_client.Expired());
                    }
                }
                else if (_offers.size() == 0) {
                    // Looks like the Discovers did not discover anything, should we retry ?
                    if (_retries++ < _maxRetries) {
                        _client.Discover(Core::NodeId());
                        _job.Schedule(Core::Time::Now().Add(_handleTime));
                    }
                    else {
                        _client.Close();
                        _parent.Failed(_client.Interface());
                    }
                }
                else if ( (_client.Lease() == _offers.front()) && (_retries++ < _maxRetries) ) {
                    // Looks like the acknwledge did not get a reply, should we retry ?
                    _client.Request(_client.Lease());
                    _job.Schedule(Core::Time::Now().Add(AckWaitTimeout));
                }
                else {
                    if (_client.Lease() == _offers.front()) {
                        hardware.Delete(Core::IPNode(_client.Lease().Address(), _client.Lease().Netmask()));
                        _offers.pop_front();
                    }

                    if (_offers.size() == 0) {
                        _client.Close();
                        _parent.Failed(_client.Interface());
                    }
                    else {
                        DHCPClient::Offer& node (_offers.front());
                        hardware.Add(Core::IPNode(node.Address(), node.Netmask()));

                        // Seems we have some offers pending and we are not Active yet, request an ACK
                        _client.Request(node);
                        _retries = 0;
                        _job.Schedule(Core::Time::Now().Add(AckWaitTimeout));
                    }
                }
            }
            const Settings& Info() const {
                return (_settings);
            }
            const std::list<Core::NodeId>& DNS() const {
                return (_client.Lease().DNS());
            }
            bool HasActiveLease() const {
                return (_client.HasActiveLease());
            }
            void Get(Entry& info) const
            {
                info.Mode = _settings.Mode();
                info.Interface = _client.Interface();
                info.Address = _settings.Address().HostAddress();
                info.Gateway = _settings.Gateway().HostAddress();
                info.Broadcast(_settings.Broadcast());
                if (_client.HasActiveLease() == true) {
                    info.Source = _client.Lease().Source().HostAddress();
                }
            }
            inline void ClearLease() {
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
            NetworkControl& _parent;
            Core::CriticalSection _adminLock;
            uint8_t _retries;
            uint8_t _maxRetries;
            uint32_t _handleTime;
            DHCPClient _client;
            std::list<DHCPClient::Offer> _offers;
            Core::WorkerPool::JobType<DHCPEngine&> _job;
            Settings _settings;
        };

    public:
        NetworkControl(const NetworkControl&) = delete;
        NetworkControl& operator=(const NetworkControl&) = delete;

        NetworkControl();
        ~NetworkControl() override;

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(NetworkControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
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

    private:
        void Save(const string& filename);
        bool Load(const string& filename, std::map<const string, const Entry>& info);

        uint32_t Reload(const string& interfaceName, const bool dynamic);
        uint32_t SetIP(Core::AdapterIterator& adapter, const Core::IPNode& ipAddress, const Core::NodeId& gateway, const Core::NodeId& broadcast, bool clearOld = false);
        void ClearIP(Core::AdapterIterator& adapter);

        void Accepted(const string& interfaceName, const DHCPClient::Offer& offer);
        void Failed(const string& interfaceName);
        void RefreshDNS();
        void Activity(const string& interface);
        void SubSystemValidation();

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

        inline void ClearLease(const string& interfaceName) {
            std::map<const string, DHCPEngine>::iterator index(_dhcpInterfaces.find(interfaceName));
            if (index != _dhcpInterfaces.end()) {
                index->second.ClearLease();
            }
        }

    private:
        Core::CriticalSection _adminLock;
        uint16_t _skipURL;
        PluginHost::IShell* _service;
        string _dnsFile;
        string _persistentStoragePath;
        uint8_t _responseTime;
        uint8_t _retries;
        std::list<Core::NodeId> _dns;
        std::list<string> _requiredSet;
        std::map<const string, DHCPEngine> _dhcpInterfaces;
        AdapterObserver _observer;
        bool _open;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // PLUGIN_NETWORKCONTROL_H
