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

#ifndef WPASUPPLICANT_CONTROLLER_H
#define WPASUPPLICANT_CONTROLLER_H

#include "Module.h"
#include "Network.h"

// Interface specification taken from:
// https://w1.fi/wpa_supplicant/devel/ctrl_iface_page.html

namespace WPEFramework {
namespace WPASupplicant {

    class Controller : public Core::StreamType<Core::SocketDatagram> {
    public:
        static uint16_t KeyPair(const Core::TextFragment& element, uint32_t& keys);
        static string BSSID(const uint64_t& bssid);
        static uint64_t BSSID(const string& bssid);

    public:
        enum events {
            CTRL_EVENT_SCAN_STARTED,
            CTRL_EVENT_SCAN_RESULTS,
            CTRL_EVENT_CONNECTED,
            CTRL_EVENT_DISCONNECTED,
            CTRL_EVENT_BSS_ADDED,
            CTRL_EVENT_BSS_REMOVED,
            CTRL_EVENT_TERMINATING,
            CTRL_EVENT_NETWORK_NOT_FOUND,
            CTRL_EVENT_NETWORK_CHANGED,
            CTRL_EVENT_SSID_TEMP_DISABLED,
            WPS_AP_AVAILABLE,
            AP_ENABLED
        };
        /* Reason codes (IEEE Std 802.11-2016, 9.4.1.7, Table 9-45) */
        enum reasons {
            WLAN_REASON_NOINFO_GIVEN,
            WLAN_REASON_UNSPECIFIED,
            WLAN_REASON_PREV_AUTH_NOT_VALID,
            WLAN_REASON_DEAUTH_LEAVING,
            WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY,
            WLAN_REASON_DISASSOC_AP_BUSY,
            WLAN_REASON_CLASS2_FRAME_FROM_NONAUTH_STA,
            WLAN_REASON_CLASS3_FRAME_FROM_NONASSOC_STA,
            WLAN_REASON_DISASSOC_STA_HAS_LEFT,
            WLAN_REASON_STA_REQ_ASSOC_WITHOUT_AUTH,
            WLAN_REASON_PWR_CAPABILITY_NOT_VALID,
            WLAN_REASON_SUPPORTED_CHANNEL_NOT_VALID,
            WLAN_REASON_BSS_TRANSITION_DISASSOC,
            WLAN_REASON_INVALID_IE,
            WLAN_REASON_MICHAEL_MIC_FAILURE,
            WLAN_REASON_4WAY_HANDSHAKE_TIMEOUT,
            WLAN_REASON_GROUP_KEY_UPDATE_TIMEOUT,
            WLAN_REASON_IE_IN_4WAY_DIFFERS,
            WLAN_REASON_GROUP_CIPHER_NOT_VALID,
            WLAN_REASON_PAIRWISE_CIPHER_NOT_VALID,
            WLAN_REASON_AKMP_NOT_VALID,
            WLAN_REASON_UNSUPPORTED_RSN_IE_VERSION,
            WLAN_REASON_INVALID_RSN_IE_CAPAB,
            WLAN_REASON_IEEE_802_1X_AUTH_FAILED,
            WLAN_REASON_CIPHER_SUITE_REJECTED,
            WLAN_REASON_TDLS_TEARDOWN_UNREACHABLE,
            WLAN_REASON_TDLS_TEARDOWN_UNSPECIFIED,
            WLAN_REASON_SSP_REQUESTED_DISASSOC,
            WLAN_REASON_NO_SSP_ROAMING_AGREEMENT,
            WLAN_REASON_BAD_CIPHER_OR_AKM,
            WLAN_REASON_NOT_AUTHORIZED_THIS_LOCATION,
            WLAN_REASON_SERVICE_CHANGE_PRECLUDES_TS,
            WLAN_REASON_UNSPECIFIED_QOS_REASON,
            WLAN_REASON_NOT_ENOUGH_BANDWIDTH,
            WLAN_REASON_DISASSOC_LOW_ACK,
            WLAN_REASON_EXCEEDED_TXOP,
            WLAN_REASON_STA_LEAVING,
            WLAN_REASON_END_TS_BA_DLS,
            WLAN_REASON_UNKNOWN_TS_BA,
            WLAN_REASON_TIMEOUT,
            WLAN_REASON_PEERKEY_MISMATCH = 45,
            WLAN_REASON_AUTHORIZED_ACCESS_LIMIT_REACHED,
            WLAN_REASON_EXTERNAL_SERVICE_REQUIREMENTS,
            WLAN_REASON_INVALID_FT_ACTION_FRAME_COUNT,
            WLAN_REASON_INVALID_PMKID,
            WLAN_REASON_INVALID_MDE,
            WLAN_REASON_INVALID_FTE,
            WLAN_REASON_MESH_PEERING_CANCELLED,
            WLAN_REASON_MESH_MAX_PEERS,
            WLAN_REASON_MESH_CONFIG_POLICY_VIOLATION,
            WLAN_REASON_MESH_CLOSE_RCVD,
            WLAN_REASON_MESH_MAX_RETRIES,
            WLAN_REASON_MESH_CONFIRM_TIMEOUT,
            WLAN_REASON_MESH_INVALID_GTK,
            WLAN_REASON_MESH_INCONSISTENT_PARAMS,
            WLAN_REASON_MESH_INVALID_SECURITY_CAP,
            WLAN_REASON_MESH_PATH_ERROR_NO_PROXY_INFO,
            WLAN_REASON_MESH_PATH_ERROR_NO_FORWARDING_INFO,
            WLAN_REASON_MESH_PATH_ERROR_DEST_UNREACHABLE,
            WLAN_REASON_MAC_ADDRESS_ALREADY_EXISTS_IN_MBSS,
            WLAN_REASON_MESH_CHANNEL_SWITCH_REGULATORY_REQ,
            WLAN_REASON_MESH_CHANNEL_SWITCH_UNSPECIFIED,
        };
        struct IConnectCallback {
            virtual ~IConnectCallback() {}

            virtual void Completed(const uint32_t result) = 0;
        };

    private:
        static constexpr uint32_t MaxConnectionTime = 3000;

        Controller() = delete;
        Controller(const Controller&) = delete;
        Controller& operator=(const Controller&) = delete;

        class ConfigInfo {
        public:
            enum state {
                DISABLED = 0x01,
                ENABLED = 0x02,
                SELECTED = 0x04
            };

        public:
            ConfigInfo()
                : _id(~0)
                , _state(0)
                , _secret()
                , _hidden(false)
            {
            }
            ConfigInfo(const uint32_t id, const bool enabled)
                : _id(id)
                , _state(enabled ? ENABLED : DISABLED)
                , _secret()
                , _hidden(false)
            {
            }
            ConfigInfo(const ConfigInfo& copy)
                : _id(copy._id)
                , _state(copy._state)
                , _secret(copy._secret)
                , _hidden(copy._hidden)
            {
            }

