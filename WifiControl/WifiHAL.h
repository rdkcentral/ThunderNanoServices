#ifndef WIFIHAL_H
#define WIFIHAL_H

#include "Module.h"
#include "Network.h"

namespace WPEFramework {
namespace WPASupplicant {   // XXX: Change ???

class Controller {
private:
    struct NetworkInfo {    // XXX: struct -> class
        std::string _ssid;
        std::string _bssid;
        uint32_t _channel;
        uint32_t _frequency;
        int32_t  _signal;
        Network::pair _pairs;
        Network::key _keys;
    };

    typedef std::map<const string, string>  KeyValue;
    struct ConfigInfo {
    public:
        inline bool Hidden() const {
            return (_hidden);
        }
        inline void Hidden (const bool state) {
            _hidden = state;
        }

    //private:
        std::string _ssid;
        string _secret;
        bool _hidden;
        KeyValue _keyvalue;
    };

    typedef std::map<const string, NetworkInfo>  NetworkInfoContainer;
    typedef std::map<const string, ConfigInfo>  EnabledContainer;

    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

public:
    static Core::ProxyType<Controller> Create () {
       return (Core::ProxyType<Controller>::Create());
    }

    Controller();
    virtual ~Controller();
    void Init();
    void Uninit();
    uint32_t Scan();
    uint32_t Connect(const string& SSID);
    uint32_t Disconnect(const string& SSID);

    inline Network::Iterator Networks() {
        Network::Iterator result;
        Core::ProxyType<Controller> channel (Core::ProxyType<Controller>(*this));

        _adminLock.Lock();

        for (NetworkInfoContainer::iterator it=_networks.begin(); it!=_networks.end(); ++it) {
            //NetworkInfo &info = it->second;
            //printf("\tSSID=%32s, BSSID=%s pairs=%d keys=%d\n", info._ssid.c_str(), info._bssid.c_str(), info._pairs, info._keys);
            result.Insert(Network(channel,
                static_cast<uint32_t>(~0),     // XXX: ??? id
                BSSID(string(it->second._bssid)),
                it->second._channel,
                it->second._signal,
                it->second._pairs,
                it->second._keys,
                it->second._ssid,
                0,                              // XXX: ??? Throughput - ap_BasicDataTransferRates
                false));
        }

        _adminLock.Unlock();
        return (result);
    }

    inline Config::Iterator Configs() { TR();
        Core::ProxyType<Controller> channel (Core::ProxyType<Controller>(*this));
        Config::Iterator result(channel);

        _adminLock.Lock();

        EnabledContainer::const_iterator index (_enabled.begin());
        while (index != _enabled.end()) {
            uint32_t id = ~0;
            bool isSelected = ssidCurrent.compare(index->first) == 0;
            //result.Insert(index->first, index->second.Id(), index->second.IsEnabled(), index->second.IsSelected());
            result.Insert(index->first, id, 1, isSelected);
            TRACE_L2("%s: Inserting %s isSelected=%d", __FUNCTION__, index->first.c_str(), isSelected);

            index++;
        }

        _adminLock.Unlock();

        result.Reset();

        return (result);

    }

    Config Create(const string& SSID) { TR();
        Config result;
        Core::ProxyType<Controller> channel (Core::ProxyType<Controller>(*this));

        _adminLock.Lock();
        EnabledContainer::iterator entry (_enabled.find(SSID));

        result = Config (channel, SSID);
        if (entry == _enabled.end()) {
            ConfigInfo config;
            config._ssid = SSID;

            NetworkInfoContainer::iterator index(_networks.find(SSID));
            if (index != _networks.end()) {
                config._hidden = false;
            } else {

                config._hidden = true;
            }
            _enabled[SSID] = config;
        }
        _adminLock.Unlock();

        return (result);

    }

    void Destroy(const string& SSID) {

        _adminLock.Lock();
        EnabledContainer::iterator entry (_enabled.find(SSID));

        if (entry != _enabled.end()) {
            _enabled.erase(entry);
        }

        _adminLock.Unlock();
    }

