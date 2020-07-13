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

#pragma once

#include <interfaces/IDolby.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Attempts to set the audio output in the provided mode.
 * 
 * @param type Dolby output type, specified by the IOutput::Type enumeration.
 * @return uint32_t possible error codes:
 *  - ERROR_NONE - All went well.
 *  - ERROR_ILLEGAL_STATE - The audio device could not be initialized.
 *  - ERROR_GENERAL - Setting the parameter has failed.
 */
uint32_t 
set_audio_output_type(const enum WPEFramework::Exchange::Dolby::IOutput::Type type);

/**
 * @brief Queries the implementation for dolby output mode.
 * 
 * @param error - Possible values:
 *  - ERROR_NONE - All went well.
 *  - ERROR_ILLEGAL_STATE - The audio device could not be initialized.
 *  - ERROR_GENERAL - The queried result could not be 
 *          mapped to Dolby::IOutput::Type enumeration.
 * 
 * @return Type returned by the query.
 */
enum WPEFramework::Exchange::Dolby::IOutput::Type 
get_audio_output_type(uint32_t& error);

#ifdef __cplusplus
}
#endif
