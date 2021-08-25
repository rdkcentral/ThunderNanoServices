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
 
#pragma once

#include "LogExport.h"
#include <mutex>
#include <condition_variable>

namespace WPEFramework {

namespace Plugin {

class LogExportImplementation: public Core::Thread
                    , public Exchange::ILogExport {
    public:
        LogExportImplementation(const LogExportImplementation& rhs) = delete;
        LogExportImplementation& operator=(const LogExportImplementation&) = delete;
        LogExportImplementation():Thread(Core::Thread::DefaultStackSize(), _T("LogExportImplementation"))
                                        , _watchList()
                                        , _dumpList()
                                        , _fileChangeObserver()
                                        , _notificationList()
                                        , _updatedFileList()
                                        , _watchListLock()
                                        , _dumpListLock()
                                        , _notificationLock()
                                        , _updateListSemaphore(0, 10)
        {
            Start();
        }

        ~LogExportImplementation() {
            Stop();
        }

        void Start();
        void Stop();
        uint32_t watch(const std::string& filepath, const bool& fromBeginning ) override;
        uint32_t unwatch(const std::string& filepath) override;
        uint32_t dump(const std::string& filepath) override;
        void UpdatedFile(const std::string& filename);
        void Register(Exchange::ILogExport::INotification* sink) override;
        void Unregister(Exchange::ILogExport::INotification* sink) override;
        uint32_t Worker() override;
        BEGIN_INTERFACE_MAP(LogExportImplementation)
        INTERFACE_ENTRY(Exchange::ILogExport)
        END_INTERFACE_MAP
    private:
        void parseBuffer(char* buffer, uint32_t size, std::string& remainingBuffer, std::list<std::string>& logLines);
        uint32_t ReadRemainingFromFile(const std::string& filename, Core::File& fileObj, uint64_t& readSize);
    private:
        class FileChangeObserver : public WPEFramework::Core::FileSystemMonitor::ICallback{
        public:
            FileChangeObserver() = delete;
            FileChangeObserver(const FileChangeObserver&) = delete;
            FileChangeObserver& operator=(const FileChangeObserver&) = delete;
            FileChangeObserver(const std::string& filename, LogExportImplementation* parent): _filename(filename)
                                                                                , _parent(parent){

            }
            void Updated() override{
                _parent->UpdatedFile(_filename);
                return;
            }
            ~FileChangeObserver(){}
        private:
            std::string _filename;
            LogExportImplementation* _parent;
        };
    private:
        //Map between filename and fileobject for reading the file for continous monitoring
        std::map<std::string, std::tuple<uint64_t, Core::File>> _watchList;
        //Map between filename and fileobject for reading the file for read once and dump
        std::map<std::string, Core::File> _dumpList;
        //Map between filename and filechange observer. For monitoring changes in the file.
        std::map<std::string, std::unique_ptr<FileChangeObserver>> _fileChangeObserver;
        //List containing the Notification object to send the logs
        std::list<Exchange::ILogExport::INotification*> _notificationList;
        //List containing the name of the files that got updated.
        std::list<std::string> _updatedFileList;
        // Lock protecting _watchList
        Core::CriticalSection _watchListLock;
        // Lock protecting _dumpList
        Core::CriticalSection _dumpListLock;
        // Lock protecting _notificationList 
        Core::CriticalSection _notificationLock;
        // Variables for providing lock and notify changes in _updatedFileList
        Core::CountingSemaphore _updateListSemaphore;
};

}
}
