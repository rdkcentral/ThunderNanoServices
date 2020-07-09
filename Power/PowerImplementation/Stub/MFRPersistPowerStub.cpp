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

/***
  * @file    : StubPersistPower.cpp
  * @info    : Stub implementations to read/save power state for persistent switching.
  */

#include "../../Implementation.h"

enum WPEFramework::Exchange::IPower::PCState power_get_persisted_state()
{
    return (WPEFramework::Exchange::IPower::PCState::On);
}

void power_set_persisted_state(const enum WPEFramework::Exchange::IPower::PCState persistedState)
{
    /* Nothing to do here. */
}