            ConfigInfo& operator=(const ConfigInfo& rhs)
            {
                _id = rhs._id;
                _state = rhs._state;
                _secret = rhs._secret;
                _hidden = rhs._hidden;

                return (*this);
            }

        public:
            inline uint32_t Id() const
            {
                return (_id);
            }
            inline bool IsEnabled() const
            {
                return (_state >= ENABLED);
            }
            inline bool IsSelected() const
            {
                return (_state >= SELECTED);
            }
            inline void State(const state value)
            {
                _state = value;
            }
            inline const string& Secret() const
            {
                return (_secret);
            }
            inline void Secret(const string& value)
            {
                _secret = value;
            }
            inline bool Hidden() const
            {
                return (_hidden);
            }
            inline void Hidden(const bool state)
            {
                _hidden = state;
            }

        private:
            uint32_t _id;
            uint16_t _state;
            string _secret;
            bool _hidden;
        };
        class NetworkInfo {

        public:
            NetworkInfo()
                : _frequency()
                , _signal()
                , _pair()
                , _key()
                , _ssid()
                , _id(~0)
                , _throughput(0)
                , _hidden(true)
            {
            }
            NetworkInfo(const uint32_t id, const string& ssid, const uint32_t frequency, const int32_t signal, const uint16_t pairs, const uint32_t keys, const uint32_t throughput)
            {
                Set(id, ssid, frequency, signal, pairs, keys, throughput);
            }
            NetworkInfo(const NetworkInfo& copy)
                : _frequency(copy._frequency)
                , _signal(copy._signal)
                , _pair(copy._pair)
                , _key(copy._key)
                , _ssid(copy._ssid)
                , _id(copy._id)
                , _throughput(copy._throughput)
                , _hidden(copy._hidden)
            {
            }
            ~NetworkInfo()
            {
            }

            NetworkInfo& operator=(const NetworkInfo& rhs)
            {
                _frequency = rhs._frequency;
                _signal = rhs._signal;
                _pair = rhs._pair;
                _key = rhs._key;
                _ssid = rhs._ssid;
                _id = rhs._id;
                _throughput = rhs._throughput;
                _hidden = rhs._hidden;

                return (*this);
            }

        public:
            bool HasId() const { return (_id != static_cast<uint32_t>(~0)) && (_id != static_cast<uint32_t>(~1)); }
            bool HasDetail() const { return (_id != static_cast<uint32_t>(~0)) || (_id == static_cast<uint32_t>(~1)); }
            uint32_t Id() const { return _id; }
            const std::string& SSID() const { return _ssid; }
            uint32_t Frequency() const { return _frequency; }
            int32_t Signal() const { return _signal; }
            uint16_t Pair() const { return _pair; }
            uint32_t Key() const { return _key; }
            uint32_t Throughput() const { return _throughput; }
            bool IsHidden() const { return _hidden; }

            void Set(const string& ssid, const uint32_t frequency, const int32_t signal, const uint16_t pairs, const uint32_t keys)
            {
                _frequency = frequency;
                _signal = signal;
                _pair = pairs;
                _key = keys;
                SetSSID(ssid);
            }
            void Set(const uint32_t id, const string& ssid, const uint32_t frequency, const int32_t signal, const uint16_t pairs, const uint32_t keys, const uint32_t throughput)
            {
                _id = id;
                _frequency = frequency;
                _signal = signal;
                _pair = pairs;
                _key = keys;
                _throughput = throughput;

                // Check SSID
                SetSSID(ssid);
            }

        private:
            void SetSSID(const string& ssid)
            {
                _hidden = (ssid[0] == '\0');
                if (_hidden == false) {
                    _ssid = ssid;
                }
            }

        private:
            uint32_t _frequency;
            int32_t _signal;
            uint16_t _pair;
            uint32_t _key;
            std::string _ssid;
            uint32_t _id;
            uint32_t _throughput;
            bool _hidden;
        };
        class Request {
        private:
            Request(const Request&) = delete;
            Request& operator=(const Request&) = delete;

        public:
            Request()
                : _request()
                , _settable(true)
            {
            }
            Request(const string& message)
                : _request(message)
#ifdef __DEBUG__
                , _original(message)
#endif // __DEBUG__
                , _settable(true)
            {
            }
            virtual ~Request()
            {
            }

        public:
            bool Set(const string& message)
            {
                if (_settable == true) {
                    _request = message;
                    _settable = false;
#ifdef __DEBUG__
                    _original = _request;
#endif // __DEBUG__
                    return (true);
                }
                return (false);
            }
            inline bool InProgress() const {
                return (_settable == false);
            }
            string& Message()
            {
                return (_request);
            }
            const string& Message() const
            {
                return (_request);
            }
#ifdef __DEBUG__
            const string& Original() const
            {
                return (_original);
            }
#endif // __DEBUG__

            void Processing(const bool processing)
            {
                _settable = (processing == false);
            }

            virtual void Completed(const string& response, const bool abort) = 0;

        private:
            string _request;
#ifdef __DEBUG__
            string _original;
#endif // __DEBUG__
            bool _settable;
        };
        class ScanRequest : public Request {
        private:
            ScanRequest() = delete;
            ScanRequest(const ScanRequest&) = delete;
            ScanRequest& operator=(const ScanRequest&) = delete;

        public:
            ScanRequest(Controller& parent)
                : Request()
                , _scanning(false)
                , _parent(parent)
                , _eventReporting(~0)
            {
            }
            virtual ~ScanRequest()
            {
            }

        public:
            inline bool Activated()
            {
                if (_scanning == false) {
                    _scanning = true;
                    return (true);
                }
                return (false);
            }
            inline void Aborted()
            {
                _scanning = false;
            }
            inline bool IsScanning() const
            {
                return (_scanning);
            }
            bool Set()
            {
                return (Request::Set(string(_TXT("SCAN_RESULTS"))));
            }
            inline void Event(const events value)
            {
                _eventReporting = value;
            }
            virtual void Completed(const string& response, const bool abort) override
            {
                if (abort == false) {
                    Core::TextFragment data(response.c_str(), response.length());
                    uint32_t marker = data.ForwardFind('\n');
                    uint32_t markerEnd = data.ForwardFind('\n', marker + 1);

                    while (marker != markerEnd) {

                        Core::TextFragment element(data, marker + 1, (markerEnd - marker - 1));
                        marker = markerEnd;
                        markerEnd = data.ForwardFind('\n', marker + 1);
                        NetworkInfo newEntry;
                        _parent.Add(Transform(element, newEntry), newEntry);
                    }
                }
                if (_eventReporting != static_cast<uint32_t>(~0)) {
                    _parent.Notify(static_cast<events>(_eventReporting));
                    _eventReporting = static_cast<uint32_t>(~0);
                }
                _scanning = false;
            }

