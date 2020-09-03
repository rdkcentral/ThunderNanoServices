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
#include "DHCPClientImplementation.h"

#include <interfaces/IIPNetwork.h>
#include <interfaces/json/JsonData_NetworkControl.h> 

namespace WPEFramework {
namespace Plugin {

    class NetworkControl : public Exchange::IIPNetwork,
                           public PluginHost::IPlugin,
                           public PluginHost::IWeb,
                           public PluginHost::JSONRPC {
    private:
        class Settings;

    public:
        class Entry : public Core::JSON::Container {
        public:
            Entry& operator=(const Entry&) = delete;
            Entry& operator=(const Settings&);

            Entry()
                : Core::JSON::Container()
                , Interface()
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

            Settings& operator= (const Settings& rhs) {
                _mode = rhs._mode;
                _address = rhs._address;
                _gateway = rhs._gateway;
                _broadcast = rhs._broadcast;

                return(*this);
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
            inline void Store(Entry& info) const
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

        class DHCPEngine : public DHCPClient::ICallback {
        public:
            DHCPEngine() = delete;
            DHCPEngine(const DHCPEngine&) = delete;
            DHCPEngine& operator=(const DHCPEngine&) = delete;

#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            DHCPEngine(NetworkControl& parent, const string& interfaceName, const uint8_t waitTimeSeconds, const uint8_t maxRetries)
                : _parent(parent)
                , _retries(0)
                , _maxRetries(maxRetries)
                , _handleTime(1000 * waitTimeSeconds)
                , _client(interfaceName, this)
                , _watchdog(*this)
                , _dns()
            {
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
            ~DHCPEngine() = default; 

        public:
            inline uint32_t Discover(const Core::NodeId& preferred)
            {
                SetWatchdog(_handleTime);
                uint32_t result = _client.Discover(preferred);
                _dns.Clear();

                return (result);
            }
            inline void UpdateMAC(const uint8_t buffer[], const uint8_t size) 
            {
                _client.UpdateMAC(buffer, size);
            }
            void Dispatch()
            {
                Retry();
            }
            const Settings& Info() const {
                return (_settings);
            }
            void Info(const Settings& info) {
                _settings = info;
            }

        private:
            void Approved(const DHCPClient::Offer& offer) override {
                // Seems we have a hit, report this to the parent..
                _watchdog.Revoke();
                _parent.Accepted(_client.Interface(), offer);
                DHCPClient::Offer::DNSIterator index(offer.DNS());
                while (index.Next()) {
                    _dns.emplace_back(index.Current());
                }
            }

            void SetWatchdog (const uint32_t delayMs) {
                _watchdog.Revoke();
                _watchdog.Schedule(Core::Time::Now().Add(delayMs));
            }

            void Retry() {
                if (_retries++ < _maxRetries) {
                    _client.Retry();
                }
                else {
                    // Report failure..
                    _parent.Failed(_client.Interface());
                }
            }

        private:
            NetworkControl& _parent;
            uint8_t _retries;
            uint8_t _maxRetries;
            uint32_t _handleTime;
            DHCPClient _client;
            Core::WorkerPool::JobType<DHCPEngine&> _watchdog;
            Settings _settings;
            std::list<Core::NodeId> _dns;
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
        void Save(const string& filename);
        bool Load(const string& filename, std::map<const string, const Settings>& info);

        uint32_t Reload(const string& interfaceName, const bool dynamic);
        uint32_t SetIP(Core::AdapterIterator& adapter, const Core::IPNode& ipAddress, const Core::NodeId& gateway, const Core::NodeId& broadcast, bool clearOld = false);
        void Accepted(const string& interfaceName, const DHCPClient::Offer& offer);
        void Failed(const string& interfaceName);
        void RefreshDNS();
        void Activity(const string& interface);
        uint16_t DeleteSection(Core::DataElementFile& file, const string& startMarker, const string& endMarker);
        inline void ClearAssignedIPs(Core::AdapterIterator& adapter)
        {
            ClearAssignedIPV4IPs(adapter);
            ClearAssignedIPV6IPs(adapter);
        }

        void ClearAssignedIPV4IPs(Core::AdapterIterator& adapter);
        void ClearAssignedIPV6IPs(Core::AdapterIterator& adapter);

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
        string _dnsFile;
        string _persistentStoragePath;
        uint8_t _responseTime;
        uint8_t _retries;
        std::list<Core::NodeId> _dns;
        std::map<const string, DHCPEngine> _dhcpInterfaces;
        AdapterObserver _observer;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // PLUGIN_NETWORKCONTROL_H
