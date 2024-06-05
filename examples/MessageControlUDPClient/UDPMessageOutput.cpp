#include "Module.h"
#include <iostream>
#include <memory>

using namespace Thunder;

class UDPMessageOutput : public Core::Messaging::IOutput {
private:
    class Channel : public Core::SocketDatagram {
    public:
        Channel(UDPMessageOutput& output, const string& binding, const uint16_t port)
            : Core::SocketDatagram(false, Core::NodeId(binding.c_str(), port), Core::NodeId(), 0, Core::Messaging::MessageUnit::DataSize)
            , _output(output)
        {
            Open(0);
        }

        ~Channel() override
        {
            Close(Core::infinite);
        }

        Channel(const Channel&) = delete;
        Channel& operator=(const Channel&) = delete;

        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
        {
            uint16_t length = 0;
            Core::Messaging::Information information;
            Core::ProxyType<Core::Messaging::IEvent> message;

            length = information.Deserialize(dataFrame, receivedSize);

            if (length != 0 && length <= receivedSize) {
                if (information.MessageMetaData().Type() == Core::Messaging::MessageType::TRACING || information.MessageMetaData().Type() == Core::Messaging::MessageType::LOGGING) {
                    message = _factory.Create();
                    length += message->Deserialize(dataFrame + length, receivedSize - length);

                    _output.Output(information, message.Origin());
                }
            }

            return length;
        }

        uint16_t SendData(uint8_t*, const uint16_t) override
        {
            // Do not do anything here
            return 0;
        }

        void StateChange() override
        {
            if (IsOpen()) {
                printf("Observing message socket\n");
            } else {
                printf("No longer observing message socket\n");
            }
        }

    private:
        Messaging::TraceFactory _factory;
        UDPMessageOutput& _output;
    };

    class WorkerThread : public Core::Thread {
    public:
        WorkerThread(Core::QueueType<string>& queue)
            : Core::Thread(Core::Thread::DefaultStackSize(), _T("WorkerThread"))
            , _lineQueue(queue)
        {
            Run();
        }
        ~WorkerThread() override
        {
            Thread::Stop();
            _lineQueue.Disable();
            Wait(Core::Thread::STOPPED, Core::infinite);
        }

        WorkerThread(const WorkerThread&) = delete;
        WorkerThread& operator=(const WorkerThread&) = delete;

    private:
        uint32_t Worker() override
        {
            string line;
            while (_lineQueue.Extract(line, Core::infinite) == true) {
                std::cout << line << std::endl;
            }
            return (Core::infinite);
        }

    private:
        Core::QueueType<string>& _lineQueue;
    };

public:
    UDPMessageOutput(const string& binding, uint16_t port, bool abbreviate)
        : _newLineQueue(256)
        , _update(_newLineQueue)
        , _abbreviated(abbreviate)
        , _channel(*this, binding, port)
    {
    }

    void Output(const Core::Messaging::Information& info, const Core::Messaging::IEvent* message)
    {
        std::ostringstream output;

        const Core::Time now(info.TimeStamp());

        if (_abbreviated == true) {
            string time(now.ToTimeOnly(true));
            output << '[' << time << "]:[" << info.MessageMetaData().Module() << "]:[" << info.MessageMetaData().Category()
                << "]: " << message->Data() << std::endl;
        } else {
            string time(now.ToRFC1123(true));
            output << '[' << time << "]:[" << Core::FileNameOnly(info.FileName().c_str()) << ':' << info.LineNumber()
                << "]:[" << info.ClassName() << "]:[" << info.MessageMetaData().Category() << "]: "
                << message->Data() << std::endl;
        }

        _newLineQueue.Post(output.str());
    }

private:
    Core::QueueType<string> _newLineQueue;
    WorkerThread _update;
    bool _abbreviated;
    Channel _channel;
};

int main(int argc, char** argv)
{
    ASSERT(argc >= 3);

    string binding(argv[1]);
    uint32_t port = std::stoi(argv[2]);
    bool abbreviate = false;
    if (argc >= 4) {
        std::istringstream(string(argv[3])) >> abbreviate;
    }

    UDPMessageOutput output(binding, port, abbreviate);

    char element;
    do {
        printf("\n>");
        element = toupper(getchar());
    } while (element != 'Q');

    std::cout << "Leaving app.\n";
    return 0;
}
