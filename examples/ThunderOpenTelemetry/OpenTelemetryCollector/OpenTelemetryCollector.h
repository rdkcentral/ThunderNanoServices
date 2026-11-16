/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

#include "Module.h"

#include <deque>

namespace Thunder {
namespace Plugin {

    // Minimal OTLP/HTTP (JSON) receiver. Accepts POSTs on /v1/traces, /v1/logs and /v1/metrics,
    // acknowledges them immediately and stores every payload as one line in <path>/<signal>.jsonl.
    // The last <history> payloads per signal are kept in memory and can be viewed with a browser:
    //   GET  /                                 viewer page
    //   GET  /data/traces|logs|metrics         kept payloads as a JSON array
    //   GET  /stats                            counters
    //   POST /clear                            drop the kept payloads and counters
    class OpenTelemetryCollector : public PluginHost::IPlugin {
    private:
        class Config : public Core::JSON::Container {
        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

            Config()
                : Core::JSON::Container()
                , Binding(_T("127.0.0.1"))
                , Port(4318)
                , Path()
                , Store(true)
                , Print(true)
                , History(200)
            {
                Add(_T("binding"), &Binding);
                Add(_T("port"), &Port);
                Add(_T("path"), &Path);
                Add(_T("store"), &Store);
                Add(_T("print"), &Print);
                Add(_T("history"), &History);
            }
            ~Config() override = default;

        public:
            Core::JSON::String Binding;
            Core::JSON::DecUInt16 Port;
            Core::JSON::String Path;
            Core::JSON::Boolean Store;
            Core::JSON::Boolean Print;
            Core::JSON::DecUInt16 History;
        };

        class Server;

        // Handles a single (keep-alive) HTTP/1.1 connection, written directly on the socket so that
        // "Expect: 100-continue" can be answered and the exporter is never left waiting for it.
        class Connection : public Core::SocketStream {
        public:
            Connection() = delete;
            Connection(Connection&&) = delete;
            Connection(const Connection&) = delete;
            Connection& operator=(Connection&&) = delete;
            Connection& operator=(const Connection&) = delete;

            Connection(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<Connection>* server);
            ~Connection() override;

        private:
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override;
            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override;
            void StateChange() override;

            bool ProcessInput();
            void Respond(const string& status, const string& contentType, const string& body);

        private:
            Server& _server;
            Core::CriticalSection _lock;
            string _input;
            string _output;
            bool _continueSent;
        };

        class Server : public Core::SocketServerType<Connection> {
        public:
            Server() = delete;
            Server(Server&&) = delete;
            Server(const Server&) = delete;
            Server& operator=(Server&&) = delete;
            Server& operator=(const Server&) = delete;

            explicit Server(OpenTelemetryCollector& parent)
                : Core::SocketServerType<Connection>()
                , _parent(parent)
            {
            }
            ~Server() = default;

            OpenTelemetryCollector& Parent()
            {
                return (_parent);
            }

        private:
            OpenTelemetryCollector& _parent;
        };

        struct Signal {
            uint32_t Requests;
            uint64_t Bytes;
            uint64_t Items;
            std::deque<string> History;
        };

    public:
        OpenTelemetryCollector(const OpenTelemetryCollector&) = delete;
        OpenTelemetryCollector& operator=(const OpenTelemetryCollector&) = delete;

        OpenTelemetryCollector();
        ~OpenTelemetryCollector() override = default;

        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        BEGIN_INTERFACE_MAP(OpenTelemetryCollector)
            INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

    private:
        // Called on the socket thread for every complete HTTP request.
        void Handle(const string& method, const string& path, const string& requestBody, string& status, string& contentType, string& responseBody);
        bool Received(const string& path, const string& body);
        Signal* Find(const string& name, const TCHAR*& label);
        string Statistics() const;

    private:
        mutable Core::CriticalSection _adminLock;
        Server _server;
        string _storagePath;
        bool _store;
        bool _print;
        uint16_t _history;
        Signal _traces;
        Signal _logs;
        Signal _metrics;
    };

} // namespace Plugin
} // namespace Thunder
