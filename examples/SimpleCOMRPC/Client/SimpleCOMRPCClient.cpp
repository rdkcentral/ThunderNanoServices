/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the License);
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

#define MODULE_NAME SimpleCOMRPCClient

#include <core/core.h>
#include <com/com.h>
#include <plugins/plugins.h>
#include "../../SimpleCOMRPCInterface/ISimpleCOMRPCInterface.h"
#include <fstream>
#include <chrono>
#include <cmath>
#include <numeric>


MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace Thunder;

class Math : public Exchange::IMath {
public:
    Math(const Math&) = delete;
    Math& operator= (const Math&) = delete;

    Math() {
    }
    ~Math() override {
    }

public:
    // Inherited via IMath
    uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum) const override
    {
        sum = A + B;
        return (Core::ERROR_NONE);
    }
    uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum) const override
    {
        sum = A - B;
        return (Core::ERROR_NONE);
    }

    BEGIN_INTERFACE_MAP(Math)
        INTERFACE_ENTRY(Exchange::IMath)
    END_INTERFACE_MAP
};

class Sink : public Exchange::IWallClock::ICallback {
public:
    Sink(const Sink&) = delete;
    Sink& operator= (const Sink&) = delete;

    Sink(const uint16_t period) 
        : _period(period) {
        printf("Sink constructed!!\n");
    };
    ~Sink() override {
        printf("Sink destructed!!\n");
    }

public:
    BEGIN_INTERFACE_MAP(Sink)
        INTERFACE_ENTRY(Exchange::IWallClock::ICallback)
    END_INTERFACE_MAP

    uint16_t Elapsed(const uint16_t seconds) override {
        printf("The wallclock reports that %d seconds have elapsed since we where armed\n", seconds);
        return(_period);
    }

private:
    const uint16_t _period;
};

enum class ServerType{
    PLUGIN_SERVER,
    STANDALONE_SERVER
};

bool ParseOptions(int argc, char** argv, Core::NodeId& comChannel, ServerType& type, string& callsign)
{
    int index = 1;
    bool showHelp = false;
    comChannel = Core::NodeId(Exchange::SimpleTestAddress);

    while ((index < argc) && (!showHelp)) {
        if (strcmp(argv[index], "-connect") == 0) {
            comChannel = Core::NodeId(argv[index + 1]);
            type = ServerType::STANDALONE_SERVER;
            index++;
        }
        else if (strcmp(argv[index], "-plugin") == 0) {
#ifdef __WINDOWS__
            comChannel = Core::NodeId("127.0.0.1:62000");
#else
            comChannel = Core::NodeId("/tmp/communicator");
#endif
            type = ServerType::PLUGIN_SERVER;
            if ((index + 1) < argc) {
                callsign = string(argv[index + 1]);
            }
            else {
                callsign = _T("SimpleCOMRPCPluginServer");
            }
            index++;
        }
        else if (strcmp(argv[index], "-h") == 0) {
            showHelp = true;
        }
        index++;
    }

    return (showHelp);
}

// Function to calculate average
double calculateAverage(const std::vector<int64_t>& times) {
    return std::accumulate(times.begin(), times.end(), 0LL) / static_cast<double>(times.size());
}

// Function to calculate standard deviation
double calculateStdDev(const std::vector<int64_t>& times, double avg) {
    double variance = 0.0;
    for (const auto& time : times) {
        variance += (time - avg) * (time - avg);
    }
    variance /= times.size();
    return std::sqrt(variance);
}

void runTest(Exchange::IRTTPerformance* performanceTestService, uint32_t dataSize, uint32_t iterations, bool inMemory)
{
    std::vector<int64_t> times;
    uint8_t* data = new uint8_t[dataSize];
    memset(buffer, 'A', size);
    uint8_t* response = new uint8_t[dataSize];
    if(inMemory)
    {
        for (uint32_t i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();

            performanceTestService->SendAndReceive(data);

            auto end = std::chrono::high_resolution_clock::now();
            auto roundTripTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            times.push_back(roundTripTime);
        }
    }
    else
    {

        for (uint32_t i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();

            performanceTestService->SendAndReceive(data, response);

            auto end = std::chrono::high_resolution_clock::now();
            auto roundTripTime = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            times.push_back(roundTripTime);
        }
    }
    free (data);
    free (response);

    int64_t bestTime = *std::min_element(times.begin(), times.end());
    double averageTime = calculateAverage(times);
    double stdDevTime = calculateStdDev(times, averageTime);
    std::cout << "Data size: " << dataSize << " bytes" << std::endl;
    std::cout << "Best time: " << bestTime << " microseconds" << std::endl;
    std::cout << "Average time: " << averageTime << " microseconds" << std::endl;
    std::cout << "Standard deviation: " << stdDevTime << " microseconds" << std::endl;
    std::cout << "----------------------------" << std::endl;

}



