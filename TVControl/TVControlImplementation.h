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

        class RequestHandler : public Core::IPCServerType<RPC::InvokeMessage>, public Core::Thread {
        private:
            struct Info {
                Core::ProxyType<RPC::InvokeMessage> message;
                Core::ProxyType<Core::IPCChannel> channel;
            };
            typedef Core::QueueType<Info> MessageQueue;

            RequestHandler(const RequestHandler&) = delete;
            RequestHandler& operator=(const RequestHandler&) = delete;

        public:
            RequestHandler()
                : Core::IPCServerType<RPC::InvokeMessage>()
                , Core::Thread()
                , _handleQueue(64)
                , _handler(RPC::Administrator::Instance())
            {
                TRACE(Trace::Information, (string(__FUNCTION__)));
                Run();
            }
            ~RequestHandler()
            {
                TRACE(Trace::Information, (string(__FUNCTION__)));
                Thread::Stop();
                _handleQueue.Disable();
                Thread::Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
            }

        public:
            virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<RPC::InvokeMessage>& data)
            {
                TRACE(Trace::Information, (string(__FUNCTION__)));
                // Oke, see if we can reference count the IPCChannel.
                Info newElement;
                newElement.channel = Core::ProxyType<Core::IPCChannel>(channel);
                newElement.message = data;

                ASSERT(newElement.channel.IsValid());

                TRACE(Trace::Information, (_T("Received an invoke request !!!")));
                _handleQueue.Insert(newElement, Core::infinite);
            }

            virtual uint32_t Worker()
            {
                TRACE(Trace::Information, (string(__FUNCTION__)));
                Info newRequest;

                while (_handleQueue.Extract(newRequest, Core::infinite)) {

                    _handler.Invoke(newRequest.channel, newRequest.message);

                    Core::ProxyType<Core::IIPC> message(newRequest.message);
                    newRequest.channel->ReportResponse(message);
                }

                return (Core::infinite);
            }

        private:
            MessageQueue _handleQueue;
            RPC::Administrator& _handler;
        };
        class ObjectMessageHandler : public Core::IPCServerType<RPC::ObjectMessage> {
        private:
            ObjectMessageHandler() = delete;
            ObjectMessageHandler(const ObjectMessageHandler&) = delete;
            ObjectMessageHandler& operator=(const ObjectMessageHandler&) = delete;

        public:
            ObjectMessageHandler(TVControlImplementation* parentInterface)
                : _parentInterface(parentInterface)
            {
                TRACE(Trace::Information, (string(__FUNCTION__)));
            }
            ~ObjectMessageHandler()
            {
                TRACE(Trace::Information, (string(__FUNCTION__)));
            }

        public:
            virtual void Procedure(Core::IPCChannel& channel, Core::ProxyType<RPC::ObjectMessage>& data)
            {
                TRACE(Trace::Information, (string(__FUNCTION__)));
                // Oke, see if we can reference count the IPCChannel.
                Core::ProxyType<Core::IPCChannel> refChannel(channel);

                ASSERT(refChannel.IsValid());

                if (refChannel.IsValid()) {
                    const uint32_t interfaceId(data->Parameters().InterfaceId());
                    const uint32_t versionId(data->Parameters().VersionId());

                    TRACE(Trace::Information, (_T("%s: interfaceId=x%x versionId=%d _parentInterface=%p"), __FUNCTION__, interfaceId, versionId, _parentInterface));
                    // Currently we only support version 1 of the TVControlImplementation.
                    if (((versionId == 1) || (versionId == static_cast<uint32_t>(~0)))) {
                        if (interfaceId == Exchange::IStreaming::ID) {
                            // Reference count our parent.
                            Exchange::IStreaming* interface = _parentInterface;
                            interface->AddRef();

                            // Allright, respond with the interface.
                            data->Response().Value(interface);
                        } else if (interfaceId == Exchange::IGuide::ID) {
                            // Reference count our parent.
                            Exchange::IGuide* interface = _parentInterface;
                            interface->AddRef();

                            // Allright, respond with the interface.
                            data->Response().Value(interface);
                        }
                    } else {
                        // Allright, respond with the interface.
                        data->Response().Value(nullptr);
                    }
                }

                Core::ProxyType<Core::IIPC> returnValue(data);
                channel.ReportResponse(returnValue);
            }

        private:
            TVControlImplementation* _parentInterface;
        };

    public:
        ExternalAccess(const Core::NodeId& source, TVControlImplementation* parentInterface)
            : RPC::Communicator(source, Core::ProxyType<RequestHandler>::Create())
            , _handler(Core::ProxyType<ObjectMessageHandler>::Create(parentInterface))
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));

            TRACE(Trace::Information, (_T("%s: parentInterface=%p"), __FUNCTION__, parentInterface));
            RPC::Communicator::CreateFactory<RPC::ObjectMessage>(1);
            RPC::Communicator::Register(_handler);

            Core::SystemInfo::SetEnvironment(_T("TUNER_CONNECTOR"), RPC::Communicator::Connector());
        }
        ~ExternalAccess()
        {
            TRACE(Trace::Information, (string(__FUNCTION__)));
            RPC::Communicator::Unregister(_handler);
            RPC::Communicator::DestroyFactory<RPC::ObjectMessage>();
        }

    private:
        Core::ProxyType<ObjectMessageHandler> _handler;
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
