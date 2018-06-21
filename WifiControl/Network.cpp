#include "Network.h"
#ifdef USE_WIFI_HAL
#include "WifiHAL.h"
#else
#include "Controller.h"
#endif

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(WPASupplicant::Network::key)

    { WPASupplicant::Network::KEY_PSK,     _TXT("PSK")     },
    { WPASupplicant::Network::KEY_EAP,     _TXT("EAP")     },
    { WPASupplicant::Network::KEY_CCMP,    _TXT("CCMP")    },
    { WPASupplicant::Network::KEY_TKIP,    _TXT("TKIP")    },
    { WPASupplicant::Network::KEY_PREAUTH, _TXT("preauth") },

ENUM_CONVERSION_END(WPASupplicant::Network::key);

ENUM_CONVERSION_BEGIN(WPASupplicant::Network::pair)

    { WPASupplicant::Network::PAIR_WEP,    _TXT("WEP")     },
    { WPASupplicant::Network::PAIR_WPA,    _TXT("WPA")     },
    { WPASupplicant::Network::PAIR_WPA2,   _TXT("WPA2")    },
    { WPASupplicant::Network::PAIR_OPEN,   _TXT("OPEN")    },
    { WPASupplicant::Network::PAIR_ESS,    _TXT("ESS")     },
    { WPASupplicant::Network::PAIR_WPS,    _TXT("WPS")     },

ENUM_CONVERSION_END(WPASupplicant::Network::pair);

ENUM_CONVERSION_BEGIN(WPASupplicant::Network::mode)

        { WPASupplicant::Network::MODE_STATION, _TXT("station") },
        { WPASupplicant::Network::MODE_AP,      _TXT("AP")      },

ENUM_CONVERSION_END(WPASupplicant::Network::mode);

namespace WPASupplicant {

string Network::BSSIDString() const {
    return _comController->BSSID(_bssid);
}

uint8_t Network::Key(const pair method) const {
    uint32_t mask = 0x1;
    uint8_t result = 0;

    if (method != 0) {
        while ((method & mask) == 0) { mask = mask << 1; result += 8; }

        // Get the proper 8 bits out of the uint 32 that make up the keys for this paring method (4 sets)
        result = ((_key >> result) & 0xFF);
    }

    return (result);
}

bool Config::IsConnected() const {
    return (_comController->Current() == _ssid);
}

bool Config::GetKey(const string& key, string& value) const {

    return (_comController->GetKey(_ssid, key, value) == Core::ERROR_NONE ? true : false);
}

bool Config::SetKey(const string& key, const string& value) {

    return (_comController->SetKey(_ssid, key, value) == Core::ERROR_NONE ? true : false);
}

bool Config::Unsecure () {
    return ( (SetKey(KEY, _T("NONE"))) &&
             (SetKey(SSIDKEY, ('\"' + _ssid + '\"'))) );
}

bool Config::Hash(const string& hash) {
    return ( (SetKey(KEY, _T("WPA-PSK"))) &&
             (SetKey(PAIR, _T("CCMP TKIP"))) &&
             (SetKey(PROTO, _T("RSN"))) &&
             (SetKey(AUTH, _T("OPEN"))) &&
             (SetKey(SSIDKEY, ('\"' + _ssid + '\"'))) &&
             (SetKey(PSK, hash)) );
}

bool Config::PresharedKey (const string& presharedKey) {
    return ( (SetKey(KEY, _T("WPA-PSK"))) &&
             (SetKey(PAIR, _T("CCMP"))) &&
             (SetKey(PROTO, _T("RSN"))) &&
             (SetKey(AUTH, _T("OPEN"))) &&
             (SetKey(SSIDKEY, ('\"' + _ssid + '\"'))) &&
             (SetKey(PSK, ('\"' + presharedKey + '\"'))) );
}

bool Config::Enterprise (const string& identity, const string& password) {
    return ( (SetKey(KEY, _T("IEEE8021X"))) &&
             (SetKey(SSIDKEY, ('\"' + _ssid + '\"'))) &&
             (SetKey(IDENTITY, identity)) &&
             (SetKey(PASSWORD, password)) &&
             (SetKey(_T("eap"), _T("PEAP"))) &&
             (SetKey(_T("phase2"), _T("auth=MSCHAPV2"))) );
}

bool Config::Hidden(const bool hidden) {
    return (_comController.IsValid() && _comController->Hidden(_ssid, hidden));
}

bool Config::Mode(const uint8_t mode) {
    Core::NumberType<uint8_t> textNumber(mode);
    return (SetKey(MODE, textNumber.Text()));
}

bool Config::IsUnsecure() const {
    string result;
    return ((GetKey(KEY, result) == true) && (result == _T("NONE")));
}

bool Config::IsWPA() const {
    string result;
    return ((GetKey(KEY, result) == true) && (result == _T("WPA-PSK")));
}

bool Config::IsEnterprise() const {
    string result;
    return ((GetKey(KEY, result) == true) && (result == _T("IEEE8021X")));
}

bool Config::IsAccessPoint() const {
    string result;
    return ((GetKey(MODE, result) == true) && (result == _T("2")));
}

bool Config::IsHidden() const {
    return ((_comController != nullptr) && (_comController->Hidden(_ssid)));
}

// Methods to apply to a Config.
bool Config::Connect() {
    return (_comController->Connect(_ssid) == Core::ERROR_NONE ? true : false);
}

bool Config::Disconnect() {
    return (_comController->Disconnect(_ssid) == Core::ERROR_NONE ? true : false);
}

bool Config::IsValid() const {
    return (_comController.IsValid() && _comController->Exists(_ssid));
}

void Config::CopyProperties (const Config& copy) {
    string value = PresharedKey();

    if (value.empty() == false) { PresharedKey(value); }

    value = Hash();

    if (value.empty() == false) { Hash(value); }

    value = Identity();

    if (value.empty() == false) { 
        string password = Password();
        Enterprise (value, password); 
    }

    Mode (Mode()); 
}

} } // WPEFramework::WPASupplicant 
