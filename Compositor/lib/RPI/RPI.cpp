/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "Module.h"

#include <interfaces/IComposition.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

namespace Thunder {
namespace Plugin {

    class CompositorImplementation : public Exchange::IComposition {
    private:
        CompositorImplementation(const CompositorImplementation&) = delete;
        CompositorImplementation& operator=(const CompositorImplementation&) = delete;

        class ClientHandler {
        private:
            using Client = std::pair<const IUnknown*, bool>;
            using Clients = std::vector<Client>;

        private:
            ClientHandler() = delete;
            ClientHandler(const ClientHandler&) = delete;
            ClientHandler& operator=(const ClientHandler&) = delete;

        public:
            ClientHandler(CompositorImplementation& parent)
                : _clients()
                , _adminLock()
                , _parent(parent)
                , _job(*this)
            {
            }
            ~ClientHandler()
            {
                _job.Revoke();
                _clients.clear();
            }

        public:
            void Offer(const Core::IUnknown* element)
            {
                _adminLock.Lock();
                element->AddRef();
                _clients.push_back({ element, true });
                _job.Submit();
                _adminLock.Unlock();
            }

            void Revoke(const Core::IUnknown* element)
            {
                _adminLock.Lock();
                element->AddRef();
                _clients.push_back({ element, false });
                _job.Submit();
                _adminLock.Unlock();
            }

            void Dispatch()
            {
                _adminLock.Lock();

                while (_clients.size()) {
                    Client client = _clients.back();
                    _clients.pop_back();
                    _adminLock.Unlock();

                    if (client.second == true) {
                        _parent.NewClientOffered(const_cast<IUnknown*>(client.first));
                    } else {
                        _parent.ClientRevoked(client.first);
                    }
                    client.first->Release();

                    _adminLock.Lock();
                }
                _adminLock.Unlock();
            }

        private:
            Clients _clients;
            Core::CriticalSection _adminLock;
            CompositorImplementation& _parent;
            Core::WorkerPool::JobType<ClientHandler&> _job;
        };

        class ExternalAccess : public RPC::Communicator {
        private:
            ExternalAccess() = delete;
            ExternalAccess(const ExternalAccess&) = delete;
            ExternalAccess& operator=(const ExternalAccess&) = delete;

        public:
            ExternalAccess(
                CompositorImplementation& parent,
                const Core::NodeId& source,
                const string& proxyStubPath,
                const Core::ProxyType<RPC::InvokeServer>& handler)
                : RPC::Communicator(source, proxyStubPath.empty() == false ? Core::Directory::Normalize(proxyStubPath) : proxyStubPath, Core::ProxyType<Core::IIPCServer>(handler))
                , _handler(parent)
            {
                uint32_t result = RPC::Communicator::Open(RPC::CommunicationTimeOut);

                if (result != Core::ERROR_NONE) {
                    TRACE(Trace::Error, (_T("Could not open RPI Compositor RPCLink server. Error: %s"), Core::NumberType<uint32_t>(result).Text()));
                } else {
                    // We need to pass the communication channel NodeId via an environment variable, for process,
                    // not being started by the rpcprocess...
                    Core::SystemInfo::SetEnvironment(_T("COMPOSITOR"), RPC::Communicator::Connector(), true);
                }
            }

            ~ExternalAccess() override = default;

        private:
            void Offer(Core::IUnknown* element, const uint32_t interfaceID VARIABLE_IS_NOT_USED) override
            {
                _handler.Offer(element);
            }

            void Revoke(const Core::IUnknown* element, const uint32_t interfaceID VARIABLE_IS_NOT_USED) override
            {
                _handler.Revoke(element);
            }

            virtual void Dangling(const Core::IUnknown* element, const uint32_t interfaceID)
            {
                if (interfaceID == Exchange::IComposition::IClient::ID) {
                    _handler.Revoke(element);
                }
            }

        private:
            CompositorImplementation::ClientHandler _handler;
        };

    public:
        CompositorImplementation()
            : _adminLock()
            , _engine()
            , _externalAccess(nullptr)
            , _observers()
            , _clients()
        {
        }

        ~CompositorImplementation() override
        {
            if (_externalAccess != nullptr) {
                delete _externalAccess;
                _engine.Release();
            }
        }

