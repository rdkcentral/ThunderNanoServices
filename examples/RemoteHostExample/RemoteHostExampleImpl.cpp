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
#include "interfaces/IRemoteHostExample.h"
#include "RemoteHostExample.h"

namespace WPEFramework {
namespace Exchange {

    class RemoteHostExampleImpl : public RPC::RemoteLinker, IRemoteHostExample {
    public: 
        RemoteHostExampleImpl() 
            : _name()
            , _subscribers()
            , _job(*this)
        {

        }

        ~RemoteHostExampleImpl() override
        {
            _job.Revoke();
        }

        uint32_t Initialize(PluginHost::IShell* service) override;
        uint32_t Greet(const string& name, string& greeting) override;

        uint32_t SubscribeTimeUpdates(IRemoteHostExample::ITimeListener* listener) override;
        uint32_t UnsubscribeTimeUpdates(IRemoteHostExample::ITimeListener* listener) override;

        BEGIN_INTERFACE_MAP(RemoteHostExampleImpl)
            INTERFACE_ENTRY(Exchange::IRemoteHostExample)
            INTERFACE_ENTRY(RPC::IRemoteLinker)
        END_INTERFACE_MAP

    private:
        friend Core::ThreadPool::JobType<RemoteHostExampleImpl&>;
        void Dispatch();

    private:
        string _name;
        std::list<IRemoteHostExample::ITimeListener*> _subscribers;
        Core::CriticalSection _adminLock;

        Core::WorkerPool::JobType<RemoteHostExampleImpl&> _job;
    };

    uint32_t RemoteHostExampleImpl::Initialize(PluginHost::IShell* service) 
    {
        Plugin::RemoteHostExample::Config config;
        config.FromString(service->ConfigLine());
        
        _name = config.Name.Value();

        _job.Reschedule(Core::Time::Now().Add(5000));

        return Core::ERROR_NONE;
    }

    uint32_t RemoteHostExampleImpl::Greet(const string& message, string& response) 
    {

        printf("#######################################\n");
        printf("%s\n", message.c_str());
        printf("#######################################\n");

        response += "Hello from the other side, my name is " + _name;

        return Core::ERROR_NONE;
    }

    uint32_t RemoteHostExampleImpl::SubscribeTimeUpdates(IRemoteHostExample::ITimeListener* listener) 
    {
        _adminLock.Lock();
        listener->AddRef();
        _subscribers.push_back(listener);
        _adminLock.Unlock();

        return Core::ERROR_NONE;
    }

    uint32_t RemoteHostExampleImpl::UnsubscribeTimeUpdates(IRemoteHostExample::ITimeListener* listener) 
    {
        _adminLock.Lock();
        listener->Release();
        _subscribers.remove(listener);
        _adminLock.Unlock();

        return Core::ERROR_NONE;
    }

    void RemoteHostExampleImpl::Dispatch()
    {
        _adminLock.Lock();
        for (auto subscriber = _parent->_subscribers.begin();  subscriber != _parent->_subscribers.end(); subscriber++) {
            if ((*subscriber)->TimeUpdate(Core::Time::Now().ToISO8601()) != Core::ERROR_NONE) {
                (*subscriber)->Release();
                subscriber = _parent->_subscribers.erase(subscriber);
            }
        }
        _parent->_adminLock.Unlock();

        _job.Reschedule(Core::Time::Now().Add(5000));
    }


    SERVICE_REGISTRATION(RemoteHostExampleImpl, 1, 0);
}
}

