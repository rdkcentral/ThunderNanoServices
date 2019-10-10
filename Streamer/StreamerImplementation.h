#pragma once

#include "Module.h"
#include <interfaces/IStream.h>
#include "Administrator.h"

namespace WPEFramework {

namespace Plugin {

    class StreamerImplementation : public Exchange::IPlayer {
    private:
        StreamerImplementation(const StreamerImplementation&) = delete;
        StreamerImplementation& operator=(const StreamerImplementation&) = delete;

        class ExternalAccess : public RPC::Communicator {
        private:
            ExternalAccess() = delete;
            ExternalAccess(const ExternalAccess&) = delete;
            ExternalAccess& operator=(const ExternalAccess&) = delete;

        public:
            ExternalAccess(
                const Core::NodeId& source,
                Exchange::IPlayer* parentInterface,
                const string& proxyStubPath,
                const Core::ProxyType<RPC::InvokeServer> & engine)
                : RPC::Communicator(source, proxyStubPath, Core::ProxyType<Core::IIPCServer>(engine))
                , _parentInterface(parentInterface)
            {
                engine->Announcements(Announcement());
                Open(Core::infinite);
            }
            ~ExternalAccess()
            {
                Close(Core::infinite);
            }

        private:
            virtual void* Aquire(const string& className, const uint32_t interfaceId, const uint32_t versionId)
            {
                void* result = nullptr;

                // Currently we only support version 1 of the IRPCLink :-)
                if (((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) && ((interfaceId == Exchange::IPlayer::ID) || (interfaceId == Core::IUnknown::ID))) {
                    // Reference count our parent
                    _parentInterface->AddRef();
                    TRACE(Trace::Information, ("Player interface aquired => %p", this));
                    // Allright, respond with the interface.
                    result = _parentInterface;
                }
                return (result);
            }

        private:
            Exchange::IPlayer* _parentInterface;
        };

    public:
        StreamerImplementation()
            : _adminLock()
            , _administrator(Player::Implementation::Administrator::Instance())
            , _engine()
            , _externalAccess(nullptr)
        {
        }

         ~StreamerImplementation() override;

        BEGIN_INTERFACE_MAP(StreamerImplementation)
        INTERFACE_ENTRY(Exchange::IPlayer)
        END_INTERFACE_MAP

        inline void Lock() const
        {
            _adminLock.Lock();
        }
        inline void Unlock() const
        {
            _adminLock.Unlock();
        }

        // IPlayer Interfaces
        uint32_t Configure(PluginHost::IShell* service) override;
        Exchange::IStream* CreateStream(const Exchange::IStream::streamtype streamType) override;

    private:
        mutable Core::CriticalSection _adminLock;
        Player::Implementation::Administrator& _administrator;
        Core::ProxyType<RPC::InvokeServer> _engine;
        ExternalAccess* _externalAccess;
    };
}
}

