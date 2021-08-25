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

#include "LogExportImplementation.h"
#define MAX_CLIENT_CONNECTION 5

namespace WPEFramework{
namespace Plugin{
    SERVICE_REGISTRATION(LogExportImplementation, 1, 0);
    void LogExportImplementation::Start(){
        Thread::Run();
    }

    void LogExportImplementation::Stop(){
        Wait(Thread::BLOCKED | Thread::STOPPED | Thread::STOPPING, Core::infinite);
        _watchList.clear();
        _dumpList.clear();
    }

    void LogExportImplementation::parseBuffer(char* buffer, uint32_t size, std::string& remainingBuffer, std::list<std::string>& logLines){
        bool brokenLine = false;
        char* saveptr;
        if(buffer[size - 1] != '\n' )
        {
            brokenLine = true;
        }
        char* token = strtok_r(buffer, "\n", &saveptr);
        std::string logStatement;
        if(remainingBuffer.empty() == false)
        {
            logStatement.append(remainingBuffer);
            remainingBuffer.clear();
        }
        while(token != NULL)
        {
            logStatement.append(token);
            token = strtok_r(NULL, "\n", &saveptr);
            if(token == NULL )
            {
                if (brokenLine == true)
                {
                    remainingBuffer.assign(logStatement);
                }
                else
                {
                    logLines.push_back(logStatement);
                }
            }
            else
            {
                logLines.push_back(logStatement);
            }
            logStatement.clear();
        }
        return;
    }

    uint32_t LogExportImplementation::ReadRemainingFromFile(const std::string& filename, Core::File& fileObj, uint64_t& readSize){
        uint8_t buffer[1024] = {'\0'};
        uint64_t currentReadSize = 0;
        std::string remainingBuffer;
        std::list<std::string> logLines;
        fileObj.LoadFileInfo();
        if(readSize < fileObj.Size()) {
            memset(buffer, 0, sizeof(buffer));
            if(fileObj.IsOpen() == false) {
                fileObj.Open(true);
                fileObj.Position(false, readSize);
            }
            while(readSize < fileObj.Size()) {
                memset(buffer, 0, sizeof(buffer));
                currentReadSize = fileObj.Read(buffer, sizeof(buffer) - 1);
                readSize += currentReadSize;
                parseBuffer((char*) buffer, currentReadSize, remainingBuffer, logLines);
            }
            if(remainingBuffer.empty() == false){
                logLines.push_back(remainingBuffer);
            }
            using LogIteratorImplementation = RPC::IteratorType<Exchange::ILogExport::INotification::ILogIterator>;
            Exchange::ILogExport::INotification::ILogIterator* logs = Core::Service<LogIteratorImplementation>::Create<Exchange::ILogExport::INotification::ILogIterator>(logLines);

            _notificationLock.Lock();
            for(auto& sink: _notificationList){
                sink->OutputMessage(filename, logs);
            }
            _notificationLock.Unlock();
            remainingBuffer.clear();
            logs->Release();
        }
        return Core::ERROR_NONE;
    }

    void LogExportImplementation::UpdatedFile(const std::string& filename){
        _updatedFileList.push_back(filename);
        _updateListSemaphore.Unlock();
        return;
    }

    uint32_t LogExportImplementation::Worker(){
        while(IsRunning() == true)
        {
            _dumpListLock.Lock();
            for(auto& fileIter: _dumpList) {
                uint64_t readSize = 0;
                ReadRemainingFromFile(fileIter.first, fileIter.second, readSize);
            }
            _dumpList.erase(_dumpList.begin(), _dumpList.end());
            _dumpListLock.Unlock();

            uint32_t result = _updateListSemaphore.Lock(500);
            if (result == Core::ERROR_NONE)
            {

                std::string updatedFilename = _updatedFileList.front();
                _updatedFileList.erase(_updatedFileList.begin());
                _watchListLock.Lock();
                std::map<std::string, std::tuple<uint64_t, Core::File>>::iterator file = _watchList.find(updatedFilename);
                if(file != _watchList.end())
                {
                    Core::File& fileObj = std::get<1>(file->second);
                    uint64_t& readSize = std::get<0>(file->second);
                    ReadRemainingFromFile(file->first, fileObj, readSize);
                }
                _watchListLock.Unlock();
            }
            std::this_thread::yield();
        }
        return(Core::infinite);
    }

    uint32_t LogExportImplementation::watch(const std::string & filepath, const bool& fromBeginning) {
        std::tuple<uint64_t, Core::File> tup;
        Core::File fileObj(filepath);
        std::map<std::string, std::tuple<uint64_t, Core::File>>::iterator iter;
        bool result;

        std::unique_ptr<FileChangeObserver> fileChangeObserver(new FileChangeObserver(filepath, this));
        WPEFramework::Core::FileSystemMonitor::Instance().Register(fileChangeObserver.get(), filepath);
        _fileChangeObserver.insert(std::pair<std::string, std::unique_ptr<FileChangeObserver>>(filepath, std::move(fileChangeObserver)));
        if(fromBeginning == true) {
            dump(filepath);
        }
        tup = std::make_tuple(fileObj.Size(), fileObj);

        _watchListLock.Lock();
        std::tie(iter, result) = _watchList.insert(std::pair<std::string, std::tuple<uint64_t, Core::File>>(filepath, tup));
        _watchListLock.Unlock();
        return (result == true) ? Core::ERROR_NONE: Core::ERROR_GENERAL;
    }

    uint32_t LogExportImplementation::unwatch(const std::string& filepath){
        auto fileObserver = _fileChangeObserver.find(filepath);
        if (fileObserver != _fileChangeObserver.end())
        {
            WPEFramework::Core::FileSystemMonitor::Instance().Unregister(fileObserver->second.get(), filepath);
            _fileChangeObserver.erase(fileObserver);
        }
        _watchListLock.Lock();
        _watchList.erase(filepath);
        _watchListLock.Unlock();
        return Core::ERROR_NONE;
    }


    void LogExportImplementation::Register(Exchange::ILogExport::INotification* sink){
        ASSERT(sink !=nullptr)
        _notificationLock.Lock();
        auto iter = std::find(_notificationList.begin(), _notificationList.end(), sink);
        if (iter == _notificationList.end()) {
            _notificationList.push_back(sink);
            sink->AddRef();
        }
        _notificationLock.Unlock();
        return;
    }
    void LogExportImplementation::Unregister(Exchange::ILogExport::INotification* sink) {
        ASSERT(sink != nullptr)
        _notificationLock.Lock();
        auto iter = std::find(_notificationList.begin(), _notificationList.end(), sink);
        if (iter != _notificationList.end()) {
            sink->Release();
            _notificationList.erase(iter);
        }
        _notificationLock.Unlock();
        return;
    }

    uint32_t LogExportImplementation::dump(const std::string& filepath){
        Core::File fileObj(filepath);
        std::map<std::string, Core::File>::iterator iter;
        bool result; 
        _dumpListLock.Lock();
        std::tie(iter, result) = _dumpList.insert(std::pair<std::string, Core::File>(filepath, fileObj));
        _dumpListLock.Unlock();
        return (result == true)?Core::ERROR_NONE: Core::ERROR_GENERAL;
    }

}

}