        private:
            uint64_t Transform(const Core::TextFragment& infoLine, NetworkInfo& source)
            {
                // First thing we find will be 6 bytes...
                uint64_t bssid = BSSID(infoLine.Text());

                // Next is the frequency
                uint16_t index = infoLine.ForwardSkip(_T(" \t"), 18);
                uint16_t end = infoLine.ForwardFind(_T(" \t"), index);

                uint32_t frequency = Core::NumberType<uint32_t>(Core::TextFragment(infoLine, index, end - index));

                // Next we will find the Signal Strength
                index = infoLine.ForwardSkip(_T(" \t"), end);
                end = infoLine.ForwardFind(_T(" \t"), index);

                int32_t signal = Core::NumberType<int32_t>(Core::TextFragment(infoLine, index, end - index));

                // Extract the flags from the information
                index = infoLine.ForwardSkip(_T(" \t"), end);
                end = infoLine.ForwardFind(_T(" \t"), index);

                uint32_t keys = 0;
                uint16_t pairs = KeyPair(Core::TextFragment(infoLine, index, end - index), keys);

                // And last but not least, we will find the SSID
                index = infoLine.ForwardSkip(_T(" \t"), end);
                source.Set(Core::TextFragment(infoLine, index, infoLine.Length() - index).Text(), frequency, signal, pairs, keys);

                return (bssid);
            }

        private:
            bool _scanning;
            Controller& _parent;
            uint32_t _eventReporting;
        };
        class StatusRequest : public Request {
        private:
            StatusRequest() = delete;
            StatusRequest(const StatusRequest&) = delete;
            StatusRequest& operator=(const StatusRequest&) = delete;

        public:
            StatusRequest(Controller& parent)
                : Request(string(_TXT("STATUS")))
                , _parent(parent)
                , _signaled(false, true)
                , _bssid(0)
                , _ssid()
                , _pair(0)
                , _key(0)
                , _eventReporting(~0)
                , _disconnectReason(reasons::WLAN_REASON_NOINFO_GIVEN)
            {
            }
            virtual ~StatusRequest()
            {
            }

        public:
            inline void Clear() const
            {
                _signaled.ResetEvent();
            }
            inline uint64_t BSSID() const
            {
                return (_bssid);
            }
            inline const string& SSID() const
            {
                return (_ssid);
            }
            inline uint16_t Key() const
            {
                return _key;
            }
            inline uint16_t Pair() const
            {
                return _pair;
            }
            inline bool Wait(const uint32_t waitTime) const
            {
                bool result = (_signaled.Lock(waitTime) == Core::ERROR_NONE ? true : false);
                return (result);
            }
            inline void Event(const events value)
            {
                _eventReporting = value;
            }
            inline void DisconnectReason(const reasons reason)
            {
                _disconnectReason = reason;
            }
            inline reasons DisconnectReason() const
            {
                return (_disconnectReason);
            }
            void Reset()
            {
                _bssid = 0;
                _ssid.clear();
                _pair = 0;
                _key = 0;
            }

        private:
            virtual void Completed(const string& response, const bool abort) override
            {
                if (abort == false) {
                    Core::TextFragment data(response.c_str(), response.length());

                    uint32_t marker = 0;
                    uint32_t markerEnd = data.ForwardFind('\n', marker);

                    while (marker != markerEnd) {

                        Core::TextSegmentIterator index(Core::TextFragment(data, marker, (markerEnd - marker)), false, '=');

                        if (index.Next() == true) {

                            string name(index.Current().Text());

                            if (index.Next() == true) {
                                if (name == Config::BSSIDKEY) {
                                    uint16_t end = index.Current().ForwardSkip(_T(" \t"), 0);
                                    _bssid = Controller::BSSID(Core::TextFragment(index.Current(), end, ~0).Text());
                                } else if (name == Config::SSIDKEY) {
                                    _ssid = index.Current().Text();
                                } else if (name == Config::KEY) {
                                    _pair = KeyPair(index.Current(), _key);
                                } else if (name == _T("mode")) {
                                    _mode = Core::EnumerateType<WPASupplicant::Network::mode>(index.Current()).Value();
                                } else if (name == _T("wpa_state")) {
                                } else if (name == _T("EAP state")) {
                                } else if (name == _T("Supplicant PAE state")) {
                                }
                            }
                        }
                        marker = (markerEnd < data.Length() ? markerEnd + 1 : markerEnd);
                        markerEnd = data.ForwardFind('\n', marker);
                    }
                }

                if (_eventReporting != static_cast<uint32_t>(~0)) {
                    _parent.Notify(static_cast<events>(_eventReporting));
                    _eventReporting = static_cast<uint32_t>(~0);
                }

                // Reload for the next run...
                Set(string(_TXT("STATUS")));

                _signaled.SetEvent();
            }

        private:
            Controller& _parent;
            mutable Core::Event _signaled;
            uint64_t _bssid;
            string _ssid;
            uint16_t _pair;
            uint32_t _key;
            WPASupplicant::Network::mode _mode;
            uint32_t _eventReporting;
            reasons _disconnectReason;
        };
        class DetailRequest : public Request {
        private:
            DetailRequest() = delete;
            DetailRequest(const DetailRequest&) = delete;
            DetailRequest& operator=(const DetailRequest&) = delete;

        public:
            DetailRequest(Controller& parent)
                : Request()
                , _parent(parent)
            {
            }
            virtual ~DetailRequest()
            {
            }

        public:
            bool Set(const uint64_t& bssid)
            {
                if (Request::Set(string(_TXT("BSS ")) + _parent.BSSID(bssid)) == true) {
                    _bssid = bssid;
                    return (true);
                }
                return (false);
            }
            virtual void Completed(const string& response, const bool abort) override
            {
                if (abort == false) {
                    Core::TextFragment data(response);

                    string ssid;
                    uint32_t id = static_cast<uint32_t>(~1);
                    uint32_t freq = 0;
                    int32_t signal = 0;
                    uint16_t pair = 0;
                    uint32_t keys = 0;
                    uint32_t throughput = 0;
                    uint32_t marker = 0;
                    uint32_t markerEnd = data.ForwardFind('\n', marker);

                    while (marker != markerEnd) {

                        Core::TextSegmentIterator index(Core::TextFragment(data, marker, (markerEnd - marker)), false, '=');

                        if (index.Next() == true) {

                            string name(index.Current().Text());

                            if (index.Next() == true) {
                                if (name == _T("id")) {
                                    id = Core::NumberType<uint32_t>(index.Current());
                                } else if (name == _T("est_throughput")) {
                                    throughput = Core::NumberType<uint32_t>(index.Current());
                                } else if (name == _T("ssid")) {
                                    ssid = index.Current().Text();
                                } else if (name == _T("freq")) {
                                    freq = Core::NumberType<uint32_t>(index.Current());
                                } else if (name == _T("level")) {
                                    signal = Core::NumberType<int32_t>(index.Current());
                                } else if (name == _T("level")) {
                                    signal = Core::NumberType<int32_t>(index.Current());
                                } else if (name == _T("flags")) {
                                    pair = KeyPair(index.Current(), keys);
                                }
                            }
                        }
                        marker = (markerEnd < data.Length() ? markerEnd + 1 : markerEnd);
                        markerEnd = data.ForwardFind('\n', marker);
                    }

                    _parent.Update(_bssid, ssid, id, freq, signal, pair, keys, throughput);
                }
            }