int main(int argc, char* argv[])
{
    // The core::NodeId can hold an IPv4, IPv6, domain, HCI, L2CAP or netlink address
    // Here we create a domain socket address
    Core::NodeId comChannel;
    ServerType type;
    string callsign;

    printf("\nSimpleCOMRPCClient is the counterpart for the SimpleCOMRPCServer\n");

    if (ParseOptions(argc, argv, comChannel, type, callsign) == true) {
        printf("Options:\n");
        printf("-connect <IP/FQDN>:<port> [default: %s]\n", Exchange::SimpleTestAddress);
        printf("-plugin <callsign> [use plugin server and not the stand-alone version]\n");
        printf("-h This text\n\n");
    }
    else
    {
        int element;
        Exchange::IWallClock* clock(nullptr);
        Sink* sink = nullptr; 
        Core::ProxyType<RPC::CommunicatorClient> client(Core::ProxyType<RPC::CommunicatorClient>::Create(comChannel));
        Math* outbound = Core::ServiceType<Math>::Create<Math>();
        printf("Channel: %s:[%d]\n\n", comChannel.HostAddress().c_str(), comChannel.PortNumber());

        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
            case 'O':
                printf("Offering our IMath interface to te otherside..!\n");
                if (client->IsOpen() == false) {
                    client->Open(2000);
                }

                if (client->IsOpen() == false) {
                    printf("Could not open a connection to the server. No exchange of interfaces happened!\n");
                }
                else {
                    uint32_t result = client->Offer<Exchange::IMath>(outbound);

                    if (result == Core::ERROR_NONE) {
                        printf("Our IMath nterface has been offered to the other side!\n");
                    }
                    else {
                        printf("Our offer has not been accepted, Error: %d!\n", result);
                    }
                }
                break;
            case 'R':
                printf("Revoking our IMath interface to te otherside..!\n");
                if (client->IsOpen() == true) {
                    uint32_t result = client->Revoke<Exchange::IMath>(outbound);

                    if (result == Core::ERROR_NONE) {
                        printf("Our IMath nterface has been Revoked from the other side!\n");
                    }
                    else {
                        printf("Our revoke has not been accepted, Error: %d!\n", result);
                    }
                }
                break;
            case 'C':
                if (clock != nullptr) {
                    printf("There is no need to create a clock, we already have one!\n");
                } else {
                    if (client->IsOpen() == false) {
                        client->Open(2000);
                    }

                    if (client->IsOpen() == false) {
                        printf("Could not open a connection to the server. No exchange of interfaces happened!\n");
                        break;
                    } else {
                        if (type == ServerType::STANDALONE_SERVER) {
                            clock = client->Acquire<Exchange::IWallClock>(3000, _T("WallClockImplementation"), ~0);
                        }
                        else {
                            Thunder::PluginHost::IShell* controller = client->Acquire<Thunder::PluginHost::IShell>(10000, _T("Controller"), ~0);
                            if (controller == nullptr) {
                                printf("Could not get the IShell* interface from the controller to execute the QueryInterfaceByCallsign!\n");
                            }
                            else {
                                clock = controller->QueryInterfaceByCallsign<Exchange::IWallClock>(callsign);
                                controller->Release();
                            }
                        }
                    }

                    if (clock == nullptr) {
                        client->Close(Core::infinite);
                        if (type == ServerType::STANDALONE_SERVER) {
                            printf("Tried aquiring the IWallclock, but it is not available\n");
                        }
                        else {
                            printf("Tried aquiring the IWallclock, but the plugin (%s) is not available\n", callsign.c_str());

                        }
                    } else {
                        printf("Acquired the IWallclock, ready for use\n");
                    }
                }
                break;
            case 'D':
                if (clock == nullptr) {
                    printf("We can not destroy the clock, because we have no clock :-)\n");
                }
                else {
                    clock->Release();
                    clock = nullptr;
                    printf("Released the IWallclock, no more service available\n");
                }
                break;
            case 'I':
                if (clock == nullptr) {
                    printf("We do not have a clock interface, so we can not get the time\n");
                }
                else {
                    printf("The Ticker is at: %llu\n", clock->Now());
                }
                break;
            case 'T':
                if (clock == nullptr) {
                    printf("We do not have a clock interface, so we can not register the callback\n");
                }
                else if (sink != nullptr) {
                    uint32_t result = clock->Disarm(sink);
                    sink->Release();
                    sink = nullptr;
                    if (result == Core::ERROR_NONE) {
                        printf("We removed the callback from the wallclock. We will nolonger be updated\n");
                    }
                    else if (result == Core::ERROR_NOT_EXIST) {
                        printf("Looks like it was not Armed, or it fired already!\n");
                    }
                    else {
                        printf("Something went wrong, the imlementation reports: %d\n", result);
                    }
                }
                else {
                    sink = Core::ServiceType<Sink>::Create<Sink>(10); // Fire each 10 Seconds
                    uint32_t result = clock->Arm(10, sink);
                    if (result == Core::ERROR_NONE) {
                        printf("We set the callback on the wallclock. We will be updated\n");
                    }
                    else {
                        printf("Something went wrong, the imlementation reports: %d\n", result);
                        sink->Release();
                    }
                }
                break;
	    case 'P':
		{
		    Exchange::IRTTPerformance* perf(nullptr);
		    if (client->IsOpen() == false) {
                        client->Open(2000);
                    }

                    if (client->IsOpen() == false) {
                        printf("Could not open a connection to the server. No exchange of interfaces happened!\n");
                        break;
                    } else {
                        if (type == ServerType::STANDALONE_SERVER) {
                             perf = client->Acquire<Exchange::IRTTPerformance>(3000, _T("PerfImplementation"), ~0);
                        }
                        else {
                            Thunder::PluginHost::IShell* controller = client->Acquire<Thunder::PluginHost::IShell>(10000, _T("Controller"), ~0);
                            if (controller == nullptr) {
                                printf("Could not get the IShell* interface from the controller to execute the QueryInterfaceByCallsign!\n");
                            }
                            else {
                                perf = controller->QueryInterfaceByCallsign<Exchange::IRTTPerformance>("RTTPerformance");
                                controller->Release();
                            }
                        }
                    }

                    if (perf == nullptr) {
                        client->Close(Core::infinite);
                        if (type == ServerType::STANDALONE_SERVER) {
                            printf("Tried aquiring the IRTTPerformance, but it is not available\n");
                        }
                        else {
                            printf("Tried aquiring the IPerformance, but the plugin (%s) is not available\n", "PerfImplementation");

                        }
                    } else {
                            printf("Acquired the IRTTPerformance, ready for use\n");
			    std::vector<size_t> dataSizes = {32, 128, 512, 1024, 4096, 8192, 16384, 32768, 65536, 1024*1024, 2*1024*1024, 4*1024*1024, 16*1024*1024};

			    // Number of iterations for each data size
			    int iterations = 100;

			    // Run the test for each data size
			    printf("****Test with separate buffers for input and output data****");
			    for (size_t dataSize : dataSizes) {
				runTest(perf, dataSize, iterations, true);
			    }
			    printf("****Test with in-memory modification of buffer****");
			    for (size_t dataSize : dataSizes) {
				runTest(perf, dataSize, iterations, false);
			    }
                    }
		}
		break;
            case 'E': exit(0); break;
            case 'Q': break;
            case'?':
                printf("Options available:\n");
                printf("=======================================================================\n");
                printf("<O> Offer the IMath interface to the server, for calculating.\n");
                printf("<C> Create the clock interface, usefull for telling the time.\n");
                printf("<D> Destroy the clock interface, we are nolonger interested in the time.\n");
                printf("<I> Information required, tell  me the current time.\n");
                printf("<T> Toggle subscription for continous updates.\n");
                printf("<Q> We are done playing around, eave the application properly.\n");
		printf("<P> Run RTT Performance tests.\n");
                printf("<E> Eject, this is an emergency, bail out, just kill the app.\n");
                printf("<?> Have no clue what I can do, tell me.\n");
            default: break;
            }

        } while (element != 'Q');

        if (sink != nullptr) {
            clock->Disarm(sink);
            sink->Release();
            sink = nullptr;
        }
        if (client->IsOpen() == true) {
            client->Close(Core::infinite);
        }

        if (outbound != nullptr) {
            outbound->Release();
        }
    }

    Core::Singleton::Dispose();

    return 0;
}
