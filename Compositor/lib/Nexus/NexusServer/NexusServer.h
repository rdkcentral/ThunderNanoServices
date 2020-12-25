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
 
#ifndef PLATFORMIMPLEMENTATION_H
#define PLATFORMIMPLEMENTATION_H

#include <core/core.h>
#include <nexus_config.h>
#include <nxserverlib.h>
#include <tracing/tracing.h>

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
                , _rectangle( {0,0,0,0} )
                , _layer(0)
            {
                TRACE(Trace::Information, (_T("Created client named: %s"), _settings.name));
            }

            static Client* Create(nxclient_t client, const NxClient_JoinSettings* settings)
            {
                Client* result = Core::Service<Client>::Create<Client>(client, settings);

                return (result);
            }
            virtual ~Client()
            {
                TRACE(Trace::Information, (_T("Destructing client named: %s"), _settings.name));
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

            string Name() const override;
            void Opacity(const uint32_t value) override;
            uint32_t Geometry(const Exchange::IComposition::Rectangle& rectangle) override;
            Exchange::IComposition::Rectangle Geometry() const override;
            uint32_t ZOrder(const uint16_t index) override;
            uint32_t ZOrder() const override;

            BEGIN_INTERFACE_MAP(Entry)
                INTERFACE_ENTRY(Exchange::IComposition::IClient)
            END_INTERFACE_MAP

        private:
            nxclient_t _client;
            NxClient_JoinSettings _settings;
            Exchange::IComposition::Rectangle _rectangle;
            uint16_t _layer;
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
        Platform(IStateChange* stateChanges, IClient* clientChanges, const string& configuration, const NEXUS_VideoFormat& format);
        virtual ~Platform();

    public:
        inline server_state State() const
        {
            return _state;
        }

    private:
        void Add(nxclient_t client, const NxClient_JoinSettings* joinSettings);
        void Remove(const char clientName[]);
        void StateChange(server_state state);
        static void CloseDown();
        template <typename... T>
        static int ClientConnect(nxclient_t client, const NxClient_JoinSettings* pJoinSettings, NEXUS_ClientSettings* pClientSettings, T... Targs);
        static void ClientDisconnect(nxclient_t client, const NxClient_JoinSettings* pJoinSettings);

    private:
        BKNI_MutexHandle _lock;
        nxserver_t _instance;
        nxserver_settings _serverSettings;
        NEXUS_PlatformSettings _platformSettings;
        NEXUS_PlatformCapabilities _platformCapabilities;
        server_state _state;
        IClient* _clientHandler;
        IStateChange* _stateHandler;
        bool _joined;
        static Platform* _implementation;
    };
}
}
#endif //PLATFORMIMPLEMENTATION_H
