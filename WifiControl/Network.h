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

#ifndef WPASUPPLICANT_NETWORK_H
#define WPASUPPLICANT_NETWORK_H

#include "Module.h"

namespace WPEFramework {
namespace WPASupplicant {

    class Controller;

    class Network {
    public:
        class Iterator {
        private:
            Iterator& operator=(const Iterator&) = delete;

        public:
            Iterator()
                : _list()
            {
                Reset();
            }
            Iterator(const Iterator& copy)
                : _list(copy._list)
                , _index(copy._index)
                , _atHead(copy._atHead)
            {
            }
            ~Iterator()
            {
            }

        public:
            inline bool IsValid() const
            {
                return ((_atHead == false) && (_index != _list.end()));
            }
            inline void Reset()
            {
                _atHead = true;
                _index = _list.begin();
            }
            inline bool Next()
            {
                if (_atHead) {
                    _atHead = false;
                } else if (_index != _list.end()) {
                    _index++;
                }

                return (_index != _list.end());
            }
            inline Network& Current()
            {
                ASSERT(IsValid());
                return *_index;
            }
            inline const Network& Current() const
            {
                ASSERT(IsValid());
                return *_index;
            }

        private:
            friend class Controller;
            inline void Insert(const Network& element)
            {
                _list.push_back(element);
            }

        private:
            std::list<Network> _list;
            std::list<Network>::iterator _index;
            bool _atHead;
        };

    public:
        enum key {
            KEY_PSK = 0x01,
            KEY_EAP = 0x02,
            KEY_CCMP = 0x04,
            KEY_TKIP = 0x08,
            KEY_PREAUTH = 0x10,
            KEY_NONE = 0x00
        };

        enum pair {
            PAIR_WPA = 0x01,
            PAIR_WPA2 = 0x02,
            PAIR_WPS = 0x04,
            PAIR_ESS = 0x08,
            PAIR_WEP = 0x10,
            PAIR_OPEN = 0x20,
            PAIR_NONE = 0x00
        };

        enum mode {
            MODE_STATION,
            MODE_AP
        };

        inline Network()
            : _comController()
            , _bssid()
            , _frequency()
            , _signal()
            , _pair(PAIR_NONE)
            , _key()
            , _ssid()
            , _id(~0)
            , _throughput(~0)
            , _hidden(true)
        {
        }

        inline Network(
            Core::ProxyType<Controller> comController,
            const uint32_t id,
            const uint64_t& bssid,
            const uint32_t frequency,
            const int32_t signal,
            const uint16_t pair,
            const uint32_t key,
            const string& ssid,
            const uint32_t throughPut,
            const bool hidden)
            : _comController(comController)
            , _bssid(bssid)
            , _frequency(frequency)
            , _signal(signal)
            , _pair(pair)
            , _key(key)
            , _ssid(ssid)
            , _id(id)
            , _throughput(~0)
            , _hidden(hidden)
        {
        }
        inline Network(const Network& copy)
            : _comController(copy._comController)
            , _bssid(copy._bssid)
            , _frequency(copy._frequency)
            , _signal(copy._signal)
            , _pair(copy._pair)
            , _key(copy._key)
            , _ssid(copy._ssid)
            , _id(copy._id)
            , _throughput(copy._throughput)
            , _hidden(copy._hidden)
        {
        }
        inline Network& operator=(const Network& rhs)
        {
            _comController = rhs._comController;
            _bssid = rhs._bssid;
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

        bool IsValid() const { return (_comController.IsValid()); }
        uint32_t Id() const { return (_id); }
        const string& SSID() const { return _ssid; }
        const uint64_t& BSSID() const { return _bssid; }
        uint32_t Frequency() const { return _frequency; }
        int32_t Signal() const { return _signal; }
        uint16_t Pair() const { return _pair; }
        uint8_t Key(const pair method) const;
        uint32_t Throughput() const { return (_throughput); }
        bool IsHidden() const { return (_hidden); }

        string BSSIDString() const;

    private:
        Core::ProxyType<Controller> _comController;
        uint64_t _bssid;
        uint32_t _frequency;
        int32_t _signal;
        uint16_t _pair;
        uint32_t _key;
        std::string _ssid;
        uint32_t _id;
        uint32_t _throughput;
        bool _hidden;
    };

    class Config {
    public:
        static constexpr const TCHAR* BSSIDKEY = _T("bssid");
        static constexpr const TCHAR* SSIDKEY = _T("ssid");
        static constexpr const TCHAR* PSK = _T("psk");
        static constexpr const TCHAR* KEY = _T("key_mgmt");
        static constexpr const TCHAR* PAIR = _T("pairwise");
        static constexpr const TCHAR* IDENTITY = _T("identity");
        static constexpr const TCHAR* PASSWORD = _T("password");
        static constexpr const TCHAR* MODE = _T("mode");
        static constexpr const TCHAR* PROTO = _T("proto");
        static constexpr const TCHAR* AUTH = _T("auth_alg");