        private:
            Controller& _parent;
            uint64_t _bssid;
        };
        class NetworkRequest : public Request {
        private:
            NetworkRequest() = delete;
            NetworkRequest(const NetworkRequest&) = delete;
            NetworkRequest& operator=(const NetworkRequest&) = delete;

        public:
            NetworkRequest(Controller& parent)
                : Request()
                , _parent(parent)
            {
            }
            virtual ~NetworkRequest()
            {
            }

        public:
            bool Set()
            {
                return (Request::Set(string(_TXT("LIST_NETWORKS"))));
            }
            virtual void Completed(const string& response, const bool abort) override
            {
                if (abort == false) {
                    Core::TextFragment data(response.c_str(), response.length());
                    uint32_t marker = data.ForwardFind('\n');
                    uint32_t markerEnd = data.ForwardFind('\n', marker + 1);

                    while (marker != markerEnd) {

                        uint64_t ssid;
                        bool current;
                        string id = Transform(Core::TextFragment(data, marker + 1, (markerEnd - marker - 1)), ssid, current);

                        // Get the SSID from here
                        _parent.Add(id, current, ssid);

                        marker = markerEnd;
                        markerEnd = data.ForwardFind('\n', marker + 0);
                    }
                }
            }

        private:
            string Transform(const Core::TextFragment& infoLine, uint64_t& bssid, bool& current)
            {
                // network id / ssid / bssid / flags
                // 0       example network any     [CURRENT]
                uint16_t index = infoLine.ForwardSkip(_T(" \t"), 0);
                uint16_t end = infoLine.ForwardFind(_T(" \t"), index);

                // determine in which area we wil find the SSID and the BSSID.
                index = infoLine.ForwardSkip(_T(" \t"), end);
                end = infoLine.ForwardFind('[', index);

                if (end < infoLine.Length()) {
                    end--;
                }

                Core::TextFragment element = Core::TextFragment(infoLine, index, (end - index));
                element.TrimEnd(_T(" /t"));
                index = element.ReverseFind(_T(" \t"));

                // network id
                bssid = BSSID(Core::TextFragment(element, index, ~0).Text());

                element = Core::TextFragment(element, 0, index);
                element.TrimEnd(_T(" \t"));

                do {
                    index = infoLine.ForwardFind('[', end);
                    end = infoLine.ForwardFind(']', index);

                } while ((end < infoLine.Length()) && (Core::TextFragment(infoLine, index + 1, end - index - 1) != "CURRENT"));

                current = (end < infoLine.Length());

                return (element.Text());
            }

        private:
            Controller& _parent;
        };
        class ConnectRequest : public Request {
        private:
            enum class connection : uint8_t {
                SELECT,
                RECONNECT,
                AUTHORIZE,
                WAITING,
                ERROR
            };

        public:
            ConnectRequest() = delete;
            ConnectRequest(const ConnectRequest&) = delete;
            ConnectRequest& operator=(const ConnectRequest&) = delete;

            ConnectRequest(Controller& parent)
                : Request()
                , _parent(parent)
                , _adminLock()
                , _state(connection::SELECT)
                , _bssid(0)
                , _ssid()
                , _callback(nullptr)
                , _error(Core::ERROR_NONE)
            {
            }
            ~ConnectRequest() override
            {
            }

        public:
            uint32_t Invoke(IConnectCallback* callback, const string& ssid, const uint64_t& bssid) {

                uint32_t result = ((bssid == 0) ? Core::ERROR_INCOMPLETE_CONFIG :
                                  (((_parent.Current().empty() == false) && (_parent.Current() == ssid)) ? Core::ERROR_ALREADY_CONNECTED :
                                  Core::ERROR_UNKNOWN_KEY));

                if (result == Core::ERROR_UNKNOWN_KEY) {

                    _adminLock.Lock();
                    if (_callback == nullptr) {

                        EnabledContainer::iterator index(_parent._enabled.find(ssid));

                        if (index != _parent._enabled.end()) {

                            result = Core::ERROR_ASYNC_FAILED;

                            if (Request::Set (string(_TXT("SELECT_NETWORK ")) + Core::NumberType<uint32_t>(index->second.Id()).Text()) == true) {

                                _bssid = bssid;
                                _ssid = ssid;
                                _state = connection::SELECT;
                                _callback = callback;
                                _adminLock.Unlock();

                                result = Core::ERROR_NONE;

                                _parent.Submit(this);

                                index->second.State(ConfigInfo::SELECTED);
                            } else {
                                _adminLock.Unlock();
                            }
                        } else {
                            _adminLock.Unlock();
                        }
                    } else {
                       _adminLock.Unlock();
                       result = Core::ERROR_INPROGRESS;
                    }
                }

                return (result);
            }
            uint32_t Revoke(IConnectCallback* callback) {

                uint32_t result = Core::ERROR_UNAVAILABLE;

                _adminLock.Lock();

                if (callback == _callback) {
                    _adminLock.Unlock();

                    _parent.Revoke(this);

                    _adminLock.Lock();
                    _callback = nullptr;
                    result = Core::ERROR_NONE;
                }
                _adminLock.Unlock();

                return (result);
            }
            void Completed(const string& response, const bool abort) override
            {
                uint32_t result = Core::ERROR_REQUEST_SUBMITTED;
                string newCommand;

                _adminLock.Lock();
                if (abort == true) {
                    result = Core::ERROR_ASYNC_ABORTED;
                } 
                else if (response != _T("OK")) {
                    result = Core::ERROR_ASYNC_FAILED;
                }
                else if (_state == connection::SELECT) {
                    newCommand = string(_TXT("RECONNECT"));
                    _state     = connection::RECONNECT;
                }
                else if (_state == connection::RECONNECT) {
                    newCommand = string(_TXT("PREAUTH ")) + BSSID(_bssid);
                    _state     = connection::AUTHORIZE;
                }
                else if (_state == connection::ERROR) {
                    result = _error;
                }
                else {
                    _state = connection::WAITING;
                }

                if ((newCommand.empty() == false) && (Request::Set(newCommand) == true)) {
                    _adminLock.Unlock();
                    _parent.Submit(this);
                } 
                else if ((_state != connection::WAITING) && (_callback != nullptr)) {
                    _callback->Completed(result);
                    _adminLock.Unlock();
                }
                else {
                    _adminLock.Unlock();
                }
            }
            void Completed(const uint32_t result) {

                _adminLock.Lock();

                if (result != Core::ERROR_NONE) {

                    _error = result;
                    _state = connection::ERROR;

                    _adminLock.Unlock();
                    if (_parent.IsSubmitted(this) == false) {
                        Request::Set(string(_TXT("DISCONNECT")));
                        _parent.Submit(this);
                    }
                }
                else if ((_callback != nullptr) && (_state == connection::WAITING)) {
                    _callback->Completed(result);
                    _adminLock.Unlock();
                } else {
                    _adminLock.Unlock();
                }
            }

