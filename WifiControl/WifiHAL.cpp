#include "Controller.h"

extern "C" {
#include <wifi_client_hal.h>
}

namespace WPEFramework {
namespace WPASupplicant {

Controller::Controller()
    :_isOperational(false)
    , _adminLock()
    , _networks()
    , _enabled()
{
    Init();
}

Controller::~Controller()
{
    Uninit();
}

void Controller::Init()
{
    int rc = RETURN_OK;
    rc = wifi_init();
    if (rc == RETURN_OK) {
        _isOperational = true;

    }
}

void Controller::Uninit()
{
    wifi_uninit();
    _isOperational = false;
}

uint32_t Controller::Scan()
{
    uint32_t result = Core::ERROR_NONE;
    int rc = RETURN_OK;

    wifi_neighbor_ap_t *neighbor_ap_array = nullptr;
    UINT uSize = 0;
    rc = wifi_getNeighboringWiFiDiagnosticResult(0, &neighbor_ap_array, &uSize);
    if (rc == RETURN_OK) {
        TRACE_L1("wifi_getNeighboringWiFiDiagnosticResult size=%d", uSize);
        for (int i = 0; i < uSize; i++) {
            wifi_neighbor_ap_t *ap = &neighbor_ap_array[i];
            //printf("\tap_SSID=%s, ap_BSSID=%s, ap_SecurityModeEnabled=%s, ap_EncryptionMode=%s\n",
            //    ap->ap_SSID, ap->ap_BSSID, ap->ap_SecurityModeEnabled, ap->ap_EncryptionMode);

            Network::pair   pairs = Network::PAIR_NONE;
            Network::key    keys  = Network::KEY_NONE;

            // ap_SecurityModeEnabled=WPA-WPA2, ap_EncryptionMode=TKIP,AES
            // ap_SecurityModeEnabled=WPA2-Enterprise, ap_EncryptionMode=TKIP,AES

            if (strstr(ap->ap_SecurityModeEnabled, "Enterprise"))
                pairs = Network::PAIR_ESS;
            else if (strstr(ap->ap_SecurityModeEnabled, "WPA2"))
                pairs = Network::PAIR_WPA2;
            else if (strstr(ap->ap_SecurityModeEnabled, "WPA"))
                    pairs = Network::PAIR_WPA;

            if (strstr(ap->ap_EncryptionMode, "TKIP"))
                keys = (Network::key) (keys | Network::KEY_TKIP);
            if (strstr(ap->ap_EncryptionMode, "AES"))
                keys = (Network::key) (keys | Network::KEY_CCMP);

            TRACE_L3("SSID=%.32s, BSSID=%s Freq=%s pairs=%d keys=%d", ap->ap_SSID, ap->ap_BSSID, ap->ap_OperatingFrequencyBand, pairs, keys);

            NetworkInfo info;
            info._ssid = ap->ap_SSID;
            info._bssid = ap->ap_BSSID;
            info._channel = ap->ap_Channel;
            info._frequency = atoi(ap->ap_OperatingFrequencyBand);
            info._signal = ap->ap_SignalStrength;
            info._pairs = pairs;
            info._keys = keys;
            _networks[info._ssid] = info;
        }
        TRACE_L1("_networks.size=%d", _networks.size());

        wifi_pairedSSIDInfo_t pairedSSIDInfo;
        memset(&pairedSSIDInfo, 0, sizeof(wifi_pairedSSIDInfo_t));
        rc = wifi_lastConnected_Endpoint(&pairedSSIDInfo);
        if (rc == RETURN_OK) {
            TRACE_L1("pairedSSIDInfo: ap_ssid='%s' ap_bssid='%s' ap_security='%s' ap_passphrase='%s'",
                pairedSSIDInfo.ap_ssid, pairedSSIDInfo.ap_bssid, pairedSSIDInfo.ap_security, pairedSSIDInfo.ap_passphrase);

            ssidCurrent = pairedSSIDInfo.ap_ssid;
            if (!ssidCurrent.empty()) {
                Create(ssidCurrent);

                string key(pairedSSIDInfo.ap_security);
                string security(pairedSSIDInfo.ap_passphrase);
                key.erase(key.find_last_not_of(" \n\r")+1);
                security.erase(security.find_last_not_of(" \n\r")+1);

                Config conf;
                Core::ProxyType<Controller> channel (Core::ProxyType<Controller>(*this));
                conf = Config (channel, ssidCurrent);
                // XXX: WPA, ES
                conf.PresharedKey(security);
            }
        }

    } else {
        TRACE(Trace::Information, ("wifi_getNeighboringWiFiDiagnosticResult FAILED"));
    }

    return result;
}

int Controller_callback(int ssidIndex, char *AP_SSID, wifiStatusCode_t *error)
{
}

uint32_t Controller::Connect(const string& SSID)
{
    uint32_t result = Core::ERROR_NONE;

    Config config (Get(SSID));
    string strMode;
    GetKey (SSID, Config::MODE, strMode);
    TRACE_L1("%s: SSID=%s PresharedKey=%s Password=%s mode=%d strMode=%s", __FUNCTION__,
        config.SSID().c_str(), config.PresharedKey().c_str(), config.Password().c_str(), config.Mode(), strMode.c_str());

    if (config.IsValid() == true) {

        char * ssid = (char *)config.SSID().c_str();
        bool   bSaveSSID = true;
        wifiSecurityMode_t mode = WIFI_SECURITY_NONE;

        if (config.IsUnsecure()) {
            mode = WIFI_SECURITY_NONE;
        }
        else if (config.IsWPA()) {
            //WIFI_SECURITY_WPA2_PSK_TKIP,
            //WIFI_SECURITY_WPA2_PSK_AES,
            mode = WIFI_SECURITY_WPA2_PSK_TKIP;
        }
        else if (config.IsEnterprise()) {
            //WIFI_SECURITY_WPA_ENTERPRISE_TKIP,
            //WIFI_SECURITY_WPA_ENTERPRISE_AES,
            //WIFI_SECURITY_WPA2_ENTERPRISE_TKIP,
            //WIFI_SECURITY_WPA2_ENTERPRISE_AES,
            mode = WIFI_SECURITY_WPA2_ENTERPRISE_TKIP;
        }
        else {
            //WIFI_SECURITY_WEP_64,
            //WIFI_SECURITY_WEP_128,
            mode = WIFI_SECURITY_WEP_128;
        }

        wifi_connectEndpoint_callback_register(Controller_callback);

        int rc = wifi_connectEndpoint(1, ssid, mode, nullptr, (char *)config.PresharedKey().c_str(),
                nullptr, bSaveSSID, nullptr, nullptr, nullptr, nullptr);
        if (rc != RETURN_OK) {
            TRACE_L1("Connect Failed rc=%d", rc);
        }
    }
    else {
        TRACE(Trace::Error, ("Config Not Found"));
    }

    return result;
}

uint32_t Controller::Disconnect(const string& SSID)
{
    wifi_disconnectEndpoint(1, (char *)SSID.c_str());
    return 0;
}

void Controller::Status()
{
    int rc = RETURN_OK;
    ULONG ulCount = 0;
    wifi_getRadioNumberOfEntries(&ulCount);
    printf("wifi_getRadioNumberOfEntries=%ld\n", ulCount);

    wifi_getSSIDNumberOfEntries(&ulCount);
    printf("SSIDNumberOfEntries=%ld\n", ulCount);

    for (int i = 0; i < ulCount; i++) {
        char ssid[64];
        *ssid = '\0';
        wifi_getSSIDName(i, ssid);
        printf("\tssid[%d] = %s\n", i, ssid);
    }
    printf("---------------------------\n");


    wifi_sta_stats_t wifiStats;
    memset(&wifiStats, 0, sizeof(wifi_sta_stats_t));
    wifi_getStats(0, &wifiStats);
    printf("sta_SSID=%s sta_BSSID=%s\n", wifiStats.sta_SSID, wifiStats.sta_BSSID);

    wifi_pairedSSIDInfo_t pairedSSIDInfo;
    memset(&pairedSSIDInfo, 0, sizeof(wifi_pairedSSIDInfo_t));
    rc = wifi_lastConnected_Endpoint(&pairedSSIDInfo);
    if (rc == RETURN_OK) {
        printf("pairedSSIDInfo: ap_ssid=%s ap_bssid=%s ap_security=%s ap_passphrase=%s\n",
                pairedSSIDInfo.ap_ssid, pairedSSIDInfo.ap_bssid, pairedSSIDInfo.ap_security, pairedSSIDInfo.ap_passphrase);
    }
}

const string& Controller::Current () const
{
    return ssidCurrent;
}

/* static */ uint64_t Controller::BSSID(const string& element) {

    uint64_t bssid = 0;

    if (element.length() >= static_cast<uint32_t>((3 * 6) - 1)) {
        // First thing we find will be 6 bytes...
        for (uint8_t pos = 0; pos < 6; pos++) {
            uint8_t msb = element[0 + (3 * pos)];
            uint8_t lsb = element[1 + (3 * pos)];
            if (::isxdigit(msb) && ::isxdigit(lsb)) {
                bssid = (bssid << 8) |
                    ((msb > '9' ? ::tolower(msb) - 'a' + 10 : msb - '0') << 4) |
                     (lsb > '9' ? ::tolower(lsb) - 'a' + 10 : lsb - '0');
            }
        }
    }
    return (bssid);
}


}
}
