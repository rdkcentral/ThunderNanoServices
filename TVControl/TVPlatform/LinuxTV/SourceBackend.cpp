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
#define crc32 crc32_func
#include "Module.h"
#undef crc32
#include "SourceBackend.h"

#include <libucsi/atsc/section.h>
#include <libucsi/dvb/section.h>

using namespace WPEFramework;

namespace LinuxDVB {

static void OnPadAdded(GstElement* element, GstPad* pad, GstElementData* data)
{
    const gchar* newPadType = gst_structure_get_name(gst_caps_get_structure(gst_pad_query_caps(pad, nullptr), 0));
    GstPad* sinkPad = nullptr;
    if (g_str_has_prefix (newPadType, "audio/x-raw"))
        sinkPad = gst_element_get_static_pad(data->audioQueue, "sink");
    else if (g_str_has_prefix (newPadType, "video/x-raw"))
        sinkPad = gst_element_get_static_pad(data->videoQueue, "sink");

    if (!gst_pad_is_linked(sinkPad)) {
        g_print("Dynamic pad created, linking audioQueue/videoQueue\n");
        GstPadLinkReturn ret = gst_pad_link(pad, sinkPad);
        if (GST_PAD_LINK_FAILED (ret))
            g_print ("Link failed.\n");
        else
            g_print ("Link succeeded.\n");
    }
    gst_object_unref(sinkPad);
}

SourceBackend::SourceBackend(SourceType type, TunerData* tunerData)
    : _sType(type)
    , _tunerData(tunerData)
    , _isScanStopped(false)
    , _isScanInProgress(false)
    , _isRunning(true)
    , _currentTunedFrequency(0)
    , _tunerCount(0)
    , _feHandle(nullptr)
    , _isGstreamerInitialized(false)
    , _sectionHandler(nullptr)
{
    _adapter = stoi(_tunerData->tunerId.substr(0, _tunerData->tunerId.find(":")));
    _demux = stoi(_tunerData->tunerId.substr(_tunerData->tunerId.find(":") + 1));

    _sectionFilterThread = std::thread(&SourceBackend::SectionFilterThread, this);

    if (!gst_init_check(nullptr, NULL, NULL)) {
        TRACE(Trace::Error, (_T("Gstreamer initialization failed.")));
        return;
    }
    _gstData.pipeline = gst_pipeline_new("pipeline");
    _gstData.source = gst_element_factory_make("dvbsrc", "source");
    _gstData.decoder = gst_element_factory_make("decodebin", "decoder");
    _gstData.audioQueue = gst_element_factory_make("queue","audioqueue");
    _gstData.videoQueue = gst_element_factory_make("queue","videoqueue");
    _gstData.videoConvert = gst_element_factory_make("videoconvert", "videoconv");
    _gstData.audioConvert = gst_element_factory_make("audioconvert", "audioconv");
    _gstData.videoSink = gst_element_factory_make("autovideosink", "videosink");
    _gstData.audioSink = gst_element_factory_make("autoaudiosink", "audiosink");

    if (!_gstData.pipeline || !_gstData.source || !_gstData.decoder || !_gstData.audioQueue || !_gstData.videoQueue || !_gstData.videoConvert
        || !_gstData.audioConvert || !_gstData.videoSink || !_gstData.audioSink) {
        TRACE(Trace::Error, (_T("One gstreamer element could not be created.")));
        return;
    }
    gst_bin_add_many(GST_BIN(_gstData.pipeline), _gstData.source, _gstData.decoder, _gstData.audioQueue, _gstData.audioConvert, _gstData.audioSink
        , _gstData.videoQueue, _gstData.videoConvert, _gstData.videoSink, nullptr);

    if (!gst_element_link(_gstData.source, _gstData.decoder) || !gst_element_link_many(_gstData.audioQueue, _gstData.audioConvert, _gstData.audioSink, nullptr)
        || !gst_element_link_many(_gstData.videoQueue, _gstData.videoConvert, _gstData.videoSink, nullptr)) {
        TRACE(Trace::Error, (_T("Gstreamer link error.")));
        return;
    }
    if (g_signal_connect(_gstData.decoder, "pad-added", G_CALLBACK(OnPadAdded), &_gstData) && g_signal_connect(_gstData.decoder, "pad-added", G_CALLBACK(OnPadAdded), &_gstData))
        _isGstreamerInitialized = true;
}

SourceBackend::~SourceBackend()
{
    TRACE(Trace::Information, (_T("~SourceBackend")));
    _isRunning = false;
    _isScanInProgress = false;
    if (_channelData.size())
        _channelData.clear();
    gst_object_unref(GST_OBJECT (_gstData.pipeline));

    _sectionFilterMutex.lock();
    _sectionFilterCondition.notify_all();
    _sectionFilterMutex.unlock();

    _sectionFilterThread.join();
    TRACE(Trace::Information, (_T("~SourceBackend")));
}

void SourceBackend::SectionFilterThread()
{
    uint8_t siBuf[BUFFER_SIZE];
    while (_isRunning) {
        if (_pollFds.empty()) {
            _sectionFilterMutex.lock();
            TRACE(Trace::Information, (_T("waiting for datawriting to start")));
            _sectionFilterCondition.wait(_sectionFilterMutex);
            _sectionFilterMutex.unlock();
        }
        memset(siBuf, 0, sizeof(siBuf));
        int32_t count = poll(_pollFds.data(), _pollFds.size(), 400);
        if (count < 0) {
            TRACE(Trace::Error, (_T("Poll error")));
            break;
        }
        if (!count) {
            TRACE(Trace::Information, (_T("Poll timeout")));
            continue;
        }
        int32_t size;
        for (size_t k = 0; k < _pollFds.size() && _isRunning; k++) {
            if (_pollFds[k].revents & (POLLIN | POLLPRI)) {
                if ((size = read(_pollFds[k].fd, siBuf + DATA_OFFSET, sizeof(siBuf))) < 0) {
                    TRACE(Trace::Error, (_T("Error calling read()")));
                    return;
                }
                memcpy(siBuf, &_currentTunedFrequency, 4);
                memcpy(siBuf + SIZE_OFFSET, &size, 2);
                TRACE(Trace::Information, (_T("Size = %d"), size));
                _sectionHandler->SectionDataCB(std::string((const char*)siBuf, size));
                break;
            }
        }
    }
}

TvmRc SourceBackend::SetHomeTS(uint32_t frequency)
{
    TRACE(Trace::Information, (_T("SetHomeTS")));
    uint32_t modulation = _tunerData->modulation;
    if (!_feHandle)
        _feHandle = OpenFE(_tunerData->tunerId);
    if (_feHandle) {
        if (frequency != _currentTunedFrequency)
            StopFilters();

        _currentTunedFrequency = frequency;
        if (TuneToFrequency(frequency, modulation, _feHandle))
            return TvmSuccess;
    }
    return TvmError;
}

TvmRc SourceBackend::StopFilter(uint16_t pid)
{
    uint32_t index;
    for (auto& p : _pollFdsMap) {
        if (pid == p.first) {
            index = p.second;
            TRACE(Trace::Information, (_T("Clear Pid = %d"), pid));
            _pollFds.erase(_pollFds.begin() + index);
            _pollFdsMap.erase(_pollFdsMap.find(pid));
        }
    }
}

TvmRc SourceBackend::StopFilters()
{
    if (_pollFds.size()) {
        for (auto& pollFd : _pollFds) {
            dvbdemux_stop(pollFd.fd);
            close(pollFd.fd);
        }
        _pollFds.clear();
    }
    _pollFdsMap.clear();
}

TvmRc SourceBackend::Tune(TSInfo& tsInfo,  TVPlatform::ITVPlatform::ITunerHandler& tunerHandler)
{
    TRACE(Trace::Information, (_T("tune freq = %u, vidPid = %" PRIu16 ", audioPid = %" PRIu16 ", pmtPid= %" PRIu16 ""), tsInfo.frequency, tsInfo.videoPid, tsInfo.audioPid, tsInfo.pmtPid));
    return SetCurrentChannel(tsInfo, tunerHandler);
}

TvmRc SourceBackend::StartFilter(uint16_t pid, TVPlatform::ITVPlatform::ISectionHandler* pSectionHandler)
{
    if (_pollFdsMap.count(pid) > 0) {
        TRACE(Trace::Information, (_T("PID already exists")));
        return TvmError;
    }

    struct pollfd pollFd;
    pollFd.events = POLLIN | POLLERR |POLLPRI;
    pollFd.fd = CreateSectionFilter(pid, 0, 0);
    TRACE(Trace::Information, (_T("Pushed into poll Fds : pid = 0x%02x"), pid));
    _pollFds.push_back(pollFd);
    _pollFdsMap.insert(std::pair<uint16_t, uint32_t>(pid, (_pollFds.size() - 1)));

    if (!_sectionHandler)
        _sectionHandler = pSectionHandler;

    _sectionFilterMutex.lock();
    _sectionFilterCondition.notify_all();
    _sectionFilterMutex.unlock();
}

TvmRc SourceBackend::StartScanning(std::vector<uint32_t> freqList, TVPlatform::ITVPlatform::ITunerHandler& tunerHandler)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    TvmRc ret = TvmError;
    std::thread th(&SourceBackend::ScanningThread, this, freqList, std::ref(tunerHandler));
    th.detach();
    return TvmSuccess;
}

