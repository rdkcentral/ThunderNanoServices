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
#include "Network.h"

#ifdef USE_WIFI_HAL
#include "WifiHAL.h"
#else
#include "Controller.h"
#endif

#include <interfaces/IConfiguration.h>
#include <interfaces/IWifiControl.h>
#include <interfaces/json/JWifiControl.h>

namespace Thunder {

namespace Plugin
{
    class WifiControlImplementation : public Exchange::IWifiControl,
                                      public Exchange::IConfiguration {
    private:
        using ConfigMap = std::map<string, ConfigInfo>;
        using SecurityMap = std::map<string, std::list<SecurityInfo>>;
        using NetworkInfoIteratorImplementation = RPC::IteratorType<INetworkInfoIterator>;
        using SecurityInfoIteratorImplementation = RPC::IteratorType<ISecurityIterator>;
        using StringIteratorImplementation = RPC::IteratorType<IStringIterator>;

    private:
        class Sink : public Core::IDispatchType<const WPASupplicant::Controller::events> {
        public:
            Sink() = delete;
            Sink(Sink&&) = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

            Sink(WifiControlImplementation& parent)
                : _parent(parent)
            {
            }
            ~Sink() override = default;

        private:
            void Dispatch(const WPASupplicant::Controller::events& event) override
            {
                _parent.WifiEvent(event);
            }

        private:
            WifiControlImplementation& _parent;
        };

        class WifiDriver {
        public:
            WifiDriver(WifiDriver&&) = delete;
            WifiDriver(const WifiDriver&) = delete;
            WifiDriver& operator=(const WifiDriver&) = delete;

            WifiDriver()
                : _interfaceName()
                , _connector()
                , _process(true)
            {
            }
            ~WifiDriver() = default;

        public:
            uint32_t Launch(const string& application, const string& connector, const string& interfaceName, const uint16_t waitTime, const string& logFile)
            {
                _interfaceName = interfaceName;
                _connector = connector;

                Core::Process::Options options(application);

                /* interface name *mandatory */
                options.Add(_T("-i")+ _interfaceName);
                /* ctrl_interface parameter *mandatory */
                options.Add(_T("-C") + _connector);

                /* driver name (can be multiple drivers: nl80211,wext) *optional */
                options.Add(_T("-Dnl80211"));

                /* global ctrl_interface group */
                options.Add(_T("-G0"));

                if (!logFile.empty()) {
                    options.Add(_T("-dd"));
                    options.Add(_T("-f") + logFile);
                }

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
                AccessPoint(AccessPoint&&) = delete;
                AccessPoint(const AccessPoint&) = delete;
                AccessPoint& operator= (const AccessPoint&) = delete;

                AccessPoint(const int32_t strength, const uint64_t& bssid, const string& SSID)
                    : _strength(strength)
                    , _bssid(bssid)
                    , _ssid(SSID) {
                }
                ~AccessPoint() = default;

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
            AutoConnect(AutoConnect&&) = delete;
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
            ~AutoConnect() override = default;

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
                _job.Reschedule(Core::Time::Now().Add(_interval));

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

                if ((_state == states::SCANNING) || (_state == states::RETRY)) {

                    MoveState(states::SCANNED);

                    _job.Submit();
                }

                _adminLock.Unlock();
            }
            void Disconnected ()
            {
                _adminLock.Lock();
                WPASupplicant::Controller::reasons reason = _controller->DisconnectReason();

                if (((reason == WPASupplicant::Controller::WLAN_REASON_NOINFO_GIVEN) ||
                     (reason == WPASupplicant::Controller::WLAN_REASON_DEAUTH_LEAVING)) &&
                    (_state == states::IDLE) && (_attempts > 0)) {
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
                    _job.Reschedule(Core::Time::Now().Add(_interval));
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

                    _job.Reschedule(Core::Time::Now().Add(_interval));
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
                    _job.Reschedule(Core::Time::Now().Add(_interval));
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

                        _job.Reschedule(Core::Time::Now().Add(_interval));
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
                else if (result == Core::ERROR_UNKNOWN_KEY) {
                    // Remove network with SSID causing this error
                    // It has wrong WPA so we can't do anything here - proper error will be returned to user
                    // Do not try to connect to this network in next iteration
                     _ssidList.pop_front();
                    _job.Submit();
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

        class WPSConnect {
        private:
            enum class states : uint8_t
            {
                IDLE,
                REQUESTED,
                SUCCESS
            };

         using Job = Core::WorkerPool::JobType<WPSConnect&>;

         public:
            WPSConnect() = delete;
            WPSConnect(WPSConnect&&) = delete;
            WPSConnect(const WPSConnect&) = delete;
            WPSConnect& operator=(const WPSConnect&) = delete;

            WPSConnect(WifiControlImplementation& parent, Core::ProxyType<WPASupplicant::Controller>& controller)
                : _adminLock()
                , _parent(parent)
                , _controller(controller)
                , _job(*this)
                , _state(states::IDLE)
                , _ssid()
            {
            }
            virtual ~WPSConnect() = default;

            uint32_t Invoke(const string& ssid, const SecurityInfo::Key key, const string& pin, const uint32_t walkTime)
            {
                uint32_t result = Core::ERROR_UNKNOWN_KEY;

                _adminLock.Lock();

                if (key == SecurityInfo::Key::PBC) {
                    result = _controller->StartWpsPbc(ssid);
                } else if (key == SecurityInfo::Key::PIN) {
                    result = _controller->StartWpsPin(ssid, pin);
                }

                if (result == Core::ERROR_NONE) {
                    _ssid = ssid;
                    _state = states::REQUESTED;
                    _job.Reschedule(Core::Time::Now().Add(walkTime * 1000));
                }
                _adminLock.Unlock();
                return result;
            }

            uint32_t Revoke()
            {
                uint32_t result = Core::ERROR_NONE;
                _adminLock.Lock();
                if (_state != states::IDLE) {
                    result = _controller->CancelWps();
                    Reset();
                }
                _adminLock.Unlock();
                return result;
            }
            void Completed(const uint32_t result)
            {
                _adminLock.Lock();
                if (_state == states::REQUESTED) {
                    if (result != Core::ERROR_NONE) {
                         _job.Revoke();
                        Reset();
                    }
                    else {
                        _state = states::SUCCESS;
                    }
                }
                _adminLock.Unlock();
            }

            void Disconnected()
            {
                _adminLock.Lock();
                if (_state == states::SUCCESS) {
                    _job.Revoke();
                    _job.Submit();
                }
                _adminLock.Unlock();
            }

            bool Active() const
            {
                bool result = false;
                _adminLock.Lock();
                if (_state != states::IDLE) {
                    result = true;
                }
                _adminLock.Unlock();
                return result;
            }

            bool Credentials() const
            {
                bool result = false;
                _adminLock.Lock();
                if (_state == states::SUCCESS) {
                    result = true;
                }
                _adminLock.Unlock();
                return result;
            }

            void Dispatch()
            {
                _adminLock.Lock();
                uint32_t result = Core::ERROR_NONE;

                if (_state == states::SUCCESS) {
                    //Lets get the Credentials
                    WPASupplicant::Network::wpsauthtypes auth;
                    string networkKey;
                    string ssid;
                    result = _controller->GetWpsCredentials(ssid,networkKey,auth);
                    if ((result == Core::ERROR_NONE) && (networkKey.empty()==false)) {
                        //Create a new ConfigInfo
                        ConfigInfo configInfo;
                        configInfo.hidden = false;
                        configInfo.ssid = ssid;


                        if (auth == WPASupplicant::Network::wpsauthtypes::WPS_AUTH_OPEN) {
                            configInfo.method = Security::OPEN;
                        } else {
                            uint8_t protocolFlags = 0;

                            switch (auth) {
                               case WPASupplicant::Network::wpsauthtypes::WPS_AUTH_WPAPSK:
                                  protocolFlags = WPASupplicant::Config::WPA;
                               break;
                               case WPASupplicant::Network::wpsauthtypes::WPS_AUTH_WPA2PSK:
                                  protocolFlags = WPASupplicant::Config::WPA2;
                               break;

                               default:
                                  // Handle other Protocol types?
                                  TRACE_GLOBAL(Trace::Information, (_T("Unknown WPA protocol type. ")));
                            }

                            configInfo.method = _parent.GetWPAProtocolType(protocolFlags);
                            configInfo.secret = networkKey;
                        }

                        _controller->CancelWps();
                        Reset();
                        _adminLock.Unlock();

                        WPASupplicant::Config profile(_controller->Create(ssid));
                        if (_parent.UpdateConfig(ssid, configInfo) != true) {
                            result = Core::ERROR_GENERAL;
                           _controller->Destroy(ssid);
                        } else {
                            result = _parent.Connect(ssid);
                        }
                    }

                    if ((result != Core::ERROR_NONE) && (result != Core::ERROR_INPROGRESS)) {
                        _parent.NotifyConnectionChange(string());
                    }
                }
                else if (_state == states::REQUESTED) {
                    //The WPS Operation didnot complete in time.
                    _controller->CancelWps();
                    Reset();
                    _adminLock.Unlock();
                    _parent.NotifyConnectionChange(string());
                }
                else{
                    _adminLock.Unlock();
                }
            }

         private:

         void Reset()
         {
             _state = states::IDLE;
             _ssid.clear();
         }

         private:
           mutable Core::CriticalSection _adminLock;
           WifiControlImplementation& _parent;
           Core::ProxyType<WPASupplicant::Controller>& _controller;
           Job _job;
           states _state;
           string _ssid;
        };

    public:
        class Setting : public Core::JSON::Container {
        public:
            Setting(Setting&&) = delete;
            Setting(const Setting&) = delete;
            Setting& operator=(const Setting&) = delete;

            Setting()
                : ConnectorDirectory(_T("wpa_supplicant"))
                , Interface(_T("wlan0"))
                , Application(_T("/usr/sbin/wpa_supplicant"))
                , AutoConnect(false)
                , RetryInterval(30)
                , MaxRetries(-1)
                , WaitTime(15)
                , LogFile()
                , WpsWalkTime(125)
                , WpsDisabled(false)
            {
                Add(_T("connector"), &ConnectorDirectory);
                Add(_T("interface"), &Interface);
                Add(_T("application"), &Application);
                Add(_T("autoconnect"), &AutoConnect);
                Add(_T("retryinterval"), &RetryInterval);
                Add(_T("maxretries"), &MaxRetries);
                Add(_T("waittime"), &WaitTime);
                Add(_T("logfile"), &LogFile);
                Add(_T("wpswalktime"), &WpsWalkTime);
                Add(_T("wpsdisabled"), &WpsDisabled);
            }
            ~Setting() override = default;

        public:
            Core::JSON::String ConnectorDirectory;
            Core::JSON::String Interface;
            Core::JSON::String Application;
            Core::JSON::Boolean AutoConnect;
            Core::JSON::DecUInt8 RetryInterval;
            Core::JSON::DecSInt32 MaxRetries;
            Core::JSON::DecUInt8 WaitTime;
            Core::JSON::String LogFile;
            Core::JSON::DecSInt32 WpsWalkTime;
            Core::JSON::Boolean WpsDisabled;
        };
        class ConfigData : public JsonData::WifiControl::ConfigInfoInfo {
        public:
            ConfigData()
                : JsonData::WifiControl::ConfigInfoInfo()
            {
            }
            ConfigData(const ConfigData& copy)
            : JsonData::WifiControl::ConfigInfoInfo()
            {
                Hidden = copy.Hidden;
                Accesspoint = copy.Accesspoint;
                Ssid = copy.Ssid;
                Secret = copy.Secret;
                Identity = copy.Identity;
                Method = copy.Method;
            }
            ConfigData& operator=(const ConfigInfo& rhs)
            {
                Hidden = rhs.hidden;
                Accesspoint = rhs.accesspoint;
                Ssid = rhs.ssid;
                Secret = rhs.secret;
                Identity = rhs.identity;
                Method = rhs.method;

                return (*this);
            }
        };
        class SSIDConfigs : public Core::JSON::Container {
        public:
            SSIDConfigs(SSIDConfigs&&) = delete;
            SSIDConfigs(const SSIDConfigs&) = delete;
            SSIDConfigs& operator=(const SSIDConfigs&) = delete;

            SSIDConfigs()
                : Core::JSON::Container()
                , Preferred()
                , Configs()
            {
                Add(_T("preferred"), &Preferred);
                Add(_T("configs"), &Configs);
            }

            ~SSIDConfigs() override = default;

        public:
            void Set(WPASupplicant::Config::Iterator& list)
            {
                while (list.Next() == true) {
                    ConfigData conf;
                    WifiControlImplementation::FillConfig(list.Current(), conf);
                    Configs.Add(conf);
                }
            }

            Core::JSON::String Preferred;
            Core::JSON::ArrayType<ConfigData> Configs;
        };

        static void FillNetworkInfo(const WPASupplicant::Network& info, NetworkInfo& net, std::list<SecurityInfo>& securities)
        {
            net.bssid = info.BSSID();
            net.frequency = info.Frequency();
            net.signal = info.Signal();
            net.ssid = info.SSID();
            net.security = Security::UNKNOWN;

            uint16_t bit = 0x1;
            uint16_t pairs = info.Pair();
            while (pairs != 0) {
                if ((bit & pairs) != 0) {
                    WPASupplicant::Network::pair method(static_cast<WPASupplicant::Network::pair>(bit));
                    Security security = GetSecurityMethod(method);

                    SecurityInfo securityInfo;
                    FillSecurityInfo(security, info.Key(method), securityInfo);
                    net.security = static_cast<Security>(static_cast<int>(net.security | security));
                    pairs &= ~bit;
                    securities.push_back(securityInfo);
                }
                bit = (bit << 1);
            }
        }

        static void FillSecurityInfo(Security method, const uint8_t keys, SecurityInfo& securityInfo)
        {
            securityInfo.keys = SecurityInfo::Key::NONE;
            securityInfo.method = method;

            uint8_t bit = 0x1;
            uint8_t value = keys;
            while (value != 0) {
                if ((bit & value) != 0) {
                    securityInfo.keys = static_cast<SecurityInfo::Key>(securityInfo.keys |
                                                    GetSecurityKey(static_cast<WPASupplicant::Network::key>(bit)));
                    value &= ~bit;
                }
                bit = (bit << 1);
            }
        }

        static void FillConfig(const WPASupplicant::Config& element, ConfigData& info)
        {
            info.Ssid = element.SSID();

            if (element.IsUnsecure() == true) {
                info.Method = Security::OPEN;
            } else if (element.IsWPA() == true) {
                info.Method = GetWPAProtocolType(element.Protocols());

                if (element.Hash().empty() == false) {
                    info.Secret = element.Hash();
                } else {
                    info.Secret = element.PresharedKey();
                }
            } else if (element.IsEnterprise() == true) {
                info.Method = Security::ENTERPRISE;
                info.Identity = element.Identity();
                info.Secret = element.Password();
            }

            info.Accesspoint = element.IsAccessPoint();
            info.Hidden = element.IsHidden();
        }

        inline void UpdateConfigList(const string& ssid, const ConfigInfo& configInfo)
        {
            _adminLock.Lock();
            ConfigMap::iterator index(_configs.find(ssid));
            if (index != _configs.end()) {
                if (configInfo.ssid.empty() == true) {
                    _controller->Destroy(ssid);
                    _configs.erase(index);
                } else {
                    index->second = configInfo;
                }
            } else {
                _configs.insert(std::pair<string, ConfigInfo>(ssid, configInfo));
            }
            _adminLock.Unlock();
        }

        inline bool UpdateConfig(const string& ssid, const ConfigInfo& settings)
        {
            bool status = true;
            if (ssid.empty() != true) {
                WPASupplicant::Config profile(_controller->Create(ssid));

                if (((settings.method & Security::WPA) != 0) || ((settings.method & Security::WPA2) != 0) ||
                    ((settings.method & Security::WPA_WPA2) != 0)) {
                    // Seems we are in WPA mode !!!
                    uint8_t protocolFlags = GetWPAProtocolFlags(settings, true);
                    status = profile.Protocols(protocolFlags);

                    if (status == true) {
                        if ((settings.key & SecurityInfo::Key::PSK_HASHED) != 0) {
                            status = profile.Hash(settings.secret);
                            TRACE_GLOBAL(Trace::Information, (_T("Setting Hash %s is %s"), settings.secret.c_str(), (status ? "Success": "Failed")));
                        } else {
                            status = profile.PresharedKey(settings.secret);
                            TRACE_GLOBAL(Trace::Information, (_T("Setting PresharedKey %s is %s"), settings.secret.c_str(), (status ? "Success": "Failed")));
                        }
                    }

                } else if ((settings.method & Security::ENTERPRISE) != 0) {
                    // Seems we are in Enterprise mode !!!
                    status = profile.Enterprise(settings.identity, settings.secret);
                    TRACE_GLOBAL(Trace::Information, (_T("Setting Enterprise values %s:%s is %s"),settings.identity.c_str(), settings.secret.c_str(), (status ? "Success": "Failed")));
                } else if ((settings.method & Security::OPEN) != 0) {
                    // Seems we are in OPEN mode !!!
                    status = profile.Unsecure();
                }

                if ((status == true) && (settings.accesspoint == true)) {
                    status = profile.Mode(settings.accesspoint ? 2 : 0);
                }
                if ((status == true) && (settings.hidden == true)) {
                    status = profile.Hidden(settings.hidden);
                }
            }

            UpdateConfigList(ssid, settings);
            return status;
        }

    public:
        WifiControlImplementation(WifiControlImplementation&&) = delete;
        WifiControlImplementation(const WifiControlImplementation&) = delete;
        WifiControlImplementation& operator=(const WifiControlImplementation&) = delete;

        WifiControlImplementation()
            : _retryInterval(0)
            , _walkTime(0)
            , _maxRetries(Core::NumberType<uint32_t>::Max())
            , _configurationStore()
            , _preferredSsid()
            , _sink(*this)
            , _wpaSupplicant()
            , _controller()
            , _autoConnect(_controller)
            , _autoConnectEnabled(false)
            , _wpsConnect(*this,_controller)
            , _wpsDisabled(false)
            , _networkChange(false)
            , _adminLock()
            , _job(*this)
        {
        }
        ~WifiControlImplementation() override
        {
#ifndef USE_WIFI_HAL

            if (_controller.IsValid()) {
                _autoConnect.Revoke();
                _wpsConnect.Revoke();
                _controller->Callback(nullptr);
                _controller->Terminate();
                _controller.Release();
            }
            _wpaSupplicant.Terminate();
#endif
            _job.Revoke();
            _ssidList.clear();
        }

    public:
        uint32_t Register(Exchange::IWifiControl::INotification* notification) override
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

        uint32_t Unregister(Exchange::IWifiControl::INotification* notification) override
        {
            ASSERT(notification);
            _adminLock.Lock();
            auto item = std::find(_notifications.begin(), _notifications.end(), notification);
            ASSERT(item != _notifications.end());
            if (item != _notifications.end()) {
                Exchange::IWifiControl::INotification* entry = *item;
                ASSERT(entry != nullptr);
                _notifications.erase(item);
                entry->Release();
            }
            _adminLock.Unlock();

            return Core::ERROR_NONE;
        }

        uint32_t Configure(PluginHost::IShell* service) override
        {
            ASSERT(service != nullptr);
            ASSERT(_controller.IsValid() == false);

            uint32_t result = Core::ERROR_GENERAL;

#ifdef USE_WIFI_HAL
            _controller = WPASupplicant::WifiHAL::Create();
            if ((_controller.IsValid() == true) && (_controller->IsOperational() == true)) {
                _controller->Scan();
                result = Core::ERROR_NONE;
            }
#else
            Setting config;
            config.FromString(service->ConfigLine());

            if (PrepareWPASupplicant(service, config) == Core::ERROR_NONE) {

                _controller = WPASupplicant::Controller::Create(service->VolatilePath() + config.ConnectorDirectory.Value(), config.Interface.Value(), 10);

                ASSERT(_controller.IsValid() == true);

                #ifndef __STUBBED__
                if (_controller->IsOperational() == false) {
                    SYSLOG(Logging::Error, (_T("Could not establish a link with WPA_SUPPLICANT")));
                    _controller.Release();
                } 
                else 
                #endif
                {
                    // The initialize prepared our path...
                    _configurationStore = service->PersistentPath() + "wpa_supplicant.conf";

                    _controller->Callback(&_sink);
                    Core::File configFile(_configurationStore);

                    if (configFile.Open(true) == true) {
                        SSIDConfigs configs;
                        Core::OptionalType<Core::JSON::Error> error;
                        configs.IElement::FromFile(configFile, error);
                        if (error.IsSet() == true) {
                            SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                        }

                        _preferredSsid = configs.Preferred.Value();
                        // iterator over the list and write back
                        Core::JSON::ArrayType<ConfigData>::Iterator configIterator(configs.Configs.Elements());

                        while (configIterator.Next()) {
                            ASSERT(configIterator.Current().Ssid.Value().empty() == false);

                            UpdateConfig(configIterator.Current().Ssid.Value(), configIterator.Current());
                        }
                    }

                    _walkTime = config.WpsWalkTime.Value();
                    _wpsDisabled = config.WpsDisabled.Value();

                    if (config.AutoConnect.Value() == false) {
                        #ifndef __STUBBED__
                        _controller->Scan();
                        #endif
                    }
                    else {
                        _autoConnectEnabled = true;
                        _retryInterval = config.RetryInterval.Value();
                        _maxRetries = config.MaxRetries.Value() == -1 ? 
                                        Core::NumberType<uint32_t>::Max() : 
                                        config.MaxRetries.Value();
                        #ifndef __STUBBED__
                        _autoConnect.Connect(_preferredSsid, _retryInterval, _maxRetries);
                        #endif  
                    }
                    result = Core::ERROR_NONE;
                }
            }
#endif
            TRACE(Trace::Information, (_T("Config path = %s"), _configurationStore.c_str()));

            // On success return empty, to indicate there is no error text.
            return (result);
        }

        uint32_t Networks(INetworkInfoIterator*& networkInfoList) const override
        {
            _adminLock.Lock();
            networkInfoList = Core::ServiceType<NetworkInfoIteratorImplementation>::Create<INetworkInfoIterator>(_networks);
            _adminLock.Unlock();
            return (networkInfoList != nullptr ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }

        uint32_t Securities(const string& SSID, ISecurityIterator*& securityMethods) const override
        {
            const string& ssid = SSIDDecode(SSID);
            _adminLock.Lock();
            SecurityMap::const_iterator index(_securities.find(ssid));
            if (index != _securities.end()) {
                securityMethods = Core::ServiceType<SecurityInfoIteratorImplementation>::Create<ISecurityIterator>(index->second);
            }
            _adminLock.Unlock();
            return (securityMethods != nullptr ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }

        uint32_t Configs(IStringIterator*& configs) const override
        {
            std::list<string> configList;

            _adminLock.Lock();
            std::transform(_configs.begin(), _configs.end(), std::back_inserter(configList),
            [](decltype(_configs)::value_type const &pair) {
                return pair.first;
            });
            _adminLock.Unlock();

            configs = Core::ServiceType<StringIteratorImplementation>::Create<IStringIterator>(configList);
            return (configs != nullptr ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }

        uint32_t Config(const string& ssid, ConfigInfo& configInfo) const override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;
            _adminLock.Lock();
            ConfigMap::const_iterator index(_configs.find(SSIDDecode(ssid)));
            if (index != _configs.end()) {
                configInfo = index->second;
                result = Core::ERROR_NONE;
            }
            _adminLock.Unlock();
            return result;
        }

        uint32_t Config(const string& ssid, const ConfigInfo& configInfo) override
        {
            UpdateConfig(SSIDDecode(ssid), configInfo);
            StoreUpdatedSsidConfig();

            return Core::ERROR_NONE;;
        }

        uint32_t Scan() override
        {
            return _controller->Scan();
        }

        uint32_t AbortScan() override
        {
            return _controller->AbortScan();
        }

        uint32_t Connect(const string& SSID) override
        {
            if (_autoConnectEnabled == true) {
                _autoConnect.Revoke();
            }

            uint32_t result;

            if(_wpsConnect.Active()){
                _wpsConnect.Revoke();
            }

            const string& ssid = SSIDDecode(SSID);
            result = _controller->Connect(ssid);
            if (result != Core::ERROR_INPROGRESS) {
                _adminLock.Lock();
                if (_preferredSsid != ssid) {
                    _preferredSsid = ssid;
                    StoreUpdatedSsidConfig();
                }
                if (_autoConnectEnabled == true) {
                    _autoConnect.SetPreferred(result == Core::ERROR_UNKNOWN_KEY ?
                                                       _T("") : _preferredSsid, _retryInterval, _maxRetries);
                    _autoConnect.UpdateStatus(result);
                }
                _adminLock.Unlock();
            }
            return result;
        }

        uint32_t Disconnect(const string& SSID) override
        {

            if (_autoConnectEnabled == true) {
                _autoConnect.Revoke();
            }

            const string& ssid = SSIDDecode(SSID);
            return _controller->Disconnect(ssid);
        }

        uint32_t Status(string& connectedSsid, bool& isScanning) const override
        {
            connectedSsid = _controller->Current();
            isScanning = _controller->IsScanning();
            return Core::ERROR_NONE;
        }

        BEGIN_INTERFACE_MAP(WifiControlImplementation)
        INTERFACE_ENTRY(Exchange::IWifiControl)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

    private:
        uint32_t PrepareWPASupplicant(const PluginHost::IShell* service, const Setting& config) {
            uint32_t result = Core::ERROR_GENERAL;

            if ((config.Application.Value().empty() == true) && (config.Application.IsNull() == false)) {
                SYSLOG(Logging::Error, (_T("WPA_SUPPLICANT application path is not set")));
            }
            else if (config.ConnectorDirectory.Value().empty() == true) {
                SYSLOG(Logging::Error, (_T("WPA_SUPPLICANT application path is not set")));
            }
            else if (config.Interface.Value().empty() == true) {
                SYSLOG(Logging::Error, (_T("No interface defines for the Wifi")));
            }
            else if (config.Application.IsNull() == true) {
                TRACE(Trace::Information, (_T("Using the WPA_SUPPLICANT that is already running")));
                result = Core::ERROR_NONE;
            }
            else if (config.WaitTime.Value() == 0) {
                SYSLOG(Logging::Error, (_T("No waiting time specified for WPA_SUPPLICANT startup")));
            }
            else {
                string connectorFullDirectory = service->VolatilePath() + config.ConnectorDirectory.Value();

                if (Core::File(connectorFullDirectory).IsDirectory()) {
                    // if directory exists remove it to clear data (eg. sockets) that can remain after previous plugin run
                    Core::Directory(connectorFullDirectory.c_str()).Destroy();
                }

                if (Core::Directory(connectorFullDirectory.c_str()).CreatePath() == false) {
                    SYSLOG(Logging::Error, (_T("Could not create connector path")));
                }
                else {
                    string logFile;

                    if (config.LogFile.IsSet()) {
                        logFile = service->VolatilePath() + config.LogFile.Value();
                    }

                    result = _wpaSupplicant.Launch(config.Application.Value(), connectorFullDirectory,
                                    config.Interface.Value(), config.WaitTime.Value(), logFile);
                                    
                    if (result != Core::ERROR_NONE) {
                        SYSLOG(Logging::Error,  (_T("Could not start WPA_SUPPLICANT")));
                    }
                }
            }
            return (result);
        }
        inline uint32_t StoreUpdatedSsidConfig()
        {
            uint32_t result = Core::ERROR_NONE;
            Core::File configFile(_configurationStore);
            if (configFile.Destroy()) {
                result = Store();
            } else {
                result = Core::ERROR_UNAVAILABLE;
            }

            return result;
        }

        inline uint32_t Store()
        {
            uint32_t result = Core::ERROR_NONE;

            Core::File configFile(_configurationStore);
            WPASupplicant::Config::Iterator list(_controller->Configs());
            SSIDConfigs configs;
            configs.Preferred = _preferredSsid;
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
                    result = Core::ERROR_WRITE_ERROR;
                }
            } else {
                if (configFile.Exists() == true) {
                    configFile.Destroy();
                }
            }
            return result;
        }

        string SSIDDecode(const string& item) const
        {
             TCHAR converted[256];

             return (string(converted, Core::URL::Decode(item.c_str(), static_cast<uint32_t>(item.length()), converted, (sizeof(converted) / sizeof(TCHAR)))));
        }

        uint8_t GetWPAProtocolFlags(const ConfigInfo& settings, const bool safeFallback) {
            uint8_t protocolFlags = 0;
            uint8_t mask = 1;

            if (settings.method != Security::UNKNOWN) {
                switch (settings.method & mask) {
                case Security::WPA:
                    protocolFlags = WPASupplicant::Config::WPA;
                    break;
                case Security::WPA2:
                    protocolFlags = WPASupplicant::Config::WPA2;
                    break;
                 case Security::WPA_WPA2:
                    protocolFlags = WPASupplicant::Config::WPA | WPASupplicant::Config::WPA2;
                    break;
                default:
                    if (safeFallback) {
                        protocolFlags = WPASupplicant::Config::WPA | WPASupplicant::Config::WPA2;
                        TRACE(Trace::Information, (_T("Unknown WPA protocol type %d. Assuming WPA/WPA2"), settings.method & mask));
                    } else {
                        TRACE(Trace::Information, (_T("Unknown WPA protocol type %d"), settings.method & mask));
                    }
                    break;
                }
            }

            return protocolFlags;
        }

        static Security GetSecurityMethod(const WPASupplicant::Network::pair pair)
        {
            Security security = Security::UNKNOWN;
            switch (pair) {
            case WPASupplicant::Network::PAIR_WPA:
                security = Security::WPA;
                break;
            case WPASupplicant::Network::PAIR_WPA2:
                security = Security::WPA2;
                break;
            case WPASupplicant::Network::PAIR_WPS:
                security = Security::WPS;
                break;
            case WPASupplicant::Network::PAIR_ESS:
                security = Security::ENTERPRISE;
                break;
            case WPASupplicant::Network::PAIR_WEP:
                security = Security::WEP;
                break;
            case WPASupplicant::Network::PAIR_OPEN:
                security = Security::OPEN;
                break;
            case WPASupplicant::Network::PAIR_NONE:
                break;
            default:
                TRACE_GLOBAL(Trace::Information, (_T("Unknow Pair valie %d"), pair));
                break;
            }
            return security;
        }

        static SecurityInfo::Key GetSecurityKey(const WPASupplicant::Network::key key)
        {
            SecurityInfo::Key securityKey = SecurityInfo::NONE;

            switch (key) {
            case WPASupplicant::Network::KEY_PSK:
                securityKey = SecurityInfo::PSK;
                break;
            case WPASupplicant::Network::KEY_EAP:
                securityKey = SecurityInfo::EAP;
                break;
            case WPASupplicant::Network::KEY_CCMP:
                securityKey = SecurityInfo::CCMP;
                break;
            case WPASupplicant::Network::KEY_TKIP:
                securityKey = SecurityInfo::TKIP;
                break;
            case WPASupplicant::Network::KEY_PREAUTH:
                securityKey = SecurityInfo::PREAUTH;
                break;
            case WPASupplicant::Network::KEY_WPS_PBC:
                securityKey = SecurityInfo::PBC;
                break;
            case WPASupplicant::Network::KEY_WPS_PIN:
                securityKey = SecurityInfo::PIN;
                break;
            case WPASupplicant::Network::KEY_NONE:
                break;
            default:
                TRACE_GLOBAL(Trace::Information, (_T("Unknow Pair valie %d"), key));
                break;
            }
            return securityKey;
        }

        static  Security GetWPAProtocolType(const uint8_t protocolFlags)
        {
            Security result = Security::UNKNOWN;
        
            if ((protocolFlags & WPASupplicant::Config::WPA2) != 0) {
                result = Security::WPA2;
            } else if ((protocolFlags & WPASupplicant::Config::WPA) != 0) {
                result = Security::WPA;
            } 

            return result;
        }
    
        void WifiEvent(const WPASupplicant::Controller::events& event)
        {
             TRACE(Trace::Information, (_T("WPASupplicant notification: %s"), Core::EnumerateType<WPASupplicant::Controller::events>(event).Data()));

            switch (event) {
            case WPASupplicant::Controller::CTRL_EVENT_SCAN_RESULTS: {
                _networks.clear();
                _securities.clear();

                WPASupplicant::Network::Iterator list(_controller->Networks());
                while (list.Next() == true) {
                    NetworkInfo network;
                    std::list<SecurityInfo> securities;
                    FillNetworkInfo(list.Current(), network, securities);

                    _networks.push_back(network);
                    if (securities.size() > 0) {
                        _securities.insert(std::pair<string, std::list<SecurityInfo>>(network.ssid, securities));
                    }
                }
                _autoConnect.Scanned();

                NotifyNetworkChange();

                break;
            }
            case WPASupplicant::Controller::CTRL_EVENT_CONNECTED: {
                NotifyConnectionChange(_controller->Current());
                break;
            }
            case WPASupplicant::Controller::CTRL_EVENT_DISCONNECTED: {
                //When WPS operation is active, hide the notification till credentials are received
                if (!_wpsConnect.Active()) {
                    NotifyConnectionChange(string());
                    _autoConnect.Disconnected();
                } else {
                    _wpsConnect.Disconnected();
                }
                break;
            }
            case WPASupplicant::Controller::CTRL_EVENT_NETWORK_CHANGED: {
                NotifyNetworkChange();
                break;
            }
            case WPASupplicant::Controller::WPS_EVENT_SUCCESS: {
                break;
            }
            case WPASupplicant::Controller::WPS_EVENT_CRED_RECEIVED: {
                _wpsConnect.Completed(Core::ERROR_NONE);
                break;
            }
            case WPASupplicant::Controller::WPS_EVENT_TIMEOUT: {
                _wpsConnect.Completed(Core::ERROR_TIMEDOUT);
                NotifyConnectionChange(string());
                break;
            }
            case WPASupplicant::Controller::WPS_EVENT_FAIL: {
                // If Credentials was already received, ignore the fail.
                if (!_wpsConnect.Credentials()) {
                    _wpsConnect.Completed(Core::ERROR_ASYNC_FAILED);
                    NotifyConnectionChange(string());
                }
                break;
            }
            case WPASupplicant::Controller::WPS_EVENT_OVERLAP: {
                _wpsConnect.Completed(Core::ERROR_PENDING_CONDITIONS);
                NotifyConnectionChange(string());
                break;
            }

            case WPASupplicant::Controller::CTRL_EVENT_BSS_ADDED:
            case WPASupplicant::Controller::CTRL_EVENT_BSS_REMOVED:
            case WPASupplicant::Controller::CTRL_EVENT_TERMINATING:
            case WPASupplicant::Controller::CTRL_EVENT_NETWORK_NOT_FOUND:
            case WPASupplicant::Controller::CTRL_EVENT_SCAN_STARTED:
            case WPASupplicant::Controller::CTRL_EVENT_SSID_TEMP_DISABLED:
            case WPASupplicant::Controller::WPS_EVENT_AP_AVAILABLE:
            case WPASupplicant::Controller::WPS_EVENT_AP_AVAILABLE_PBC:
            case WPASupplicant::Controller::WPS_EVENT_AP_AVAILABLE_PIN:
            case WPASupplicant::Controller::WPS_EVENT_ACTIVE:
            case WPASupplicant::Controller::WPS_EVENT_DISABLE:
            case WPASupplicant::Controller::AP_ENABLED:
                break;
           }
        }

        void NotifyConnectionChange(const string& ssid)
        {
            _adminLock.Lock();
            _ssidList.push_back(ssid);
            _job.Submit();
            _adminLock.Unlock();
        }

        void NotifyNetworkChange()
        {
            _adminLock.Lock();
            _networkChange = true;
            _job.Submit();
            _adminLock.Unlock();
        }

        friend Core::ThreadPool::JobType<WifiControlImplementation&>;
        void Dispatch()
        {
             CheckNetworkChange();
             CheckConnectionChange();
        }

        inline void CheckConnectionChange()
        {
            _adminLock.Lock();

            while (_ssidList.size()) {
                std::list<string>::iterator ssid(_ssidList.begin());
                if (ssid != _ssidList.end()) {
                    for (auto& notification : _notifications) {
                        _adminLock.Unlock();
                        notification->ConnectionChange(*ssid);
                        _adminLock.Lock();
                    }
                    _ssidList.erase(ssid);
                }
            }
            _adminLock.Unlock();
        }

        inline void CheckNetworkChange()
        {
            _adminLock.Lock();
            if (_networkChange) {
                for (auto& notification : _notifications) {
                    _adminLock.Unlock();
                    notification->NetworkChange();
                    _adminLock.Lock();
                }
            }
            _adminLock.Unlock();
        }

    private:
        uint8_t _retryInterval;
        uint32_t _walkTime;
        uint32_t _maxRetries;
        string _configurationStore;
        string _preferredSsid;
        Sink _sink;
        WifiDriver _wpaSupplicant;
        Core::ProxyType<WPASupplicant::Controller> _controller;
        AutoConnect _autoConnect;
        bool _autoConnectEnabled;
        WPSConnect _wpsConnect;
        std::list<NetworkInfo> _networks;
        ConfigMap _configs;
        SecurityMap _securities;
        bool _wpsDisabled;
        bool _networkChange;
        mutable Core::CriticalSection _adminLock;
        std::vector<Exchange::IWifiControl::INotification*> _notifications;
        std::list<string> _ssidList;
        Core::WorkerPool::JobType<WifiControlImplementation&> _job;
    };

    SERVICE_REGISTRATION(WifiControlImplementation, 1, 0)
} // namespace Plugin
} // namespace Thunder
