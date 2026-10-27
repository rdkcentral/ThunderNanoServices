/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Metrological
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

#include "Module.h"
#include <qa_interfaces/IBenchmarkPayload.h>

#include <vector>

namespace Thunder {
namespace Plugin {

    class BenchmarkImplementation
        : public QualityAssurance::IBenchmarkPayload {
    public:
        BenchmarkImplementation(const BenchmarkImplementation&) = delete;
        BenchmarkImplementation& operator=(const BenchmarkImplementation&) = delete;
        BenchmarkImplementation(BenchmarkImplementation&&) = delete;
        BenchmarkImplementation& operator=(BenchmarkImplementation&&) = delete;

        BenchmarkImplementation() = default;

        ~BenchmarkImplementation() override = default;

        BEGIN_INTERFACE_MAP(BenchmarkImplementation)
            INTERFACE_ENTRY(QualityAssurance::IBenchmarkPayload)
        END_INTERFACE_MAP

        // -------------------------------------------------------------------
        // IBenchmarkPayload — echo/loopback endpoints for COM-RPC measurement
        // -------------------------------------------------------------------

        Core::hresult GetPayloadTypes(IPayloadTypeIterator*& types) const override
        {
            std::vector<PayloadType> payloads = {
                PAYLOAD_SMALL, PAYLOAD_MEDIUM, PAYLOAD_LARGE
            };
            using Iterator = RPC::IteratorType<IPayloadTypeIterator>;
            types = Core::ServiceType<Iterator>::Create<IPayloadTypeIterator>(payloads);
            return Core::ERROR_NONE;
        }

        Core::hresult SendUint16(const uint16_t /* value */) override { return Core::ERROR_NONE; }
        Core::hresult SendUint32(const uint32_t /* value */) override { return Core::ERROR_NONE; }
        Core::hresult SendUint64(const uint64_t /* value */) override { return Core::ERROR_NONE; }
        Core::hresult SendBool(const bool /* value */) override { return Core::ERROR_NONE; }
        Core::hresult SendFloat(const float /* value */) override { return Core::ERROR_NONE; }
        Core::hresult SendDouble(const double /* value */) override { return Core::ERROR_NONE; }
        Core::hresult SendString(const string& /* value */) override { return Core::ERROR_NONE; }

        Core::hresult SendSampleData(const IBenchmarkPayload::SampleData& /* data */) override
        {
            return Core::ERROR_NONE;
        }

        Core::hresult SendNoPayload() override { return Core::ERROR_NONE; }

        Core::hresult SendBuffer(const uint16_t /* bufferSize */, const uint8_t[] /* buffer */) override
        {
            return Core::ERROR_NONE;
        }

        Core::hresult SendUint32Vector(const std::vector<uint32_t>& /* data */) override
        {
            return Core::ERROR_NONE;
        }

        Core::hresult SendReceiveUint16(const uint16_t input, uint16_t& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        Core::hresult SendReceiveUint32(const uint32_t input, uint32_t& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        Core::hresult SendReceiveUint64(const uint64_t input, uint64_t& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        Core::hresult SendReceiveBool(const bool input, bool& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        Core::hresult SendReceiveFloat(const float input, float& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        Core::hresult SendReceiveDouble(const double input, double& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        Core::hresult SendReceiveString(const string& input, string& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        Core::hresult SendReceiveSampleData(const IBenchmarkPayload::SampleData& input, IBenchmarkPayload::SampleData& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        Core::hresult SendReceiveBuffer(uint16_t& bufferSize, uint8_t buffer[]) const override
        {
            // Echo — buffer and size remain unchanged
            (void)bufferSize;
            (void)buffer;
            return Core::ERROR_NONE;
        }

        Core::hresult SendReceiveUint32Vector(const std::vector<uint32_t>& input, std::vector<uint32_t>& output) const override
        {
            output = input;
            return Core::ERROR_NONE;
        }

        Core::hresult Add(const uint32_t a, const uint32_t b, uint32_t& result) const override
        {
            result = a + b;
            return Core::ERROR_NONE;
        }

    };

    SERVICE_REGISTRATION(BenchmarkImplementation, 1, 0)

} // namespace Plugin
} // namespace Thunder
