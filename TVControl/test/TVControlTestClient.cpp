#include <core/core.h>
#include <interfaces/IGuide.h>
#include <interfaces/IStreaming.h>
#include <com/com.h>

using namespace WPEFramework;

class StreamingNotificationListener
    : public Exchange::IStreaming::INotification {
private:
    // StreamingNotificationListener() = delete;
    StreamingNotificationListener(const StreamingNotificationListener&) = delete;

public:
    explicit StreamingNotificationListener()
    {
        printf("TVControlTestClient::Notification constructed. Line: %d\n", __LINE__);
    }
    virtual ~StreamingNotificationListener()
    {
        TRACE_L1("TVControlTestClient::Notification destructed. Line: %d", __LINE__);
    }

    // IStreaming::INotification methods
    void ScanningStateChanged(const uint32_t state)
    {
        printf("Inside %s : state = %u\n", __PRETTY_FUNCTION__, state);
    }
    void CurrentChannelChanged(const string& lcn)
    {
        printf("Inside %s\n channel changed to  = %s", __PRETTY_FUNCTION__, lcn.c_str());
    }
    void TestNotification(const string& msg)
    {
        printf("Inside %s\n", __PRETTY_FUNCTION__);
    }

    BEGIN_INTERFACE_MAP(StreamingNotificationListener)
        INTERFACE_ENTRY(Exchange::IStreaming::INotification)
    END_INTERFACE_MAP
};

class GuideNotificationListener
    : public Exchange::IGuide::INotification {
private:
    // GuideNotificationListener() = delete;
    GuideNotificationListener(const GuideNotificationListener&) = delete;

public:
    explicit GuideNotificationListener()
    {
        printf("TVControlTestClient::Notification constructed. Line: %d\n", __LINE__);
    }
    virtual ~GuideNotificationListener()
    {
        TRACE_L1("TVControlTestClient::Notification destructed. Line: %d", __LINE__);
    }
    // IGuide::INotification methods
    void EITBroadcast()
    {
        printf("Inside %s\n", __PRETTY_FUNCTION__);
    }

    void EmergencyAlert()
    {
        printf("Inside %s\n", __PRETTY_FUNCTION__);
    }

    void ParentalControlChanged()
    {
        printf("Inside %s\n", __PRETTY_FUNCTION__);
    }

    void ParentalLockChanged(const string& lcn)
    {
        printf("Inside %s lock changed for %s\n", __PRETTY_FUNCTION__, lcn.c_str());
    }

    void TestNotification(const string& msg)
    {
        printf("Inside %s\n", __PRETTY_FUNCTION__);
    }

    BEGIN_INTERFACE_MAP(GuideNotificationListener)
        INTERFACE_ENTRY(Exchange::IGuide::INotification)
    END_INTERFACE_MAP
};

class TVControlTestClient {
public:
    TVControlTestClient();
    ~TVControlTestClient();

    void loadProxyStubs(const std::string &proxyStubPath);
    void init();
    void loop();
    void help();
private:
    TVControlTestClient(const TVControlTestClient&) = delete;

    TVControlTestClient& operator=(const TVControlTestClient&) = delete;

    typedef WPEFramework::RPC::InvokeServerType<4, 1> RPCService;

    Core::NodeId nodeId;
    Exchange::IGuide* _guide;
    Exchange::IStreaming* _tuner;
    GuideNotificationListener* _guideNotificationListener;
    StreamingNotificationListener* _streamingNotificationListener;
    std::list <Core::Library> proxyStubs;
    Core::ProxyType<RPC::CommunicatorClient> _client;
    Core::ProxyType<RPCService> _service;
};

TVControlTestClient::TVControlTestClient()
    : nodeId(_T(TUNER_PROCESS_NODE_ID))
    , _guide(nullptr)
    , _tuner(nullptr)
    , _guideNotificationListener(nullptr)
    , _streamingNotificationListener(nullptr)
    , _client(Core::ProxyType<RPC::CommunicatorClient>::Create(nodeId))
    , _service(Core::ProxyType<RPCService>::Create(Core::Thread::DefaultStackSize()))
{
}

