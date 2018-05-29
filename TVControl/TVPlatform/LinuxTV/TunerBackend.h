/*
 * Copyright (C) 2017 TATA ELXSI
 * Copyright (C) 2017 Metrological
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TUNER_BACKEND_H_
#define TUNER_BACKEND_H_

#include "TVCommon.h"
#include "SourceBackend.h"

#include <math.h>
#include <sstream>

#define DVB_ADAPTER_SCAN 6
#define CONFIGFILE "TVConfig.txt"
typedef std::map<std::string, std::string> ConfigInfo;

namespace LinuxDVB {
class TvTunerBackend {
public:
    TvTunerBackend(uint32_t, std::unique_ptr<TunerData>, bool);
    virtual ~TvTunerBackend();

    SourceType GetSrcType() { return _sType; };
    void GetSignalStrength(double*);
    TvmRc StartScanning(std::vector<uint32_t>, TVPlatform::ITVPlatform::ITunerHandler&);
    TvmRc StopScanning();
    TvmRc SetHomeTS(uint32_t);
    TvmRc Tune(TSInfo&, TVPlatform::ITVPlatform::ITunerHandler&);
    TvmRc StartFilter(uint16_t, TVPlatform::ITVPlatform::ISectionHandler*);
    TvmRc StopFilter(uint16_t);
    TvmRc StopFilters();
    TvmRc GetChannelMap(ChannelMap&);
    TvmRc GetTSInfo(TSInfoList&);
    void UpdateTunerCount(uint32_t);
    bool IsScanning();
    std::vector<uint32_t>& GetFrequencyList();

    std::unique_ptr<struct TunerData> _tunerData;

private:
    uint32_t BaseOffset(uint32_t, ChannelList);
    uint32_t FrequencyStep(uint32_t, ChannelList);
    int32_t GetSupportedSourcesTypeList(SrcTypesVector*);
    void GetAvailableSrcList(SrcTypesVector*);
    void InitializeSourceList();
    void GetSources();
    void SetModulation(std::string&);
    void ConfigureTuner(int32_t);
    void GetConfiguration();
    void PopulateFreq();

private:
    std::unique_ptr<SourceBackend> _source;
    ChannelList _channel;
    SourceType _sType;
    SourceType* _srcTypeListPtr;
    int32_t _supportedSysCount;
    SrcTypesVector _srcList; // List of src type.
    ConfigInfo _configValues;
    uint32_t _tunerIndex;
    uint32_t _tunerCount;
};

} // namespace LinuxDVB

#endif // TUNER_BACKEND_H_