    Config Get(const string& SSID) {
         Config result;

        _adminLock.Lock();

        EnabledContainer::iterator entry (_enabled.find(SSID));

        if (entry != _enabled.end()) {
            result = Config (Core::ProxyType<Controller>(*this), SSID);
            printf("%s: Config Found ssid=%s\n", __FUNCTION__, SSID.c_str());
        }
        else {
            printf("%s: Config Not Found ssid=%s\n", __FUNCTION__, SSID.c_str());
        }

        _adminLock.Unlock();

        return (result);
    }

    inline uint32_t SetKey (const string& SSID, const string& key, const string& value) {

        uint32_t result = Core::ERROR_NONE;

        TRACE_L2("%s: SSID=%s key=%s value=%s", __FUNCTION__, SSID.c_str(), key.c_str(), value.c_str());

       _adminLock.Lock();

        EnabledContainer::iterator index(_enabled.find(SSID));

        if (index != _enabled.end()) {
            index->second._keyvalue[key] = value;
            if ((key == Config::PSK) || (key == Config::PASSWORD)) {
                    index->second._secret = value;
            }
        }
        else {
            TRACE(Trace::Error, ("%s: '%s' Not Enabled", __FUNCTION__, SSID.c_str()));
        }

        _adminLock.Unlock();

        return (result);
    }

    inline uint32_t GetKey (const uint32_t id, const string& key, string& value) const {
        uint32_t result = Core::ERROR_NONE;

        return (result);
    }

    inline uint32_t GetKey (const string& SSID, const string& key, string& value) const {

        uint32_t result = Core::ERROR_NONE;
       _adminLock.Lock();

        EnabledContainer::const_iterator index(_enabled.find(SSID));

        if (index != _enabled.end()) {
            if ((key == Config::PSK) || (key == Config::PASSWORD)) {
                value = index->second._secret;
            }
            else {
                //const KeyValue &kv(index->second._keyvalue);
                KeyValue::const_iterator it(index->second._keyvalue.find(key));
                if (it != index->second._keyvalue.end()) {
                    value = it->second;
                }
            }
        }
        else {
            TRACE(Trace::Error, ("%s: '%s' Not Enabled", __FUNCTION__, SSID.c_str()));
        }
        TRACE_L2("%s:%d SSID=%s key=%s value=%s", __FUNCTION__, __LINE__, SSID.c_str(), key.c_str(), value.c_str());

        _adminLock.Unlock();

        return (result);
    }

    inline bool Hidden(const string& SSID) const {

         bool result = false;

        _adminLock.Lock();

        EnabledContainer::const_iterator entry(_enabled.find(SSID));

        if (entry != _enabled.end()) {
            result = entry->second.Hidden();
        }

        _adminLock.Unlock();

        return (result);
    }

    inline bool Hidden(const string& SSID, const bool value) {

         bool result = false;

        _adminLock.Lock();

        EnabledContainer::iterator entry (_enabled.find(SSID));

        if (entry != _enabled.end()) {
            entry->second.Hidden(value);
            result = true;
        }

        _adminLock.Unlock();

        return (result);
    }

    inline bool Exists(const string& SSID) const {

        _adminLock.Lock();

        EnabledContainer::const_iterator index(_enabled.find(SSID));

        bool result = (index != _enabled.end());

        _adminLock.Unlock();

        return (result);
    }

    inline bool IsOperational () const {
        return (_isOperational);
    }

    inline bool IsScanning () const {
        return (false);
    }

    inline uint32_t Debug(const uint32_t level) {
        uint32_t result = Core::ERROR_NONE;
        return result;
    }

    const string& Current () const;
    static uint64_t BSSID(const string& bssid);
    static string BSSID(const uint64_t& bssid);

private:
    void Status();

private:
    mutable Core::CriticalSection _adminLock;
    NetworkInfoContainer _networks;
    EnabledContainer _enabled;
    bool _isOperational;
    string ssidCurrent;
};

} }

#endif
