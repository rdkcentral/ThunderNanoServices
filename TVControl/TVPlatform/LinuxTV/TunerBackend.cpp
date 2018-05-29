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
#include "Module.h"
#include "TunerBackend.h"

using namespace WPEFramework;

namespace LinuxDVB {

TvTunerBackend::TvTunerBackend(uint32_t tunerCnt, std::unique_ptr<TunerData> tunerPtr, bool isDvb)
    : _tunerData(std::move(tunerPtr))
    , _srcTypeListPtr(nullptr)
    , _supportedSysCount(0)
    , _tunerIndex(tunerCnt)
    , _tunerCount(0)
{
    if (isDvb)
        _sType = DvbC;
    else
        _sType = Atsc;
    InitializeSourceList();
    ConfigureTuner(_tunerIndex);
}

TvTunerBackend::~TvTunerBackend()
{
    _tunerData = nullptr;
    if (_srcTypeListPtr)
        delete _srcTypeListPtr;
    _supportedSysCount = 0;
}

void TvTunerBackend::InitializeSourceList()
{
    GetAvailableSrcList(&_srcList);
}

void TvTunerBackend::UpdateTunerCount(uint32_t tunerCount)
{
    _source->UpdateTunerCount(tunerCount);
}

void TvTunerBackend::ConfigureTuner(int32_t tunerCnt)
{
    TRACE(Trace::Information, (_T("Getting Modulation for Tuner:%d"), tunerCnt));

    // Get configuration.
    GetConfiguration();
    char tmpStr[20];
    snprintf(tmpStr, 20, "TUNER_%d_MODULATION", tunerCnt + 1);
    std::string tunerStr = std::string(tmpStr);
    std::string modulation;
    if (_configValues.find(tunerStr) != _configValues.end()) {
        TRACE(Trace::Information, (_T("Modulation  found = %s"), _configValues.find(tunerStr)->second));
        modulation.assign(_configValues.find(tunerStr)->second);
    } else {
        modulation.assign("8VSB");
        TRACE(Trace::Information, (_T("Setting default modulation")));
    }
    SetModulation(modulation);
    PopulateFreq();
}

void TvTunerBackend::SetModulation(std::string& modulation)
{
    struct dvbfe_handle* feHandle = OpenFE(_tunerData->tunerId);
    if (feHandle) {
        // Get tuner info .
        struct dvb_frontend_info feInfo;
        if (ioctl(feHandle->fd, FE_GET_INFO, &feInfo) == -1)
            TRACE(Trace::Error, (_T("Unable to determine frontend type")));

        // Set modulation .
        if (!modulation.compare("8VSB")) {
            if (feHandle->type == DVBFE_TYPE_ATSC && (feInfo.caps & FE_CAN_8VSB)) {
                _channel = AtscVsb; // FIXME: check and remove.

                // Set the modulation.
                struct dtv_property p[] = { {.cmd = DTV_MODULATION } };
                struct dtv_properties cmdseq = {.num = 1, .props = p };
                struct dtv_property* propPtr;
                propPtr = p;
                propPtr->u.data = VSB_8;

                // Set the current to platform.
                if (ioctl(feHandle->fd, FE_SET_PROPERTY, &cmdseq) == -1)
                    TRACE(Trace::Error, (_T("Failed to set  Srource %d at plarform "), AtscVsb));

                TRACE(Trace::Information, (_T("Modulation set to 8VSB")));
                propPtr->u.data = 0;
                _tunerData->modulation = DVBFE_ATSC_MOD_VSB_8;
            } else
                TRACE(Trace::Information, (_T("Insufficient capability details")));
        } else
            TRACE(Trace::Information, (_T("In sufficient details")));
        dvbfe_close(feHandle);
    } else
        TRACE(Trace::Error, (_T("Failed to open frontend")));
}

void TvTunerBackend::PopulateFreq()
{
    for (uint32_t channel = 0; channel < 68; channel++) {
        if (BaseOffset((channel + 2), _channel) != -1)
            _tunerData->frequency.push_back((uint32_t)(BaseOffset((channel + 2), _channel) + ((channel + 2) * FrequencyStep((channel + 2), _channel))));
    }
}

void TvTunerBackend::GetSignalStrength(double* signalStrength)
{
    struct dvbfe_handle* feHandle = OpenFE(_tunerData->tunerId);
    if (feHandle) {
        struct dvbfe_info feInfo;
        ioctl(feHandle->fd, FE_READ_SIGNAL_STRENGTH, &feInfo.signal_strength);
        *signalStrength = static_cast<double>(10 * (log10(feInfo.signal_strength)) + 30);
        dvbfe_close(feHandle);
    } else
        TRACE(Trace::Error, (_T("Failed to open frontend")));
}

// Modulation return the base offsets for specified channellist and channel.
uint32_t TvTunerBackend::BaseOffset(uint32_t channel, ChannelList channelList)
{
    switch (channelList) {
    case AtscQam: // ATSC cable, US EIA/NCTA Std Cable center freqs + IRC list.
    case DvbCBr: // BRAZIL - same range as ATSC IRCi.
        switch (channel) {
        case 2 ... 4:
            return 45000000;
        case 5 ... 6:
            return 49000000;
        case 7 ... 13:
            return 135000000;
        case 14 ... 22:
            return 39000000;
        case 23 ... 94:
            return 81000000;
        case 95 ... 99:
            return -477000000;
        case 100 ... 133:
            return 51000000;
        default:
            return -1;
        }
    case AtscVsb: // ATSC terrestrial, US NTSC center freqs.
        switch (channel) {
        case 2 ... 4:
            return 45000000;
        case 5 ... 6:
            return 49000000;
        case 7 ... 13:
            return 135000000;
        case 14 ... 69:
            return 389000000;
        default:
            return -1;
        }
    case IsdbT6MHz: // ISDB-T, 6 MHz central frequencies.
        switch (channel) {
        // Channels 7-13 are reserved but aren't used yet.
        // case  7 ... 13: return  135000000;
        case 14 ... 69:
            return 389000000;
        default:
            return -1;
        }
    case DvbTAu: // AUSTRALIA, 7MHz step list.
        switch (channel) {
        case 5 ... 12:
            return 142500000;
        case 21 ... 69:
            return 333500000;
        default:
            return -1;
        }
    case DvbTDe: // GERMANY.
    case DvbTFr: // FRANCE, +/- offset 166kHz & +offset 332kHz & +offset 498kHz.
    case DvbTGb: // UNITED KINGDOM, +/- offset.
        switch (channel) {
        case 5 ... 12:
            return 142500000; // VHF unused in FRANCE, skip those in offset loop.
        case 21 ... 69:
            return 306000000;
        default:
            return -1;
        }

    case DvbCQam: // EUROPE.
        switch (channel) {
        case 0 ... 1:
        case 5 ... 12:
            return 73000000;
        case 22 ... 90:
            return 138000000;
        default:
            return -1;
        }
    case DvbCFi: // FINLAND, QAM128.
        switch (channel) {
        case 1 ... 90:
            return 138000000;
        default:
            return -1;
        }
    case DvbCFr: // FRANCE, needs user response.
        switch (channel) {
        case 1 ... 39:
            return 107000000;
        case 40 ... 89:
            return 138000000;
        default:
            return -1;
        }
    default:
        return -1;
    }
}

// Return the freq step size for specified channellist.
uint32_t TvTunerBackend::FrequencyStep(uint32_t channel, ChannelList channelList)
{
    switch (channelList) {
    case AtscQam:
    case AtscVsb:
    case DvbCBr:
    case IsdbT6MHz:
        return 6000000; // atsc, 6MHz step.
    case DvbTAu:
        return 7000000; // dvb-t australia, 7MHz step.
    case DvbTDe:
    case DvbTFr:
    case DvbTGb:
        switch (channel) { // dvb-t europe, 7MHz VHF ch5..12, all other 8MHz.
        case 5 ... 12:
            return 7000000;
        case 21 ... 69:
            return 8000000;
        default:
            return 8000000; // should be never reached.
        }
    case DvbCQam:
    case DvbCFi:
    case DvbCFr:
        return 8000000; // dvb-c, 8MHz step.
    default:
        return 0;
    }
}

void TvTunerBackend::GetAvailableSrcList(SrcTypesVector* outSourceList)
{
    // Get avaiable source list from platform.
    if (GetSupportedSourcesTypeList(outSourceList) < 0)
        TRACE(Trace::Error, (_T("Failed to get supported source list")));
    // Create private list of sources.
    GetSources();
}

void TvTunerBackend::GetSources()
{
    if (_srcTypeListPtr && _supportedSysCount) {
        // Read supported type list from the private list and create list of source objects.
        for (int32_t i = 0; i < _supportedSysCount; i++) {
            if (_srcTypeListPtr[i] == _sType) {
                std::unique_ptr<SourceBackend> sInfo = std::make_unique<SourceBackend>(_srcTypeListPtr[i], _tunerData.get());
                _source = std::move(sInfo);
            }
        }
    } else
        TRACE(Trace::Information, (_T("Private source list already created")));
}

int32_t TvTunerBackend::GetSupportedSourcesTypeList(SrcTypesVector* outSourceTypesList)
{
    if (!(_srcTypeListPtr && _supportedSysCount)) {
        struct dtv_property p = {.cmd = DTV_ENUM_DELSYS };
        struct dtv_properties cmdName = {.num = 1, .props = &p };
        struct dvbfe_handle* feHandle = OpenFE(_tunerData->tunerId);
        if (feHandle) {
            if (ioctl(feHandle->fd, FE_GET_PROPERTY, &cmdName) == -1) {
                TRACE(Trace::Error, (_T("FE_GET_PROPERTY failed")));
                dvbfe_close(feHandle);
                return -1;
            }

            _supportedSysCount = cmdName.props->u.buffer.len;
            TRACE(Trace::Information, (_T("Number of supported Source = %d"), _supportedSysCount));

            // Create an array of Type.
            _srcTypeListPtr = (SourceType*)new SourceType[cmdName.props->u.buffer.len];

            for (int32_t i = 0; i < _supportedSysCount; i++) {
                // Map the list to W3C spec.
                switch (cmdName.props->u.buffer.data[i]) {
                case SYS_DVBC_ANNEX_A:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    // FIXME: _srcTypeListPtr[i] = ;
                    break;
                case SYS_DVBC_ANNEX_B:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = DvbC;
                    break;
                case SYS_DVBT:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = DvbT;
                    break;
                case SYS_DSS:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    // FIXME: _srcTypeListPtr[i] = ;
                    break;
                case SYS_DVBS:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = DvbS;
                    break;
                case SYS_DVBS2:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = DvbS2;
                    break;
                case SYS_DVBH:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = DvbH;
                    break;
                case SYS_ISDBT:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = IsdbT;
                    break;
                case SYS_ISDBS:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = IsdbS;
                    break;
                case SYS_ISDBC:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = IsdbC;
                    break;
                case SYS_ATSC:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = Atsc;
                    break;
                case SYS_ATSCMH:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = AtscMH;
                    break;
#if DVB_API_VERSION_MINOR == 10
                case SYS_DTMB:
#else
                case SYS_DMBTH:
#endif
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = Dtmb;
                    break;
                case SYS_CMMB:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = Cmmb;
                    break;
                case SYS_DAB:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    // FIXME: _srcTypeListPtr[i] = ;
                    break;
                case SYS_DVBT2:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = DvbT2;
                    break;
                case SYS_TURBO:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    // FIXME _srcTypeListPtr[i] = ;
                    break;
                case SYS_DVBC_ANNEX_C:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    // FIXME _srcTypeListPtr[i] = ;
                    break;
                case SYS_UNDEFINED:
                    TRACE(Trace::Information, (_T("STP: CASE = %d"), cmdName.props->u.buffer.data[i]));
                    _srcTypeListPtr[i] = Undifined;
                    break;
                default:
                    TRACE(Trace::Information, (_T("ST: DEFAULT")));
                    _srcTypeListPtr[i] = Undifined;
                    break;
                }
            }
            dvbfe_close(feHandle);
            if (!_supportedSysCount) {
                outSourceTypesList->length = _supportedSysCount;
                outSourceTypesList->types = nullptr;
                TRACE(Trace::Error, (_T("driver returned 0 supported delivery source type!")));
                return -1;
            }
        } else {
            outSourceTypesList->length = _supportedSysCount;
            outSourceTypesList->types = nullptr;
            TRACE(Trace::Error, (_T("Failed to open frontend")));
            return -1;
        }

    }

    // Update the number of supported Sources.
    outSourceTypesList->length = _supportedSysCount;
    // Update source type ptr.
    outSourceTypesList->types = _srcTypeListPtr;
    return _supportedSysCount;
}

TvmRc TvTunerBackend::StartScanning(std::vector<uint32_t> freqList, TVPlatform::ITVPlatform::ITunerHandler& tunerHandler)
{
    return _source->StartScanning(freqList, tunerHandler);
}

bool TvTunerBackend::IsScanning()
{
    return _source->IsScanning();
}

TvmRc TvTunerBackend::GetChannelMap(ChannelMap& chanMap)
{
    return _source->GetChannelMap(chanMap);
}

TvmRc TvTunerBackend::GetTSInfo(TSInfoList& tsInfoList)
{
    return _source->GetTSInfo(tsInfoList);
}

TvmRc TvTunerBackend::StopScanning()
{
    return _source->StopScanning();
}

void TvTunerBackend::GetConfiguration()
{
    std::ifstream fileStream(CONFIGFILE);
    std::string line;
    while (std::getline(fileStream, line)) {
        std::istringstream isLine(line);
        std::string key;
        if (std::getline(isLine, key, '=')) {
            std::string value;
            if (key[0] == '#')
                continue;
            if (std::getline(isLine, value))
                _configValues.insert(make_pair(key, value));
        }
    }
    if (_configValues.empty())
        TRACE(Trace::Error, (_T("TVConfig file is MISSING")));
    TRACE(Trace::Information, (_T("Configuration details")));
    for (auto& config : _configValues)
        TRACE(Trace::Information, (_T("%s :: %s"), config.first, config.second));
}

TvmRc TvTunerBackend::SetHomeTS(uint32_t primaryFreq)
{
    return _source->SetHomeTS(primaryFreq);
}

TvmRc TvTunerBackend::StopFilter(uint16_t pid)
{
    return _source->StopFilter(pid);
}

TvmRc TvTunerBackend::StopFilters()
{
    return _source->StopFilters();
}

TvmRc TvTunerBackend::StartFilter(uint16_t pid, TVPlatform::ITVPlatform::ISectionHandler* pSectionHandler)
{
    return _source->StartFilter(pid, pSectionHandler);
}

std::vector<uint32_t>& TvTunerBackend::GetFrequencyList()
{
    return _source->GetFrequencyList();
}

TvmRc TvTunerBackend::Tune(TSInfo& tsInfo, TVPlatform::ITVPlatform::ITunerHandler& tunerHandler)
{
    return _source->Tune(tsInfo, tunerHandler);

}
} // namespace LinuxDVB
