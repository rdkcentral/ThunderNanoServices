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

#include "ES1Benchmark.h"
#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace WPEFramework {
namespace Plugin {

    namespace {
        static Metadata<ES1Benchmark> metadata(
            // Version (Major, Minor, Patch)
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );

        // Helper function to get Unix epoch time in microseconds
        inline uint64_t GetUnixMicroseconds() {
            return std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
        }
    }

    const string ES1Benchmark::Initialize(PluginHost::IShell* /* service */)
    {
        Exchange::JES1Benchmark::Register(*this, this);

        // Fire-and-forget HTTP trigger to cold-startup benchmark server (port 8080)
        std::thread([] {
            uint64_t boot_us = GetUnixMicroseconds();
            char req[256];
            ::snprintf(req, sizeof(req),
                "GET /trigger?host=127.0.0.1&port=55555&boot_time_us=%llu HTTP/1.0\r\n"
                "Host: 127.0.0.1\r\n\r\n",
                static_cast<unsigned long long>(boot_us));

            int sock = ::socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) return;
            struct timeval tv { 2, 0 };
            ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
            ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            struct sockaddr_in addr {};
            addr.sin_family      = AF_INET;
            addr.sin_port        = ::htons(8080);
            addr.sin_addr.s_addr = ::inet_addr("127.0.0.1");
            if (::connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0) {
                ::send(sock, req, ::strlen(req), 0);
            }
            ::close(sock);
        }).detach();

        return string();
    }

    void ES1Benchmark::Deinitialize(PluginHost::IShell* /* service */)
    {
        Exchange::JES1Benchmark::Unregister(*this);
    }

    string ES1Benchmark::Information() const
    {
        return string("ES1 JSON-RPC round-trip benchmark echo plugin");
    }

    uint32_t ES1Benchmark::EchoString(const string& value, string& echo, uint64_t& ts2, uint64_t& ts3)
    {
        // TS2: Unix epoch timestamp at API entry (after deserialization)
        ts2 = GetUnixMicroseconds();
        
        echo = value;
        
        // TS3: Unix epoch timestamp before return (before serialization)
        ts3 = GetUnixMicroseconds();
        
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoArray(const std::vector<uint8_t>& values, std::vector<uint8_t>& echo, uint64_t& ts2, uint64_t& ts3)
    {
        ts2 = GetUnixMicroseconds();
        echo = values;
        ts3 = GetUnixMicroseconds();
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoMixedArray(const std::vector<Exchange::IES1Benchmark::MixedElement>& elements, std::vector<Exchange::IES1Benchmark::MixedElement>& echo, uint64_t& ts2, uint64_t& ts3)
    {
        ts2 = GetUnixMicroseconds();
        echo = elements;
        ts3 = GetUnixMicroseconds();
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoNestedObjects(const std::vector<Exchange::IES1Benchmark::NestedObject>& objects, std::vector<Exchange::IES1Benchmark::NestedObject>& echo, uint64_t& ts2, uint64_t& ts3)
    {
        ts2 = GetUnixMicroseconds();
        echo = objects;
        ts3 = GetUnixMicroseconds();
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoUint32(const uint32_t value, uint32_t& echo, uint64_t& ts2, uint64_t& ts3)
    {
        ts2 = GetUnixMicroseconds();
        echo = value;
        ts3 = GetUnixMicroseconds();
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoUint64(const uint64_t value, uint64_t& echo, uint64_t& ts2, uint64_t& ts3)
    {
        ts2 = GetUnixMicroseconds();
        echo = value;
        ts3 = GetUnixMicroseconds();
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoBool(const bool value, bool& echo, uint64_t& ts2, uint64_t& ts3)
    {
        ts2 = GetUnixMicroseconds();
        echo = value;
        ts3 = GetUnixMicroseconds();
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoFloat(const float value, float& echo, uint64_t& ts2, uint64_t& ts3)
    {
        ts2 = GetUnixMicroseconds();
        echo = value;
        ts3 = GetUnixMicroseconds();
        return Core::ERROR_NONE;
    }

    uint32_t ES1Benchmark::EchoDouble(const double value, double& echo, uint64_t& ts2, uint64_t& ts3)
    {
        ts2 = GetUnixMicroseconds();
        echo = value;
        ts3 = GetUnixMicroseconds();
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
