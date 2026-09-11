#include "Module.h"
#include "TestDelegatedRelease.h"

namespace Thunder {
namespace Plugin {

    namespace {
        static Metadata<TestDelegatedRelease> metadata(
            1, 0, 0,
            {},
            {},
            {}
        );
    }

    const string TestDelegatedRelease::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        _service = service;
        _service->AddRef();

        return {};
    }

    void TestDelegatedRelease::Deinitialize(PluginHost::IShell* service)
    {
        // The peer references are intentionally left for channel teardown to release.
        if (_service != nullptr) {
            ASSERT(_service == service);
            _service->Release();
            _service = nullptr;
        }
    }

    Core::hresult TestDelegatedRelease::Ping(uint32_t& value)
    {
        value = 0x12345678;
        return Core::ERROR_NONE;
    }

    Core::hresult TestDelegatedRelease::HoldPeer(QualityAssurance::ITestDelegatedReleasePeer* peer)
    {
        ASSERT(peer != nullptr);

        if (peer == nullptr) {
            return Core::ERROR_BAD_REQUEST;
        }

        peer->AddRef();
        _peers.push_back(peer);

        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace Thunder