        BEGIN_INTERFACE_MAP(CompositorImplementation)
        INTERFACE_ENTRY(Exchange::IComposition)
        END_INTERFACE_MAP

    private:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Connector(_T("/tmp/compositor"))
            {
                Add(_T("connector"), &Connector);
            }

            ~Config() override = default;

        public:
            Core::JSON::String Connector;
        };

        struct ClientData {
            ClientData()
                : currentRectangle()
                , clientInterface(nullptr)
            {
            }
            ClientData(Exchange::IComposition::IClient* client, Exchange::IComposition::ScreenResolution resolution)
                : currentRectangle()
                , clientInterface(client)
            {
                clientInterface->AddRef();
                currentRectangle.x = 0;
                currentRectangle.y = 0;
                currentRectangle.width = Exchange::IComposition::WidthFromResolution(resolution);
                currentRectangle.height = Exchange::IComposition::HeightFromResolution(resolution);
            }
            ~ClientData()
            {
                if (clientInterface != nullptr) {
                    clientInterface->Release();
                }
            }

            Exchange::IComposition::Rectangle currentRectangle;
            Exchange::IComposition::IClient* clientInterface;
        };

    public:
        uint32_t Configure(PluginHost::IShell* service) override
        {
            uint32_t result = Core::ERROR_NONE;

            string configuration(service->ConfigLine());
            Config config;
            config.FromString(service->ConfigLine());

            _engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::IWorkerPool::Instance());
            ASSERT(_engine.IsValid() == true);

            _externalAccess = new ExternalAccess(*this, Core::NodeId(config.Connector.Value().c_str()), service->ProxyStubPath(), _engine);
            ASSERT(_externalAccess != nullptr);

            if (_externalAccess->IsListening() == true) {
                PlatformReady(service);

            } else {
                delete _externalAccess;
                _externalAccess = nullptr;
                _engine.Release();
                TRACE(Trace::Error, (_T("Could not report PlatformReady as there was a problem starting the Compositor RPC %s"), _T("server")));
                result = Core::ERROR_OPENING_FAILED;
            }
            return result;
        }

        void Register(Exchange::IComposition::INotification* notification) override
        {
            ASSERT(notification != nullptr);

            _adminLock.Lock();

            std::list<Exchange::IComposition::INotification*>::iterator it(
                std::find(_observers.begin(), _observers.end(), notification));

            ASSERT(it == _observers.end());

            if (it == _observers.end()) {
                notification->AddRef();
                _observers.push_back(notification);

                auto index(_clients.begin());
                while (index != _clients.end()) {
                    notification->Attached(index->first, index->second.clientInterface);
                    index++;
                }
            }
            _adminLock.Unlock();
        }

        void Unregister(Exchange::IComposition::INotification* notification) override
        {
            ASSERT(notification != nullptr);

            _adminLock.Lock();
            std::list<Exchange::IComposition::INotification*>::iterator index(
                std::find(_observers.begin(), _observers.end(), notification));

            ASSERT(index != _observers.end());

            if (index != _observers.end()) {
                _observers.erase(index);
                notification->Release();
            }
            _adminLock.Unlock();
        }

    public:
        uint32_t Resolution(const Exchange::IComposition::ScreenResolution format) override
        {
            TRACE(Trace::Information, (_T("Could not set screenresolution to %s. Not supported for Rapberry Pi compositor"), Core::EnumerateType<Exchange::IComposition::ScreenResolution>(format).Data()));
            return (Core::ERROR_UNAVAILABLE);
        }

        Exchange::IComposition::ScreenResolution Resolution() const override
        {
            return Exchange::IComposition::ScreenResolution::ScreenResolution_720p;
        }

    private:
        using ClientDataContainer = std::map<string, ClientData>;
        using ConstClientDataIterator = ClientDataContainer::const_iterator;

        ConstClientDataIterator GetClientIterator(const string& callsign) const
        {
            return _clients.find(callsign);
        }

