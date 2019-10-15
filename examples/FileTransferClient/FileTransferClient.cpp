#ifndef MODULE_NAME
#define MODULE_NAME FileTransferClient
#endif

#include <core/core.h>
#include <sys/inotify.h>
#include <iostream>
#include <fstream>
#include <list>

#undef EXTERNAL

using namespace WPEFramework;

namespace WPEFramework {

    class TextConnector : public Core::SocketDatagram {

        class FileUpdate : public Core::Thread {
            private:
                FileUpdate() = delete;
                FileUpdate(const FileUpdate&) = delete;
                FileUpdate& operator=(const FileUpdate&) = delete;

            public:
                FileUpdate(TextConnector& parent, const std::string& path)
                    : Core::Thread(Core::Thread::DefaultStackSize(), _T("FileUpdate"))
                    , _parent(parent)
                    , _path(path)
                    , _newLineQueue()
                {
                    Run();
                }
                virtual ~FileUpdate()
                {
                    Stop();
                }

                void Stop()
                {
                    _parent.Unlock();
                    Core::Thread::Block();
                    Wait(Core::Thread::INITIALIZED | Core::Thread::BLOCKED | Core::Thread::STOPPED, Core::infinite);

                    _newLineQueue.clear();
                }

            private:
                virtual uint32_t Worker()
                {
                    if (_parent.WaitForNewLine(Core::infinite) == true) {
                        std::string line;
                        uint16_t lineNo;

                        do {
                            _parent.GetLine(line, lineNo);
                            _newLineQueue.emplace_back(line);
                        } while (lineNo > 0);

                        // Store to the file
                        std::ofstream storeFile(_path, std::ios_base::app);
                        while(_newLineQueue.size() > 0) {
                            storeFile << _newLineQueue.front();
                            _newLineQueue.pop_front();
                        }
                        _newLineQueue.clear();
                    }
                    return (Core::infinite);
                }

            private:
                TextConnector& _parent;
                std::string _path;
                std::list<std::string> _newLineQueue;
        };

        private:        
            static constexpr uint16_t MAX_BUFFER_LENGHT = 50*1024;

            TextConnector() = delete;
            TextConnector(const TextConnector& copy) = delete;
            TextConnector& operator=(const TextConnector&) = delete;

        public:
            TextConnector(const uint16_t port, const std::string& path)
                : Core::SocketDatagram(false, Core::NodeId("0.0.0.0", port), Core::NodeId(), 0, MAX_BUFFER_LENGHT)
                , _newLineSignal(false, true)
                , _adminLock()
                , _receivedQueue()
                , _update(*this, path)
                
            {
                Open(0);
            }

            virtual ~TextConnector()
            {
                _update.Stop();

                _receivedQueue.clear();
                Close(Core::infinite);
            }

        public: 
            uint16_t ReceiveData(uint8_t *dataFrame, const uint16_t receivedSize) override
            {
                _adminLock.Lock();

                std::string text(reinterpret_cast<const char*>(dataFrame), static_cast<size_t>(receivedSize));
                _receivedQueue.emplace_back(text);                            
                _newLineSignal.SetEvent(); 

                _adminLock.Unlock();
                
                return receivedSize;
            }

            uint16_t SendData(uint8_t *dataFrame, const uint16_t maxSendSize) override
            {
                // Do not do anything here
                return maxSendSize;
            }

            void StateChange() override
            {
                // Do not do anything here
            }

            bool WaitForNewLine(const uint32_t time)
            {
                uint32_t status = _newLineSignal.Lock(time);
                _newLineSignal.ResetEvent();

                return (status == 0);
            }

            void Unlock()
            {
                _newLineSignal.Unlock();
            }

            void GetLine(std::string& text, uint16_t& size)
            {
                _adminLock.Lock();

                if (_receivedQueue.size() > 0) {
                    text = _receivedQueue.front();
                    _receivedQueue.pop_front();
                    size = static_cast<size_t>(_receivedQueue.size());
                }

                _adminLock.Unlock();
            }

        private:
            Core::Event _newLineSignal;
            Core::CriticalSection _adminLock;
            std::list<std::string> _receivedQueue;
            FileUpdate _update;
    };

    class Config : public Core::JSON::Container {
        private:
            Config(const Config &) = delete;
            Config &operator=(const Config &) = delete;

        public:
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
            config.FromFile(configFile);
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
