#include "TestSmartConsumer.h"

namespace Thunder {
namespace Plugin {

    namespace {
        static Metadata<TestSmartConsumer> metadata(1, 0, 0, {}, {}, {});
    }

    TestSmartConsumer::Config::Config()
        : Core::JSON::Container()
        , ProviderCallsign(_T("TestSmartProvider"))
    {
        Add(_T("providercallsign"), &ProviderCallsign);
    }

    TestSmartConsumer::TestSmartConsumer()
        : _service(nullptr)
        , _providerCallsign(_T("TestSmartProvider"))
    {
    }

    TestSmartConsumer::~TestSmartConsumer()
    {
        ASSERT(_service == nullptr);
    }

    const string TestSmartConsumer::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        Config config;
        config.FromString(service->ConfigLine());
        _providerCallsign = config.ProviderCallsign.Value();

        _service = service;
        _service->AddRef();

        const uint32_t result = SmartMath::Open(_service, _providerCallsign);
        if (result != Core::ERROR_NONE) {
            _service->Release();
            _service = nullptr;

            return _T("Failed to open provider monitor");
        }

        Exchange::JMath::Register(*this, this);

        return {};

    }

    void TestSmartConsumer::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service == _service);

        Exchange::JMath::Unregister(*this);

        SmartMath::Close();
        _service->Release();
        _service = nullptr;
    }

    string TestSmartConsumer::Information() const
    {
        return {};
    }

    uint32_t TestSmartConsumer::Add(const uint16_t a, const uint16_t b, uint16_t& sum) const
    {
        const Exchange::IMath* provider = SmartMath::Interface();
        if (provider == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }

        const uint32_t result = provider->Add(a, b, sum);
        provider->Release();
        return result;
    }

    uint32_t TestSmartConsumer::Sub(const uint16_t a, const uint16_t b, uint16_t& sum) const
    {
        const Exchange::IMath* provider = SmartMath::Interface();
        if (provider == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }

        const uint32_t result = provider->Sub(a, b, sum);
        provider->Release();
        return result;
    }

    SERVICE_REGISTRATION(TestSmartConsumer, 1, 0)

} // namespace Plugin
} // namespace Thunder
