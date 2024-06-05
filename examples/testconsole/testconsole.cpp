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

// WebConsole.cpp : Defines the entry point for the console application.
//

#include "Protocols.h"

using namespace Thunder;

class ProvisioningClient : public Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Core::ProxyPoolType<Web::Response>&> {
public:
    class Info : public Core::JSON::Container {
    private:
        Info(const Info&);
        Info& operator=(const Info&);

    public:
        Info()
        {
            Add(_T("q"), &Provisioning);
        }
        ~Info()
        {
        }

    public:
        Core::JSON::String Provisioning;
    };

private:
    typedef Web::JSONBodyType<Info> ProvisioningInfo;
    typedef Web::WebLinkType<Core::SocketStream, Web::Response, Web::Request, Core::ProxyPoolType<Web::Response>&> BaseClass;

    // All requests needed by any instance of this socket class, is coming from this static factory.
    // This means that all Requests, received are shared among all WebServer sockets, hopefully limiting
    // the number of requests that need to be created.
    static Core::ProxyPoolType<Web::Response> _responseFactory;
    static Core::ProxyPoolType<ProvisioningInfo> _infoBodyFactory;

    ProvisioningClient();
    ProvisioningClient(const ProvisioningClient& copy);
    ProvisioningClient& operator=(const ProvisioningClient&);

public:
    ProvisioningClient(const Core::NodeId& remoteNode)
        : BaseClass(2, _responseFactory, false, remoteNode.AnyInterface(), remoteNode, 2048, 2048)
        , _inProgress(true, true)
        , _provisionRequest(Core::ProxyType<Web::Request>::Create())
    {
    }
    ~ProvisioningClient() override
    {
        Close(Core::infinite);
    }

public:
    bool Provision()
    {
        bool result = false;

        if (_inProgress.Lock(0) == 0) {
            // We locked it, we are in control.
            Open(0);
        }

        return (result);
    }
    // Notification of a Partial Request received, time to attach a body..
    virtual void LinkBody(Core::ProxyType<Web::Response>& element)
    {
        // Time to attach a Command Body
        element->Body<ProvisioningInfo>(_infoBodyFactory.Element());
    }
    virtual void Received(Core::ProxyType<Web::Response>& response)
    {
        string received;
        response->ToString(received);
        printf(_T("Received: %s\n"), received.c_str());
    }
    virtual void Send(const Core::ProxyType<Web::Request>& request)
    {
        ASSERT(request == _provisionRequest);

        // Seems we send, it. Free to submit another one :-)
        _inProgress.Unlock();

        printf("Request is send !!!\n");
        string received;
        request->ToString(received);
        printf(_T("Send: %s\n"), received.c_str());
    }
    virtual void StateChange()
    {
        printf(_T("State change: "));
        if (IsOpen()) {
            char encodedURL[256];

            string test("NGR0000000000000,1,1441271052,ZrH0ImCkN5/RnAjJ55JreiATeUiYrnvCupAH3e34CKs=");

            Core::URL::Encode(test.c_str(), test.length(), encodedURL, sizeof(encodedURL));
            _provisionRequest->Verb = Web::Request::HTTP_GET;
            // _provisionRequest->Path = "";
            _provisionRequest->Host = "provisioning.metrological.com";
            _provisionRequest->Query = string("operator=liberty&q=") + encodedURL;
            _provisionRequest->Connection = Web::Request::CONNECTION_CLOSE;
            _provisionRequest->AcceptEncoding = Web::ENCODING_GZIP;

            Submit(_provisionRequest);
        }
        else {
            _inProgress.Unlock();
            printf(_T("[4] Closed - %s\n"), (IsSuspended() ? _T("SUSPENDED") : _T("OK")));
        }
    }

private:
    Core::Event _inProgress;
    Core::ProxyType<Web::Request> _provisionRequest;
};

Core::ProxyPoolType<Web::Response> ProvisioningClient::_responseFactory(2);
Core::ProxyPoolType<ProvisioningClient::ProvisioningInfo> ProvisioningClient::_infoBodyFactory(2);