TVControlTestClient::~TVControlTestClient()
{
    if (_tuner != nullptr) {
        _tuner->Unregister(_streamingNotificationListener);
        _tuner->Release();
    }
    if (_guide != nullptr) {
        _guide->Release();
        _guide->Unregister(_guideNotificationListener);
    }
}

void TVControlTestClient::init()
{
    printf("Opening connection to server\n");
    _client->CreateFactory<RPC::InvokeMessage>(2);
    if (_client->Open(RPC::CommunicationTimeOut) == Core::ERROR_NONE) {
        SleepMs(100);
        _client->Register(_service);

        // Oke we could open the channel for tunel, lets get the interface.
        _tuner = _client->Create<Exchange::IStreaming>(_T("Tuner"));
        SleepMs(100);

        // Oke we could open the channel for guide, lets get the interface.
        _guide = _client->Create<Exchange::IGuide>(_T("Guide"));
    } else
        _client.Release();


    if (_tuner != nullptr) {
        printf("got IStreaming. The interface pointer = %p\n", _tuner);
        _streamingNotificationListener = Core::Service<StreamingNotificationListener>::Create<StreamingNotificationListener>();
        printf("Registering tuner notification\n");
        _tuner->Register(_streamingNotificationListener);
    } else
        printf("Could not load the IStreaming interface pointer = %p\n", _tuner);

    if (_guide != nullptr) {
        printf("got IGuide. The interface pointer = %p\n", _guide);
        _guideNotificationListener = Core::Service<GuideNotificationListener>::Create<GuideNotificationListener>();
        printf("Registering guide notification\n");
        _guide->Register(_guideNotificationListener);
    } else
        printf("Could not load the IGuide interface pointer = %p\n", _guide);

}

void TVControlTestClient::loadProxyStubs(const std::string &proxyStubPath)
{
    printf("Loading proxy stubs\n");
    // Time to load the ProxyStubs, that are used for InterProcess communication.
    Core::Directory index(proxyStubPath.c_str(), _T("*.so"));

    while (index.Next()) {
        Core::Library library(index.Current().c_str());

        if (library.IsLoaded())
            proxyStubs.push_back(library);
    }
}

