/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological B.V.
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

#pragma once

#include <interfaces/IComposition.h>

namespace Compositor {

/**
 * @brief Gets the width from a IComposition::ScreenResolution.
 *
 * @param resolution The resolution to parse
 * @return uint32_t width in pixels.
 */
static inline uint32_t WidthFromResolution(const WPEFramework::Exchange::IComposition::ScreenResolution resolution)
{
    // Assume an invalid width equals 0
    uint32_t width = 0;

    switch (resolution) {
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_480p: // 720x480
        width = 720;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_576i: // 1024x576 interlaced
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_576p: // 1024x576 progressive
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_576p50Hz: // 1024x576 progressive
        width = 1024;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_720p: // 1280x720 progressive
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_720p50Hz: // 1280x720 @ 50 Hz
        width = 1280;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p: // 1920x1080 progressive
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080i: // 1920x1080 interlaced
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p24Hz: // 1920x1080 progressive @ 24 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080i25Hz: // 1920x1080 interlaced @ 25 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p25Hz: // 1920x1080 progressive @ 25 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080i50Hz: // 1920x1080 interlaced  @ 50 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p50Hz: // 1920x1080 progressive @ 50 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p60Hz: // 1920x1080 progressive @ 60 Hz
        width = 1920;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_2160p50Hz: // 4K, 3840x2160 progressive @ 50 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_2160p60Hz: // 4K, 3840x2160 progressive @ 60 Hz
        width = 3840;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_4320p30Hz: // 8K, 7680x4320 progressive @ 30 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_4320p60Hz: // 8K, 7680x4320 progressive @ 60 Hz
        width = 7680;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_480i: // Unknown according to the standards (?)
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown:
    default:
        width = 0;
    }

    return width;
}

/**
 * @brief Gets the height from a IComposition::ScreenResolution.
 *
 * @param resolution The resolution to parse
 * @return uint32_t height in pixels.
 */
static inline uint32_t HeightFromResolution(const WPEFramework::Exchange::IComposition::ScreenResolution resolution)
{
    // Assume an invalid height equals 0
    uint32_t height = 0;

    switch (resolution) {
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_480i: // 720x480 interlaced
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_480p: // 720x480 progressive
        height = 480;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_576i: // 1024x576 interlaced
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_576p: // 1024x576 progressive
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_576p50Hz: // 1024x576 progressive
        height = 576;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_720p: // 1280x720 progressive
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_720p50Hz: // 1280x720 progressive @ 50 Hz
        height = 720;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p: // 1920x1080 progressive
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080i: // 1920x1080 interlaced
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p24Hz: // 1920x1080 progressive @ 24 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080i25Hz: // 1920x1080 interlaced @ 25 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p25Hz: // 1920x1080 progressive @ 25 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080i50Hz: // 1920x1080 interlaced @ 50 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p50Hz: // 1920x1080 progressive @ 50 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p60Hz: // 1920x1080 progressive @ 60 Hz
        height = 1080;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_2160p50Hz: // 4K, 3840x2160 progressive @ 50 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_2160p60Hz: // 4K, 3840x2160 progressive @ 60 Hz
        height = 2160;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_4320p30Hz: // 8K, 7680x4320 progressive @ 30 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_4320p60Hz: // 8K, 7680x4320 progressive @ 60 Hz
        height = 4320;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown:
    default:
        height = 0;
    }

    return height;
}

/**
 * @brief factor to be used to remove the need of using float values on the interface.
 *
 */
constexpr uint16_t RefreshRateFactor = 1000;

/**
 * @brief Gets the refresh rate from a IComposition::ScreenResolution.
 *
 * @param resolution The resolution to parse
 * @return uint16_t the refresh rate multiplied with @RefreshRateFactor (59940 / @RefreshRateFactor -> 59.940Hz)
 */
static inline uint16_t RefreshRateFromResolution(const WPEFramework::Exchange::IComposition::ScreenResolution resolution)
{
    uint16_t rate = 0;

    switch (resolution) {
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p24Hz: // 1920x1080 progressive @ 24 Hz
        rate = 24000;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080i25Hz: // 1920x1080 interlaced @ 25 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p25Hz: // 1920x1080 progressive @ 25 Hz
        rate = 25000;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p30Hz: // 1920x1080 progressive @ 30 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_2160p30Hz: // 4K, 3840x2160 progressive @ 30 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_4320p30Hz: // 8K, 7680x4320 progressive @ 30 Hz
        rate = 30000;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_576p50Hz: // 1024x576 progressive
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_720p50Hz: // 1280x720 progressive @ 50 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080i50Hz: // 1920x1080 interlaced @ 50 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p50Hz: // 1920x1080 progressive @ 50 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_2160p50Hz: // 4K, 3840x2160 progressive @ 50 Hz
        rate = 50000;
        break;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_480i: // 720x480 interlaced
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_480p: // 720x480 progressive
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_576i: // 1024x576 interlaced
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_576p: // 1024x576 progressive
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_720p: // 1280x720 progressive
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080i: // 1920x1080 interlaced
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p: // 1920x1080 progressive
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_1080p60Hz: // 1920x1080 progressive @ 60 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_2160p60Hz: // 4K, 3840x2160 progressive @ 60 Hz
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_4320p60Hz: // 8K, 7680x4320 progressive @ 60 Hz
        rate = 60000;
    case WPEFramework::Exchange::IComposition::ScreenResolution::ScreenResolution_Unknown:
    default:
        rate = 0;
    }

    return rate;
};
}
