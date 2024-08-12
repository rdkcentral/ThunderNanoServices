/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#include "Module.h"

#include <CECTypes.h>

#include <core/core.h>

namespace Thunder {
ENUM_CONVERSION_BEGIN(cec_adapter_role_t)
    { CEC_DEVICE_TV, _TXT("tv") },
    { CEC_DEVICE_RECORDER, _TXT("recorder") },
    { CEC_DEVICE_TUNER, _TXT("tuner") },
    { CEC_DEVICE_PLAYBACK, _TXT("playback") },
    { CEC_DEVICE_AUDIOSYSTEM, _TXT("audiosystem") },
    { CEC_DEVICE_SWITCH, _TXT("switch") },
    { CEC_DEVICE_VIDEOPROCESSOR, _TXT("videoprocessor") },
ENUM_CONVERSION_END(cec_adapter_role_t);
}