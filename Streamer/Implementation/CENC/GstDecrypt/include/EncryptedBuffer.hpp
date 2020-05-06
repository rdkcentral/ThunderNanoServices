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

#include <gst/gstbuffer.h>
#include <gst/gstprotection.h>

namespace WPEFramework {
namespace CENCDecryptor {
    class EncryptedBuffer {
    public:
        EncryptedBuffer() = delete;
        EncryptedBuffer(GstBuffer*);

        EncryptedBuffer(const EncryptedBuffer&) = default;
        EncryptedBuffer& operator=(const EncryptedBuffer&) = default;

        GstBuffer* Buffer();
        GstBuffer* SubSample();
        size_t SubSampleCount();
        GstBuffer* IV();
        GstBuffer* KeyId();

        bool IsClear();
        bool IsValid(); // TODO: name?

        void StripProtection();

    protected:
        virtual void ExtractDecryptMeta(GstBuffer*);

        GstBuffer* _buffer;
        GstBuffer* _subSample;
        size_t _subSampleSize;
        GstBuffer* _initialVec;
        GstBuffer* _keyId;

        bool _isClear;
        GstProtectionMeta* _protectionMeta;
    };
}
}
