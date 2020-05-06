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

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

#include "IExchangeFactory.hpp"
#include "IGstDecryptor.hpp"
#include <memory>

G_BEGIN_DECLS

#define GST_TYPE_CENCDECRYPT (gst_cencdecrypt_get_type())
#define GST_CENCDECRYPT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_CENCDECRYPT, GstCencDecrypt))
#define GST_CENCDECRYPT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_CENCDECRYPT, GstCencDecryptClass))
#define GST_IS_CENCDECRYPT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_CENCDECRYPT))
#define GST_IS_GST_TYPE_CENCDECRYPT_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_CENCDECRYPT))

struct GstCencDecryptImpl;

struct GstCencDecrypt {
    GstBaseTransform base_cencdecrypt;
    std::unique_ptr<GstCencDecryptImpl> _impl;
};

struct GstCencDecryptClass {
    GstBaseTransformClass base_cencdecrypt_class;
};

GType gst_cencdecrypt_get_type(void);

G_END_DECLS