TvmRc SourceBackend::GetChannelMap(ChannelMap& chanMap)
{
    TvmRc rc = TvmSuccess;
    for (auto& channel : _channelData) {
        ChannelDetails chan;
        chan.programNumber = channel.programNumber;
        chan.frequency = channel.frequency;
        if (channel.videoPid)
            chan.type = ChannelDetails::Normal;
        else if (channel.audioPid)
            chan.type = ChannelDetails::Radio;
        else
            chan.type = ChannelDetails::Data;
        if (chan.type != ChannelDetails::Data)
            chanMap.push_back(chan);
    }
    return rc;
}

std::vector<uint32_t>& SourceBackend::GetFrequencyList()
{
    return _frequencyList;
}

void SourceBackend::ResumeFiltering()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    if (!_pollFds.size())
        return;
    if (_currentTunedFrequency && !_playbackInProgress)
        SetHomeTS(_currentTunedFrequency); //Resetting to the last tuned channel after scan.
    for (auto& pollFd : _pollFds)
        dvbdemux_start(pollFd.fd);
    TRACE(Trace::Information, (string(__FUNCTION__)));
}

bool SourceBackend::PauseFiltering()
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    if (!_pollFds.size())
        return false;

    for (auto& pollFd : _pollFds)
        dvbdemux_stop(pollFd.fd);
    return true;
}

