/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#define MODULE_NAME FileTransferClient
#endif

#include <core/core.h>
#include <iostream>
#include <fstream>

using namespace WPEFramework;

namespace WPEFramework {

    class TextConnector : public Core::SocketDatagram {

        private:
            typedef Core::QueueType<string> EventsQueue;  

        class FileUpdate : public Core::Thread {
            public:
                FileUpdate() = delete;
                FileUpdate(const FileUpdate&) = delete;
                FileUpdate& operator=(const FileUpdate&) = delete;

                FileUpdate(EventsQueue& queue, const std::string& path)
                    : Core::Thread(Core::Thread::DefaultStackSize(), _T("FileUpdate"))
                    , _lineQueue(queue)
                    , _storeFile(path, std::ios_base::app)
                {
                    Run();
                }
                virtual ~FileUpdate()
                {
                    Thread::Stop();
                    _lineQueue.Disable();
                    Wait(Core::Thread::STOPPED, Core::infinite);
                }

            private:
                virtual uint32_t Worker()
                {
                    string line;
                        while (_lineQueue.Extract(line, Core::infinite) == true) { _storeFile << line; }
                    return (Core::infinite);
                }

            private:
                EventsQueue& _lineQueue;
                std::ofstream _storeFile;
        };

        private:     
            static constexpr uint16_t MAX_BUFFER_LENGHT = 50*1024;

        public:
            TextConnector() = delete;
            TextConnector(const TextConnector& copy) = delete;
            TextConnector& operator=(const TextConnector&) = delete;

            TextConnector(const uint16_t port, const std::string& path)
                : Core::SocketDatagram(false, Core::NodeId("0.0.0.0", port), Core::NodeId(), 0, MAX_BUFFER_LENGHT)
                , _newLineQueue(256)
                , _update(_newLineQueue, path)                 
            {
                Open(0);
            }

            virtual ~TextConnector()
            {
                Close(Core::infinite);
            }

            uint16_t ReceiveData(uint8_t *dataFrame, const uint16_t receivedSize) override
            {                            
                _newLineQueue.Post(string(reinterpret_cast<const char*>(dataFrame), static_cast<size_t>(receivedSize)));
                
                return receivedSize;
            }

            uint16_t SendData(uint8_t *dataFrame, const uint16_t maxSendSize) override
            {
                // Do not do anything here
                return 0;
            }

            void StateChange() override
            {
                if (IsOpen()) {
                    printf ("Observing file\n"); 
                } else {
                    printf("No longer observing file\n");
                }                
            }

        private:
            EventsQueue _newLineQueue;
            FileUpdate _update;
    };

    class Config : public Core::JSON::Container {
        public:
            Config(const Config &) = delete;
            Config &operator=(const Config &) = delete;

            class NetworkNode : public Core::JSON::Container
            {
                public:
                    NetworkNode()
                        : Core::JSON::Container(), Port(2201), Binding("0.0.0.0")
                    {
                        Add(_T("port"), &Port);
                        Add(_T("binding"), &Binding);
                    }
                    NetworkNode(const NetworkNode &copy)
                        : Core::JSON::Container(), Port(copy.Port), Binding(copy.Binding)
                    {
                        Add(_T("port"), &Port);
                        Add(_T("binding"), &Binding);
                    }
                    ~NetworkNode()
                    {
                    }
                    NetworkNode &operator=(const NetworkNode &RHS)
                    {
                        Port = RHS.Port;
                        Binding = RHS.Binding;
                        return (*this);
                    }

                public:
                    Core::JSON::DecUInt16 Port;
                    Core::JSON::String Binding;
            };

        public:
            Config()
                : FilePath(_T(EMPTY_STRING)), Source()
            {
                Add(_T("filepath"), &FilePath);
                Add(_T("destination"), &Source);
            }
            ~Config() override {}

        public:
            Core::JSON::String FilePath;
            NetworkNode Source;
    };

    void ShowMenu()
    {
        printf("Enter\n"
            "\t-interface : Interface to listen [obligatory]\n"
            "\t-port : UDP port [obligatory]\n"
            "\t-path : where the logs are stored [obligatory]\n"
            "\t-h : Help\n"
            "\t-q : Quit\n");
    }

    void ParseOptions(int argc, char** argv, Config& config)
    {
        int index = 1;

        while (index < argc) {
            if (strcmp(argv[index], "-interface") == 0) {
                std::string binding(argv[index + 1]);
                config.Source.Binding = binding;
                index++;
            } else if (strcmp(argv[index], "-port") == 0) {
                config.Source.Port = atoi(argv[index + 1]);
                index++;
            } else if (strcmp(argv[index], "-path") == 0) {
                std::string path(argv[index + 1]);
                config.FilePath = path;
                index++;
            } else if (strcmp(argv[index], "-h") == 0) {
                ShowMenu();
            }
            index++;
        }
    }
}

int main(int argc, char** argv)
{
    printf("Enter application.\n");

    Config config;
    ParseOptions(argc, argv, config);

    if ((config.Source.Port.IsSet() == false) ||
        (config.Source.Binding.IsSet() == false) ||
        (config.FilePath.IsSet() == false)) {

        Core::File configFile("FileTransferClient.json", false);
        if (configFile.Exists() && configFile.Open(true)) {
            config.IElement::FromFile(configFile);
            printf(_T("Get configuration from config file\n"));
        } else {
            printf(_T("Wrong parameter, use option -h to check required input arguments\n"));
            return (0);
        }
    }

    printf(_T("Interface IP   : %s\n"), config.Source.Binding.Value().c_str());
    printf(_T("Port           : %d\n"), config.Source.Port.Value());
    printf(_T("Path           : %s\n"), config.FilePath.Value().c_str());

    if (config.Source.Port.IsSet() &&
        config.FilePath.IsSet()) {

        TextConnector connector(config.Source.Port.Value(), config.FilePath.Value());

        int element;
        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
            case '?':
            case 'H':
                ShowMenu();
            }

            SleepMs(200);
        } while (element != 'Q');
    } 

    printf("Leaving app.\n");
    Core::Singleton::Dispose();

    return (0);
}
