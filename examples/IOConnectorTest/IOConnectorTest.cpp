/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#ifndef MODULE_NAME
#define MODULE_NAME IOConnectorTest
#endif

#include <com/com.h>
#include <core/core.h>
#include <websocket/websocket.h>

#include <interfaces/IExternal.h>
#include <interfaces/IInputPin.h>

#define INPUT_PIN_ID    16
#define OUTPUT_PIN_ID   20

namespace Thunder {
namespace Plugin {

    class Sink : public Exchange::IExternal::ICatalog::INotification {
    public:
        Sink(const Sink&) = delete;
        Sink& operator=(const Sink&) = delete;

        Sink() = default;
        ~Sink() override = default;

    public:
        void Activated(Exchange::IExternal* /* source */) override
        {
            printf("Sink::Activated called\n");
        }
        void Deactivated(Exchange::IExternal* /* source */) override
        {
            printf("Sink::Deactivated called\n");
        }
        BEGIN_INTERFACE_MAP(Sink)
        INTERFACE_ENTRY(Exchange::IExternal::ICatalog::INotification)
        END_INTERFACE_MAP
    };

    class InputPinSink : public Exchange::IInputPin::INotification {
    public:
        InputPinSink(const InputPinSink&) = delete;
        InputPinSink& operator=(const InputPinSink&) = delete;

        InputPinSink() = default;
        ~InputPinSink() override = default;

    public:
        void Marker(const uint32_t marker)
        {
            printf("InputPinSink::Marker for %d called\n", marker);
        }
        BEGIN_INTERFACE_MAP(InputPinSink)
        INTERFACE_ENTRY(Exchange::IInputPin::INotification)
        END_INTERFACE_MAP
    };
}
}

using namespace Thunder;

bool ParseOptions(int argc, char** argv, Core::NodeId& comChannel)
{
    int index = 1;
    const char* hostname = _T("/tmp/communicator");
    bool showHelp = false;

    while ((index < argc) && (!showHelp)) {
        if (strcmp(argv[index], "-remote") == 0) {
            hostname = argv[index + 1];
            index++;
        } else if (strcmp(argv[index], "-h") == 0) {
            showHelp = true;
        }
        index++;
    }

    if (!showHelp) {
        comChannel = Core::NodeId(hostname);
    }

    return (showHelp);
}

void ShowMenu()
{
    printf("Enter\n"
           "\tA : Activate output GPIO pin\n"
           "\tD : De-activate output GPIO pin\n"
           "\tR : Read input GPIO pin\n"
           "\tM : Add markers for input GPIO pin\n"
           "\tN : Remove markers for input GPIO pin\n"
           "\tH : Help\n"
           "\tQ : Quit\n");
}

void Activity(const JsonObject& param)
{
    printf("Value for GPIO pin %d changed to %s\n", INPUT_PIN_ID, param["value"].String().c_str());
}

int main(int argc, char** argv)
{
    {
        Core::NodeId comChannel;
        ShowMenu();
        int element;

        ParseOptions(argc, argv, comChannel);

        // Setup framework to connect to COMRPC server
        Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> engine(Core::ProxyType<RPC::InvokeServerType<1, 0, 4>>::Create());
        Core::ProxyType<RPC::CommunicatorClient> client(
            Core::ProxyType<RPC::CommunicatorClient>::Create(
                comChannel,
                Core::ProxyType<Core::IIPCServer>(engine)));

        ASSERT(client.IsValid() == true);

        // Open COMRPC client connection.
        if (client->Open(2000) != Core::ERROR_NONE) {
            printf("Failed to open COMRPC link with the server. Is the server running ?\n");
        }

        // Setup JSONRPC
        Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:80")));
        TCHAR localCallsign[32];
        sprintf(localCallsign, _T("%d"), static_cast<uint32_t>(INPUT_PIN_ID));
        JSONRPC::LinkType<Core::JSON::IElement> jsonrpc(_T("IOConnector.1"), localCallsign, false, "");
        jsonrpc.Subscribe<JsonObject>(1000, _T("activity"), &Activity);

        // Acquire interfaces over comrpc and register notification sinks
        Exchange::IExternal::ICatalog* comrpc = client->Acquire<Exchange::IExternal::ICatalog>(2000, _T("IOConnector"), ~0);
        ASSERT(comrpc != nullptr);
        Core::SinkType<Plugin::Sink> sink;
        comrpc->Register(&sink);

        Exchange::IInputPin::ICatalog* icomrpc = client->Acquire<Exchange::IInputPin::ICatalog>(2000, _T("IOConnector"), ~0);
        ASSERT(icomrpc != nullptr);
        Exchange::IInputPin* inputPin = icomrpc->IInputPinResource(INPUT_PIN_ID);
        ASSERT(inputPin != nullptr);
        Core::SinkType<Plugin::InputPinSink> isink;
        inputPin->Register(&isink);

        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
            case 'A': {
                TCHAR method[32];
                sprintf(method, _T("pin@%d"), static_cast<uint32_t>(OUTPUT_PIN_ID));

                Core::JSON::String response;
                Core::JSON::DecSInt32 value{1};
                uint32_t result = jsonrpc.Invoke(2000, method, value, response);
                if (result != Core::ERROR_NONE) {
                    printf("Failed to activate output GPIO pin\n");
                }
                break;
            }
            case 'D': {
                TCHAR method[32];
                sprintf(method, _T("pin@%d"), static_cast<uint32_t>(OUTPUT_PIN_ID));

                Core::JSON::String response;
                Core::JSON::DecSInt32 value{0};
                uint32_t result = jsonrpc.Invoke(2000, method, value, response);
                if (result != Core::ERROR_NONE) {
                    printf("Failed to activate output GPIO pin\n");
                }
                break;
            }
            case 'R': {
                TCHAR method[32];
                sprintf(method, _T("pin@%d"), static_cast<uint32_t>(INPUT_PIN_ID));

                Core::JSON::DecSInt32 value;
                uint32_t result = jsonrpc.Invoke<void, Core::JSON::DecSInt32>(2000, method, value);
                if (result != Core::ERROR_NONE) {
                    printf("Failed to read input GPIO pin\n");
                } else {
                    printf("Input GPIO pin value: %d\n", value.Value());
                }
                break;
            }
            case 'M': {
                std::array<uint32_t, 3> markers = { 1000, 5000, 10000 };
                for (const auto& m : markers) {
                    inputPin->AddMarker(m);
                }
                break;
            }
            case 'N': {
                std::array<uint32_t, 3> markers = { 1000, 5000, 10000 };
                for (const auto& m : markers) {
                    inputPin->RemoveMarker(m);
                }
                break;
            }
            case '?':
            case 'H':
                ShowMenu();
            }

        } while (element != 'Q');

        inputPin->Release();
        icomrpc->Release();
        comrpc->Release();

        // Close comrpc connection
        client->Close(Core::infinite);
        client.Release();
    }

    printf("Leaving app.\n");

    Core::Singleton::Dispose();

    return (0);
}