void SourceBackend::ScanningThread(std::vector<uint32_t> freqList, TVPlatform::ITVPlatform::ITunerHandler& tunerHandler)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    _isScanInProgress = true;

    if (_channelData.size())
        _channelData.clear();

    PauseFiltering();

    if (_playbackInProgress) {
        StopPlayBack();
        _channelNo = 0;
    }
    if (!_feHandle) {
        _feHandle = OpenFE(_tunerData->tunerId);
    }
    if (_feHandle) {
        TRACE(Trace::Information, (string(__FUNCTION__)));
        if (_frequencyList.size())
            _frequencyList.clear();

        std::vector<uint32_t> frequencyList;
        if (freqList.size())
            frequencyList = freqList;
        else
            frequencyList = _tunerData->frequency;

        uint32_t modulation = _tunerData->modulation;
        for (auto& frequency : frequencyList) {
            if (TuneToFrequency(frequency, modulation, _feHandle)) {
                TRACE(Trace::Information, (string(__FUNCTION__)));
                _frequencyList.push_back(frequency);
                switch (_sType) {
                case Atsc:
                case AtscMH:
                    AtscScan(frequency, modulation);
                    break;
                case DvbT:
                case DvbT2:
                case DvbC:
                case DvbC2:
                case DvbS:
                case DvbS2:
                case DvbH:
                case DvbSh:
                    DvbScan();
                    break;
                default:
                    TRACE(Trace::Error, (_T("Type Not supported")));
                    break;
                }
            }
            if (_isScanStopped) {
                tunerHandler.ScanningStateChanged(Stopped);
                break;
            }
        }
        if (_isScanStopped)
            _isScanStopped = false;
        else
            tunerHandler.ScanningStateChanged(Completed);
    }
    TRACE(Trace::Information, (string(__FUNCTION__)));
    ResumeFiltering();
    _isScanInProgress = false;
}

