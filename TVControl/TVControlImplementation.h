#ifndef __TVCONTROLIMPLEMENTATION_H
#define __TVCONTROLIMPLEMENTATION_H

#include "TVControl.h"
#include "ITableData.h"
#include "ITableDataHandler.h"
#include "ITuner.h"
#include <interfaces/ITVPlatform.h>

namespace WPEFramework {
namespace Plugin {

class TVControlImplementation
    : public Exchange::IStreaming
    , public Exchange::IGuide
    , public TVPlatform::ITVPlatform::ITunerHandler
    , public ITableDataHandler {

public:
    class Config : public Core::JSON::Container {
    private:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

    public:
        Config()
            : Core::JSON::Container()
              , Location()
        {
            Add(_T("location"), &Location);
        }
        ~Config()
        {
        }

    public:
        Core::JSON::String Location;
};

private:
    TVControlImplementation(const TVControlImplementation&) = delete;
    TVControlImplementation& operator=(const TVControlImplementation&) = delete;

private:
    class ExternalAccess : public RPC::Communicator {
    private:
        ExternalAccess() = delete;
        ExternalAccess(const ExternalAccess&) = delete;
        ExternalAccess& operator=(const ExternalAccess&) = delete;

    public:
        ExternalAccess(const Core::NodeId& source, TVControlImplementation* parentInterface, const string& proxyStubPath)
            : RPC::Communicator(source, Core::ProxyType< RPC::InvokeServerType<4,1> >::Create(), proxyStubPath)
            , _interface(parentInterface)
        {
            //TRACE(Trace::Information, (_T("%s: parentInterface=%p"), __FUNCTION__, parentInterface));
            TRACE_L1("%s: parentInterface=%p", __FUNCTION__, parentInterface);
            //RPC::Communicator::CreateFactory<RPC::ObjectMessage>(1);
            //RPC::Communicator::Register(_handler);

            Core::SystemInfo::SetEnvironment(_T("TUNER_CONNECTOR"), RPC::Communicator::Connector());
        }
        ~ExternalAccess()
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
        }

    private:
        virtual void* Aquire (const string& /* className */, const uint32_t interfaceId, const uint32_t /* version */) {

            void* result = nullptr;

            if (interfaceId == Exchange::IStreaming::ID) {
                result = static_cast<Exchange::IStreaming*>(_interface);
                _interface->AddRef();
            }
            else if (interfaceId == Exchange::IGuide::ID) {
                result = static_cast<Exchange::IGuide*>(_interface);
                _interface->AddRef();
            }

            return (nullptr);
        }

    private:
        TVControlImplementation* _interface;
    };

public:
    TVControlImplementation();
    virtual ~TVControlImplementation();

    BEGIN_INTERFACE_MAP(TVControlImplementation)
        INTERFACE_ENTRY(Exchange::IStreaming)
        INTERFACE_ENTRY(Exchange::IGuide)
    END_INTERFACE_MAP

    // IStreaming methods
    uint32_t Configure(PluginHost::IShell* service);
    void Register(Exchange::IStreaming::INotification* sink);
    void Unregister(Exchange::IStreaming::INotification* sink);

    // IGuide methods.
    void Register(Exchange::IGuide::INotification* sink);
    void Unregister(Exchange::IGuide::INotification* sink);

    void StartScan();
    void StopScan();
    void SetCurrentChannel(const std::string&);
    void Test(const string& name);

    uint32_t StartParser(PluginHost::IShell* service);
    bool SetParentalControlPin(const string&, const string&);
    bool SetParentalControl(const string& Pin, const bool isLocked);
    bool IsParentalControlled();
    const std::string GetCurrentChannel();
    bool IsParentalLocked(const string&);
    bool SetParentalLock(const string& pin, bool isLocked , const string&);
    const std::string GetChannels();
    const std::string GetPrograms(); // Get all programs of all channels for 3hr.
    const std::string GetCurrentProgram(const string&);
    const std::string GetAudioLanguages(const uint32_t);
    const std::string GetSubtitleLanguages(const uint32_t);
    bool IsScanning();

    // ITunerHandler methods.
    void ScanningStateChanged(const ScanningState);
    void StreamingFrequencyChanged(uint32_t);
     void CurrentChannelChanged(const string&);
    // ITableDataHandler methods.
    void EITBroadcast();
    void EmergencyAlert();
    void ParentalControlChanged();
    void ParentalLockChanged(const string&);

private:
    Core::CriticalSection _adminLock;
    PluginHost::IShell* _service;
    std::list<Exchange::IStreaming::INotification*> _streamingClients;
    std::list<Exchange::IGuide::INotification*> _guideClients;
    Core::Library _tvPlatformSystem;
    Core::NodeId _nodeId;
    ExternalAccess* _externalAccess;
    ITuner* _tuner;
    ITableData* _tableData;
    ChannelMap chanMap;
};
}
}

#endif
