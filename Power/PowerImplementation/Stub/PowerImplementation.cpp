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
 
#include "../../Implementation.h"

namespace {

static void*              _userData = nullptr;
static power_state_change _callback = nullptr;
static enum PCState       _state    = WPEFramework::Exchange::IPower::PCStatus::PCSuccess;

void power_initialize(power_state_change callback, void* userData, const char*) {
    ASSERT ((_callback != nullptr) ^ (callback != nullptr));
    _userData = userData;
    _callback = callback;
}

void power_deinitialize() {
    _callback = nullptr;
    _userData = nullptr;
}

uint32_t power_set_state(const enum WPEFramework::Exchange::IPower::PCState state, const uint32_t sleepTime) {
    _state = state;
    if (_callback != nullptr) {
        _callback(_userData);
    }
    return (Core::ERROR_NONE);
}

WPEFramework::Exchange::IPower::PCState power_get_state() {
    status = _status;
    return (_state);
}

}