bool SourceBackend::ProcessPMT(int32_t pmtFd, TSInfo& channel)
{
    int32_t size;
    uint8_t siBuf[4096];

    TRACE(Trace::Information, (string(__FUNCTION__)));
    // Read the section.
    if ((size = read(pmtFd, siBuf, sizeof(siBuf))) < 0)
        return false;

    TRACE(Trace::Information, (string(__FUNCTION__)));
    // Parse section.
    struct section* section = section_codec(siBuf, size);
    if (!section)
        return false;

    TRACE(Trace::Information, (string(__FUNCTION__)));
    // Parse section_ext.
    struct section_ext* sectionExt = section_ext_decode(section, 0);
    if (!sectionExt)
        return false;

    TRACE(Trace::Information, (string(__FUNCTION__)));
    // Parse PMT.
    struct mpeg_pmt_section* pmt = mpeg_pmt_section_codec(sectionExt);
    if (!pmt)
        return false;

    TRACE(Trace::Information, (string(__FUNCTION__)));
    struct mpeg_pmt_stream* curStream;
    mpeg_pmt_section_streams_for_each(pmt, curStream)
    {
        TRACE(Trace::Information, (_T("stream_type : %x pid : %x"), curStream->stream_type, curStream->pid));
        if ((!channel.videoPid) && (curStream->stream_type == 0X02))
            channel.videoPid = curStream->pid;
        if ((!channel.audioPid) && (curStream->stream_type == 0X81))
            channel.audioPid = curStream->pid;
    }
}

TvmRc SourceBackend::StopScanning()
{
    _isScanStopped = true;
    return TvmSuccess;
}

bool SourceBackend::StartPlayBack(uint32_t frequency, uint32_t modulation, uint16_t pmtPid, uint16_t videoPid, uint16_t audioPid)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    if (!_isGstreamerInitialized) {
        TRACE(Trace::Error, (_T("Gstreamer not initialized.")));
        return false;
    }
    char pidSet[20];
    if ((!pmtPid)&& (!videoPid) && (!audioPid)) {
        TRACE(Trace::Error, (_T("Invalid pid cannot do playback")));
        return false;
    }
    snprintf(pidSet, 20, "%d:%d:%d", pmtPid, videoPid, audioPid);
    g_object_set(G_OBJECT(_gstData.source), "frequency", frequency,
        "adapter", _adapter,
        "delsys", 11, // delivery system : atsc
        "modulation", 7, // modulation : 8vsb
        "pids", pidSet, nullptr);

    TRACE(Trace::Information, (_T("Starting playback.")));
    gst_element_set_state(_gstData.pipeline, GST_STATE_PLAYING);
    _playbackInProgress = true;
    return true;
}

bool SourceBackend::StopPlayBack()
{
    if (!_isGstreamerInitialized) {
        TRACE(Trace::Error, (_T("Gstreamer not initialized.")));
        return false;
    }
    TRACE(Trace::Information, (_T("Stopping playback.")));
    gst_element_set_state(_gstData.pipeline, GST_STATE_NULL);
    _playbackInProgress = false;
    return true;
}

TvmRc SourceBackend::SetCurrentChannel(TSInfo& tsInfo, TVPlatform::ITVPlatform::ITunerHandler& tunerHandler)
{
    TRACE(Trace::Information, (_T("Set Channel invoked")));
    TvmRc ret = TvmError;
    _channelChangeCompleteMutex.lock();
    _channelChangeState = TvmError;
    _channelChangeCompleteMutex.unlock();
    if (_channelNo !=  tsInfo.programNumber) {
        _channelNo = tsInfo.programNumber;
        std::thread th(&SourceBackend::SetCurrentChannelThread, this, std::ref(tsInfo), std::ref(tunerHandler));
        th.detach();
        _channelChangeCompleteMutex.lock();
        _channelChangeCompleteCondition.wait(_channelChangeCompleteMutex);
        _channelChangeCompleteMutex.unlock();
    }
    return _channelChangeState;
}

void SourceBackend::SetCurrentChannelThread(TSInfo& tsInfo, TVPlatform::ITVPlatform::ITunerHandler& tunerHandler)
{
    TRACE(Trace::Information, (_T("Tune to Channel(program number):: %" PRIu64 ""), _channelNo));
    PauseFiltering();

    if (_feHandle) {
        dvbfe_close(_feHandle);
        _feHandle = nullptr;
    }

    if (_playbackInProgress)
        StopPlayBack();

    uint32_t modulation = _tunerData->modulation;
    if (StartPlayBack(tsInfo.frequency, modulation, tsInfo.pmtPid, tsInfo.videoPid, tsInfo.audioPid)) {
        _channelChangeState = TvmSuccess;
        if (_tunerCount == 1) {
            if (_currentTunedFrequency != tsInfo.frequency) {
                _currentTunedFrequency = tsInfo.frequency;
                StopFilters();
                tunerHandler.StreamingFrequencyChanged(_currentTunedFrequency);
            } else
                ResumeFiltering();
        }
    }
    _channelChangeCompleteMutex.lock();
    _channelChangeCompleteCondition.notify_all();
    _channelChangeCompleteMutex.unlock();
}

