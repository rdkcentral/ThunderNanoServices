/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
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

/** 
 * @defgroup devicesettingshalapi Device Settings HAL API list
 * Describe the details about Device Settings HAL API specifications.
 *
 * <b> Following abbreviations present in HAL API </b>
 *
 * @par Abbreviations
 * - BE:      Big-Endian.
 * - cb:      Callback function (suffix).
 * - DS:      Device Settings.
 * - FPD:     Front-Panel Display.
 * - HAL:     Hardware Abstraction Layer.
 * - LE:      Little-Endian.
 * - LS:      Least Significant.
 * - MBZ:     Must be zero.
 * - MS:      Most Significant.
 * - RDK:     Reference Design Kit.
 * - _t:      Type (suffix).
 * - HDCP:    High-bandwidth Digital Copy Protection.
 * - DTCP:    Digital Transmission Copy Protection.
 * - EDID:    Extended Display Information Data.
 * 
 * @ingroup DSSETTINGS_HAL
 */

/**
* @defgroup devicesettings
* @{
* @defgroup hal
* @{
**/

#ifndef _DS_HOST_H_
#define _DS_HOST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dsError.h"
#include "dsTypes.h"

/** @addtogroup DSHAL_HOST_API Device Settings HAL Host Public API
 *  @ingroup devicesettingshalapi
 *
 *  Described herein are the DeviceSettings HAL types and functions that are part of
 *  the Host subsystem. The Host subsystem manages system-specific hardware operations.
 *
 *  @{
 */

/**
 * @brief Initialize the Host sub-system.
 *
 * This function must initialize all the system specific drivers to be initialized  prior
 * to any driver specific calls.
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t dsHostInit();

/**
 * @brief Set the power mode.
 *
 * This function sets the power mode of the host to active or standby and turns on/off
 * all the ouput ports.
 *
 * @param [in] newPower  The power mode of the host (::dsPOWER_STANDBY or ::dsPOWER_ON)
 * @return Device Settings error code
 * @retval    ::dsError_t
 *
 * @note dsPOWER_OFF is not currently being used.
 */
dsError_t dsSetHostPowerMode(int newPower);

/**
 * @brief Get the current power mode.
 *
 * This function gets the current power mode of the host. 
 *
 * @param [out] *currPower  The address of a location to hold the host's current power
 *                          mode on return. It returns one of:
 *                              - ::dsPOWER_OFF
 *                              - ::dsPOWER_STANDBY
 *                              - ::dsPOWER_ON
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t dsGetHostPowerMode(int* currPower);

/**
 * @brief Terminate the Host sub-system.
 *
 * This function resets the data structures used within the Host module and releases any
 * handles specific to the host module.
 *
 * @return Device Settings error code
 * @retval    ::dsError_t
 */
dsError_t dsHostTerm();

/**
 * @brief This function returns the preferred sleep mode which is persisted.
 *
 * @param[out] pMode Data will be copied to this. This shall be preallocated before the call.
 * @return Device Settings error code
 * @retval dsERR_NONE If sucessfully dsGetPreferredSleepMode api has been called using IARM support.
 * @retval dsERR_GENERAL General failure.
 */
dsError_t dsGetPreferredSleepMode(dsSleepMode_t* pMode);

/**
 * @brief This function sets the preferred sleep mode which needs to be persisted.
 *
 * @param[in] mode Sleep mode that is expected to be persisted.
 * @return Device Settings error code
 * @retval dsERR_NONE If sucessfully dsSetPreferredSleepMode api has been called using IARM support.
 * @retval dsERR_GENERAL General failure.
 */
dsError_t dsSetPreferredSleepMode(dsSleepMode_t mode);

/**
 * @brief This function gets the CPU temperature in centrigade.
 *
 * @param[in] cpuTemperature The address of a location to hold the CPU Temperature
 * @return Device Settings error code
 * @retval dsERR_NONE If sucessfully dsGetCPUTemperature api has been called using IARM support.
 * @retval dsERR_GENERAL General failure.
 */
dsError_t dsGetCPUTemperature(float* cpuTemperature);

/**
 * @brief Get DS HAL API Version.
 *
 * In 4 byte VersionNumber, Two Most significant Bytes are Major number
 * and Two Least Significant Bytes are minor number.
 *
 * @param[out] versionNumber 4 Bytes of version number of DS HAL
 *
 * @return Returns 4 byte Version Number
 * @retval dsERR_NONE Successfully got the version number from dsHAL.
 * @retval dsERR_GENERAL Failed to get the version number.
 */
dsError_t dsGetVersion(uint32_t* versionNumber);

/**
 * @brief Allows the application to set the runtime version of the dsHAL
 *
 * @param[in] versionNumber 4 Bytes of version number of DS HAL
 *
 * @retval dsERR_NONE Successfully set the version for dsHAL.
 * @retval dsERR_GENERAL Failed to set the version.
 */
dsError_t dsSetVersion(uint32_t versionNumber);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif
#endif /* _DS_HOST_H_ */

/** @} */
/** @} */
