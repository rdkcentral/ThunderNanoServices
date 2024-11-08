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
 
#pragma once
#include <unordered_map>
#include <iostream>
#include <fstream>
#include "../FileTransfer/Module.h"

#ifndef __APPLE__
#include <sys/inotify.h>
#endif

namespace Thunder {
namespace Plugin
{
    class FileObserver : public Core::FileSystemMonitor::ICallback {
        public:
            struct ICallback
            {
                virtual ~ICallback() = default;
                virtual void NewLine(const string &) = 0;
            };

        public:
            FileObserver(const FileObserver &) = delete;
            FileObserver &operator=(const FileObserver &) = delete;
            FileObserver()
                : _callback(nullptr)
                , _position(0)
                , _path()
                , _job(*this)
            {
            }
            ~FileObserver()
            {
                _job.Revoke();

                // Please Unregister before destructing!!!
                ASSERT(_callback == nullptr);
                if (_callback != nullptr)
                {
                    Unregister();
                }
            }

        public:
            void Register(const string &entry, ICallback *callback, bool fullFile = false)
            {
                ASSERT((_callback == nullptr) && (callback != nullptr));

                if (fullFile == false) {
                    _position = GetCurrentPosition(entry);
                } else {
                    _position = 0;
                }

                _path = entry;
                _callback = callback;
                Core::FileSystemMonitor::Instance().Register(this, _path);
            }
            void Unregister()
            {
                ASSERT(_callback != nullptr);

                // First make sure the dispatcher Job will longer be fired
                Core::FileSystemMonitor::Instance().Unregister(this, _path);

                // Potentially the Job might still be waiting, let’s kill it
                _job.Revoke();

                _path = EMPTY_STRING;
                _position = 0;
                _callback = nullptr;
            }

        private:
            long int GetCurrentPosition(const string &filePath) const
            {
                long int pos = 0;
                std::ifstream file(filePath.c_str());
                if (file) {
                    file.seekg(0, file.end);
                    pos = file.tellg();
                }
                return pos;
            }

            friend Core::ThreadPool::JobType<FileObserver&>;
            void Dispatch()
            {
                TRACE(Trace::Information, (_T("FileObserver: job is dispatched")));
                std::ifstream file(_path);
                if (file) {
                    file.seekg(_position, file.beg);
                    std::string str;
                    while ((std::getline(file, str)) && (str.size() > 0)) {
                        ASSERT(_callback != nullptr);
                        _callback->NewLine(str);
                    }
                }
                _position = GetCurrentPosition(_path);
            }
            void Updated()
            {
                _job.Submit();
            }

        private:
            ICallback *_callback;
            long int _position;
            string _path;

            Core::WorkerPool::JobType<FileObserver&> _job;
        };

    class FileTransfer : public PluginHost::IPlugin {
        private:

            static constexpr uint16_t MAX_BUFFER_LENGHT = 1024;
            static constexpr uint16_t TIMEOUT_MS = 0;

            class TextChannel : public Core::SocketDatagram {
                public:
                    TextChannel()
                        : Core::SocketDatagram(false, Core::NodeId().Origin(), Core::NodeId(), MAX_BUFFER_LENGHT, 0)
                        , _sendQueue()
                        , _offset(0)
                    {
                    }
                    ~TextChannel() override
                    {
                        _sendQueue.clear();
                        _offset = 0;
                        Close(Core::infinite);
                    }

                    void SetDestination(const string& binding, const uint16_t &port)
                    {
                        Core::NodeId logNode(binding.c_str(), port);
                        LocalNode(logNode.Origin());
                        RemoteNode(logNode);

                        Open(TIMEOUT_MS);
                    }

