#ifndef PLATFORMIMPLEMENTATION_H
#define PLATFORMIMPLEMENTATION_H

#include <core/core.h>
#include <nexus_config.h>
#include <nxserverlib.h>

#include <interfaces/IComposition.h>

namespace WPEFramework {
namespace Broadcom {

    class Platform {
    private:
        Platform() = delete;
        Platform(const Platform&) = delete;
        Platform& operator=(const Platform&) = delete;

    public:
        class Client : public Exchange::IComposition::IClient {
        private:
            Client() = delete;
            Client(const Client&) = delete;
            Client& operator=(const Client&) = delete;

        public:
            Client(nxclient_t client, const NxClient_JoinSettings* settings)
                : _client(client)
                , _settings(*settings)
            {
                TRACE_L1("Created client named: %s", _settings.name);
            }

            static Client* Create(nxclient_t client, const NxClient_JoinSettings* settings)
            {
                Client* result = Core::Service<Client>::Create<Client>(client, settings);

                return (result);
            }
            virtual ~Client()
            {
                TRACE_L1("Destructing client named: %s", _settings.name);
            }

        public:
            inline bool IsActive() const
            {
                return (_client != nullptr);
            }
            inline nxclient_t Handle() const
            {
                return _client;
            }
            inline const char* Id() const
            {
                return (_settings.name);
            }
            virtual string Name() const override;
            virtual void Kill() override;
            virtual void Opacity(const uint32_t value) override;
            virtual void Geometry(const uint32_t X, const uint32_t Y, const uint32_t width, const uint32_t height) override;
            virtual void Visible(const bool visible) override;
            virtual void SetTop() override;
            virtual void SetInput() override;

            BEGIN_INTERFACE_MAP(Entry)
                INTERFACE_ENTRY(Exchange::IComposition::IClient)
            END_INTERFACE_MAP

        private:
            nxclient_t _client;
            NxClient_JoinSettings _settings;
        };

        enum server_state {
            FAILURE = 0x00,
            UNITIALIZED = 0x01,
            INITIALIZING = 0x02,
            OPERATIONAL = 0x03,
            DEINITIALIZING = 0x04,
        };

        struct IClient {
            virtual ~IClient() {}

            virtual void Attached(Exchange::IComposition::IClient*) = 0;

            virtual void Detached(const char name[]) = 0;
        };

        struct IStateChange {

            virtual ~IStateChange() {}

            // Signal changes on the subscribed namespace..
            virtual void StateChange(server_state state) = 0;
        };

    public:
        Platform(IStateChange* stateChanges, IClient* clientChanges, const string& configuration);
        virtual ~Platform();

    public:
        inline server_state State() const {
            return _state;
        }

    private:
        void Add(nxclient_t client, const NxClient_JoinSettings* joinSettings);
        void Remove(const char clientName[]);
        void StateChange(server_state state);
        static void CloseDown();
        static int ClientConnect(nxclient_t client, const NxClient_JoinSettings* pJoinSettings, NEXUS_ClientSettings* pClientSettings);
        static void ClientDisconnect(nxclient_t client, const NxClient_JoinSettings* pJoinSettings);

    private:
        BKNI_MutexHandle _lock;
        nxserver_t _instance;
        nxserver_settings _serverSettings;
        NEXUS_PlatformSettings _platformSettings;
        NEXUS_PlatformCapabilities _platformCapabilities;
        NxClient_JoinSettings _joinSettings;
        server_state _state;
        IClient* _clientHandler;
        IStateChange* _stateHandler;
        static Platform* _implementation;
    };
}
}
#endif //PLATFORMIMPLEMENTATION_H
