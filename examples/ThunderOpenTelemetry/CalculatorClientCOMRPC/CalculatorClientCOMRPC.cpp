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


//********************************** */


// NOTE generated with AI, might not adhere to all proper Thunder conventions


 //********************************** */




#define MODULE_NAME CalculatorClientCOMRPC

#include <com/com.h>
#include <core/core.h>
#include <interfaces/IMath.h>
#include <plugins/plugins.h>

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

    class CalculatorClientCOMRPC : private RPC::SmartInterfaceType<Exchange::IMathExtended> {
    private:
        using BaseClass = RPC::SmartInterfaceType<Exchange::IMathExtended>;

        class Notification : public Exchange::IMathExtended::INotification {
        public:
            explicit Notification(CalculatorClientCOMRPC& parent)
                : _parent(parent)
            {
            }
            ~Notification() override = default;

            void MultiplyResult(const uint32_t result) override
            {
                _parent.MultiplyResult(result);
            }

            BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(Exchange::IMathExtended::INotification)
            END_INTERFACE_MAP

        private:
            CalculatorClientCOMRPC& _parent;
        };

    public:
        CalculatorClientCOMRPC()
            : BaseClass()
            , _notification(*this)
            , _interface(nullptr)
            , _received(false)
            , _result(0)
        {
        }
        CalculatorClientCOMRPC(const CalculatorClientCOMRPC&) = delete;
        CalculatorClientCOMRPC& operator=(const CalculatorClientCOMRPC&) = delete;

        ~CalculatorClientCOMRPC()
        {
            Close();
        }

        uint32_t Open()
        {
            uint32_t result = BaseClass::Open(3000, BaseClass::Connector(), _T("CalculatorProxy"));
            if (result == Core::ERROR_NONE) {
                _interface = BaseClass::Interface();
                if (_interface == nullptr) {
                    result = Core::ERROR_UNAVAILABLE;
                } else {
                    result = _interface->Register(&_notification);
                    if (result != Core::ERROR_NONE) {
                        _interface->Release();
                        _interface = nullptr;
                    }
                }
            }

            return result;
        }

        void Close()
        {
            if (_interface != nullptr) {
                _interface->Unregister(&_notification);
                _interface->Release();
                _interface = nullptr;
            }
            BaseClass::Close(Core::infinite);
        }

        uint32_t Multiply(const uint16_t A, const uint16_t B)
        {
            if (_interface == nullptr) {
                return Core::ERROR_UNAVAILABLE;
            }

            {
                std::lock_guard<std::mutex> lock(_adminLock);
                _received = false;
            }

            return _interface->Multiply(A, B);
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
        void MultiplyResult(const uint32_t result)
        {
            {
                std::lock_guard<std::mutex> lock(_adminLock);
                _result = result;
                _received = true;
            }
            _resultAvailable.notify_one();
        }

    private:
        Core::SinkType<Notification> _notification;
        Exchange::IMathExtended* _interface;
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

    CalculatorClientCOMRPC client;
    uint32_t result = client.Open();
    if (result != Thunder::Core::ERROR_NONE) {
        std::fprintf(stderr, "Failed to connect to CalculatorProxy: %u\n", result);
        return 2;
    }

    result = client.Multiply(A, B);
    if (result != Thunder::Core::ERROR_NONE) {
        std::fprintf(stderr, "Multiply failed: %u\n", result);
        return 3;
    }

    uint32_t product = 0;
    if (client.WaitForResult(product) == false) {
        std::fprintf(stderr, "Timed out waiting for MultiplyResult.\n");
        return 4;
    }

    std::printf("%u * %u = %u\n", A, B, product);
    return 0;
}
