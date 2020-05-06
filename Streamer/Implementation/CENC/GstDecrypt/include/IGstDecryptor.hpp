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

#include "EncryptedBuffer.hpp"
#include "GstBufferView.hpp"
#include "IExchangeFactory.hpp"

#include <gst/gstbuffer.h>
#include <gst/gstevent.h>

namespace WPEFramework {
namespace CENCDecryptor {
    class IGstDecryptor {
    public:
        virtual gboolean Initialize(std::unique_ptr<IExchangeFactory>,
            const std::string& keysystem,
            const std::string& origin,
            BufferView& initData) = 0;

        virtual GstFlowReturn Decrypt(std::shared_ptr<EncryptedBuffer>) = 0;

        virtual ~IGstDecryptor(){};
    };
}
}
