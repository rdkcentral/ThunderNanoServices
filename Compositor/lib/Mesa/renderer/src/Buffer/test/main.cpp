#ifndef MODULE_NAME
#define MODULE_NAME Plugin_Compositor_Implementation
#endif

#include <cstdio>

#include <localtracer/localtracer.h>
#include <tracing/tracing.h>

#include <IBuffer.h>

using namespace WPEFramework;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

constexpr char module[] = "BufferTest";

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
        tracer.EnableMessage("Plugin_Compositor_Implementation", module, true);
    }

    TRACE_GLOBAL(Trace::Information, ("Start %s build %s", argv[0], __TIMESTAMP__));
    uint16_t testNumber(1);

    TRACE_GLOBAL(Trace::Information, ("Test %d", testNumber++));
    Core::ProxyType<Compositor::Interfaces::IBuffer::IAllocator> allocator1 = Compositor::Interfaces::IBuffer::IAllocator::Instance(Compositor::Interfaces::IBuffer::Usage::USE_FOR_RENDERING);
    assert(allocator1.operator->() != nullptr);

    TRACE_GLOBAL(Trace::Information, ("Test %d", testNumber++));
    Core::ProxyType<Compositor::Interfaces::IBuffer::IAllocator> allocator2 = Compositor::Interfaces::IBuffer::IAllocator::Instance(Compositor::Interfaces::IBuffer::Usage::USE_FOR_DISPLAYING);
    assert(allocator2.operator->() != nullptr);
    assert(allocator1.operator->() != allocator2.operator->());

    TRACE_GLOBAL(Trace::Information, ("Test %d", testNumber++));
    Core::ProxyType<Compositor::Interfaces::IBuffer::IAllocator> allocator3 = Compositor::Interfaces::IBuffer::IAllocator::Instance(Compositor::Interfaces::IBuffer::Usage::USE_FOR_RENDERING);
    assert(allocator3.operator->() != nullptr);
    assert(allocator3.operator->() == allocator1.operator->());

    allocator3.Release();
    allocator2.Release();
    allocator1.Release();

    TRACE_GLOBAL(Trace::Information, ("Testing Done..."));
    tracer.Close();
    WPEFramework::Core::Singleton::Dispose();

    return 0;
}