// The factory for the CommandResponse messages needs to be created somewhere..
/* static */ Thunder::Core::ProxyPoolType<Thunder::Web::Response> Thunder::TestSystem::WebClient::_responseFactory(5);
/* static */ Thunder::Core::ProxyPoolType<Thunder::Web::TextBody> Thunder::TestSystem::WebClient::_textBodyFactory(5);

/* static */ Thunder::Core::ProxyPoolType<Thunder::Web::Response> Thunder::TestSystem::JSONWebClient::_responseFactory(5);
/* static */ Thunder::Core::ProxyPoolType<Thunder::TestSystem::CommandBody> Thunder::TestSystem::JSONWebClient::_commandBodyFactory(5);
const uint8_t STRESS_ENGINES = 10;

class ConsoleOptions : public Thunder::Core::Options {
public:
    ConsoleOptions(int argumentCount, TCHAR* arguments[])
        : Thunder::Core::Options(argumentCount, arguments, _T("v:hsp:d"))
        , LogLevel()
        , SSL(false)
        , Port(80)
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

//static const TCHAR __HASH_1__[] = _T("The quick brown fox jumps over the lazy dog");
//static const TCHAR __HASH_2__[] = _T("The quick brown fox jumps over the lazy cog");
//static const TCHAR __HASH_3__[] = _T("");
//// HASH1 => 2fd4e1c6 7a2d28fc ed849ee1 bb76e739 1b93eb12
//// Base64 => L9ThxnotKPzthJ7hu3bnORuT6xI=
//Thunder::Core::SHA1 hash1(reinterpret_cast<const uint8_t*>(&__HASH_1__[0]), (sizeof(__HASH_1__) - 1));
//const uint8_t* hashCode = hash1.Result();
//for (uint8_t teller = 0; teller < 20; teller++)
//{
//    printf ("%02X", hashCode[teller]);
//}
//printf("\n");
//string baseHash1;
//Thunder::Core::ToString(hash1.Result(), 5 * sizeof(uint32_t), true, baseHash1);
//printf("Hash1 = %s\n", baseHash1.c_str());

//// Lets see if we can translate it back
//uint8_t buffer[24];
//uint16_t bufferLength = sizeof(buffer);
//uint16_t location = Thunder::Core::FromString(baseHash1, buffer, bufferLength);

//if (baseHash1[location] == '=')
//{
//    printf ("bufferlength : %d\n", bufferLength);
//    for (uint8_t teller = 0; teller < 20; teller++)
//    {
//        printf ("%02X", buffer[teller]);
//    }
//}
//printf("\n");

//// HASH2 => de9f2c7f d25e1b3a fad3e85a 0bd17d9b 100db4b3
//// Base64 => 3p8sf9JeGzr60+haC9F9mxANtLM=
//Thunder::Core::SHA1 hash2(reinterpret_cast<const uint8_t*>(&__HASH_2__[0]), (sizeof(__HASH_2__)-1));
//string baseHash2;
//Thunder::Core::ToString(hash2.Result(), 5 * sizeof(uint32_t), true, baseHash2);
//printf("Hash2 = %s\n", baseHash2.c_str());

//// HASH3 => da39a3ee 5e6b4b0d 3255bfef 95601890 afd80709
//// Base64 => 2jmj7l5rSw0yVb/vlWAYkK/YBwk=
//Thunder::Core::SHA1 hash3(reinterpret_cast<const uint8_t*>(&__HASH_3__[0]), (sizeof(__HASH_3__)-1));
//string baseHash3;
//Thunder::Core::ToString(hash3.Result(), 5 * sizeof(uint32_t), true, baseHash3);
//printf("Hash3 = %s\n", baseHash3.c_str());

typedef enum {
    ExecuteShell,
    WiFiSettings,
    FanControl,
    PlayerControl

} CommandType;

namespace Thunder {
    ENUM_CONVERSION_BEGIN(CommandType)

    { ExecuteShell, _TXT("ExecuteShell") },
    { WiFiSettings, _TXT("WiFiSettings") },
    { FanControl, _TXT("FanControl") },
    { PlayerControl, _TXT("PlayerControl") },