void TVControlTestClient::loop()
{
    if (_tuner == nullptr)
        return;

    char inp;
    char str[16];
    do {
        printf(">>");
        inp = ::toupper(::getc(stdin));
        while ((::getc(stdin)) != '\n') { } // clear the rest.

        switch (inp) {
        case 'S':
            printf(">> Start scanning Invoked\n");
            _tuner->StartScan();
            break;
        case 'P': {
            printf(">> Enter oldPin: ");
            fgets(str, sizeof(str)-1, stdin);
            if (str[strlen(str) - 1] == '\n')
                str[strlen(str) - 1] = '\0';
            std::string oldPin = str;
            printf("oldPin = %s\n", oldPin.c_str());
            printf(">> Enter newPin: ");
            fgets(str, sizeof(str)-1, stdin);
            if (str[strlen(str) - 1] == '\n')
                str[strlen(str) - 1] = '\0';
            std::string newPin = str;
            printf("newPin = %s\n", newPin.c_str());
            _guide->SetParentalControlPin(oldPin, newPin);
            break;
        }
        case 'Y': {
            printf(">> Enter isLocked ");
            int isLocked = ::atoi(::fgets(str, sizeof(str)-1, stdin));
            printf(">> Enter Pin: ");
            fgets(str, sizeof(str)-1, stdin);
            if (str[strlen(str) - 1] == '\n')
                str[strlen(str) - 1] = '\0';
            std::string pin = str;
            printf("pin = %s\n", pin.c_str());
            _guide->SetParentalControl(pin, (bool)isLocked);
            break;
        }
        case 'I': {
            printf(">> IsParentalControlled: %d \n", _guide->IsParentalControlled());
            break;
        }
        case 'L': {
            printf(">> Enter Pin: ");
            fgets(str, sizeof(str)-1, stdin);
            if (str[strlen(str) - 1] == '\n')
                str[strlen(str) - 1] = '\0';
            std::string pin = str;
            printf("pin = %s\n", pin.c_str());
            printf(">> Enter isLocked ");
            int isLocked = ::atoi(::fgets(str, sizeof(str)-1, stdin));
            printf(">> Enter channelNumber ");
            std::string channelNumber = ::fgets(str, sizeof(str)-1, stdin);
            printf(" Set lock to %s \n", channelNumber.c_str());
            _guide->SetParentalLock(pin, (bool)isLocked, channelNumber);
            break;
        }
        case 'C': {
            std::string channels = _guide->GetChannels();
            if (channels.size() > 0)
                printf("Channels = %s\n", channels.c_str());
            break;
        }
        case 'E': {
            std::string program =  _guide->GetPrograms();
            if (program.size() > 0)
                printf("Programs = %s\n", program.c_str());
            break;
        }
        case 'F': {
            printf(">> Enter channelNumber ");
            std::string channelNumber = ::fgets(str, sizeof(str)-1, stdin);
            std::string program = _guide->GetCurrentProgram(channelNumber);
            if  (program.size() > 0)
                printf("CurrentProgram = %s\n", program.c_str());
            break;
        }
        case 'A': {
            printf(">> Enter event id ");
            uint32_t channelNumber = ::atoi(::fgets(str, sizeof(str)-1, stdin));
            printf("AudioLanguages = %s\n", _guide->GetAudioLanguages(channelNumber).c_str());
            break;
        }
        case 'B': {
            printf(">> Enter event id ");
            uint32_t channelNumber = ::atoi(::fgets(str, sizeof(str)-1, stdin));
            printf("SubtileLanguages = %s\n", _guide->GetSubtitleLanguages(channelNumber).c_str());
            break;
        }
        case 'Z':
            _tuner->Test(_T("ThisIsAText"));
            break;

        case '?':
            help();
            break;

        case 'T': {
            printf(">> Enter channel number(lcn) : ");
            std::string channelNumber = ::fgets(str, sizeof(str)-1, stdin);
            _tuner->SetCurrentChannel(channelNumber);
            break;
        }

        case 'M' : {
            printf(">> Enter channel number(lcn) : ");
            std::string channelNumber = ::fgets(str, sizeof(str)-1, stdin);
            printf("Channel %s is %s\n", channelNumber.c_str(), (_guide->IsParentalLocked(channelNumber)) ? "locked." : "not locked.");
            break;
        }
        case 'D' : {
            std::string channel = _tuner->GetCurrentChannel();
            if (channel.size() > 0)
                printf("Current channel = %s\n", channel.c_str());
            break;
        }
        case 'X' : {
            printf("Is scanning = %d\n", _tuner->IsScanning());
            break;
        }
        default:
            printf("\n");
        }
    } while (inp != 'Q');
}

void TVControlTestClient::help()
{
    printf("Options to use:\n");
    printf(" p: \tSet Parental Control Pin\n");
    printf(" y: \tSet Parental Control\n");
    printf(" i: \tIs Parental Controlled\n");
    printf(" l: \tSet Parental Lock\n");
    printf(" m: \tIs Parental Locked\n");

    printf(" c: \tGet Channels\n");
    printf(" e: \tGet Programs\n");
    printf(" f: \tGet Current Programs\n");
    printf(" a: \tGet Audio Languages\n");
    printf(" b: \tGet Subtile Languages\n");
    printf(" d: \tGet Current Channel\n");

    printf(" s: \tStart Scan\n");
    printf(" k: \tStop scan\n");
    printf(" t: \tSet Current Channel\n");
    printf(" z: \tTest\n");
    printf(" x: \tIs Scanning\n");
    printf(" ?: \tShow this help\n");
    printf(" -----------------------------------------------\n");
    printf(" q: Quit application\n");
}

int main(int argc, char ** argv)
{
    {
        string proxyStubPath = _T("/usr/lib/wpeframework/proxystubs");
        if (argc >= 2)
            proxyStubPath = string(argv[1]);

        TVControlTestClient testClient;
        testClient.loadProxyStubs(proxyStubPath);
        testClient.init();

        testClient.loop();
    }
    Core::Singleton::Dispose();

    return (0);
}
