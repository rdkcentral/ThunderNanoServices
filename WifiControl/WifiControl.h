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
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

        public:
            Sink(WifiControl& parent)
                : _parent(parent)
            {
            }
            virtual ~Sink()
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
        private:
            WifiDriver(const WifiDriver&) = delete;
            WifiDriver& operator=(const WifiDriver&) = delete;

        public:
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
            uint32_t Lauch(const string& connector, const string& interfaceName, const uint16_t waitTime)
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

        class WifiConnector
        {
        private:
            using Job = Core::WorkerPool::JobType<WifiConnector&>;
            using SsidList = std::list <std::pair<int32_t, const string> >;

            enum states
            {
                IDLE,
                SCANNING,
                PROCESSING,
                CONNECTING,
                RECONNECTING,
                AUTHENTICATION,
            };

        public:
            WifiConnector() = delete;
            WifiConnector(const WifiConnector&) = delete;
            WifiConnector& operator=(const WifiConnector&) = delete;

            WifiConnector(WifiControl* parent, const uint8_t scheduleInterval)
                : _adminLock()
                , _parent(*parent)
                , _job(*this)
                , _state(IDLE)
                , _prevState(IDLE)
                , _ssidList()
                , _schedInterval(scheduleInterval * 1000)
                , _exchange(string(_TXT("")))
                , _bssid(0)
            {
                ASSERT(parent != nullptr);
            }
            ~WifiConnector()
            {
            }
            void Dispatch()
            {
                uint32_t scheduleTime = ~0;

                _adminLock.Lock();

                if (_state == SCANNING) {

                    _parent._controller->Scan();
                    scheduleTime = _schedInterval;

                } else if (_state == PROCESSING) {

                    /* Arrange SSIDs in sorted order as per signal strength */
                    WPASupplicant::Network::Iterator list(
                            _parent._controller->Networks());
                    while (list.Next() == true) {
                        const WPASupplicant::Network& net = list.Current();
                        if (_parent._controller->Get(net.SSID()).IsValid()) {

                            int32_t strength(net.Signal());
                            if (net.SSID().compare(_parent.PreferredSSID()) == 0) {
                                strength = Core::NumberType<int32_t>::Max();
                            }
                            auto index(_ssidList.begin());
                            while ((index != _ssidList.end())
                                    && (index->first > strength)) {
                                index++;
                            }
                            _ssidList.emplace(index,
                                    std::pair<int32_t, string>(strength,
                                            net.SSID()));
                        }
                    }
                    _state = _prevState = CONNECTING;
                }
                if (_state >= CONNECTING) {

                    if (_prevState != _state) {
                        _prevState = _state;
                    } else {
                        // Move to next SSID
                        _state = CONNECTING;
                    }
                    if (_state == CONNECTING) {

                        if (_ssidList.empty() == false) {

                            _parent._controller->SelectNetwork(_exchange,
                                    _ssidList.front().second, _bssid);
                            _ssidList.pop_front();
                        } else {
                            TRACE(Trace::Warning, (_T("Tried all opportunities, retrying again")));
                            _state = SCANNING;
                        }
                    } else if (_state == RECONNECTING) {

                        _parent._controller->Reconnect(_exchange);
                    } else if (_state == AUTHENTICATION) {

                        _parent._controller->Authenticate(_exchange, _bssid);
                    }

                    scheduleTime = _schedInterval;
                }
                _adminLock.Unlock();

                if (scheduleTime != static_cast<uint32_t>(~0)) {
                    _job.Schedule(Core::Time::Now().Add(scheduleTime));
                }
            }
            void Scanned()
            {
                _adminLock.Lock();
                if (_state == SCANNING) {
                    _state = PROCESSING;
                    _adminLock.Unlock();

                    _job.Revoke();
                    _job.Submit();
                } else {
                    _adminLock.Unlock();
                }
            }
            void Connect()
            {
                if (_parent.PreferredSSID().empty() == false)
                {
                    _adminLock.Lock();
                    _state = SCANNING;
                    _adminLock.Unlock();

                    _job.Revoke();
                    _job.Submit();
                }
            }
            void Reset()
            {
                _adminLock.Lock();
                _state = IDLE;
                _adminLock.Unlock();

                _job.Revoke();
                _ssidList.clear();
            }
            void Disconnected()
            {
                _adminLock.Lock();
                if (_state >= CONNECTING) {
                    _adminLock.Unlock();

                    _job.Revoke();
                    _job.Submit();
                } else {
                    _adminLock.Unlock();
                    Reset();
                    Connect();
                }
            }
            void OKReceived()
            {
                bool stateChanged = true;

                _adminLock.Lock();

                if (_state == CONNECTING) {
                    if (_bssid != 0) {
                        _state = RECONNECTING;
                    } else {
                        stateChanged = false;
                    }
                } else if (_state == RECONNECTING) {
                    _state = AUTHENTICATION;
                } else if (_state == AUTHENTICATION) {
                    stateChanged = false;
                }

                _adminLock.Unlock();

                if (stateChanged == true) {
                    _job.Revoke();
                    _job.Submit();
                }
            }

        private:
            Core::CriticalSection _adminLock;
            WifiControl& _parent;
            Job _job;
            states _state;
            states _prevState;
            SsidList _ssidList;
            uint32_t _schedInterval;
            WPASupplicant::Controller::CustomRequest _exchange;
            uint64_t _bssid;
        };

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Connector(_T("/var/run/wpa_supplicant"))
                , Interface(_T("wlan0"))
                , Application(_T("/usr/sbin/wpa_supplicant"))
                , AutoConnect(false)
            {
                Add(_T("connector"), &Connector);
                Add(_T("interface"), &Interface);
                Add(_T("application"), &Application);
                Add(_T("autoconnect"), &AutoConnect);
            }
            virtual ~Config()
            {
            }

        public:
            Core::JSON::String Connector;
            Core::JSON::String Interface;
            Core::JSON::String Application;
            Core::JSON::Boolean AutoConnect;
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
                info.Type = JsonData::WifiControl::TypeType::WPA;
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

        static void UpdateConfig(WPASupplicant::Config& profile, const JsonData::WifiControl::ConfigInfo& settings)
        {

            if (settings.Hash.IsSet() == true) {
                // Seems we are in WPA mode !!!
                profile.Hash(settings.Hash.Value());
            } else if (settings.Psk.IsSet() == true) {
                // Seems we are in WPA mode !!!
                profile.PresharedKey(settings.Psk.Value());
            } else if ((settings.Identity.IsSet() == true) && (settings.Password.IsSet() == true)) {
                // Seems we are in Enterprise mode !!!
                profile.Enterprise(settings.Identity.Value(), settings.Password.Value());
            } else if ((settings.Identity.IsSet() == false) && (settings.Password.IsSet() == false)) {
                // Seems we are in UNSECURE mode !!!
                profile.Unsecure();
            }

            if (settings.Accesspoint.IsSet() == true) {
                profile.Mode(settings.Accesspoint.Value() ? 2 : 0);
            }
            if (settings.Hidden.IsSet() == true) {
                profile.Hidden(settings.Hidden.Value());
            }
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
        const string& PreferredSSID() const
        {
            return (_preferredSsid);
        }
        uint32_t Connect(const std::string& ssid)
        {
            if (ssid.compare(_controller->Current()) != 0) {

                _preferredSsid.assign(ssid);
                if (_controller->Current().empty() == true) {
                    _wifiConnector.Connect();
                } else {
                    _controller->Disconnect(_controller->Current());
                }
            }
            return Core::ERROR_NONE;
        }
        uint32_t Disconnect(const std::string& ssid)
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;
            if (ssid.compare(_controller->Current()) == 0) {

                _preferredSsid.clear();
                result = _controller->Disconnect(_controller->Current());
            }
            return result;
        }

        BEGIN_INTERFACE_MAP(WifiControl)
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
        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> PostMethod(Core::TextSegmentIterator& index, const Web::Request& request);
        Core::ProxyType<Web::Response> DeleteMethod(Core::TextSegmentIterator& index);

        void WifiEvent(const WPASupplicant::Controller::events& event);

    private:
        uint8_t _skipURL;
        PluginHost::IShell* _service;
        string _configurationStore;
        Sink _sink;
        WifiDriver _wpaSupplicant;
        Core::ProxyType<WPASupplicant::Controller> _controller;
        WifiConnector _wifiConnector;
        bool _autoConnect;
        std::string _preferredSsid;
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
    };

} // namespace Plugin
} // namespace WPEFramework

#endif // WIFICONTROL_H
