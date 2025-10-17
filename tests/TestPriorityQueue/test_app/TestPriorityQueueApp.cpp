/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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
#include <interfaces/IMath.h>
#include <condition_variable>

int main(int argc, char* argv[])
{
    // Determine number of worker threads
    uint32_t workers = 4;

    if (argc > 1) {
        const long v = std::strtol(argv[1], nullptr, 10);

        if (v > 0) {
            workers = static_cast<uint32_t>(v);
        }
    }

    const Thunder::Core::NodeId node(Thunder::RPC::SmartInterfaceType<Thunder::PluginHost::IShell>::Connector());

    // Open clients
    std::unique_ptr<Thunder::RPC::SmartInterfaceType<Thunder::Exchange::IMath>[]> clients(new Thunder::RPC::SmartInterfaceType<Thunder::Exchange::IMath>[workers]);
    bool openFailure = false;

    for (uint32_t i = 0; i < workers; ++i) {
        const std::string name = "TestPriorityQueue" + std::to_string(i + 1);
        const uint32_t result = clients[i].Open(4000, node, name);

        if (result != Thunder::Core::ERROR_NONE) {
            std::cerr << "Open failed for " << name << " result: " << result << std::endl;
            openFailure = true;
            break;
        }
    }

    if (openFailure == true) {
        for (uint32_t i = 0; i < workers; ++i) {
            if (clients[i].IsOperational() == true) {
                clients[i].Close(4000);
            }
        }
        return 1;
    }

    // Acquire interface pointers
    std::vector<Thunder::Exchange::IMath*> mathIfs;
    mathIfs.reserve(workers);

    for (uint32_t i = 0; i < workers; ++i) {
        Thunder::Exchange::IMath* interface = clients[i].Interface();

        if (interface == nullptr) {
            std::cerr << "Failed to acquire IMath interface." << std::endl;

            for (auto* p : mathIfs) {
                if (p != nullptr) {
                    p->Release();
                }
            }
            for (uint32_t j = 0; j < workers; ++j) {
                clients[j].Close(4000);
            }
            return 1;
        }
        mathIfs.push_back(interface);
    }

    std::vector<uint16_t> sums(workers, 0);
    std::vector<uint32_t> results(workers, Thunder::Core::ERROR_GENERAL);
    std::mutex mutex;
    std::condition_variable cv;
    uint32_t ready = 0;
    bool go = false;

    // Launch threads
    std::vector<std::thread> threads;
    threads.reserve(workers);

    for (uint32_t i = 0; i < workers; ++i) {
        threads.emplace_back([&, i]() {
            {
                std::unique_lock<std::mutex> lock(mutex);
                if (++ready == workers) {
                    go = true;
                    cv.notify_all();
                } else {
                    cv.wait(lock, [&]() { return go; });
                }
            }
            const uint16_t A = static_cast<uint16_t>((i + 1) * 10);
            const uint16_t B = static_cast<uint16_t>(i + 1);
            results[i] = mathIfs[i]->Add(A, B, sums[i]);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Report results and clean up
    for (uint32_t i = 0; i < workers; ++i) {
        std::cout << "Worker " << i << " - result: " << results[i] << "; sum: " << sums[i] << std::endl;
    }

    for (auto* iface : mathIfs) {
        iface->Release();
    }

    for (uint32_t i = 0; i < workers; ++i) {
        clients[i].Close(4000);
    }

    return 0;
}