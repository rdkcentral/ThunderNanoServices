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

namespace WPEFramework{
namespace Plugin{
    SERVICE_REGISTRATION(SyslogExport, 1, 0);
    bool SyslogExport::Attach(PluginHost::Channel& channel){
        bool retVal = false;
        SYSLOG(Logging::Startup, (_T("Received attach request with state: %d"), channel.State()));
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
    
    string SyslogExport::Inbound(const uint32_t ID, const string& value) {
        (void)ID;
        (void)value;
        return "";
    } 

    const string SyslogExport::Initialize(PluginHost::IShell* service){
        Config config;
        std::string returnMessage = "";
        config.FromString(service->ConfigLine());
        Logging::SyslogMonitor::Instance().RegisterClient(&_notification);
        uint32_t maxClientConnection = config.MaxClientConnection.IsSet() ? config.MaxClientConnection.Value(): max_client_connection;
        if( maxClientConnection != 0){
            Core::ProxyType<SyslogExport::WebsocketSink> websocketSink(Core::ProxyType<SyslogExport::WebsocketSink>::Create(maxClientConnection));
            _sinkManager.AddOuputSink(websocketSink);
        }
        
        return returnMessage;
    }

    void SyslogExport::Deinitialize(PluginHost::IShell* service){
        (void)service;
        Logging::SyslogMonitor::Instance().UnregisterClient(&_notification);
        _sinkManager.RemoveOutputSink();
        return;
    }

    void SyslogExport::OutputMessage(const std::string& logLine){
        _sinkManager.HandleMessage(logLine);
        return;
    }

    void SyslogExport::SinkManager::HandleMessage(const std::string& logLine)
    {
        _websocketSink->DeliverMessage(logLine);
        return;
    }

    void SyslogExport::SinkManager::AddOuputSink(Core::ProxyType<SyslogExport::WebsocketSink>& outputSink)
    {
        _websocketSink = outputSink;
        return;
    }

    void SyslogExport::SinkManager::RemoveOutputSink()
    {
        _websocketSink.Release();
    }


    bool SyslogExport::SinkManager::AddChannel(WPEFramework::PluginHost::Channel& channel)
    {
        return _websocketSink->AddChannel(channel);
    }

    void SyslogExport::SinkManager::RemoveChannel(uint32_t channelId)
    {
        _websocketSink->RemoveChannel(channelId);
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