void SourceBackend::DvbScan()
{
}

bool SourceBackend::TuneToFrequency(uint32_t frequency, uint32_t modulation, struct dvbfe_handle* feHandle)
{
    struct dvbfe_info feInfo;
    dvbfe_get_info(feHandle, DVBFE_INFO_FEPARAMS, &feInfo, DVBFE_INFO_QUERYTYPE_IMMEDIATE, 0);

    TRACE(Trace::Information, (string(__FUNCTION__)));
    switch (_sType) {
    case Atsc:
    case AtscMH:
        if (DVBFE_TYPE_ATSC != feInfo.type) {
            TRACE(Trace::Error, (_T("Only ATSC frontend supported currently")));
            return false;
        }
        switch (modulation) {
        case DVBFE_ATSC_MOD_VSB_8:
            feInfo.feparams.frequency = frequency;
            feInfo.feparams.inversion = DVBFE_INVERSION_AUTO;
            feInfo.feparams.u.atsc.modulation = DVBFE_ATSC_MOD_VSB_8;
            break;
        default:
            TRACE(Trace::Error, (_T("Modulation not supported")));
            return false;
        }
        break;
    default:
        TRACE(Trace::Information, (_T("Only Atsc supported")));
        return false;
    }
    TRACE(Trace::Information, (_T("tuning to %u Hz, please wait..."), frequency));

    if (dvbfe_set(feHandle, &feInfo.feparams, 3 * 1000)) {
        TRACE(Trace::Error, (_T("Cannot lock to %u Hz in 3 seconds"), frequency));
        return false;
    }
    TRACE(Trace::Information, (_T("tuner locked.")));
    return true;
}

#define POLL_MAX_TIMEOUT 10
void SourceBackend::MpegScan(uint32_t frequency)
{
    int32_t patFd = -1;
    if ((patFd = CreateSectionFilter(PidPat, TablePat, 1)) < 0) {
        TRACE(Trace::Error, (_T("Failed to create PAT section filter.")));
        return;
    }
    struct pollfd pollFd;
    pollFd.fd = patFd; // PAT
    pollFd.events = POLLIN | POLLPRI | POLLERR;
    bool flag = true;
    int32_t timeout = 0;

    while (flag && (timeout < POLL_MAX_TIMEOUT)) {
        int32_t count = poll(&pollFd, 1, 100);
        if (!count) {
            TRACE(Trace::Information, (_T("Poll fd status pollFd = %d"), pollFd.revents));
            timeout++;
            continue;
        }

        timeout = 0;
        if (count < 0) {
            TRACE(Trace::Error, (_T("Poll error")));
            break;
        }
        if (pollFd.revents & (POLLIN | POLLPRI))  {
            TRACE(Trace::Information, (_T("Got PAT data parse it")));
            if (ProcessPAT(frequency, pollFd.fd)) {
                flag = false;
            } else
                TRACE(Trace::Error, (_T("Failed to get PAT data")));
        }
    }
    if (patFd != -1) {
        dvbdemux_stop(patFd);
        close(patFd);
    }
}

void SourceBackend::AtscScan(uint32_t frequency, uint32_t modulation)
{
    switch (modulation) {
    case DVBFE_ATSC_MOD_VSB_8:
        if (!PopulateChannelData(frequency))
            TRACE(Trace::Error, (_T("Failed to populate channel data")));
        break;
    default:
        TRACE(Trace::Error, (_T("Modulation not supported")));
        return;
    }
}

bool SourceBackend::PopulateChannelData(uint32_t frequency)
{
    MpegScan(frequency);
    for (auto& channel : _channelData) {
        TRACE(Trace::Information, (_T("LCN:%d, pmtPid:%d, videoPid:%d, audioPid:%d, frequency : %u"), channel.programNumber, channel.pmtPid, channel.videoPid, channel.audioPid, frequency));
    }
    return true;
}

