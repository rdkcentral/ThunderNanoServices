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

#ifndef WIFICONTROL_H
#define WIFICONTROL_H

#include "Module.h"
#include "Network.h"
#ifdef USE_WIFI_HAL
#include "WifiHAL.h"
#else
#include "Controller.h"
#endif
#include <interfaces/json/JsonData_WifiControl.h>

namespace WPEFramework {
namespace Plugin {

    class WifiControl : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    private:
        class Sink : public Core::IDispatchType<const WPASupplicant::Controller::events> {
        public:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

            Sink(WifiControl& parent)
                : _parent(parent)
            {
            }
            ~Sink() override
            {
            }

        private:
            virtual void Dispatch(const WPASupplicant::Controller::events& event) override
            {
                _parent.WifiEvent(event);
            }

        private:
            WifiControl& _parent;
        };
        class WifiDriver {
        public:
            WifiDriver(const WifiDriver&) = delete;
            WifiDriver& operator=(const WifiDriver&) = delete;

            WifiDriver()
                : _interfaceName()
                , _connector()
                , _process(true)
            {
            }
            ~WifiDriver()
            {
            }

        public:
            uint32_t Launch(const string& connector, const string& interfaceName, const uint16_t waitTime)
            {
                _interfaceName = interfaceName;
                _connector = connector;

                Core::Process::Options options(_T("/usr/sbin/wpa_supplicant"));
                /* interface name *mandatory */
                options.Add(_T("-i")+ _interfaceName);

                /* ctrl_interface parameter *mandatory */
                options.Add(_T("-C") + _connector);

                /* driver name (can be multiple drivers: nl80211,wext) *optional */
                options.Add(_T("-Dnl80211"));

                /* global ctrl_interface group */
                options.Add(_T("-G0"));

#ifdef __DEBUG__
                options.Add(_T("-dd"));
                options.Add(_T("-f/tmp/wpasupplicant.log"));
#endif

                TRACE(Trace::Information, (_T("Launching %s."), options.Command().c_str()));
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
            inline void Terminate()
            {
                _process.Kill(false);
                _process.WaitProcessCompleted(1000);
            }

        private:
            string _interfaceName;
            string _connector;
            Core::Process _process;
            uint32_t _pid;
        };
        class AutoConnect : public WPASupplicant::Controller::IConnectCallback
        {
        private:
            class AccessPoint 
            {
            public:
                AccessPoint() = delete;
                AccessPoint(const AccessPoint&) = delete;
                AccessPoint& operator= (const AccessPoint&) = delete;

                AccessPoint(const int32_t strength, const uint64_t& bssid, const string& SSID) 
                    : _strength(strength)
                    , _bssid(bssid)
                    , _ssid(SSID) {
                }
                ~AccessPoint() {
                }

            public:
                const string& SSID() const {
                    return(_ssid);
                }
                const uint64_t& BSSID() const {
                    return(_bssid);
                }
                int32_t Signal() const {
                    return (_strength);
                }

            private:
                int32_t _strength;
                uint64_t _bssid;
                string _ssid;
            };
            using Job = Core::WorkerPool::JobType<AutoConnect&>;
            using SSIDList = std::list<AccessPoint>;

            enum class states : uint8_t
            {
                IDLE,
                SCANNING,
                SCANNED,
                CONNECTING,
                RETRY
            };

        public:
            AutoConnect() = delete;
            AutoConnect(const AutoConnect&) = delete;
            AutoConnect& operator=(const AutoConnect&) = delete;

            AutoConnect(Core::ProxyType<WPASupplicant::Controller>& controller)
                : _adminLock()
                , _controller(controller)
                , _job(*this)
                , _state(states::IDLE)
                , _ssidList()
                , _interval(0)
                , _attempts(0)
                , _preferred()
            {
            }
            ~AutoConnect() override
            {
            }

        public:
            uint32_t Connect(const string& SSID, const uint8_t scheduleInterval, const uint32_t attempts) 
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                ASSERT (_controller.IsValid());

                _adminLock.Lock();

                if (_state == states::IDLE) {

                    _preferred = SSID;
                    _attempts = attempts;
                    _interval = (scheduleInterval * 1000);
                    result = Scan();
                }

                _adminLock.Unlock();

