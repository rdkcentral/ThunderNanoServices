/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

        const uint32_t result = _smartMath.Open(_service, _providerCallsign);

        if (result != Core::ERROR_NONE) {
            _service->Release();
            _service = nullptr;

            return _T("Failed to open provider interface");
        }

        QualityAssurance::JSmartConsumer::Register(*this, this);

        return {};
    }

    void TestSmartConsumer::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(service == _service);

        QualityAssurance::JSmartConsumer::Unregister(*this);
        _smartMath.Close();

        _service->Release();
        _service = nullptr;
    }

    string TestSmartConsumer::Information() const
    {
        return {};
    }

    uint32_t TestSmartConsumer::Calculate(const uint16_t a, const uint16_t b, uint16_t& addResult, uint16_t& subResult)
    {
        Exchange::IMath* math = _smartMath.Interface();

        if (math == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }

        uint32_t result = math->Add(a, b, addResult);

        if (result == Core::ERROR_NONE) {
            result = math->Sub(a, b, subResult);
        }

        math->Release();

        return result;
    }

    SERVICE_REGISTRATION(TestSmartConsumer, 1, 0)

} // namespace Plugin
} // namespace Thunder