        private:
            Controller& _parent;
            Core::CriticalSection _adminLock;
            connection _state;
            uint64_t _bssid;
            string _ssid;
            IConnectCallback* _callback;
            uint32_t _error;
        };
        class CustomRequest : public Request {
        private:
            CustomRequest() = delete;
            CustomRequest(const CustomRequest&) = delete;
            CustomRequest& operator=(const CustomRequest&) = delete;

        public:
            CustomRequest(const string& custom)
                : Request(custom)
                , _signaled(false, true)
                , _response()
                , _result(Core::ERROR_NONE)
            {
            }
            virtual ~CustomRequest()
            {
            }

        public:
            CustomRequest& operator=(const string& newCommand)
            {

                if (Request::Set(newCommand) == true) {
                    _signaled.ResetEvent();
                }

                return (*this);
            }
            virtual void Completed(const string& response, const bool abort) override
            {

                _result = (abort == false ? Core::ERROR_NONE : Core::ERROR_ASYNC_ABORTED);
                _response = response;
                _signaled.SetEvent();
            }
            inline uint32_t Result() const
            {
                return (_result);
            }
            inline const string& Response() const
            {
                return (_response);
            }
            inline bool Wait(const uint32_t waitTime)
            {
                bool result = (_signaled.Lock(waitTime) == Core::ERROR_NONE ? true : false);
                return (result);
            }

        private:
            Core::Event _signaled;
            string _response;
            uint32_t _result;
        };
        typedef std::map<const uint64_t, NetworkInfo> NetworkInfoContainer;
        typedef std::map<const string, ConfigInfo> EnabledContainer;
        typedef Core::StreamType<Core::SocketDatagram> BaseClass;

    protected:
        Controller(const string& supplicantBase, const string& interfaceName, const uint16_t waitTime)
            : BaseClass(false, Core::NodeId(), Core::NodeId(), 512, 32768)
            , _adminLock()
            , _requests()
            , _networks()
            , _enabled()
            , _error(Core::ERROR_UNAVAILABLE)
            , _callback(nullptr)
            , _scanRequest(*this)
            , _detailRequest(*this)
            , _networkRequest(*this)
            , _statusRequest(*this)
            , _connectRequest(*this)
        {
            string remoteName(Core::Directory::Normalize(supplicantBase) + interfaceName);

            if (Core::File(remoteName).Exists() == true) {

                string data(
                    Core::Directory::Normalize(supplicantBase) + _T("wpa_ctrl_") + interfaceName + '_' + Core::NumberType<uint32_t>(::getpid()).Text());

                LocalNode(Core::NodeId(data.c_str()));

                RemoteNode(Core::NodeId(remoteName.c_str()));

                _error = BaseClass::Open(MaxConnectionTime);

                if ((_error == Core::ERROR_NONE) && (Probe(waitTime) == true)) {

                    CustomRequest exchange(string(_TXT("ATTACH")));

                    Submit(&exchange);

                    if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {
                        _error = Core::ERROR_COULD_NOT_SET_ADDRESS;
                    }
                    else if (SetKey("bss_expiration_age", "180") != Core::ERROR_NONE) {
                        _error = Core::ERROR_GENERAL;
                    }
                    else if (SetKey("autoscan", "periodic:120") != Core::ERROR_NONE) {
                        _error = Core::ERROR_GENERAL;
                    }
                    else {
                        Submit(&_statusRequest);
                    }

                    Revoke(&exchange);
                }
            }
        }

    public:
        static Core::ProxyType<Controller> Create(const string& supplicantBase, const string& interfaceName, const uint16_t waitTime)
        {
            return (Core::ProxyType<Controller>::Create(supplicantBase, interfaceName, waitTime));
        }
        virtual ~Controller()
        {

            if (IsClosed() == false) {

                CustomRequest exchange(string(_TXT("DETACH")));

                Submit(&exchange);

                if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {

                    // We are disconnected
                    TRACE(Trace::Information, (_T("Could not detach from the supplicant. %d"), __LINE__));
                }
                if (BaseClass::Close(MaxConnectionTime) != Core::ERROR_NONE) {
                    TRACE(Trace::Information, (_T("Could not close the channel. %d"), __LINE__));
                }

                Revoke(&exchange);

                // Now abort, for anything still in there.
                Abort();
            }
        }

    public:
        inline bool IsStatusUpdated(const uint32_t waitTime) const
        {
            return (_statusRequest.Wait(waitTime) == true);
        }
        inline bool IsOperational() const
        {
            return (_error == Core::ERROR_NONE);
        }
        inline bool IsScanning() const
        {
            _adminLock.Lock();
            const bool result = _scanRequest.IsScanning();
            _adminLock.Unlock();
            return result;
        }
        inline const string Current() const
        {
            _adminLock.Lock();
            const string current = (_statusRequest.SSID());
            _adminLock.Unlock();
            return current;
        }
        inline const reasons DisconnectReason() const
        {
            _adminLock.Lock();
            const reasons reason = _statusRequest.DisconnectReason();
            _adminLock.Unlock();
            return reason;
        }
        inline uint32_t Error() const
        {
            return (_error);
        }
        inline uint32_t Scan()
        {

            uint32_t result = Core::ERROR_INPROGRESS;
            _adminLock.Lock();
            const bool activated = _scanRequest.Activated();
            _adminLock.Unlock();

            if (activated == true) {
                result = Core::ERROR_NONE;

                CustomRequest exchange(string(_TXT("SCAN")));

                Submit(&exchange);

                if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {
                    _adminLock.Lock();
                    _scanRequest.Aborted();
                    _adminLock.Unlock();
                    result = Core::ERROR_UNAVAILABLE;
                }

                Revoke(&exchange);
            }

            return (result);
        }
        inline uint32_t Ping()
        {

            uint32_t result = Core::ERROR_NONE;

            CustomRequest exchange(string(_TXT("PING")));

            Submit(&exchange);

            if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("PONG"))) {

                result = Core::ERROR_UNAVAILABLE;
            }

            Revoke(&exchange);

