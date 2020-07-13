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

#include "Module.h"
#include <interfaces/IPower.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function definition to signal a power state change.
 */
typedef void (*power_state_change)(void* userData, enum WPEFramework::Exchange::IPower::PCState newState);

/**
 * Function to initialize and deinitialize the module
 */
void power_initialize(power_state_change callback, void* userData, const char* config,
        const enum WPEFramework::Exchange::IPower::PCState persistedState);
void power_deinitialize();

/**
 * Function to request a power state change 
 */
uint32_t power_set_state(const enum WPEFramework::Exchange::IPower::PCState state, const uint32_t sleepTime);

/**
 * Function to request the current power state
 */
enum WPEFramework::Exchange::IPower::PCState power_get_state();

/**
 * Function to check power state is supported by platform
 */
bool is_power_state_supported(const enum WPEFramework::Exchange::IPower::PCState state);

/**
 * Function to request the persistent power state from platform.
 */
enum WPEFramework::Exchange::IPower::PCState power_get_persisted_state();

/**
 * Function to save current power state for persistent recovery in platform.
 */
void power_set_persisted_state(const enum WPEFramework::Exchange::IPower::PCState);

#ifdef __cplusplus
}
#endif
