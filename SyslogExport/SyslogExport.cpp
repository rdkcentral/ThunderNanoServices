/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological Management
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

#include "SyslogExport.h"
#include <chrono>

namespace WPEFramework{
namespace Plugin{
    SERVICE_REGISTRATION(SyslogExport, 1, 0);
    bool SyslogExport::Attach(PluginHost::Channel& channel){
        bool retVal = false;
        if(channel.State() == WPEFramework::PluginHost::Channel::ChannelState::TEXT)
        {
            retVal = _sinkManager.AddChannel(channel);
        }
        return retVal;
    }

    void SyslogExport::Detach(PluginHost::Channel& channel) {
        _sinkManager.RemoveChannel(channel.Id());
        return;
    }
    Core::ProxyType<Core::JSON::IElement> SyslogExport::Inbound(const string& identifier){
        Core::ProxyType<Core::JSON::IElement> result;
        (void)identifier;
        return result;
    }

    Core::ProxyType<Core::JSON::IElement> SyslogExport::Inbound(const uint32_t ID, const Core::ProxyType<Core::JSON::IElement>& element){
        Core::ProxyType<Core::JSON::IElement> result;
        (void)ID;
        (void)element;
        return result;

    }

    const string SyslogExport::Initialize(PluginHost::IShell* service){
        std::string returnMessage = "";
        _config.FromString(service->ConfigLine());
        Logging::SyslogMonitor::Instance().RegisterClient(&_notification);
        uint32_t maxClientConnection = _config.MaxClientConnection.IsSet() ? _config.MaxClientConnection.Value(): max_client_connection;
        if( maxClientConnection != 0){
            std::unique_ptr<SyslogExport::WebsocketSink> websocketSink(new WebsocketSink(maxClientConnection));
            _sinkManager.AddOuputSink(std::move(websocketSink));
        }
        _sinkManager.Start();
        
        return returnMessage;
    }

    void SyslogExport::Deinitialize(PluginHost::IShell* service){
        Logging::SyslogMonitor::Instance().UnregisterClient(&_notification);
        _sinkManager.RemoveOutputSink();
        _sinkManager.Stop();
        return;
    }

    void SyslogExport::OutputMessage(const std::string& logLine){
        _sinkManager.HandleMessage(logLine);
        return;
    }

    void SyslogExport::SinkManager::Start(){
        Thread::Run();
        return;
    }

    void SyslogExport::SinkManager::Stop(){
        Thread::Block();
        Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
        return;
    }

    uint32_t SyslogExport::SinkManager::Worker(){
        do{
            ProcessMessageList();
            std::this_thread::yield();
        }while(IsRunning() == true);
        return Core::infinite;
    }


    void SyslogExport::SinkManager::HandleMessage(const std::string& logLine)
    {
        ASSERT(logs != nullptr);
        std::lock_guard<std::mutex> lock(_mutex);
        _logList.push(logLine);
        _condVar.notify_one();
        return;
    }

    void SyslogExport::SinkManager::AddOuputSink(std::unique_ptr<SyslogExport::WebsocketSink> outputSink)
    {
        _websocketSink = std::move(outputSink);
        return;
    }

    void SyslogExport::SinkManager::RemoveOutputSink()
    {
        _websocketSink.reset();
    }


    bool SyslogExport::SinkManager::AddChannel(WPEFramework::PluginHost::Channel& channel)
    {
        return _websocketSink->AddChannel(channel);
    }

    void SyslogExport::SinkManager::RemoveChannel(uint32_t channelId)
    {
        _websocketSink->RemoveChannel(channelId);
    }

    uint32_t SyslogExport::SinkManager::ProcessMessageList() {
        std::unique_lock<std::mutex> lock(_mutex);
        bool satisfied = _logList.size() > 0;
        if(satisfied == false){
            satisfied = _condVar.wait_for(lock,std::chrono::seconds(1),[this](){ return _logList.size() > 0;});
        }
        if(satisfied == true) {
            std::string logData = _logList.front();
            _logList.pop();
            lock.unlock();
            _websocketSink->DeliverMessage(logData);
        }
        return Core::ERROR_NONE;
    }

    void SyslogExport::WebsocketSink::DeliverMessage(const std::string& logLine) {
        _lock.Lock();
        for(auto& channel: _exportChannelMap)
        {
            channel.second->Submit(logLine);
        }
        _lock.Unlock();
        return;
    }

    bool SyslogExport::WebsocketSink::AddChannel(WPEFramework::PluginHost::Channel& channel)
    {
        bool accepted = false;
        _lock.Lock();

        if ((_maxConnections !=0) && (_maxConnections - _exportChannelMap.size() > 0)) {
            ASSERT(0 == _exportChannelMap.count(channel.Id()));
            auto ret = _exportChannelMap.insert(std::make_pair(channel.Id(), &channel));
            accepted = std::get<1>(ret);
        }

        _lock.Unlock();
        return accepted;
    }

    void SyslogExport::WebsocketSink::RemoveChannel(uint32_t channelId)
    {
        _exportChannelMap.erase(channelId);
        return;
    }
}

}