            return (result);
        }
        inline uint32_t Debug(const uint32_t level)
        {

            uint32_t result = Core::ERROR_NONE;

            CustomRequest exchange(string(_TXT("LEVEL ")) + Core::NumberType<uint32_t>(level).Text());

            Submit(&exchange);

            if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {

                result = Core::ERROR_UNAVAILABLE;
            }

            Revoke(&exchange);

            return (result);
        }
        inline void Callback(Core::IDispatchType<const events>* callback)
        {
            _adminLock.Lock();
            ASSERT((callback == nullptr) ^ (_callback == nullptr));
            _callback = callback;
            _adminLock.Unlock();
        }
        inline Network Get(const uint64_t& id)
        {

            Network result;

            _adminLock.Lock();

            NetworkInfoContainer::iterator index(_networks.find(id));

            if (index != _networks.end()) {
                result = Network(Core::ProxyType<Controller>(*this),
                    (index->second.HasId() ? index->second.Id() : static_cast<uint32_t>(~0)),
                    index->first,
                    index->second.Frequency(),
                    index->second.Signal(),
                    index->second.Pair(),
                    index->second.Key(),
                    index->second.SSID(),
                    index->second.Throughput(),
                    index->second.IsHidden());
            }

            _adminLock.Unlock();

            return (result);
        }
        inline uint32_t Terminate()
        {
            uint32_t result = Core::ERROR_NONE;

            CustomRequest exchange(string(_TXT("TERMINATE")));

            Submit(&exchange);

            if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {

                result = Core::ERROR_ASYNC_ABORTED;
            }

            Revoke(&exchange);

            return (result);
        }

        inline Config::Iterator Configs()
        {
            Core::ProxyType<Controller> channel(Core::ProxyType<Controller>(*this));
            Config::Iterator result(channel);

            _adminLock.Lock();

            EnabledContainer::const_iterator index(_enabled.begin());

            while (index != _enabled.end()) {
                result.Insert(index->first, index->second.Id(), index->second.IsEnabled(), index->second.IsSelected());

                index++;
            }

            _adminLock.Unlock();

            result.Reset();

            return (result);
        }
        inline Network::Iterator Networks()
        {
            Core::ProxyType<Controller> channel(Core::ProxyType<Controller>(*this));
            Network::Iterator result;

            _adminLock.Lock();

            NetworkInfoContainer::const_iterator index(_networks.begin());

            while (index != _networks.end()) {
                result.Insert(Network(channel,
                    (index->second.HasId() ? index->second.Id() : static_cast<uint32_t>(~0)),
                    index->first,
                    index->second.Frequency(),
                    index->second.Signal(),
                    index->second.Pair(),
                    index->second.Key(),
                    index->second.SSID(),
                    index->second.Throughput(),
                    index->second.IsHidden()));
                index++;
            }

            _adminLock.Unlock();

            result.Reset();

            return (result);
        }
        inline uint32_t Enable()
        {
            uint32_t result = Core::ERROR_NONE;

            CustomRequest exchange(string(_TXT("ENABLE_NETWORK all")));

            Submit(&exchange);

            if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {

                result = Core::ERROR_ASYNC_ABORTED;
            } else {
                _adminLock.Lock();
                if (_networkRequest.Set() == true) {
                    _adminLock.Unlock();
                    // Refresh the enabled network list !!
                    Submit(&_networkRequest);
                } else {
                    _adminLock.Unlock();
                }
            }

            Revoke(&exchange);

            return (result);
        }
        inline uint32_t Disable()
        {
            uint32_t result = Core::ERROR_NONE;

            CustomRequest exchange(string(_TXT("DISABLE_NETWORK all")));

            Submit(&exchange);

            if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {

                result = Core::ERROR_ASYNC_ABORTED;
            }

            Revoke(&exchange);

            return (result);
        }
        Config Get(const string& SSID)
        {
            Config result;

            _adminLock.Lock();

            // See if the given SSID is enabled.
            EnabledContainer::iterator entry(_enabled.find(SSID));

            if (entry != _enabled.end()) {
                // This is an existing config. Easy Piecie.
                //Config call internally calls Lock so unlocking here to avoid dead lock
                _adminLock.Unlock();
                result = Config(Core::ProxyType<Controller>(*this), SSID);
            } else {
                _adminLock.Unlock();
            }

            return (result);
        }
        Config Create(const string& SSID)
        {
            Config result;

            _adminLock.Lock();

            // See if the given SSID is enabled.
            EnabledContainer::iterator entry(_enabled.find(SSID));

            if (entry != _enabled.end()) {
                //Config call internally calls Lock so unlocking here to avoid dead lock
                _adminLock.Unlock();
                // This is an existing config. Easy Piecie.
                result = Config(Core::ProxyType<Controller>(*this), SSID);
            } else {
                uint32_t id = ~0;

                _enabled[SSID] = ConfigInfo(~0, false);
                _adminLock.Unlock();

                // We have bo entry for this SSID, lets create one.
                CustomRequest exchange(string(_TXT("ADD_NETWORK")));

                Submit(&exchange);


                if ((exchange.Wait(MaxConnectionTime) == true) && (exchange.Response() != _T("FAIL"))) {

                    const string& idtext = exchange.Response();
                    id = Core::NumberType<uint32_t>(idtext.c_str(), idtext.length());
                }

                Revoke(&exchange);

                _adminLock.Lock();

                _enabled[SSID] = ConfigInfo(id, false);
                //Config internally calls Lock so unlocking here to avoid dead lock
                _adminLock.Unlock();
                result = Config(Core::ProxyType<Controller>(*this), SSID);
            }

            return (result);
        }
        void Destroy(const string& SSID)
        {
            Config result;

            _adminLock.Lock();

            // See if the given SSID is enabled.
            EnabledContainer::iterator entry(_enabled.find(SSID));

            if ((entry != _enabled.end()) && (entry->second.Id() != static_cast<uint32_t>(~0))) {

                // This is an existing config. Easy Piecie.
                //Config internally calls Lock so unlocking here to avoid dead lock
                _adminLock.Unlock();
                result = Config(Core::ProxyType<Controller>(*this), SSID);

                // We have bo entry for this SSID, lets create one.
                CustomRequest exchange(string(_TXT("REMOVE_NETWORK ")) + Core::NumberType<uint32_t>(entry->second.Id()).Text());

                Submit(&exchange);

                if ((exchange.Wait(MaxConnectionTime) == true) && (exchange.Response() != _T("FAIL"))) {

                    _adminLock.Lock();
                    _enabled.erase(entry);
                    _adminLock.Unlock();
                }

                Revoke(&exchange);
            } else {
                _adminLock.Unlock();
            }
        }
        inline uint64_t BSSIDFromSSID(const string& SSID) const
        {
            uint64_t result = 0;
            int32_t strength(Core::NumberType<int32_t>::Min());

            _adminLock.Lock();

            NetworkInfoContainer::const_iterator index(_networks.begin());

            while (index != _networks.end()) {
                if ((index->second.SSID() == SSID) && (index->second.Signal() > strength)) {
                    strength = index->second.Signal();
                    result = index->first;
                }
                index++;
            }

            _adminLock.Unlock();

            return (result);
        }
        inline uint32_t Connect(const string& SSID)
        {

            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            EnabledContainer::iterator index(_enabled.find(SSID));

            if (index != _enabled.end()) {

                bool AP(false);
                uint64_t bssid(0);
                string value;
                uint32_t id = index->second.Id();

                _adminLock.Unlock();

                if (GetKey(id, Config::MODE, value) == Core::ERROR_NONE) {
                    uint32_t mode(Core::NumberType<uint32_t>(Core::TextFragment(value.c_str(), value.length())));

                    // Its an access point, we are done. BSSID == 0
                    AP = (mode == 2);
                }

                if (AP == false) {
                    bssid = BSSIDFromSSID(SSID);
                }

                if ((AP == true) || (bssid != 0)) {
                    result = Connect(SSID, bssid);
                } else {
                    TRACE(Trace::Information, (_T("No associated BSSID to connect to and not defined as AccessPoint. (%llu)"), bssid));
                    result = Core::ERROR_UNAVAILABLE;
                }
            } else {
                _adminLock.Unlock();
            }

            return (result);
        }
        inline uint32_t Connect(const string& SSID, const uint64_t& bssid)
        {
            class ConnectSink : public IConnectCallback {
            public:
                ConnectSink(const ConnectSink&) = delete;
                ConnectSink& operator= (const ConnectSink&) = delete;
                ConnectSink() : _signal(false, true), _result(Core::ERROR_TIMEDOUT) {}
                ~ConnectSink() override {};

            public:
                uint32_t Wait(const uint32_t waitTime) {
                    return (_signal.Lock(waitTime) == Core::ERROR_NONE ? _result : Core::ERROR_TIMEDOUT);
                }
                void Completed(const uint32_t result) override {
                    _result = result;
                    _signal.SetEvent();
                }

            private:
                Core::Event _signal;
                uint32_t _result;

            } waitSink;

            uint32_t result = Connect(&waitSink, SSID, bssid);

            if (result == Core::ERROR_NONE) {
                result = waitSink.Wait(5 * MaxConnectionTime);

                _connectRequest.Revoke(&waitSink);
            }

            return (result);
        }
        inline uint32_t Connect(IConnectCallback* sink, const string& SSID, const uint64_t bssid)
        {
            return (_connectRequest.Invoke(sink, SSID, bssid));
        }
        inline uint32_t Revoke(IConnectCallback* sink)
        {
            return (_connectRequest.Revoke(sink));
        }
        inline uint32_t Disconnect(const string& SSID)
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            EnabledContainer::iterator index(_enabled.find(SSID));