    ENUM_CONVERSION_END(CommandType)
}

class CommandParameters : public Core::JSON::Container {
private:
    CommandParameters(const CommandParameters&);
    CommandParameters& operator=(const CommandParameters&);

public:
    CommandParameters()
    {
        Add(_T("speed"), &Speed);
        Add(_T("duration"), &Duration);
        Add(_T("command"), &Command);
        Add(_T("settings"), &Settings);
    }
    ~CommandParameters()
    {
    }

public:
    Core::JSON::OctSInt16 Speed;
    Core::JSON::DecUInt16 Duration;
    Core::JSON::EnumType<CommandType> Command;
    Core::JSON::ArrayType<Core::JSON::DecUInt16> Settings;
};

class CommandRequest : public Thunder::Core::JSON::Container {
private:
    CommandRequest(const CommandRequest&);
    CommandRequest& operator=(const CommandRequest&);

public:
    CommandRequest()
    {
        Add(_T("id"), &_Id);
        Add(_T("name"), &Name);
        Add(_T("baseAddress"), &BaseAddress);
        Add(_T("trickFlag"), &TrickFlag);
        Add(_T("parameters"), &Params);
    }
    ~CommandRequest()
    {
    }

public:
    Core::JSON::DecUInt32 _Id;
    Core::JSON::String Name;
    Core::JSON::HexUInt32 BaseAddress;
    Core::JSON::Boolean TrickFlag;
    CommandParameters Params;
};

void TestParser1()
{
    Core::ProxyType<Core::JSON::LabelType<CommandRequest> > command = Core::ProxyType<Core::JSON::LabelType<CommandRequest> >::Create();
    Core::ProxyType<Core::JSON::LabelType<CommandRequest> > received = Core::ProxyType<Core::JSON::LabelType<CommandRequest> >::Create();

    command->_Id = 35;
    command->Name = _T("PoepChinees");
    command->BaseAddress = 0x567;
    command->TrickFlag = true;
    command->Params.Duration = 44;
    command->Params.Speed = -12;
    command->Params.Settings.Add(Core::JSON::DecUInt16(6, true));
    command->Params.Settings.Add(Core::JSON::DecUInt16(14, true));
    command->Params.Settings.Add(Core::JSON::DecUInt16(22, true));
    command->Params.Command = PlayerControl;

    string text, response, typeIdentifier;
    Thunder::Core::JSON::Tester<1, Core::JSON::LabelType<CommandRequest> > smallParser;

    // Convert command to JSON(text)
    smallParser.ToString(command, text);
    // Convert JSON(text) to received
    smallParser.FromString(text, received);
    // Convert received to JSON(response)
    smallParser.ToString(received, response);

    if (response == text) {
        fprintf(stderr, "Parsing (smallParser) Successfull\n");
    }
    else {
        fprintf(stderr, "Parsing (smallParser) Failed\n");
    }

    text.clear();
    response.clear();

    //Thunder::Core::JSON::Tester<2048, Core::JSON::LabelType<CommandRequest> > bigParser;
    // Convert command to JSON(text)
    //bigParser.ToString(command, text);
    // Convert JSON(text) to received
    //bigParser.FromString(text, received);
    // Convert received to JSON(response)
    //bigParser.ToString(received, response);

    if (response == text) {
        fprintf(stderr, "Parsing (bigParser) Successfull\n");
    }
    else {
        fprintf(stderr, "Parsing (bigParser) Failed\n");
    }
}

void TestParser2()
{
    const char* text = "{ \"id\"          : 35,\n"
                       "  \"name\"        : \"PoepChinees\",     \n"
                       "  \"baseAddress\" : 0x567,               \n"
                       "  \"trickFlag\"   : true,                \n"
                       "  \"parameters\"  :                      \n"
                       "                   { \"speed\"    : -014,\n"
                       "                     \"duration\" : 44,  \n             "
                       "                     \"command\"  : \"PlayerControl\",\n"
                       "                     \"settings\" : [ 6, 14, 22] \n     "
                       "                   } \n"
                       "}   ";

    string identifier;
    Core::JSON::Tester<1, CommandRequest> smallParser;
    Core::ProxyType<CommandRequest> command = Core::ProxyType<CommandRequest>::Create();

    // Convert JSON(text) to received
    smallParser.FromString(text, command);
}

#ifdef __WIN32__
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char** argv)
#endif
{
    ConsoleOptions options(argc, argv);

    fprintf(stderr, "TestConsole\n"
                    "By Pierre Wielders <pierre@wielders.net>\n");

    if ((argc < 2) || (options.RequestUsage())) {
        fprintf(stderr, "Usage: TestConsole "
                        "<server address> [-p=<p>]\n");
        return (1);
    }

    //	TestParser1 ();
    //	TestParser2 ();

    // Give the server process some time to come up... Only needed for debugging both processes..
    SleepS(5);

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)

    // Scope to make sure that the sockets are destructed before we close the singletons !!!
    {
        // ------------------------------------------------------------------------------------
        // Test stuff
        // ------------------------------------------------------------------------------------
        // Create a parser to see what we will send..
        string message;

        // Factories for JSON objects..
        Thunder::TestSystem::JSONObjectFactory<Thunder::Core::JSON::LabelType<Thunder::DataContainer::Command>>::Instance().CreateFactory<Thunder::Core::JSON::LabelType<Thunder::DataContainer::Command>>(5);

        // ------------------------------------------------------------------------------------
        // [1] BASE TEXT client
        // ------------------------------------------------------------------------------------
        Thunder::TestSystem::TextConnector textConnector(Thunder::Core::NodeId(options.Command(), 12341));

        // ------------------------------------------------------------------------------------
        // [2] BASE JSON client
        // ------------------------------------------------------------------------------------
        Thunder::TestSystem::JSONConnector<Core::JSON::IElement> JSONConnector(Thunder::Core::NodeId(options.Command(), 12342));

        // ------------------------------------------------------------------------------------
        // [3] WEB Client
        // ------------------------------------------------------------------------------------
        Thunder::TestSystem::WebClient webConnector(Thunder::Core::NodeId(options.Command(), 12343));
        Thunder::Core::ProxyType<Thunder::Web::Request> webRequest(Thunder::Core::ProxyType<Thunder::Web::Request>::Create());
        Thunder::Core::ProxyType<Thunder::Web::TextBody> webRequestBody(Thunder::Core::ProxyType<Thunder::Web::TextBody>::Create());
        webRequest->Body<Thunder::Web::TextBody>(webRequestBody);

        // ------------------------------------------------------------------------------------
        // [4] JSON WEB Client
        // ------------------------------------------------------------------------------------
        Thunder::TestSystem::JSONWebClient jsonWebConnector(Thunder::Core::NodeId(options.Command(), 12344));
        Thunder::Core::ProxyType<Thunder::Web::Request> jsonRequest(Thunder::Core::ProxyType<Thunder::Web::Request>::Create());
        Thunder::Core::ProxyType<Thunder::TestSystem::CommandBody> jsonRequestBody(Thunder::Core::ProxyType<Thunder::TestSystem::CommandBody>::Create());
        jsonRequest->Body<Thunder::TestSystem::CommandBody>(jsonRequestBody);

        // ------------------------------------------------------------------------------------
        // [5] WEB SOCKET TEXT LOCAL client
        // ------------------------------------------------------------------------------------
        Thunder::TestSystem::EchoWebSocketClient webSocketLocalConnection(Thunder::Core::NodeId(options.Command(), 12345));

        // ------------------------------------------------------------------------------------
        // [6] WEB SOCKET JSON LOCAL client
        // ------------------------------------------------------------------------------------
        Thunder::TestSystem::JSONWebSocketClient<Core::JSON::IElement> webSocketJSONConnection(Thunder::Core::NodeId(options.Command(), 12346));

        // ------------------------------------------------------------------------------------
        // [7] WEB SOCKET TEXT REMOTE client
        // ------------------------------------------------------------------------------------
        Thunder::TestSystem::EchoWebSocketClient webSocketRemoteConnection(Thunder::Core::NodeId("echo.websocket.org", 80));

        // ------------------------------------------------------------------------------------
        // [8] STRESS TEST TEXT SOCKET
        // ------------------------------------------------------------------------------------
        Thunder::TestSystem::StressInstanceType<Thunder::TestSystem::StressTextConnector>* stressEngines[STRESS_ENGINES];
        for (uint8_t engine = 0; engine < STRESS_ENGINES; engine++) {
            stressEngines[engine] = new Thunder::TestSystem::StressInstanceType<Thunder::TestSystem::StressTextConnector>(Thunder::Core::NodeId(options.Command(), 12348));
        }

        // ------------------------------------------------------------------------------------
        // [9] FILE WEB SERVER
        // ------------------------------------------------------------------------------------
        Thunder::TestSystem::FileClientConnector transferFileConnection;

        // ------------------------------------------------------------------------------------
        // [A] Provision Request
        // ------------------------------------------------------------------------------------
        ProvisioningClient provisioningClient(Thunder::Core::NodeId("provisioning.metrological.com", 80));

        int32_t randomNumber = 4526;
        // Thunder::Core::Random<int32_t> (randomNumber, -70000, +70000);
        printf("Random startup %d\n", randomNumber);

POP_WARNING()

        Thunder::Core::Directory pluginDirectory(_T("/usr/src"));

        while (pluginDirectory.Next() == true) {
            Thunder::Core::File file(pluginDirectory.Current().c_str());

            if (file.Exists()) {
                if (file.IsDirectory()) {
                    printf("Directory Found: %s\n", file.Name().c_str());
                }
                else {
                    printf("File Found: %s\n", file.Name().c_str());
                }
            }
        };

        /* create a client websocket using dumb increment protocol */
        char keyPress;
        int testCount = 0;
        bool openState = false;

        printf("Ready to start processing events, start with 0 to connect.\n");
        do {
            keyPress = toupper(getchar());

            switch (keyPress) {
            case 'A': {
                provisioningClient.Provision();
                [[fallthrough]];
            }
            case 'B': {
                if (openState == true) {
                    webSocketLocalConnection.Ping();
                }
                break;
            }
            case '0': {
                if (openState == false) {
                    printf("Opening up sockets...\n");
                    openState = true;
                    textConnector.Open(0);
                    JSONConnector.Open(0);
                    webConnector.Open(0);
                    jsonWebConnector.Open(0);
                    webSocketLocalConnection.Open(0);
                    webSocketJSONConnection.Open(0);
                    printf("Opening up an external connection. Subject to change !!!!.\n");
                    uint32_t result = webSocketRemoteConnection.Open(1000);
                    SleepMs(1000);
                    printf("Attempted to open external connection. Result: <%d> !!!!.\n", result);
                }
                else {
                    printf("Cosing up sockets...\n");
                    openState = false;
                    textConnector.Close(0);
                    JSONConnector.Close(0);
                    webConnector.Close(0);
                    jsonWebConnector.Close(0);
                    webSocketLocalConnection.Close(0);
                    webSocketJSONConnection.Close(0);
                    webSocketRemoteConnection.Close(0);
                }
                break;
            }
            case '1': {
                string message;
                for (int teller = 0; teller < 1600; teller++) {
                    message += static_cast<TCHAR>('0' + (teller % 10));
                }
                textConnector.Submit(message);
                break;
            }
            case '2': {
                Thunder::Core::ProxyType<Thunder::Core::JSON::LabelType<Thunder::DataContainer::Command> > sendObject = Thunder::Core::ProxyType<Thunder::Core::JSON::LabelType<Thunder::DataContainer::Command> >::Create();

                sendObject->Identifier = testCount++;
                sendObject->Name = _T("TestCommand");
                JSONConnector.Submit(Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>(sendObject));
                break;
            }
            case '3': {
                webRequest->Verb = Thunder::Web::Request::HTTP_GET;
                *webRequestBody = string("Just a nice body to send");
                webConnector.Submit(webRequest);
                break;
            }
            case '4': {
                jsonRequestBody->Identifier = testCount++;
                jsonRequestBody->Name = _T("TestCase4");
                jsonRequestBody->Params.Speed = 4321;

                jsonWebConnector.Submit(jsonRequest);
                break;
            }
            case '5': {
                webSocketLocalConnection.Submit(_T("TestCase5"));
                break;
            }
            case '6': {
                Thunder::Core::ProxyType<Thunder::Core::JSON::LabelType<Thunder::DataContainer::Command> > sendObject = Thunder::Core::ProxyType<Thunder::Core::JSON::LabelType<Thunder::DataContainer::Command> >::Create();

                sendObject->Identifier = testCount++;
                sendObject->Name = _T("TestCase6");
                sendObject->Params.Duration = 100;

                webSocketJSONConnection.Submit(Core::ProxyType<Core::JSON::IElement>(sendObject));
                break;
            }
            case '7': {
                webSocketRemoteConnection.Submit(_T("WebSockets should be able to rock :-)  !!!!"));
                break;
            }
            case '8': {
                printf("Current Time: %s\n", Thunder::Core::Time::Now().ToRFC1123(true).c_str());
                for (uint8_t engine = 0; engine < STRESS_ENGINES; engine++) {
                    stressEngines[engine]->Fire(10000);
                }
                break;
            }
            case '9': {
                printf("Current Time: %s\n", Thunder::Core::Time::Now().ToRFC1123(true).c_str());
                stressEngines[0]->DirectFire(10000);
                break;
            }
            case 'D': {
                Core::File file("F:/windows/TestArea/download/test.lib");
                if (file.Create() == true) {
                    Core::URL urlTarget(_T("http://127.0.0.1:12349/test/test.lib"));
                    transferFileConnection.Download(urlTarget, file,0);
                }
                else {
                    printf(_T("Error [%d] creating file [%s]"), file.ErrorCode(), file.Name().c_str());
                }
                break;
            }
            case 'U': {
                Core::File file("F:/windows/TestArea/test/test.lib");
                if (file.Open(true) == true) {
                    Core::URL urlTarget(_T("http://127.0.0.1:12349/upload/test.lib"));
                    transferFileConnection.Upload(urlTarget, file);
                }
                else {
                    printf(_T("Error [%d] creating file [%s]"), file.ErrorCode(), file.Name().c_str());
                }
                break;
            }
            case 'C': {
                printf("Current status: %s\n", Thunder::Core::Time::Now().ToRFC1123(true).c_str());

                for (uint8_t engine = 0; engine < STRESS_ENGINES; engine++) {
                    printf("\nEngine number:          %d\n", engine);
                    printf("Pending message size:    %d\n", stressEngines[engine]->Pending());
                    printf("Error count:             %d\n", stressEngines[engine]->Errors());
                }
                [[fallthrough]];
            }
            case 'T': {
                printf("Total status: %s\n", Thunder::Core::Time::Now().ToRFC1123(true).c_str());

                uint32_t minSize = static_cast<uint32_t>(~0);
                uint32_t maxSize = 0;
                uint32_t pendingMessages = 0;
                uint32_t errors = 0;
                uint32_t receivedSize = 0;
                uint32_t sendSize = 0;
                uint32_t sendMessages = 0;
                uint32_t receivedMessages = 0;

                for (uint8_t engine = 0; engine < STRESS_ENGINES; engine++) {
                    if (stressEngines[engine]->MinSize() < minSize) {
                        minSize = stressEngines[engine]->MinSize();
                    }
                    if (stressEngines[engine]->MaxSize() > maxSize) {
                        maxSize = stressEngines[engine]->MaxSize();
                    }
                    pendingMessages += stressEngines[engine]->Pending();
                    errors += stressEngines[engine]->Errors();
                    receivedMessages += stressEngines[engine]->MessagesReceived();
                    sendMessages += stressEngines[engine]->MessagesSend();
                    receivedSize += stressEngines[engine]->SizeReceived();
                    sendSize += stressEngines[engine]->SizeSend();
                }
                printf("Min random message size: %d\n", minSize);
                printf("Max random message size: %d\n", maxSize);
                printf("Pending message size:    %d\n", pendingMessages);
                printf("Error count:             %d\n", errors);
                printf("Messages Send:           %d\n", sendMessages);
                printf("Messages Received:       %d\n", receivedMessages);
                printf("Size Send:               %d Mb\n", sendSize);
                printf("Size Received:           %d Mb\n", receivedSize);
            }
            }

        } while (keyPress != 'Q');
    }

    // Clear the factory we created..
    Thunder::TestSystem::JSONObjectFactory<Thunder::Core::JSON::LabelType<Thunder::DataContainer::Command>>::Instance().DestroyFactory(Thunder::Core::JSON::LabelType<Thunder::DataContainer::Command>::Id());
    Thunder::Core::Singleton::Dispose();

    printf("\nLeaving the main App !!!\n");

    return 0;
}