                return (result);
            }
            void SetPreferred(const string& ssid, const uint8_t scheduleInterval, const uint32_t attempts)
            {
                _adminLock.Lock();

                _preferred.assign(ssid);
                _attempts = attempts;
                _interval = (scheduleInterval * 1000);

                _adminLock.Unlock();
            }
            inline uint32_t Scan()
            {
                MoveState (states::SCANNING);

                _controller->Scan();
                _job.Schedule(Core::Time::Now().Add(_interval));

                return Core::ERROR_NONE;
            }
            uint32_t Revoke()
            {
                _adminLock.Lock();

                MoveState(states::IDLE);

                _attempts = 0;
                _interval = 0;
                _controller->Revoke(this);

                _adminLock.Unlock();

                return(Core::ERROR_INPROGRESS);
            }
            void Scanned()
            {
                _adminLock.Lock();

                if ( (_state == states::SCANNING) || (_state == states::RETRY) ) {

                    MoveState(states::SCANNED);

                    _job.Submit();
               }

                _adminLock.Unlock();
            }
            void Disconnected ()
            {
                _adminLock.Lock();
                WPASupplicant::Controller::reasons reason = _controller->DisconnectReason();

                if ((reason == WPASupplicant::Controller::WLAN_REASON_NOINFO_GIVEN)
                        && (_state == states::IDLE) && (_attempts > 0)) {
                    Scan();
                }
                _adminLock.Unlock();
            }

            void Dispatch()
            {
                _adminLock.Lock();

                // Oke, the Job timed out, or we need a new CONNECTION request....
                if (_state == states::SCANNING) {
                    // Seems that the Scan did not complete in time. Lets reschedule for later...
                    _state = states::RETRY;
                    _job.Schedule(Core::Time::Now().Add(_interval));
                }
                else if (_state == states::SCANNED) {

                    // This is a transitional state due to the fact that the  call to
                    // _controller->Get(net.SSID()) takes a long time, it should not 
                    // be executed on an independend worker thread.

                    _ssidList.clear();

                    /* Arrange SSIDs in sorted order as per signal strength */
                    WPASupplicant::Network::Iterator list(_controller->Networks());
                    while (list.Next() == true) {
                        const WPASupplicant::Network& net = list.Current();
                        if (_controller->Get(net.SSID()).IsValid()) {

                            int32_t strength(net.Signal());
                            if (net.SSID().compare(_preferred) == 0) {
                                strength = Core::NumberType<int32_t>::Max();
                            }

                            SSIDList::iterator index(_ssidList.begin());
                            while ((index != _ssidList.end()) && (index->Signal() > strength)) {
                                index++;
                            }
                            _ssidList.emplace(index, strength, net.BSSID(), net.SSID());
                        }
                    }

                    if (_ssidList.size() == 0) {
                        _state = states::RETRY;
                    }
                    else {
                        _state = states::CONNECTING;
                        _controller->Connect(this, _ssidList.front().SSID(), _ssidList.front().BSSID());
                    }

                    _job.Schedule(Core::Time::Now().Add(_interval));
                }
                else if (_state == states::CONNECTING) {
                    // If we sre still in the CONNECTING mode, it must mean that previous CONNECT Failed
                    if (_ssidList.size() > 0) {
                        _ssidList.pop_front();
                    }

                    _controller->Revoke(this);
                    if (_ssidList.size() == 0) {
                        _state = states::RETRY;
                    }
                    else {
                        _controller->Connect(this, _ssidList.front().SSID(), _ssidList.front().BSSID());
                    }
                    _job.Schedule(Core::Time::Now().Add(_interval));
                }
                else if (_state == states::RETRY) {
                    if (_attempts == 0) {
                        _state = states::IDLE;
                    }
                    else {
                        if (_attempts != static_cast<uint32_t>(~0)) {
                            --_attempts;
                        }
                        _state = states::SCANNING;
                        _controller->Scan();

                        _job.Schedule(Core::Time::Now().Add(_interval));
                    }
                }

                _adminLock.Unlock();
            }

            void UpdateStatus(const uint32_t result) {

                if ((result != Core::ERROR_NONE) && (result != Core::ERROR_ALREADY_CONNECTED)) {
                    _adminLock.Lock();

                    _state = states::CONNECTING;
                    _adminLock.Unlock();

                    _job.Submit();
                }
            }

        private:
            void MoveState(const states newState) {
                _state = states::IDLE;

                _adminLock.Unlock();

                _job.Revoke();

                _adminLock.Lock();

                _state = newState;
            }

            void Completed(const uint32_t result) override {

                _controller->Revoke(this);

                _adminLock.Lock();

                if ((result == Core::ERROR_NONE) || (result == Core::ERROR_ALREADY_CONNECTED)) {

                    MoveState(states::IDLE);

                    _ssidList.clear();
                }
                else {
                    MoveState(states::CONNECTING);

                    _job.Submit();
                }
                _adminLock.Unlock();
            }


        private:
            Core::CriticalSection _adminLock;
            Core::ProxyType<WPASupplicant::Controller>& _controller;
            Job _job;
            states _state;
            SSIDList _ssidList;
            uint32_t _interval;
            uint32_t _attempts;
            string _preferred;
        };

