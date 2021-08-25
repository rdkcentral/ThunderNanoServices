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

#include "LogExport.h"
#include <chrono>
#define MAX_CLIENT_CONNECTION 5

namespace WPEFramework{
namespace Plugin{
    Core::ProxyPoolType<LogExport::LogData> logDataFactory(10);
    SERVICE_REGISTRATION(LogExport, 1, 0);
    std::map<uint32_t, WPEFramework::PluginHost::Channel*> LogExport::WebsocketSink::_exportChannelMap;
    bool LogExport::Attach(PluginHost::Channel& channel){
        WebsocketSink::getInstance().AddChannel(channel);
        return true;
    }

    void LogExport::Detach(PluginHost::Channel& channel) {
        WebsocketSink::getInstance().RemoveChannel(channel.Id());
        return;
    }

    void LogExport::Inbound(Web::Request& request){
        (void)request;
        return;
    }

    Core::ProxyType<Web::Response> LogExport::Process(const Web::Request& request){
        Core::ProxyType<Web::Response> result;
        (void)request;
        return result;
    }

    Core::ProxyType<Core::JSON::IElement> LogExport::Inbound(const string& identifier){
        Core::ProxyType<Core::JSON::IElement> result;
        (void)identifier;
        return result;
    }

    Core::ProxyType<Core::JSON::IElement> LogExport::Inbound(const uint32_t ID, const Core::ProxyType<Core::JSON::IElement>& element){
        Core::ProxyType<Core::JSON::IElement> result;
        (void)ID;
        (void)element;
        return result;

    }

    const string LogExport::Initialize(PluginHost::IShell* service){
        std::string returnMessage = "";
        _config.FromString(service->ConfigLine());
        _implementation = service->Root<Exchange::ILogExport>(_connectionId, 2000, _T("LogExportImplementation"));
        if(_implementation != nullptr){
            _implementation->Register(&_notification);
            if (_config.Console.IsSet() == true){
                std::unique_ptr<LogExport::IOutputSink> consoleSink(new ConsoleSink());
                _sinkManager.AddOuputSink(std::move(consoleSink));
            }
            uint32_t maxClientConnection = _config.MaxClientConnection.IsSet() ? _config.MaxClientConnection.Value(): MAX_CLIENT_CONNECTION;
            if( maxClientConnection != 0){
                std::unique_ptr<LogExport::IOutputSink> websocketSink(new WebsocketSink(maxClientConnection));
                _sinkManager.AddOuputSink(std::move(websocketSink));
            }
            _sinkManager.Start();
        }
        else {
            returnMessage = "LogExport Plugin could not initialize";
        }
        return returnMessage;
    }

    void LogExport::Deinitialize(PluginHost::IShell* service){
        if(_implementation != nullptr){
            _implementation->Release();
            RPC::IRemoteConnection* connection(service->RemoteConnection(_connectionId));
            // The process can disappear in the meantime...
            if (connection != nullptr) {
                // But if it did not dissapear in the meantime, forcefully terminate it. Shoot to kill :-)
                connection->Terminate();
                connection->Release();
            }
            _sinkManager.Stop();
        }
        return;
    }

    uint32_t LogExport::OutputMessage(const std::string& filename, Exchange::ILogExport::INotification::ILogIterator* loglines){
        return _sinkManager.HandleMessage(filename, loglines);
    }

    void LogExport::SinkManager::Start(){
        Thread::Run();
        return;
    }

    void LogExport::SinkManager::Stop(){
        Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
        std::queue<Core::ProxyType<LogData>> empty;
        _logList.swap(empty);
        _sinkList.clear();
        return;
    }

    uint32_t LogExport::SinkManager::Worker(){
        do{
            ProcessMessageList();
            std::this_thread::yield();
        }while(IsRunning() == true);
        return Core::infinite;
    }

    Core::ProxyType<LogExport::LogData> LogExport::SinkManager::GetLogData(){
        return logDataFactory.Element();
    }

    uint32_t LogExport::SinkManager::HandleMessage(const std::string& filename, Exchange::ILogExport::INotification::ILogIterator* logs)
    {
        ASSERT(logs != nullptr);
        std::lock_guard<std::mutex> lock(_mutex);
        Core::ProxyType<LogExport::LogData> logData = GetLogData();
        logData->FileName = filename;
        logData->Message.Clear();
        std::string logLine;
        while(logs->Next(logLine) == true){
            Core::JSON::String logMessage;
            logMessage = logLine;
            logData->Message.Add(logMessage);
        }
        _logList.emplace(std::move(logData));
        _condVar.notify_one();
        return Core::ERROR_NONE;
    }

    uint32_t LogExport::SinkManager::AddOuputSink(std::unique_ptr<LogExport::IOutputSink> outputSink)
    {
        _sinkLock.Lock();
        _sinkList.push_back(std::move(outputSink));
        _sinkLock.Unlock();
        return Core::ERROR_NONE;
    }


    uint32_t LogExport::SinkManager::ProcessMessageList() {
        std::unique_lock<std::mutex> lock(_mutex);
        if(_logList.size() == 0){
            while(true){
                bool satisfied = _condVar.wait_for(lock,std::chrono::seconds(1),[this](){ return _logList.size() > 0;});
                if(satisfied == true ){
                    break;
                }
            }
        }
        Core::ProxyType<LogExport::LogData> logData = std::move(_logList.front());
        _logList.pop();
        lock.unlock();
        _sinkLock.Lock();
        for(auto& sink : _sinkList)
        {
            sink->ProcessMessage(logData);
            sink->DeliverMessage();
        }
        _sinkLock.Unlock();
        return Core::ERROR_NONE;
    }

    uint32_t LogExport::ConsoleSink::ProcessMessage(Core::ProxyType<LogExport::LogData>& logData){
        logData->Message.ToString(_processedMessage);
        return Core::ERROR_NONE;
    }

    uint32_t LogExport::ConsoleSink::DeliverMessage(){
        return Core::ERROR_NONE;
    }

    Core::ProxyType<LogExport::LogData> LogExport::WebsocketSink::GetLogData(){
        return logDataFactory.Element();
    }
    uint32_t LogExport::WebsocketSink::ProcessMessage(Core::ProxyType<LogExport::LogData>& logdata) {
        std::string logDataStr;
        _logData = logdata;
        _logData->Message.ToString(logDataStr);
        return Core::ERROR_NONE;
    }

    uint32_t LogExport::WebsocketSink::DeliverMessage() {
        std::string logDataStr;
        _lock.Lock();
        for(auto& channel: _exportChannelMap)
        {
            channel.second->Submit(Core::proxy_cast<Core::JSON::IElement>(_logData));
        }
        _lock.Unlock();
        return Core::ERROR_NONE;
    }

    bool LogExport::WebsocketSink::AddChannel(WPEFramework::PluginHost::Channel& channel)
    {
        bool accepted = false;
        _lock.Lock();

        if ((_maxConnections !=0) && (_maxConnections - _exportChannelMap.size() > 0)) {
            ASSERT(0 == _exportChannelMap.count(channel.Id()));
            _exportChannelMap.insert(std::make_pair(channel.Id(), &channel));
            accepted = true;
        }

        _lock.Unlock();
        return accepted;
    }

    void LogExport::WebsocketSink::RemoveChannel(uint32_t channelId)
    {
        _exportChannelMap.erase(channelId);
        return;
    }
}

}
