#ifndef LINUXTVCONTROL_H
#define LINUXTVCONTROL_H

#include "TVCommon.h"
#include "TunerBackend.h"
#include <interfaces/ITVPlatform.h>

#include <libudev.h>
namespace TVPlatform {

class TVPlatformImplementation : public ITVPlatform {
public:
    TVPlatformImplementation();
    ~TVPlatformImplementation();

    TvmRc Init() {}
    TvmRc Deinit() {}
    TvmRc Tune(uint32_t) {}
    TvmRc Tune(TSInfo&, ITunerHandler&);
    TvmRc Scan(std::vector<uint32_t>, ITunerHandler&);
    TvmRc StopScanning();
    TvmRc GetChannelMap(ChannelMap&);
    TvmRc GetTSInfo(TSInfoList&);
    TvmRc Disconnect() {}
    TvmRc SetHomeTS(uint32_t, uint32_t);
    TvmRc StartFilter(uint16_t, uint8_t, ISectionHandler*);
    TvmRc StopFilter(uint16_t, uint8_t);
    TvmRc StopFilters();
    std::vector<uint32_t>& GetFrequencyList();
    void SetDbs(bool isDvb) { _isDvb = isDvb; }
    void SetTuneParameters(const std::string&) {}
    bool IsScanning();

private:
    LinuxDVB::TvTunerBackend* GetTuner(bool);
    void TunerChangedListener();
    void InitializeTuners();
    void CreateTunerId(int32_t, int32_t, std::string&);
    void UpdateTunerList(std::string&, TunerChangedOperation);

    uint32_t _tunerCount;
    std::thread _tunerThread;
    bool _isRunning;
    bool _isDvb;
    std::vector<std::unique_ptr<LinuxDVB::TvTunerBackend> > _tunerList;
    LinuxDVB::TvTunerBackend* _acquiredTuner;
    bool _isStreaming;
};

} // namespace TVPlatform

#endif