    public:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Connector(_T("/var/run/wpa_supplicant"))
                , Interface(_T("wlan0"))
                , Application(_T("/usr/sbin/wpa_supplicant"))
                , Preferred()
                , AutoConnect(false)
                , RetryInterval(30)
            {
                Add(_T("connector"), &Connector);
                Add(_T("interface"), &Interface);
                Add(_T("application"), &Application);
                Add(_T("preferred"), &Preferred);
                Add(_T("autoconnect"), &AutoConnect);
                Add(_T("retryinterval"), &RetryInterval);
            }
            virtual ~Config()
            {
            }

        public:
            Core::JSON::String Connector;
            Core::JSON::String Interface;
            Core::JSON::String Application;
            Core::JSON::String Preferred;
            Core::JSON::Boolean AutoConnect;
            Core::JSON::DecUInt8 RetryInterval;
        };

        static void FillNetworkInfo(const WPASupplicant::Network& info, JsonData::WifiControl::NetworkInfo& net)
        {
            net.Bssid = std::to_string(info.BSSID());
            net.Frequency = info.Frequency();
            net.Signal = info.Signal();
            net.Ssid = info.SSID();

            uint16_t bit = 0x1;
            uint16_t pairs = info.Pair();
            while (pairs != 0) {
                if ((bit & pairs) != 0) {
                    WPASupplicant::Network::pair method(static_cast<WPASupplicant::Network::pair>(bit));

                    JsonData::WifiControl::PairsInfo pairing;
                    WifiControl::FillPairing(method, info.Key(method), pairing);
                    net.Pairs.Add(pairing);
                    pairs &= ~bit;
                }
                bit = (bit << 1);
            }
        }

        static void FillPairing(const enum WPASupplicant::Network::pair method, const uint8_t keys, JsonData::WifiControl::PairsInfo& pairing)
        {
            pairing.Method = string(Core::EnumerateType<WPASupplicant::Network::pair>(method).Data());

            uint8_t bit = 0x1;
            uint8_t value = keys;
            while (value != 0) {
                if ((bit & value) != 0) {
                    Core::JSON::String textKey;
                    textKey = string(Core::EnumerateType<WPASupplicant::Network::key>(static_cast<WPASupplicant::Network::key>(bit)).Data());
                    pairing.Keys.Add(textKey);
                    value &= ~bit;
                }
                bit = (bit << 1);
            }
        }

        static void FillConfig(const WPASupplicant::Config& element, JsonData::WifiControl::ConfigInfo& info)
        {
            info.Ssid = element.SSID();

            if (element.IsUnsecure() == true) {
                info.Type = JsonData::WifiControl::TypeType::UNSECURE;
            } else if (element.IsWPA() == true) {
                info.Type = GetWPAProtocolType(element.Protocols());

                if (element.Hash().empty() == false) {
                    info.Hash = element.Hash();
                } else {
                    info.Psk = element.PresharedKey();
                }
            } else if (element.IsEnterprise() == true) {
                info.Type = JsonData::WifiControl::TypeType::ENTERPRISE;
                info.Identity = element.Identity();
                info.Password = element.Password();
            }

            info.Accesspoint = element.IsAccessPoint();
            info.Hidden = element.IsHidden();
        }

        static bool UpdateConfig(WPASupplicant::Config& profile, const JsonData::WifiControl::ConfigInfo& settings)
        {
            bool status = true;

            if (settings.Hash.IsSet() == true || settings.Psk.IsSet() == true) {

                // Seems we are in WPA mode !!!
                uint8_t protocolFlags = GetWPAProtocolFlags(settings, true);
                status = profile.Protocols(protocolFlags);

                if (status == true) {
                    if (settings.Hash.IsSet() == true) {
                        status = profile.Hash(settings.Hash.Value());
                        TRACE_GLOBAL(Trace::Information, (_T("Setting Hash %s is %s"), settings.Hash.Value().c_str(), (status ? "Success": "Failed")));
                    } else {
                        status = profile.PresharedKey(settings.Psk.Value());
                        TRACE_GLOBAL(Trace::Information, (_T("Setting PresharedKey %s is %s"), settings.Psk.Value().c_str(), (status ? "Success": "Failed")));
                    }
                }

            } else if ((settings.Identity.IsSet() == true) && (settings.Password.IsSet() == true)) {
                // Seems we are in Enterprise mode !!!
                status = profile.Enterprise(settings.Identity.Value(), settings.Password.Value());
                TRACE_GLOBAL(Trace::Information, (_T("Setting Enterprise values %s:%s is %s"),settings.Identity.Value().c_str(), settings.Password.Value().c_str(), (status ? "Success": "Failed")));
            } else if ((settings.Identity.IsSet() == false) && (settings.Password.IsSet() == false)) {
                // Seems we are in UNSECURE mode !!!
                status = profile.Unsecure();
            }

            if ((status == true) && (settings.Accesspoint.IsSet() == true)) {
                status = profile.Mode(settings.Accesspoint.Value() ? 2 : 0);
            }
            if ((status == true) && (settings.Hidden.IsSet() == true)) {
                status = profile.Hidden(settings.Hidden.Value());
            }
            return status;
        }