                    void NewLine(const string& text)
                    {
                        _adminLock.Lock();

                        _sendQueue.emplace_back(text);
                        bool trigger = (_sendQueue.size() == 1);

                        _adminLock.Unlock();

                        if (trigger == true)
                        {
                            Trigger();
                        }
                    }
                private:
                    // Methods to extract and insert data into the socket buffers
                    uint16_t SendData(uint8_t *dataFrame, const uint16_t maxSendSize) override
                    {
                        uint16_t result = 0;

                        _adminLock.Lock();

                        if (_offset == static_cast<uint32_t>(~0)) {
                            // We are done with this entry it has been sent!!! discard it.
                            _sendQueue.pop_front();
                            _offset = 0;
                        }

                        if (_sendQueue.size() > 0) {
                            const string& sendObject = _sendQueue.front();

                            // Do we still need to send data from the text..
                            if (_offset < (sendObject.size() * sizeof(TCHAR))) {
                                result = (((sendObject.size() * sizeof(TCHAR)) - _offset) > maxSendSize ? maxSendSize : ((sendObject.size() * sizeof(TCHAR)) - _offset));

                                _offset += SendCharacters(dataFrame, &(sendObject.c_str()[(_offset / sizeof(TCHAR))]), (_offset % sizeof(TCHAR)), result);
                            }

                            // See if we can write the closing marker
                            if ((maxSendSize != result) && (_offset >= (sendObject.size() * sizeof(TCHAR))) && (_offset < ((sendObject.size() + (_terminator.SizeOf())) * sizeof(TCHAR)))) {
                                uint8_t markerSize = (static_cast<uint8_t>(_terminator.SizeOf()) * sizeof(TCHAR));
                                uint8_t markerOffset = ((sendObject.size() * sizeof(TCHAR)) - _offset);
                                uint16_t size = ((markerSize - markerOffset) > (maxSendSize - result) ? (maxSendSize - result) : (markerSize - markerOffset));

                                _offset += SendCharacters(&(dataFrame[result]), &(_terminator.Marker()[(markerOffset / sizeof(TCHAR))]), (markerOffset % sizeof(TCHAR)), size);
                                result += size;

                                if ((size + markerOffset) == markerSize) {
                                    // Report as completed on the next run
                                    _offset = static_cast<uint32_t>(~0);
                                }
                            }

                            // If we went through this entry we must have processed something....
                            ASSERT(result != 0);
                        }

                        _adminLock.Unlock();

                        return (result);
                    }
                    uint16_t ReceiveData(uint8_t*, const uint16_t receivedSize) override
                    {
                        // No clue what to do with the data that we receive
                        return (receivedSize);
                    }
                    // Signal a state change, Opened, Closed or Accepted
                    void StateChange() override
                    {
                    }

                    uint16_t SendCharacters(uint8_t *dataFrame, const TCHAR stream[], const uint8_t delta VARIABLE_IS_NOT_USED, const uint16_t total)
                    {
                        ASSERT(delta == 0);

                        // Copying from an aligned position..
                        ::memcpy(dataFrame, stream, total);

                        return (total);
                    }

                private:
                    Core::CriticalSection _adminLock;
                    std::list<string> _sendQueue;
                    uint32_t _offset;
                    Core::TerminatorCarriageReturn _terminator;
            };

            class OnChangeFile: public FileObserver::ICallback
            {
                public:
                    OnChangeFile(TextChannel *parent)
                        : _adminLock()
                        , _parent(*parent)
                    {
                    }
                    ~OnChangeFile()
                    {
                    }
                private:
                    OnChangeFile(const OnChangeFile &) = delete;
                    OnChangeFile &operator=(const OnChangeFile &) = delete;

                    void NewLine(const string &text) override
                    {
                        _adminLock.Lock();

                        _parent.NewLine(text);

                        _adminLock.Unlock();
                    }

               Core::CriticalSection _adminLock;
               TextChannel &_parent;
            };

            class Config : public Core::JSON::Container {
                private:
                    Config(const Config &) = delete;
                    Config &operator=(const Config &) = delete;

                public:
                    class NetworkNode : public Core::JSON::Container
                    {
                        public:
                            NetworkNode()
                                : Core::JSON::Container(), Port(2201), Binding("0.0.0.0")
                            {
                                Add(_T("port"), &Port);
                                Add(_T("binding"), &Binding);
                            }
                            NetworkNode(const NetworkNode& copy)
                                : Core::JSON::Container(), Port(copy.Port), Binding(copy.Binding)
                            {
                                Add(_T("port"), &Port);
                                Add(_T("binding"), &Binding);
                            }
                            ~NetworkNode()
                            {
                            }
                            NetworkNode &operator=(const NetworkNode &RHS)
                            {
                                Port = RHS.Port;
                                Binding = RHS.Binding;
                                return (*this);
                            }

                        public:
                            Core::JSON::DecUInt16 Port;
                            Core::JSON::String Binding;
                    };

                public:
                    Config()
                        : FilePath(_T("/var/log/messages")), FullFile(false), Destination()
                    {
                        Add(_T("filepath"), &FilePath);
                        Add(_T("fullfile"), &FullFile);
                        Add(_T("destination"), &Destination);
                    }
                    ~Config() override {}

                public:
                    Core::JSON::String FilePath;
                    Core::JSON::Boolean FullFile;
                    NetworkNode Destination;
            };

            public:
                FileTransfer(const FileTransfer &) = delete;
                FileTransfer &operator=(const FileTransfer &) = delete;
                FileTransfer()
                    : _logOutput()
                    , _observer()
                    , _fileUpdate(&_logOutput)
                {
                }
                ~FileTransfer() override = default;

                BEGIN_INTERFACE_MAP(FileTransfer)
                INTERFACE_ENTRY(PluginHost::IPlugin)
                END_INTERFACE_MAP

                // IPlugin methods
                const string Initialize(PluginHost::IShell *service) override;
                void Deinitialize(PluginHost::IShell *service) override;
                string Information() const override;

            private:
                TextChannel _logOutput;
                FileObserver _observer;
                OnChangeFile _fileUpdate;
    };
} // namespace Plugin
} // namespace Thunder
