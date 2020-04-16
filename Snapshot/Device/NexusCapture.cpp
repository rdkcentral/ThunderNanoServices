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

#include <interfaces/ICapture.h>

#include <nexus_config.h>
#include <nexus_surface.h>
#include <nexus_surface_client.h>
#include <nxclient.h>

namespace WPEFramework {
namespace Plugin {

    class Initializer {
    private:
        Initializer(const Initializer&) = delete;
        Initializer& operator=(const Initializer&) = delete;

    public:
        Initializer()
        {
            VARIABLE_IS_NOT_USED NEXUS_Error rc = NxClient_Join(NULL);
            ASSERT(!rc);
        }

        ~Initializer()
        {
            NxClient_Uninit();
        }
    };

    class NexusCapture : public Exchange::ICapture {
    private:
        NexusCapture(const NexusCapture&) = delete;
        NexusCapture& operator=(const NexusCapture&) = delete;

    public:
        NexusCapture()
        {
        }
        virtual ~NexusCapture()
        {
        }

        BEGIN_INTERFACE_MAP(NexusCapture)
        INTERFACE_ENTRY(Exchange::ICapture)
        END_INTERFACE_MAP

        virtual const TCHAR* Name() const
        {
            return (_T("NexusCapture"));
        }

        virtual bool Capture(IStore& storer)
        {

            unsigned width = 1280, height = 720; // TODO: read from device or make it configurable
            NEXUS_SurfaceMemory mem;
            NEXUS_SurfaceHandle surface;
            NEXUS_SurfaceCreateSettings createSettings;
            NEXUS_Error rc;

            NEXUS_Surface_GetDefaultCreateSettings(&createSettings);
            createSettings.pixelFormat = NEXUS_PixelFormat_eA8_R8_G8_B8;
            createSettings.width = width;
            createSettings.height = height;

            surface = NEXUS_Surface_Create(&createSettings);

            rc = NxClient_Screenshot(NULL, surface);

            if (rc == 0) {

                NEXUS_Surface_GetMemory(surface, &mem); /* only needed for flush */
                NEXUS_Surface_Flush(surface);

                storer.R8_G8_B8_A8(static_cast<const unsigned char*>(mem.buffer), width, height);
            }

            // Release all surface related allocations
            NEXUS_Surface_Destroy(surface);

            return (rc ? false : true);
        }
    };
}

/* static */ Exchange::ICapture* Exchange::ICapture::Instance()
{
    static Plugin::Initializer initializeDisplay;

    return (Core::Service<Plugin::NexusCapture>::Create<Exchange::ICapture>());
}
}
