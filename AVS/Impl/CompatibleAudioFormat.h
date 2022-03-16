 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "TraceCategories.h"

#include <AVSCommon/Utils/AudioFormat.h>

namespace WPEFramework {
namespace Plugin {

    namespace AudioFormatCompatibility {
        // Audio format compatibility must much those settings as those are hardcoded into the SDK itself
        // Without additional logic for converting parameters different values will not work
        static constexpr unsigned int SAMPLE_RATE_HZ = 16000;
        static constexpr unsigned int SAMPLE_SIZE_IN_BITS = 16;
        static constexpr unsigned int NUM_CHANNELS = 1;
        static constexpr alexaClientSDK::avsCommon::utils::AudioFormat::Encoding ENCODING = alexaClientSDK::avsCommon::utils::AudioFormat::Encoding::LPCM;
        static constexpr alexaClientSDK::avsCommon::utils::AudioFormat::Endianness ENDIANESS = alexaClientSDK::avsCommon::utils::AudioFormat::Endianness::LITTLE;

        static bool IsCompatible(alexaClientSDK::avsCommon::utils::AudioFormat other)
        {
            if (ENCODING != other.encoding) {
                TRACE_GLOBAL(AVSClient, (_T("Incompatible audio format encoding")));
                return false;
            }
            if (ENDIANESS != other.endianness) {
                TRACE_GLOBAL(AVSClient, (_T("Incompatible audio format endianess")));
                return false;
            }
            if (SAMPLE_RATE_HZ != other.sampleRateHz) {
                TRACE_GLOBAL(AVSClient, (_T("Incompatible audio format sample rate")));
                return false;
            }
            if (SAMPLE_SIZE_IN_BITS != other.sampleSizeInBits) {
                TRACE_GLOBAL(AVSClient, (_T("Incompatible audio format sample size in bits")));
                return false;
            }
            if (NUM_CHANNELS != other.numChannels) {
                TRACE_GLOBAL(AVSClient, (_T("Incompatible audio format number of channels")));
                return false;
            }

            return true;
        }

    } // namespace AudioFormatCompatibility
} // namespace Plugin
} // namespace WPEFramework
