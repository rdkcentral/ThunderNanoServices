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

namespace WPEFramework {
namespace CENCDecryptor {
    class BufferView {
    public:
        BufferView() = delete;
        BufferView(const BufferView&) = delete;
        BufferView& operator=(const BufferView&) = delete;

        explicit BufferView(GstBuffer* buffer, GstMapFlags flags)
            : _buffer(buffer)
        {
            gst_buffer_map(_buffer, &_dataView, flags);
        }

        gsize Size() { return _dataView.size; }
        guint8* Raw() { return _dataView.data; }

        ~BufferView()
        {
            gst_buffer_unmap(_buffer, &_dataView);
        }

    private:
        GstBuffer* _buffer;
        GstMapInfo _dataView;
    };
}
}
