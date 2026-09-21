#include "TestSmartProvider.h"

namespace Thunder {
namespace Plugin {

    namespace {
        static Metadata<TestSmartProvider> metadata(1, 0, 0, {}, {}, {});
    }

    const string TestSmartProvider::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        return {};
    }

    void TestSmartProvider::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
    }

    string TestSmartProvider::Information() const
    {
        return {};
    }

    uint32_t TestSmartProvider::Add(const uint16_t a, const uint16_t b, uint16_t& sum) const
    {
        sum = static_cast<uint16_t>(a + b);
        return Core::ERROR_NONE;
    }

    uint32_t TestSmartProvider::Sub(const uint16_t a, const uint16_t b, uint16_t& sum) const
    {
        sum = static_cast<uint16_t>(a - b);
        return Core::ERROR_NONE;
    }

    SERVICE_REGISTRATION(TestSmartProvider, 1, 0)

} // namespace Plugin
} // namespace Thunder