            if ((index != _enabled.end()) && (index->second.IsEnabled() == true)) {
                _adminLock.Unlock();

                result = Core::ERROR_NONE;
                CustomRequest exchange(string(_TXT("DISCONNECT")));

                Submit(&exchange);

                if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {

                    result = Core::ERROR_ASYNC_ABORTED;
                } else {
                    _statusRequest.Reset();
                }

                Revoke(&exchange);
            } else {
                if (_statusRequest.SSID() == SSID)
                    _statusRequest.Reset();
                _adminLock.Unlock();
            }

            return (result);
        }

    private:
        friend class Network;
        friend class Config;

        inline void Notify(const events value)
        {

            if (value == WPASupplicant::Controller::CTRL_EVENT_SSID_TEMP_DISABLED) {
                _connectRequest.Completed(Core::ERROR_INVALID_SIGNATURE);
            }
            else if (value == WPASupplicant::Controller::CTRL_EVENT_NETWORK_NOT_FOUND) {
                _connectRequest.Completed(Core::ERROR_UNKNOWN_KEY);
            }
            else if (value == WPASupplicant::Controller::CTRL_EVENT_CONNECTED) {
                _connectRequest.Completed(Core::ERROR_NONE);
            }

            _adminLock.Lock();

            if (_callback != nullptr) {
                _callback->Dispatch(value);
            }
            _adminLock.Unlock();
        }
        inline bool Probe(const uint16_t waitTime)
        {

            uint32_t slices(waitTime * 10);

            bool status = false;
            do {
                if (BaseClass::IsOpen() == false) {
                    BaseClass::Close(100);

                    if (BaseClass::Open(100) != Core::ERROR_NONE) {
                        break;
                    }
                }

                slices -= (slices > 5 ? 5 : slices);

                CustomRequest exchange(string(_TXT("PING")));
                Submit(&exchange);

                if ((exchange.Wait(500) == true) && (exchange.Response() == _T("PONG"))) {
                    status = true;
                }

                Revoke(&exchange);
            } while ((slices != 0) && (status == false));

            return status;
        }

        inline bool Exists(const string& SSID) const
        {

            _adminLock.Lock();

            EnabledContainer::const_iterator index(_enabled.find(SSID));

            bool result = (index != _enabled.end());

            _adminLock.Unlock();

            return (result);
        }
        inline bool IsEnabled(const string& SSID) const
        {

            _adminLock.Lock();

            EnabledContainer::const_iterator index(_enabled.find(SSID));

            bool result = ((index != _enabled.end()) && (index->second.IsEnabled() == true));

            _adminLock.Unlock();

            return (result);
        }
        inline bool IsSelected(const string& SSID) const
        {

            _adminLock.Lock();

            EnabledContainer::const_iterator index(_enabled.find(SSID));

            bool result = ((index != _enabled.end()) && (index->second.IsSelected() == true));

            _adminLock.Unlock();

            return (result);
        }
        inline bool Hidden(const string& SSID) const
        {

            bool result = false;

            _adminLock.Lock();

            // See if the given SSID is enabled.
            EnabledContainer::const_iterator entry(_enabled.find(SSID));

            if (entry != _enabled.end()) {
                // This is an existing config. Easy Piecie.
                result = entry->second.Hidden();
            }

            _adminLock.Unlock();

            return (result);
        }
        inline bool Hidden(const string& SSID, const bool value)
        {

            bool result = false;

            _adminLock.Lock();

            // See if the given SSID is enabled.
            EnabledContainer::iterator entry(_enabled.find(SSID));

            if (entry != _enabled.end()) {
                // This is an existing config. Easy Piecie.
                entry->second.Hidden(value);
                result = true;
            }

            _adminLock.Unlock();

            return (result);
        }

        inline uint32_t Enable(const string& SSID)
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            EnabledContainer::iterator index(_enabled.find(SSID));

            if ((index != _enabled.end()) && (index->second.Id() > 0)) {
                _adminLock.Unlock();

                result = Core::ERROR_NONE;

                CustomRequest exchange(string(_TXT("ENABLE_NETWORK ")) + Core::NumberType<uint32_t>(index->second.Id()).Text());

                Submit(&exchange);

                if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {

                    result = Core::ERROR_ASYNC_ABORTED;
                } else if (_networkRequest.Set() == true) {
                    // Refresh the enabled network list !!
                    Submit(&_networkRequest);
                    index->second.State(ConfigInfo::ENABLED);
                }

                Revoke(&exchange);
            } else {
                _adminLock.Unlock();
            }

            return (result);
        }
        inline uint32_t Disable(const string& SSID)
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            EnabledContainer::iterator index(_enabled.find(SSID));

            if ((index != _enabled.end()) && (index->second.Id() > 0)) {
                _adminLock.Unlock();

                result = Core::ERROR_NONE;

                CustomRequest exchange(string(_TXT("DISABLE_NETWORK ")) + Core::NumberType<uint32_t>(index->second.Id()).Text());

                Submit(&exchange);

                if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {

                    result = Core::ERROR_ASYNC_ABORTED;
                } else if (_networkRequest.Set() == true) {
                    // Refresh the enabled network list !!
                    Submit(&_networkRequest);
                    index->second.State(ConfigInfo::DISABLED);
                }

                Revoke(&exchange);
            } else {
                _adminLock.Unlock();
            }

            return (result);
        }
        inline uint32_t SetKey(const string& key, const string& value) 
        {
            uint32_t result = Core::ERROR_NONE;

            CustomRequest exchange(string(_TXT("SET ")) + key + ' ' + value);

            Submit(&exchange);

            if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {

                result = Core::ERROR_ASYNC_ABORTED;
            } 

            Revoke(&exchange);

            return (result);
        }
        inline uint32_t GetKey(const string& key, string& value) const
        {
            uint32_t result;

            CustomRequest exchange(string(_TXT("GET ")) + key);

            Submit(&exchange);

            if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {

                result = Core::ERROR_ASYNC_ABORTED;
            } 
            else {
                result = Core::ERROR_NONE;
                value = exchange.Response();
            }

            Revoke(&exchange);

            return (result);
        }
        inline uint32_t SetKey(const string& SSID, const string& key, const string& value)
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            EnabledContainer::iterator index(_enabled.find(SSID));

            if (index != _enabled.end()) {
                _adminLock.Unlock();

                result = Core::ERROR_NONE;

                CustomRequest exchange(string(_TXT("SET_NETWORK ")) + Core::NumberType<uint32_t>(index->second.Id()).Text() + ' ' + key + ' ' + value);

                Submit(&exchange);

                if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() != _T("OK"))) {

                    result = Core::ERROR_ASYNC_ABORTED;
                } else if ((key == Config::PSK) || (key == Config::PASSWORD)) {
                    // These elements are not returned upon request, Save them ourselves.
                    index->second.Secret(value);
                }

                Revoke(&exchange);
            } else {
                _adminLock.Unlock();
            }

            return (result);
        }
        inline uint32_t GetKey(const uint32_t id, const string& key, string& value) const
        {

            uint32_t result = Core::ERROR_UNKNOWN_KEY;
            CustomRequest exchange(string(_TXT("GET_NETWORK ")) + Core::NumberType<uint32_t>(id).Text() + ' ' + key);

            Submit(&exchange);

            if ((exchange.Wait(MaxConnectionTime) == false) || (exchange.Response() == _T("FAIL"))) {
                result = Core::ERROR_ASYNC_ABORTED;
            } else {
                result = Core::ERROR_NONE;
                value = exchange.Response();
            }

            Revoke(&exchange);

            return (result);
        }

        inline uint32_t GetKey(const string& SSID, const string& key, string& value) const
        {
            uint32_t result = Core::ERROR_UNKNOWN_KEY;

            _adminLock.Lock();

            EnabledContainer::const_iterator index(_enabled.find(SSID));

            if (index != _enabled.end()) {

                result = Core::ERROR_NONE;
                string secret = index->second.Secret();

                uint32_t id = index->second.Id();

                _adminLock.Unlock();

                if (((result = GetKey(id, key, value)) == Core::ERROR_NONE) && ((key == Config::PSK) || (key == Config::PASSWORD))) {

                    // These elements are not returned upon request, Save them ourselves.
                    value = secret;
                }
            } else {
                _adminLock.Unlock();
            }

            return (result);
        }
        // These methods (add/add/update) are assumed to be running in a locked context.
        // Completion of requests are running in a locked context, so oke to update maps/lists
        void Add(const uint64_t& bssid, const NetworkInfo& entry);
        void Add(const string& ssid, const bool current, const uint64_t& bssid);
        void Update(const string& status);
        void Update(const uint64_t& bssid, const string& ssid, const uint32_t id, uint32_t frequency, const int32_t signal, const uint16_t pairs, const uint32_t keys, const uint32_t throughput);
        void Update(const uint64_t& bssid, const uint32_t id, const uint32_t throughput);
        void Update(const string& ssid, const uint32_t id, const bool succeeded);
        void Reevaluate();
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize);
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize);

        void Revoke(const Request* id) const
        {
            _adminLock.Lock();

            std::list<Request*>::iterator index(std::find(_requests.begin(), _requests.end(), id));

            if (index != _requests.end()) {
                bool retrigger(index == _requests.begin());
                (*index)->Processing(false);
                _requests.erase(index);
                _adminLock.Unlock();

                if (retrigger == true) {
                    const_cast<Controller*>(this)->Trigger();
                }
            } else {
                _adminLock.Unlock();
            }
        }

        virtual void StateChange()
        {
            TRACE(Trace::Information, (_T("StateChange: %s\n"), IsOpen() ? _T("true") : _T("false")));
        }

        void Abort()
        {
            _adminLock.Lock();

            while (_requests.size() != 0) {
                Request* current = _requests.front();
                _requests.pop_front();
                current->Processing(false);
                current->Completed(EMPTY_STRING, true);
            }

            _adminLock.Unlock();
        }

        void Submit(Request* data) const
        {
            _adminLock.Lock();

            ASSERT(std::find(_requests.begin(), _requests.end(), data) == _requests.end());

            data->Processing(true);
            _requests.push_back(data);

            if (_requests.size() == 1) {
                _adminLock.Unlock();

                const_cast<Controller*>(this)->Trigger();
            } else {
#ifdef __DEBUG__
                TRACE(Trace::Information, (_T("Submit does not trigger, there are %d messages pending [%s,%s]"), static_cast<unsigned int>(_requests.size()), _requests.front()->Original().c_str(), _requests.front()->Message().c_str()));
#endif
                _adminLock.Unlock();
            }
        }

        inline bool IsSubmitted(Request* id) const
        {
            bool status = false;

            _adminLock.Lock();
            std::list<Request*>::iterator index(std::find(_requests.begin(), _requests.end(), id));
            if (index != _requests.end()) {
                _adminLock.Unlock();

                const_cast<Controller*>(this)->Trigger();
                status = true;
            } else {
                _adminLock.Unlock();
            }

            return status;
        }

    private:
        mutable Core::CriticalSection _adminLock;
        mutable std::list<Request*> _requests;
        NetworkInfoContainer _networks;
        EnabledContainer _enabled;
        uint32_t _error;
        Core::IDispatchType<const events>* _callback;
        ScanRequest _scanRequest;
        DetailRequest _detailRequest;
        NetworkRequest _networkRequest;
        StatusRequest _statusRequest;
        ConnectRequest _connectRequest;
    };
}
} // namespace WPEFramework::WPASupplicant

#endif // WPASUPPLICANT_CONTROLLER_H
