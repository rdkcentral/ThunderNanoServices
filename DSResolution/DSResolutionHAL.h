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
 
#ifndef DSRESOLUTION_HAL_H
#define DSRESOLUTION_HAL_H

#include "Module.h"

extern "C" {
#include "dsVideoPort.h"
}

namespace WPEFramework {
class DSResolutionHAL {
private:
    DSResolutionHAL(const DSResolutionHAL&) = delete;
    DSResolutionHAL& operator=(const DSResolutionHAL&) = delete;

public:
    enum PixelResolution {
        PixelResolution_720x480,
        PixelResolution_720x576,
        PixelResolution_1280x720,
        PixelResolution_1920x1080,
        PixelResolution_3840x2160,
        PixelResolution_4096x2160,
        PixelResolution_Unknown
    };

    DSResolutionHAL();
    virtual ~DSResolutionHAL();
    const PixelResolution Resolution();
    bool Resolution(const PixelResolution resolution);
    inline bool IsOperational() const
    {
        return (_isOperational);
    }

private:
    const std::map<const PixelResolution, const dsVideoResolution_t> pixelFormatLookup = {
        { PixelResolution_720x480, dsVIDEO_PIXELRES_720x480 },
        { PixelResolution_720x576, dsVIDEO_PIXELRES_720x576 },
        { PixelResolution_1280x720, dsVIDEO_PIXELRES_1280x720 },
        { PixelResolution_1920x1080, dsVIDEO_PIXELRES_1920x1080 },
        { PixelResolution_3840x2160, dsVIDEO_PIXELRES_3840x2160 },
        { PixelResolution_4096x2160, dsVIDEO_PIXELRES_4096x2160 },
        { PixelResolution_Unknown, dsVIDEO_PIXELRES_MAX }
    };

private:
    int _vopHandle;
    int _vopType;
    bool _enabled;
    bool _connected;
    bool _isOperational;
    dsError_t _dsRet;
};

} /* namespace WPEFramework */
#endif /* DSRESOLUTION_HAL_H */
