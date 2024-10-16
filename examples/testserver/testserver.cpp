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

#include "EchoProtocol.h"

namespace Thunder {
namespace TestSystem {

    extern "C" {

    class ConsoleOptions : public Thunder::Core::Options {
    public:
        ConsoleOptions(int argumentCount, TCHAR* arguments[])
            : Thunder::Core::Options(argumentCount, arguments, _T("v:hsp:d"))
            , LogLevel()
            , SSL(false)
            , Port(9999)
            , Version(1)
        {
            Parse();
        }
        ~ConsoleOptions()
        {
        }

    public:
        Thunder::Core::OptionalType<uint8_t> LogLevel;
        bool SSL;
        uint16_t Port;
        uint16_t Version;

    private:
        virtual void Option(const TCHAR option, const TCHAR* argument)
        {
            switch (option) {
            case 'd':
                LogLevel = atoi(argument);
                break;
            case 's':
                SSL = true;
                break;
            case 'p':
                Port = atoi(argument);
                break;
            case 'v':
                Version = atoi(argument);
                break;
            case 'h':
            default:
                RequestUsage(true);
                break;
            }
        }
    };

    int main(int argc, char** argv)
    {
        ConsoleOptions options(argc, argv);

        fprintf(stderr, "TestServer\n"
                        "By Pierre Wielders <pierre@wielders.net>\n");

        if (options.RequestUsage()) {
            fprintf(stderr, "Usage: TestServer "
                            "<server address> [-p=<p>]\n");
            return (1);
        }

        const TCHAR* address = options.Command();

        if (address == nullptr) {
            address = _T("0.0.0.0");
        }

       //-------------------------------------------------------------------------------------------
        // [5] TEXT WEBSOCKET SERVER !!!!!
        //-------------------------------------------------------------------------------------------
        fprintf(stderr, "[5] Building Web Sockets\n");

        Core::SocketServerType<EchoWebSocketServer> _textWebSocketServer(Thunder::Core::NodeId(address, 12345));

        // Start listening
        _textWebSocketServer.Open(0);

        char keyPress;

        do {
            keyPress = toupper(getchar());

            switch (keyPress) {
            default:
                printf("you pressed %c, only q and s have a meaning here\n", keyPress);
                break;
            }

        } while (keyPress != 'Q');

        printf("\nLeaving the main App !!!\n");
        return 0;
    }

    } // extern "C"
}
} // namespace Thunder::TestSystem
