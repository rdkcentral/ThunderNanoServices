#ifndef __ITUNER_H
#define __ITUNER_H

#include "EPGData.h"
#include "ITableData.h"
#include <interfaces/IStreaming.h>
#include <interfaces/ITVPlatform.h>

namespace WPEFramework {
namespace Plugin {

class ITuner {
public:
    class Config : public Core::JSON::Container {
    private:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

    public:
        Config()
            : Core::JSON::Container()
            , Dvb(true)
        {
            Add(_T("dvb"), &Dvb);
            Add(_T("tuneparameters"), &TuneParameters);
            Add(_T("frequencylist"), &FrequencyList);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::Boolean Dvb;
        Core::JSON::String TuneParameters;
        Core::JSON::String FrequencyList;
    };

private:
    ITuner(const ITuner&) = delete;
    ITuner& operator=(const ITuner&) = delete;

public:
    ITuner(TVPlatform::ITVPlatform* tvPlatform);
    virtual ~ITuner();
    static ITuner* GetInstance(TVPlatform::ITVPlatform* tvPlatform);

    const string Initialize(PluginHost::IShell*);
    void Deinitialize(PluginHost::IShell*);

    void StartScan(TVPlatform::ITVPlatform::ITunerHandler&);
    void StopScan();
    void SetCurrentChannel(const string&, TVPlatform::ITVPlatform::ITunerHandler&);
    std::string GetCurrentChannel();
    bool IsScanning();
    void Test(const string& name);
    void StoreFrequencyListInDB();
    bool StoreTSInfoInDB();

private:
    void ParseFrequencies(const std::string, std::vector<uint32_t>&);
    Core::CriticalSection _adminLock;
    PluginHost::IShell* _service;
    TVPlatform::ITVPlatform* _tvPlatform;
    Config _config;
    EPGDataBase& _epgDB;
    string _currentChannel;
    std::vector<uint32_t> _frequencyList;
};
}
}

#endif
