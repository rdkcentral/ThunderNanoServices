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
 
#include "DSResolutionHAL.h"

namespace WPEFramework {
ENUM_CONVERSION_BEGIN(DSResolutionHAL::PixelResolution){ DSResolutionHAL::PixelResolution_720x480, _TXT("720x480") },
    { DSResolutionHAL::PixelResolution_720x576, _TXT("720x576") },
    { DSResolutionHAL::PixelResolution_1280x720, _TXT("1280x720") },
    { DSResolutionHAL::PixelResolution_1920x1080, _TXT("1920x1080") },
    { DSResolutionHAL::PixelResolution_3840x2160, _TXT("3840x2160") },
    { DSResolutionHAL::PixelResolution_4096x2160, _TXT("4096x2160") },
    { DSResolutionHAL::PixelResolution_Unknown, _TXT("Unknown") },

    ENUM_CONVERSION_END(DSResolutionHAL::PixelResolution)

        DSResolutionHAL::DSResolutionHAL()
    : _vopHandle(0)
    , _vopType(dsVIDEOPORT_TYPE_HDMI)
    , _enabled(false)
    , _connected(false)
    , _isOperational(false)
    , _dsRet(dsERR_UNKNOWN)
{
    if (dsHostInit() == dsERR_NONE) {
        if (dsVideoPortInit() == dsERR_NONE) {
            if (dsGetVideoPort((dsVideoPortType_t)_vopType, 0, &_vopHandle) == dsERR_NONE) {
                if (dsIsVideoPortEnabled(_vopHandle, &_enabled) == dsERR_NONE) {
                    if (dsIsDisplayConnected(_vopHandle, &_connected) == dsERR_NONE)
                        _isOperational = true;
                }
            }
        }
    }
}

DSResolutionHAL::~DSResolutionHAL()
{
    _isOperational = false;
}

const DSResolutionHAL::PixelResolution DSResolutionHAL::Resolution()
{
    TRACE(Trace::Information, (string(_T("Get Resolution"))));
    int _vopHandle = 0;
    dsVideoPortResolution_t resolution;
    dsVideoResolution_t format;
    if (dsGetResolution(_vopHandle, &resolution) == dsERR_NONE) {
        format = resolution.pixelResolution;

        const auto index = std::find_if(pixelFormatLookup.cbegin(), pixelFormatLookup.cend(),
            [format](const std::pair<const DSResolutionHAL::PixelResolution, const dsVideoResolution_t>& found) { return found.second == format; });

        if (index != pixelFormatLookup.cend()) {
            TRACE(Trace::Information, (_T("Resolution is %d "), index->second));
            return index->first;
        }
    }
    TRACE(Trace::Information, (_T("Resolution is unknown")));
    return DSResolutionHAL::PixelResolution_Unknown;
}

bool DSResolutionHAL::Resolution(const DSResolutionHAL::PixelResolution pixelResolution)
{
    bool status = false;
    TRACE(Trace::Information, (string(_T("Set Resolution"))));
    int _vopHandle = 0;
    dsVideoPortResolution_t resolution;
    dsVideoResolution_t format;
    if (dsGetResolution(_vopHandle, &resolution) == dsERR_NONE) {

        const auto index(pixelFormatLookup.find(pixelResolution));
        if (index == pixelFormatLookup.cend() || index->second == PixelResolution_Unknown || index->second == resolution.pixelResolution) {
            TRACE(Trace::Information, (_T("Output resolution is already %d"), pixelResolution));
            return status;
        }
        if (resolution.pixelResolution != index->second) {
            resolution.pixelResolution = static_cast<dsVideoResolution_t>(index->second);
            if (dsSetResolution(_vopHandle, &resolution) != dsERR_NONE) {
                TRACE(Trace::Information, (_T("Setting resolution is failed: %d"), index->second));
            } else {
                TRACE(Trace::Information, (_T("New resolution is %d "), index->second));
                status = true;
            }
        }
    }
    return status;
}

} /* namespace WPEFramework */
