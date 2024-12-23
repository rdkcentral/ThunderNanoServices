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

#ifndef MODULE_NAME
#define MODULE_NAME CompositorBufferTest
#endif

#include <core/core.h>
#include <localtracer/localtracer.h>
#include <interfaces/ICompositionBuffer.h>
#include <messaging/messaging.h>

#include <IRenderer.h>
#include <IBuffer.h>

#include <drm_fourcc.h>
#include <xf86drm.h>

using namespace Thunder;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

void GetNodess(const uint32_t type, std::vector<std::string>& list)
{
    const int nDrmDevices(drmGetDevices2(0, nullptr, 0));

    assert(nDrmDevices > 0);

    drmDevicePtr* devices = static_cast<drmDevicePtr*>(ALLOCA(nDrmDevices * sizeof(drmDevicePtr)));

    drmGetDevices2(0 /* flags */, devices, nDrmDevices);

    for (int i = 0; i < nDrmDevices; i++) {
        switch (type) {
        case DRM_NODE_PRIMARY: // card<num>, always created, KMS, privileged
        case DRM_NODE_CONTROL: // ControlD<num>, currently unused
        case DRM_NODE_RENDER: // Solely for render clients, unprivileged
        {
            if ((1 << type) == (devices[i]->available_nodes & (1 << type))) {
                list.push_back(std::string(devices[i]->nodes[type]));
            }
            break;
        }
        case DRM_NODE_MAX:
        default: // Unknown (new) node type
            ;
        }
    }

    drmFreeDevices(devices, nDrmDevices);
}

// uint32_t GetConnectors(const int fd, const uint32_t type)
// {
//     uint32_t bitmask = 0;

//     drmModeResPtr resources = drmModeGetResources(fd);

//     if (nullptr != resources) {
//         for (int i = 0; i < resources->count_connectors; i++) {

//             drmModeConnectorPtr connector = drmModeGetConnector(fd, resources->connectors[i]);

//             if (nullptr != connector) {
//                 const char* conn_name = drmModeGetConnectorTypeName(connector->connector_type);

//                 if (conn_name == NULL) {
//                     conn_name = "Unknown";
//                 }

//                 uint32_t possible_crtcs = drmModeConnectorGetPossibleCrtcs(fd, drm_conn);

//                 if (possible_crtcs == 0) {
//                     TRACE_GLOBAL(Trace::Error, ("No crtcs found on %s", conn_name));
//                 }

//                 TRACE_GLOBAL(Trace::Error, ("Connector type type=%s id=%d connection=0x%02X crtcs=0x%04X",
//                         conn_name, connector->connector_type_id, connector->connection, possible_crtcs));

//                 if ((type == connector->connector_type) && (connector->connection == DRM_MODE_CONNECTED)) {
//                     bitmask = bitmask | (1 << i);
//                 }

//                 drmModeFreeConnector(connector);
//             }
//         }

//         drmModeFreeResources(resources);
//     }

//     return bitmask;
// }

int main(int /*argc*/, const char* argv[])
{
    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();
    Messaging::ConsolePrinter printer(false);

    tracer.Callback(&printer);

    const std::vector<string> modules = {
        "CompositorBufferTest",
        "CompositorBuffer"
    };

    for (auto module : modules) {
        tracer.EnableMessage(module, "", true);
    }

    TRACE_GLOBAL(Trace::Information, ("Start %s build %s", argv[0], __TIMESTAMP__));
    uint16_t testNumber(1);

    std::vector<std::string> cards;
    GetNodess((DRM_NODE_PRIMARY), cards);

    std::vector<std::string> renders;
    GetNodess((DRM_NODE_RENDER), renders);

    int fdCard = open("/dev/dri/card0", O_RDWR);
    int fdRender = open("/dev/dri/renderD128", O_RDWR);

    assert(fdCard > 0);
    assert(fdRender > 0);

    {
        TRACE_GLOBAL(Trace::Information, ("Test %d", testNumber++));

        uint64_t mods[1] = { DRM_FORMAT_MOD_LINEAR };
        Compositor::PixelFormat format(DRM_FORMAT_XRGB8888, (sizeof(mods) / sizeof(mods[0])), mods);

        Core::ProxyType<Exchange::ICompositionBuffer> buffer = Compositor::CreateBuffer(fdRender, 1920, 1080, format);

        assert(buffer.operator->());
        assert(buffer->Width() == 1920);
        assert(buffer->Height() == 1080);
        assert(buffer->Format() == format.Type());

        buffer.Release();
    }

    {
        TRACE_GLOBAL(Trace::Information, ("Test %d", testNumber++));

        Compositor::PixelFormat format(DRM_FORMAT_XRGB8888);

        Core::ProxyType<Exchange::ICompositionBuffer> buffer = Compositor::CreateBuffer(fdCard, 1920, 1080, format);

        assert(buffer.operator->());

        assert(buffer->Width() == 1920);
        assert(buffer->Height() == 1080);
        assert(buffer->Format() == format.Type());

        Exchange::ICompositionBuffer::IIterator* index = buffer->Acquire(Compositor::DefaultTimeoutMs);
        assert(index != nullptr);

        while (index->Next() == true) {

            VARIABLE_IS_NOT_USED const uint32_t offset(index->Offset());
            VARIABLE_IS_NOT_USED const uint32_t stride(index->Stride());

            // assert(plane->Accessor() >= 0);
            assert(offset == 0);
            assert(stride == 1920 * (32 / 8));
        }

        index->Reset();
        index->Next();

        buffer.Release();
    }

    TRACE_GLOBAL(Trace::Information, ("Testing Done..."));
    tracer.Close();
    Core::Singleton::Dispose();

    return 0;
}