int32_t SourceBackend::CreateSectionFilter(uint16_t pid, uint8_t tableId, bool isMpegTable)
{
    int32_t demuxFd = -1;

    // Open the demuxer.
    if ((demuxFd = dvbdemux_open_demux(_adapter, _demux, 0)) < 0)
        return -1;

    // Create a section filter.
    uint8_t filter[18];
    memset(filter, 0, sizeof(filter));
    uint8_t mask[18];
    memset(mask, 0, sizeof(mask));
    if (isMpegTable) {
        filter[0] = tableId;
        mask[0] = 0xFF;
    } else {
        filter[0] = 0xC0;
        mask[0] = 0xF0;
    }
    if (dvbdemux_set_buffer(demuxFd, 1024 * 1024))
        TRACE(Trace::Error, (_T("Set demux buffer size failed")));

    if (dvbdemux_set_section_filter(demuxFd, pid, filter, mask, 1, 0)) {
        close(demuxFd);
        return -1;
    }
    return demuxFd;
}

bool SourceBackend::ProcessPAT(uint32_t frequency, int32_t patFd)
{
    int32_t size;
    uint8_t siBuf[4096];
    struct pollfd pollfd;

    TRACE(Trace::Information, (string(__FUNCTION__)));
    // Read the section.
    if ((size = read(patFd, siBuf, sizeof(siBuf))) < 0)
        return false;

    TRACE(Trace::Information, (string(__FUNCTION__)));
    // Parse section.
    struct section* section = section_codec(siBuf, size);
    if (!section)
        return false;

    TRACE(Trace::Information, (string(__FUNCTION__)));
    // Parse sectionExt.
    struct section_ext* sectionExt = section_ext_decode(section, 0);
    if (!sectionExt)
        return false;

    TRACE(Trace::Information, (string(__FUNCTION__)));
    // Parse PAT.
    struct mpeg_pat_section* pat = mpeg_pat_section_codec(sectionExt);
    if (!pat)
        return false;

    TRACE(Trace::Information, (string(__FUNCTION__)));
    // Try and find the requested program.
    struct mpeg_pat_program* curProgram;
    int32_t pmtFd = -1;
    mpeg_pat_section_programs_for_each(pat, curProgram)
    {
        TRACE(Trace::Information, (_T("Program Number:- %d PMT Pid:- %u frequency  :- %d"), curProgram->program_number, curProgram->pid, frequency));
        TSInfo channel;
        memset(&channel, 0, sizeof(channel));
        channel.programNumber = curProgram->program_number;
        channel.pmtPid = curProgram->pid;
        channel.frequency = frequency;
        if ((pmtFd = CreateSectionFilter(curProgram->pid, TablePmt, 1)) < 0)
            return false;
        pollfd.fd = pmtFd;
        pollfd.events = POLLIN | POLLPRI | POLLERR |POLLHUP | POLLNVAL;
        bool pmtOk = false;
        while (!pmtOk) {
            int32_t count = poll(&pollfd, 1, 100);
            if (count < 0) {
                TRACE(Trace::Error, (_T("Poll error")));
                break;
            }
            if (!count) {
                TRACE(Trace::Information, (_T("Poll fd status dmxfd  timeout")));
                continue;
            }
            if (pollfd.revents & (POLLIN | POLLPRI)) {
                ProcessPMT(pollfd.fd, channel); // FIXME:PMT updation to be taken care of.
                pmtOk = true;
            }
        }
        _channelData.push_back(channel);
        if (pmtFd != -1) {
            dvbdemux_stop(pmtFd);
            close(pmtFd);
        }
    }
    // Remember the PAT version.
    return true;
}

TvmRc SourceBackend::GetTSInfo(TSInfoList& tsInfoList)
{
    TRACE(Trace::Information, (string(__FUNCTION__)));
    TvmRc rc = TvmSuccess;
    for (auto& channel : _channelData) {
        TSInfo tsInfo;
        memset(&tsInfo, 0, sizeof(tsInfo));
        tsInfo.programNumber = channel.programNumber;
        tsInfo.frequency = channel.frequency;
        tsInfo.audioPid = channel.audioPid;
        tsInfo.videoPid = channel.videoPid;
        tsInfo.pmtPid = channel.pmtPid;
        if (tsInfo.audioPid || tsInfo.videoPid) {
            tsInfoList.push_back(tsInfo);

            TRACE(Trace::Information, (_T("LCN:%" PRIu16 ", pmtPid:%" PRIu16 ", videoPid:%" PRIu16 ", audioPid:%" PRIu16 " \
                frequency : %u"), tsInfo.programNumber,
                tsInfo.pmtPid, tsInfo.videoPid, tsInfo.audioPid, tsInfo.frequency));
        }
    }
    return rc;
}
} // namespace LinuxDVB
