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
 
#include "../Module.h"

#include <bcm_host.h>
#include <interfaces/ICapture.h>

namespace WPEFramework {
namespace Plugin {

    class Initializer {
    private:
        Initializer(const Initializer&) = delete;
        Initializer& operator=(const Initializer&) = delete;

    public:
        Initializer()
        {
            // Init GPU resources.
            bcm_host_init();
        }

        ~Initializer()
        {
            // De-init GPU resources.
            bcm_host_deinit();
        }
    };

    class Dispmanx : public Exchange::ICapture {
    private:
        Dispmanx(const Dispmanx&) = delete;
        Dispmanx& operator=(const Dispmanx&) = delete;

    public:
        Dispmanx()
        {
        }

        virtual ~Dispmanx()
        {
        }

        BEGIN_INTERFACE_MAP(Dispmanx)
        INTERFACE_ENTRY(Exchange::ICapture)
        END_INTERFACE_MAP

        virtual const TCHAR* Name() const
        {
            return (_T("Dispmanx"));
        }

        virtual bool Capture(ICapture::IStore& storer)
        {

            DISPMANX_DISPLAY_HANDLE_T display;
            DISPMANX_MODEINFO_T info;
            DISPMANX_RESOURCE_HANDLE_T resource;
            VC_IMAGE_TYPE_T type = VC_IMAGE_ARGB8888;
            DISPMANX_TRANSFORM_T transform = static_cast<DISPMANX_TRANSFORM_T>(0);
            VC_RECT_T rect;
            uint32_t vc_image_ptr;
            int VARIABLE_IS_NOT_USED status = 0;

            display = vc_dispmanx_display_open(0);

            status = vc_dispmanx_display_get_info(display, &info);
            ASSERT(status == 0);

            uint8_t* buffer = new uint8_t[info.width * 4 * info.height];
            ASSERT(buffer != nullptr);

            resource = vc_dispmanx_resource_create(type, info.width, info.height, &vc_image_ptr);

            vc_dispmanx_snapshot(display, resource, transform);

            status = vc_dispmanx_rect_set(&rect, 0, 0, info.width, info.height);
            ASSERT(status == 0);

            status = vc_dispmanx_resource_read_data(resource, &rect, buffer, info.width * 4);
            ASSERT(status == 0);

            status = vc_dispmanx_resource_delete(resource);
            ASSERT(status == 0);

            status = vc_dispmanx_display_close(display);
            ASSERT(status == 0);

            // Save the buffer to file
            bool result = storer.R8_G8_B8_A8(static_cast<const unsigned char*>(buffer), info.width, info.height);

            delete[] buffer;

            return result;
        }
    };
}

/* static */ Exchange::ICapture* Exchange::ICapture::Instance()
{
    static Plugin::Initializer initializeDisplay;

    return (Core::Service<Plugin::Dispmanx>::Create<Exchange::ICapture>());
}
}
