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

#ifndef SOURCE_BACKEND_H_
#define SOURCE_BACKEND_H_

#include "TVCommon.h"
#include <interfaces/ITVPlatform.h>

#include <chrono>
#include <fstream>
#include <libdvbapi/dvbdemux.h>
#include <poll.h>
#include <thread>
#include <tracing/tracing.h>

#include <gst/gst.h>
#include <glib.h>

#define BUFFER_SIZE 4096
#define DATA_OFFSET 6
#define SIZE_OFFSET 4

struct GstElementData {
    GstElement* pipeline;
    GstElement* source;
    GstElement* decoder;
    GstElement* audioQueue;
    GstElement* videoQueue;
    GstElement* audioConvert;
    GstElement* videoConvert;
    GstElement* audioSink;
    GstElement* videoSink;
};

namespace LinuxDVB {
class TvControlBackend;

class SourceBackend {
public:
    SourceBackend(SourceType, TunerData*);
    ~SourceBackend();

    TvmRc StartScanning(std::vector<uint32_t>, TVPlatform::ITVPlatform::ITunerHandler&);
    TvmRc StopScanning();
    TvmRc SetHomeTS(uint32_t);
    TvmRc Tune(TSInfo&, TVPlatform::ITVPlatform::ITunerHandler&);
    TvmRc StartFilter(uint16_t, TVPlatform::ITVPlatform::ISectionHandler*);
    TvmRc StopFilter(uint16_t);
    TvmRc StopFilters();
    TvmRc GetChannelMap(ChannelMap&);
    TvmRc GetTSInfo(TSInfoList&);
    SourceType SrcType() { return _sType; }
    bool IsScanning() { return _isScanInProgress; }
    std::vector<uint32_t>& GetFrequencyList();
    void UpdateTunerCount(uint32_t tunerCount) { _tunerCount = tunerCount; }

private:
    bool StartPlayBack(uint32_t, uint32_t, uint16_t, uint16_t, uint16_t);
    bool StopPlayBack();
    bool TuneToFrequency(uint32_t, uint32_t, struct dvbfe_handle*);
    void AtscScan(uint32_t, uint32_t);
    void MpegScan(uint32_t);
    void DvbScan();
    bool ProcessPAT(uint32_t, int32_t);
    bool ProcessPMT(int32_t, TSInfo&);
    int32_t CreateSectionFilter(uint16_t, uint8_t, bool);
    void SectionFilterThread();
    void ScanningThread(std::vector<uint32_t>, TVPlatform::ITVPlatform::ITunerHandler&);
    TvmRc SetCurrentChannel(TSInfo&, TVPlatform::ITVPlatform::ITunerHandler&);
    void SetCurrentChannelThread(TSInfo&, TVPlatform::ITVPlatform::ITunerHandler&);
    bool PopulateChannelData(uint32_t);
    void ResumeFiltering();
    bool PauseFiltering();

private:
    SourceType _sType;
    TunerData* _tunerData;
    std::thread _sectionFilterThread;
    int32_t _adapter;
    int32_t _demux;

    volatile bool _isScanStopped;
    volatile bool _isRunning;
    volatile bool _isScanInProgress;

    uint64_t _channelNo;

    std::mutex _channelChangeCompleteMutex;
    std::condition_variable_any _channelChangeCompleteCondition;
    TvmRc _channelChangeState;

    std::mutex _scanCompleteMutex;
    std::condition_variable_any _scanCompleteCondition;

    TSInfoList _channelData;

    std::vector<struct pollfd> _pollFds;
    std::map<uint16_t, uint32_t> _pollFdsMap;
    std::mutex _sectionFilterMutex;
    std::condition_variable_any _sectionFilterCondition;
    TVPlatform::ITVPlatform::ISectionHandler* _sectionHandler;
    struct dvbfe_handle* _feHandle;
    std::vector<uint32_t> _frequencyList;
    uint32_t _currentTunedFrequency;
    uint32_t _tunerCount;
    bool _playbackInProgress;

    GstElementData _gstData;
    bool _isGstreamerInitialized;
};

} // namespace LinuxDVB
#endif // SOURCE_BACKEND_H_
