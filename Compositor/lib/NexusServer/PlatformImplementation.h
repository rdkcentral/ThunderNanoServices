#ifndef PLATFORMIMPLEMENTATION_H
#define PLATFORMIMPLEMENTATION_H

#include <typeinfo>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <string.h>
#include <map>

#include <nexus_config.h>
#include <nxserverlib.h>
#include <thread>


namespace WPEFramework {
namespace Broadcom {

    class Platform {
    private:
        Platform() = delete;

        Platform(const Platform&) = delete;

        Platform& operator=(const Platform&) = delete;

    public:
        class Client {
        private:
            Client() = delete;

            Client(const Client&) = delete;

            Client& operator=(const Client&) = delete;

        public:
            Client(nxclient_t client, const NxClient_JoinSettings* settings)
                : _client(client)
                , _settings(*settings)
            {
                printf("Created client named: %s", _settings.name);
            }

            virtual ~Client()
            {
                printf("Destructing client named: %s", _settings.name);
            }

        public:
            inline bool IsActive() const
            {
                return (_client != nullptr);
            }

            inline const char* Id() const
            {
                return (_settings.name);
            }

            ::std::string Name() const
            {
                return (::std::string(Id()));
            }

            inline nxclient_t Handle() const
            {
                return _client;
            }

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

        struct ICallback {
            virtual ~ICallback() {}

            virtual void Attached(const char clientName[]) = 0;

            virtual void Detached(const char clientName[]) = 0;

            // Signal changes on the subscribed namespace..
            virtual void StateChange(server_state state) = 0;
        };

    public:
        Platform(ICallback* callback, const bool authentication, const uint8_t hardwareDelay, const uint16_t graphicsHeap,
            const uint8_t boxMode, const uint16_t irMode);

        virtual ~Platform();

    public:
        inline server_state State() const {
            return _state;
        }

        Client* GetClient(const ::std::string name);

    private:
        void PlatformReady();

        void Add(nxclient_t client, const NxClient_JoinSettings* joinSettings);

        void Remove(const char clientName[]);

        void StateChange(server_state state);

        static int ClientConnect(nxclient_t client, const NxClient_JoinSettings* pJoinSettings,
            NEXUS_ClientSettings* pClientSettings);

        static void ClientDisconnect(nxclient_t client, const NxClient_JoinSettings* pJoinSettings);

        static void HardwareReady(const uint8_t seconds, Platform* parent);

    private:
        BKNI_MutexHandle _lock;
        nxserver_t _instance;
        nxserver_settings _serverSettings;
        NEXUS_PlatformSettings _platformSettings;
        NEXUS_PlatformCapabilities _platformCapabilities;
        NxClient_JoinSettings _joinSettings;
        server_state _state;
        ::std::map< ::std::string, Client> _nexusClients;
        ICallback* _clientHandler;
        ::std::thread _hardwareready;
        static Platform* _implementation;
    };
}
}
#endif //PLATFORMIMPLEMENTATION_H