        void NewClientOffered(IUnknown* element)
        {
            Exchange::IComposition::IClient* client = element->QueryInterface<Exchange::IComposition::IClient>();

            ASSERT(client != nullptr);
            if (client != nullptr) {

                const string name(client->Name());
                if (name.empty() == true) {
                    ASSERT(false);
                    TRACE(Trace::Information, (_T("Registration of a nameless client.")));
                } else {
                    _adminLock.Lock();

                    ClientDataContainer::iterator element(_clients.find(name));

                    if (element != _clients.end()) {
                        _adminLock.Unlock();
                        // as the old one may be dangling becayse of a crash let's remove that one, this is the most logical thing to do
                        ClientRevoked(element->second.clientInterface);

                        TRACE(Trace::Information, (_T("Replace client %s."), name.c_str()));
                        _adminLock.Lock();
                    } else {
                        TRACE(Trace::Information, (_T("Added client %s."), name.c_str()));
                    }

                    client->AddRef();
                    _clients.emplace(std::piecewise_construct,
                        std::forward_as_tuple(name),
                        std::forward_as_tuple(client, Resolution()));

                    _adminLock.Unlock();
                    for (auto&& index : _observers) {
                        index->Attached(name, client);
                    }
                }
                client->Release();
            }
        }

        void ClientRevoked(const IUnknown* client)
        {
            // note do not release by looking up the name, client might live in another process and the name call might fail if the connection is gone
            ASSERT(client != nullptr);

            _adminLock.Lock();
            auto it = _clients.begin();
            while ((it != _clients.end()) && (it->second.clientInterface != client)) {
                ++it;
            }

            if (it != _clients.end()) {
                string name(it->first);
                Core::IUnknown* client = it->second.clientInterface;
                ;
                _clients.erase(it);
                _adminLock.Unlock();

                TRACE(Trace::Information, (_T("Remove client %s."), name.c_str()));
                for (auto index : _observers) {
                    // note as we have the name here, we could more efficiently pass the name to the
                    // caller as it is not allowed to get it from the pointer passes, but we are going
                    // to restructure the interface anyway
                    index->Detached(name);
                }

                client->Release();

            } else {
                client->Release();
                _adminLock.Unlock();
            }

            TRACE(Trace::Information, (_T("Client detached completed")));
        }

        void PlatformReady(PluginHost::IShell* service)
        {
            PluginHost::ISubSystem* subSystems(service->SubSystems());
            ASSERT(subSystems != nullptr);
            if (subSystems != nullptr) {
                subSystems->Set(PluginHost::ISubSystem::GRAPHICS, nullptr);
                subSystems->Release();
            }
        }

        Exchange::IComposition::Rectangle FindClientRectangle(const string& name) const
        {
            Exchange::IComposition::Rectangle rectangle = Exchange::IComposition::Rectangle();

            _adminLock.Lock();

            const ClientData* clientdata = FindClientData(name);

            if (clientdata != nullptr) {
                rectangle = clientdata->currentRectangle;
            }

            _adminLock.Unlock();

            return rectangle;
        }

        uint32_t SetClientRectangle(const string& name, const Exchange::IComposition::Rectangle& rectangle)
        {

            _adminLock.Lock();

            ClientData* clientdata = FindClientData(name);

            if (clientdata != nullptr) {
                clientdata->currentRectangle = rectangle;
            }

            _adminLock.Unlock();

            return (clientdata != nullptr ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }

        IClient* FindClient(const string& name) const
        {
            IClient* client = nullptr;

            _adminLock.Lock();

            const ClientData* clientdata = FindClientData(name);

            if (clientdata != nullptr) {
                client = clientdata->clientInterface;
                ASSERT(client != nullptr);
                client->AddRef();
            }

            _adminLock.Unlock();

            return client;
        }

        const ClientData* FindClientData(const string& name) const
        {
            const ClientData* clientdata = nullptr;
            auto iterator = GetClientIterator(name);
            if (iterator != _clients.end()) {
                clientdata = &(iterator->second);
            }
            return clientdata;
        }

        ClientData* FindClientData(const string& name)
        {
            return const_cast<ClientData*>(static_cast<const CompositorImplementation&>(*this).FindClientData(name));
        }

        mutable Core::CriticalSection _adminLock;
        Core::ProxyType<RPC::InvokeServer> _engine;
        ExternalAccess* _externalAccess;
        std::list<Exchange::IComposition::INotification*> _observers;
        ClientDataContainer _clients;
    };

    SERVICE_REGISTRATION(CompositorImplementation, 1, 0)

} // namespace Plugin
} // namespace Thunder