        class NetworkList : public Core::JSON::Container {
        public:
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
            void Set(WPASupplicant::Network::Iterator& list)
            {
                list.Reset();
                while (list.Next() == true) {
                    JsonData::WifiControl::NetworkInfo net;
                    WifiControl::FillNetworkInfo(list.Current(), net);
                    Networks.Add(net);
                }
            }

            Core::JSON::ArrayType<JsonData::WifiControl::NetworkInfo> Networks;
        };

        class ConfigList : public Core::JSON::Container {
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
            void Set(WPASupplicant::Config::Iterator& list)
            {
                while (list.Next() == true) {
                    JsonData::WifiControl::ConfigInfo conf;
                    WifiControl::FillConfig(list.Current(), conf);
                    Configs.Add(conf);
                }
            }

            Core::JSON::ArrayType<JsonData::WifiControl::ConfigInfo> Configs;
        };

    private:
        WifiControl(const WifiControl&) = delete;
        WifiControl& operator=(const WifiControl&) = delete;

    public:
        WifiControl();

        virtual ~WifiControl()
        {
            UnregisterAll();
        }

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
        // Helper methods
        static uint8_t GetWPAProtocolFlags(const JsonData::WifiControl::ConfigInfo& settings, const bool safeFallback);
        static JsonData::WifiControl::TypeType GetWPAProtocolType(const uint8_t protocolFlags);
    
    private:
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index);

        void WifiEvent(const WPASupplicant::Controller::events& event);
        void RegisterAll();
        void UnregisterAll();
        uint32_t endpoint_delete(const JsonData::WifiControl::DeleteParamsInfo& params);
        uint32_t endpoint_store();
        uint32_t endpoint_scan();
        uint32_t endpoint_connect(const JsonData::WifiControl::DeleteParamsInfo& params);
        uint32_t endpoint_disconnect(const JsonData::WifiControl::DeleteParamsInfo& params);
        uint32_t get_status(JsonData::WifiControl::StatusData& response) const;
        uint32_t get_networks(Core::JSON::ArrayType<JsonData::WifiControl::NetworkInfo>& response) const;
        uint32_t get_configs(Core::JSON::ArrayType<JsonData::WifiControl::ConfigInfo>& response) const;
        uint32_t get_config(const string& index, JsonData::WifiControl::ConfigInfo& response) const;
        uint32_t set_config(const string& index, const JsonData::WifiControl::ConfigInfo& param);
        uint32_t set_debug(const Core::JSON::DecUInt32& param);
        void event_scanresults(const Core::JSON::ArrayType<JsonData::WifiControl::NetworkInfo>& list);
        void event_networkchange();
        void event_connectionchange(const string& ssid);

        inline uint32_t Connect(const string& ssid) {

            if (_autoConnectEnabled == true) {
                _autoConnect.Revoke();
            }

            uint32_t result = _controller->Connect(ssid);

            if ((result != Core::ERROR_INPROGRESS) && (_autoConnectEnabled == true)) {
                _autoConnect.SetPreferred(ssid, _retryInterval, ~0);
                _autoConnect.UpdateStatus(result);
            }

            return result;
        }
        inline uint32_t Disconnect(const string& ssid) {

            if (_autoConnectEnabled == true) {
                _autoConnect.Revoke();
            }

            return _controller->Disconnect(ssid);
        }
        inline uint32_t Store() {
            uint32_t result = Core::ERROR_NONE;

            Core::File configFile(_configurationStore);
            WPASupplicant::Config::Iterator list(_controller->Configs());
            WifiControl::ConfigList configs;
            configs.Set(list);
            if (configs.Configs.IsSet()) {
                if (configFile.Create() == true) {
                    if (configs.IElement::ToFile(configFile) == true) {
                    } else {
                        TRACE(Trace::Information, (_T("Failed to write config settings to %s"), _configurationStore.c_str()));
                        result = Core::ERROR_WRITE_ERROR;
                    }
                } else {
                    TRACE(Trace::Information, (_T("Failed to create config file %s"), _configurationStore.c_str()));
                    result = Core::ERROR_WRITE_ERROR;
                }
            } else {
                if (configFile.Exists() == true) {
                    configFile.Destroy();
                }
            }
            return result;
        }

        BEGIN_INTERFACE_MAP(WifiControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    private:
        uint8_t _skipURL;
        uint8_t _retryInterval;
        PluginHost::IShell* _service;
        string _configurationStore;
        Sink _sink;
        WifiDriver _wpaSupplicant;
        Core::ProxyType<WPASupplicant::Controller> _controller;
        AutoConnect _autoConnect;
        bool _autoConnectEnabled;
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // WIFICONTROL_H