        enum wpa_protocol : uint8_t {
            WPA = 0x1,
            WPA2 = 0x2,
        };

    public:
        class Iterator {
        private:
            Iterator& operator=(const Iterator&) = delete;
            struct Info {
                uint32_t id;
                string ssid;
                uint8_t state;
            };

        public:
            Iterator()
                : _list()
                , _comController()
            {
                Reset();
            }
            Iterator(const Core::ProxyType<Controller>& source)
                : _list()
                , _comController(source)
            {
                Reset();
            }
            Iterator(const Iterator& copy)
                : _list(copy._list)
                , _index(copy._index)
                , _atHead(copy._atHead)
            {
            }
            ~Iterator()
            {
            }

        public:
            inline bool IsValid() const
            {
                return ((_atHead == false) && (_index != _list.end()));
            }
            inline void Reset()
            {
                _atHead = true;
                _index = _list.begin();
            }
            inline bool Next()
            {
                if (_atHead) {
                    _atHead = false;
                } else if (_index != _list.end()) {
                    _index++;
                }

                return (_index != _list.end());
            }
            uint32_t Id() const
            {
                ASSERT(IsValid());
                return (_index->id);
            }
            bool IsConnected() const
            {
                ASSERT(IsValid());
                return (_index->state > 1);
            }
            inline Config Current() const
            {
                ASSERT(IsValid());
                return (Config(_comController, _index->ssid));
            }

        private:
            friend class Controller;

            inline void Insert(const string& ssid, const uint32_t id, const bool enabled, const bool selected)
            {
                ASSERT(_comController.IsValid() == true);

                Info newElement;
                newElement.id = id;
                newElement.ssid = ssid;
                newElement.state = (enabled == false ? 0 : (selected == false ? 1 : 2));

                _list.push_back(newElement);
            }

        private:
            std::list<Info> _list;
            std::list<Info>::iterator _index;
            bool _atHead;
            Core::ProxyType<Controller> _comController;
        };

    public:
        Config()
            : _comController()
        {
        }
        Config(const Core::ProxyType<Controller>& controller, const string& ssid)
            : _comController(controller)
            , _ssid(ssid)
        {
            ASSERT(_comController.IsValid() == true);
        }
        Config(const Config& copy)
            : _comController(copy._comController)
            , _ssid(copy._ssid)
        {
            ASSERT(_comController.IsValid() == true);
            CopyProperties(copy);
        }
        ~Config()
        {
        }

        Config& operator=(const Config& rhs)
        {
            _comController = rhs._comController;
            _ssid = rhs._ssid;

            CopyProperties(rhs);

            return (*this);
        }

    public:
        bool IsValid() const;
        bool IsConnected() const;
        bool IsUnsecure() const;
        bool IsWPA() const;
        bool IsEnterprise() const;
        bool IsAccessPoint() const;
        bool IsHidden() const;
        uint8_t Protocols() const; 

        // Methods to apply to a network.
        bool Connect();
        bool Disconnect();

        bool Mode(const uint8_t mode);
        bool Hidden(const bool hidden);
        bool Unsecure();
        bool Hash(const string& hash);
        bool PresharedKey(const string& presharedKey);
        bool Enterprise(const string& identity, const string& password);
        bool Protocols(const uint8_t protocolFlags);  


        inline const string& SSID() const
        {
            return (_ssid);
        }
        inline string PresharedKey() const
        {
            string result;
            if (GetKey(PSK, result) == true) {
                result = ((result[0] == '\"') && (result.length() > 2) ? string(&(result.c_str()[1]), result.length() - 2) : _T(""));
            }

            return (result);
        }
        inline string Hash() const
        {
            string result;
            if (GetKey(PSK, result) == true) {
                result = ((result[0] != '\"') && (result.length() > 0) ? result : _T(""));
            }

            return (result);
        }
        inline string Identity() const
        {
            string result;
            return (GetKey(IDENTITY, result) ? result : _T(""));
        }
        inline string Password() const
        {
            string result;
            return (GetKey(PASSWORD, result) ? result : _T(""));
        }
        inline uint8_t Mode() const
        {
            string result;
            return (GetKey(MODE, result) ? Core::NumberType<uint8_t>(result.c_str(), result.length()).Value() : 0);
        }

    protected:
        bool GetKey(const string& key, string& value) const;
        bool SetKey(const string& key, const string& value);

        void CopyProperties(const Config& copy);

    private:
        Core::ProxyType<Controller> _comController;
        string _ssid;
    };
}
} // namespace WPEFramework::WPASupplicant

#endif // WPASUPPLICANT_NETWORK_H
