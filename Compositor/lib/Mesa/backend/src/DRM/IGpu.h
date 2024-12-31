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

#include "../Module.h"
#include <IOutput.h>
#include <DRMTypes.h>
#include <xf86drmMode.h>

namespace Thunder {

    namespace Compositor {

        namespace Backend {

            class Connector;

        } // namespace Backend

        struct IBackend {
            virtual ~IBackend() = default;

            virtual int Descriptor() const = 0;
            virtual void Commit(Backend::Connector*) = 0;
        };

    } // namespace Compositor
} // namespace Thunder
