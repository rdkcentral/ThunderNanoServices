#ifndef MODULE_NAME
#define MODULE_NAME CompositorBufferTest
#endif

#include <localtracer/localtracer.h>
#include <tracing/tracing.h>

#include <IAllocator.h>
#include <IBuffer.h>
#include <IRenderer.h>

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

using namespace WPEFramework;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

void GetDRMNodes(const uint32_t type, std::vector<std::string>& list)
{
    const int nDrmDevices(drmGetDevices2(0, nullptr, 0));

    assert(nDrmDevices > 0);

    drmDevicePtr devices[nDrmDevices];

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

int main(int argc, const char* argv[])
{
    Messaging::LocalTracer& tracer = Messaging::LocalTracer::Open();
    Messaging::ConsolePrinter printer(false);

    tracer.Callback(&printer);

    std::vector<string> modules = {
        "Error",
        "Information",
        "Buffer"
    };

    for (auto module : modules) {
        tracer.EnableMessage("CompositorBufferTest", module, true);
        tracer.EnableMessage("CompositorBuffer", module, true);
    }

    TRACE_GLOBAL(Trace::Information, ("Start %s build %s", argv[0], __TIMESTAMP__));
    uint16_t testNumber(1);

    std::vector<std::string> cards;
    GetDRMNodes((DRM_NODE_PRIMARY), cards);

    std::vector<std::string> renders;
    GetDRMNodes((DRM_NODE_RENDER), renders);

    int fdCard = open("/dev/dri/card1", O_RDWR);
    int fdRender = open("/dev/dri/renderD129", O_RDWR);

    assert(fdCard > 0);
    assert(fdRender > 0);

    TRACE_GLOBAL(Trace::Information, ("Test %d", testNumber++));
    Core::ProxyType<Compositor::Interfaces::IAllocator> allocator1 = Compositor::Interfaces::IAllocator::Instance(fdCard);
    assert(allocator1.operator->() != nullptr);

    TRACE_GLOBAL(Trace::Information, ("Test %d", testNumber++));
    Core::ProxyType<Compositor::Interfaces::IAllocator> allocator2 = Compositor::Interfaces::IAllocator::Instance(fdRender);
    assert(allocator2.operator->() != nullptr);
    assert(allocator1.operator->() != allocator2.operator->());

    {
        TRACE_GLOBAL(Trace::Information, ("Test %d", testNumber++));
        Core::ProxyType<Compositor::Interfaces::IAllocator> allocator3 = Compositor::Interfaces::IAllocator::Instance(fdRender);
        assert(allocator3.operator->() != nullptr);
        assert(allocator3.operator->() == allocator2.operator->());
        allocator3.Release();
    }

    {
        TRACE_GLOBAL(Trace::Information, ("Test %d", testNumber++));

        uint64_t mods[1] = { DRM_FORMAT_MOD_LINEAR };
        Compositor::PixelFormat format(DRM_FORMAT_XRGB8888, (sizeof(mods) / sizeof(mods[0])), mods);

        WPEFramework::Core::ProxyType<Compositor::Interfaces::IBuffer> buffer = allocator2->Create(1920, 1080, format);

        assert(buffer.operator->());
        assert(buffer->Width() == 1920);
        assert(buffer->Height() == 1080);
        assert(buffer->Format() == format.Type());

        buffer.Release();
    }

    {
        TRACE_GLOBAL(Trace::Information, ("Test %d", testNumber++));

        Compositor::PixelFormat format(DRM_FORMAT_XRGB8888);

        WPEFramework::Core::ProxyType<Compositor::Interfaces::IBuffer> buffer = allocator1->Create(1920, 1080, format);

        assert(buffer.operator->());

        assert(buffer->Width() == 1920);
        assert(buffer->Height() == 1080);
        assert(buffer->Format() == format.Type());

        buffer.Release();
    }

    allocator2.Release();
    allocator1.Release();

    sleep(2);

    TRACE_GLOBAL(Trace::Information, ("Testing Done..."));
    tracer.Close();
    WPEFramework::Core::Singleton::Dispose();

    return 0;
}