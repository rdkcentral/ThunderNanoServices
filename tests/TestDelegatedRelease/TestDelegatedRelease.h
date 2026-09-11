#pragma once

#include "Module.h"

#include <qa_interfaces/ITestDelegatedRelease.h>

#include <vector>

namespace Thunder {
namespace Plugin {

    class TestDelegatedRelease : public PluginHost::IPlugin,
                                 public QualityAssurance::ITestDelegatedRelease {
    public:
        TestDelegatedRelease() = default;
        ~TestDelegatedRelease() override = default;

        TestDelegatedRelease(const TestDelegatedRelease&) = delete;
        TestDelegatedRelease& operator=(const TestDelegatedRelease&) = delete;

        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override { return {}; }

        Core::hresult Ping(uint32_t& value) override;
        Core::hresult HoldPeer(QualityAssurance::ITestDelegatedReleasePeer* peer) override;

        BEGIN_INTERFACE_MAP(TestDelegatedRelease)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(QualityAssurance::ITestDelegatedRelease)
        END_INTERFACE_MAP

    private:
        PluginHost::IShell* _service = nullptr;
        std::vector<QualityAssurance::ITestDelegatedReleasePeer*> _peers;
    };

} // namespace Plugin
} // namespace Thunder