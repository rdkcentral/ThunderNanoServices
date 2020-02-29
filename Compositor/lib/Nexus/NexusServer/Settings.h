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
 
#include <nexus_types.h>
#include <interfaces/IComposition.h>

namespace WPEFramework {
namespace Plugin {
    static const std::map<const Exchange::IComposition::ScreenResolution, const NEXUS_VideoFormat> formatLookup = {
        { Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown, NEXUS_VideoFormat_eUnknown },
        { Exchange::IComposition::ScreenResolution::ScreenResolution_480i, NEXUS_VideoFormat_eNtsc },
        { Exchange::IComposition::ScreenResolution::ScreenResolution_480p, NEXUS_VideoFormat_e480p },
        { Exchange::IComposition::ScreenResolution::ScreenResolution_720p, NEXUS_VideoFormat_e720p },
        { Exchange::IComposition::ScreenResolution::ScreenResolution_720p50Hz, NEXUS_VideoFormat_e720p50hz },
        { Exchange::IComposition::ScreenResolution::ScreenResolution_1080p24Hz, NEXUS_VideoFormat_e1080p24hz },
        { Exchange::IComposition::ScreenResolution::ScreenResolution_1080i50Hz, NEXUS_VideoFormat_e1080i50hz },
        { Exchange::IComposition::ScreenResolution::ScreenResolution_1080p50Hz, NEXUS_VideoFormat_e1080p50hz },
        { Exchange::IComposition::ScreenResolution::ScreenResolution_1080p60Hz, NEXUS_VideoFormat_e1080p60hz },
        { Exchange::IComposition::ScreenResolution::ScreenResolution_2160p50Hz, NEXUS_VideoFormat_e3840x2160p50hz },
        { Exchange::IComposition::ScreenResolution::ScreenResolution_2160p60Hz, NEXUS_VideoFormat_e3840x2160p60hz }
    };
} // namespace Plugin
} // namespace WPEFramework
