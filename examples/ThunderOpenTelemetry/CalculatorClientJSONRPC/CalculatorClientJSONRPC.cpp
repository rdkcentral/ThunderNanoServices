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

#define MODULE_NAME CalculatorClientJSONRPC

#include <core/core.h>
#include <websocket/websocket.h>

#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <mutex>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace {

    using namespace Thunder;

    class MultiplyParams : public Core::JSON::Container {
    public:
        MultiplyParams(const uint16_t A, const uint16_t B)
            : Core::JSON::Container()
            , A(A)
            , B(B)
        {
            Add(_T("a"), &this->A);
            Add(_T("b"), &this->B);
        }
        MultiplyParams(const MultiplyParams&) = delete;
        MultiplyParams& operator=(const MultiplyParams&) = delete;

        Core::JSON::DecUInt16 A;
        Core::JSON::DecUInt16 B;
    };

    class MultiplyResultParams : public Core::JSON::Container {
    public:
        MultiplyResultParams()
            : Core::JSON::Container()
            , Result(0)
        {
            Add(_T("result"), &Result);
        }
        MultiplyResultParams(const MultiplyResultParams&) = delete;
        MultiplyResultParams& operator=(const MultiplyResultParams&) = delete;

        Core::JSON::DecUInt32 Result;
    };

    class CalculatorClientJSONRPC : private JSONRPC::SmartLinkType<Core::JSON::IElement> {
    private:
        using BaseClass = JSONRPC::SmartLinkType<Core::JSON::IElement>;

    public:
        CalculatorClientJSONRPC()
            : BaseClass(_T("CalculatorProxy"), _T("CalculatorClientJSONRPC"))
            , _subscribed(false)
            , _received(false)
            , _result(0)
        {
        }
        CalculatorClientJSONRPC(const CalculatorClientJSONRPC&) = delete;
        CalculatorClientJSONRPC& operator=(const CalculatorClientJSONRPC&) = delete;

        ~CalculatorClientJSONRPC() override
        {
            if (_subscribed == true) {
                BaseClass::Unsubscribe(3000, _T("multiplyresult"));
            }
        }

        uint32_t Subscribe()
        {
            const uint32_t result = BaseClass::Subscribe<MultiplyResultParams>(
                3000, _T("multiplyresult"), &CalculatorClientJSONRPC::MultiplyResult, this);
            _subscribed = (result == Core::ERROR_NONE);
            return result;
        }

        uint32_t Multiply(const uint16_t A, const uint16_t B)
        {
            {
                std::lock_guard<std::mutex> lock(_adminLock);
                _received = false;
            }

            const MultiplyParams parameters(A, B);
            return BaseClass::Invoke<MultiplyParams, void>(3000, _T("multiply"), parameters);
        }

        bool WaitForResult(uint32_t& result)
        {
            std::unique_lock<std::mutex> lock(_adminLock);
            const bool received = _resultAvailable.wait_for(lock, std::chrono::seconds(5), [this]() {
                return _received;
            });

            if (received == true) {
                result = _result;
            }

            return received;
        }

    private:
        void MultiplyResult(const MultiplyResultParams& parameters)
        {
            {
                std::lock_guard<std::mutex> lock(_adminLock);
                _result = parameters.Result.Value();
                _received = true;
            }
            _resultAvailable.notify_one();
        }

    private:
        bool _subscribed;
        std::mutex _adminLock;
        std::condition_variable _resultAvailable;
        bool _received;
        uint32_t _result;
    };

    bool ParseOperand(const char value[], uint16_t& operand)
    {
        char* end = nullptr;
        errno = 0;
        const unsigned long parsed = std::strtoul(value, &end, 10);

        if ((errno != 0) || (end == value) || (*end != '\0') || (parsed > std::numeric_limits<uint16_t>::max())) {
            return false;
        }

        operand = static_cast<uint16_t>(parsed);
        return true;
    }

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::fprintf(stderr, "Usage: %s <A> <B>\n", argv[0]);
        return 1;
    }

    uint16_t A = 0;
    uint16_t B = 0;
    if ((ParseOperand(argv[1], A) == false) || (ParseOperand(argv[2], B) == false)) {
        std::fprintf(stderr, "Operands must be unsigned 16-bit integers.\n");
        return 1;
    }

    CalculatorClientJSONRPC client;
    uint32_t result = client.Subscribe();
    if (result != Thunder::Core::ERROR_NONE) {
        std::fprintf(stderr, "Failed to subscribe to CalculatorProxy.multiplyresult: %u\n", result);
        return 2;
    }

    result = client.Multiply(A, B);
    if (result != Thunder::Core::ERROR_NONE) {
        std::fprintf(stderr, "CalculatorProxy.multiply failed: %u\n", result);
        return 3;
    }

    uint32_t product = 0;
    if (client.WaitForResult(product) == false) {
        std::fprintf(stderr, "Timed out waiting for CalculatorProxy.multiplyresult.\n");
        return 4;
    }

    std::printf("%u * %u = %u\n", A, B, product);
    return 0;
}
