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

#include "OpenTelemetryCollector.h"
#include "Viewer.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

namespace Thunder {
namespace Plugin {

    namespace {

        Metadata<OpenTelemetryCollector> metadata(
            1, 0, 0,
            {},
            {},
            {});

        string Trim(const string& text)
        {
            const size_t begin = text.find_first_not_of(" \t");
            const size_t end = text.find_last_not_of(" \t");
            return (begin == string::npos ? string() : text.substr(begin, end - begin + 1));
        }

        string ToLower(string text)
        {
            std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return (text);
        }

        bool EndsWith(const string& text, const string& suffix)
        {
            return ((text.size() >= suffix.size()) && (text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0));
        }

        uint64_t Count(const string& text, const string& pattern)
        {
            uint64_t result = 0;
            size_t pos = text.find(pattern);
            while (pos != string::npos) {
                ++result;
                pos = text.find(pattern, pos + pattern.size());
            }
            return (result);
        }

        string StatisticsToJSON(const TCHAR name[], uint32_t requests, uint64_t bytes, uint64_t items)
        {
            return (string(_T("\"")) + name + _T("\":{\"requests\":") + std::to_string(requests)
                + _T(",\"bytes\":") + std::to_string(bytes)
                + _T(",\"items\":") + std::to_string(items) + _T("}"));
        }
    }

    // -------------------------------------------------------------------------------------------
    // Connection
    // -------------------------------------------------------------------------------------------

    OpenTelemetryCollector::Connection::Connection(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<Connection>* server)
        : Core::SocketStream(false, connector, remoteId, 4096, 16384)
        , _server(static_cast<Server&>(*server))
        , _lock()
        , _input()
        , _output()
        , _continueSent(false)
    {
    }

    OpenTelemetryCollector::Connection::~Connection()
    {
        Close(Core::infinite);
    }

    uint16_t OpenTelemetryCollector::Connection::SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
    {
        uint16_t result = 0;

        _lock.Lock();
        if (_output.empty() == false) {
            result = static_cast<uint16_t>(std::min<size_t>(_output.size(), maxSendSize));
            ::memcpy(dataFrame, _output.data(), result);
            _output.erase(0, result);
        }
        _lock.Unlock();

        return (result);
    }

    uint16_t OpenTelemetryCollector::Connection::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
    {
        _lock.Lock();
        _input.append(reinterpret_cast<const char*>(dataFrame), receivedSize);
        const bool respond = ProcessInput();
        _lock.Unlock();

        if (respond == true) {
            Trigger();
        }

        return (receivedSize);
    }

    void OpenTelemetryCollector::Connection::StateChange()
    {
        if (IsOpen() == false) {
            _lock.Lock();
            _input.clear();
            _output.clear();
            _continueSent = false;
            _lock.Unlock();
        }
    }

    // Must be called with _lock taken. Returns true if something was queued for sending.
    bool OpenTelemetryCollector::Connection::ProcessInput()
    {
        bool queued = false;
        bool done = false;

        while (done == false) {
            const size_t headerEnd = _input.find("\r\n\r\n");

            if (headerEnd == string::npos) {
                done = true;
            } else {
                const size_t requestLineEnd = _input.find("\r\n");
                const string requestLine = _input.substr(0, requestLineEnd);
                const size_t firstSpace = requestLine.find(' ');
                const size_t secondSpace = (firstSpace == string::npos ? string::npos : requestLine.find(' ', firstSpace + 1));

                const string method = requestLine.substr(0, firstSpace);
                const string path = (firstSpace == string::npos ? string() : requestLine.substr(firstSpace + 1, (secondSpace == string::npos ? string::npos : secondSpace - firstSpace - 1)));

                size_t contentLength = 0;
                bool expectContinue = false;
                bool chunked = false;

                size_t position = requestLineEnd + 2;
                while (position < headerEnd) {
                    size_t lineEnd = _input.find("\r\n", position);
                    if ((lineEnd == string::npos) || (lineEnd > headerEnd)) {
                        lineEnd = headerEnd;
                    }
                    const string line = _input.substr(position, lineEnd - position);
                    position = lineEnd + 2;

                    const size_t colon = line.find(':');
                    if (colon != string::npos) {
                        const string name = ToLower(Trim(line.substr(0, colon)));
                        const string value = ToLower(Trim(line.substr(colon + 1)));

                        if (name == _T("content-length")) {
                            contentLength = static_cast<size_t>(::strtoul(value.c_str(), nullptr, 10));
                        } else if (name == _T("expect")) {
                            expectContinue = (value == _T("100-continue"));
                        } else if (name == _T("transfer-encoding")) {
                            chunked = (value.find(_T("chunked")) != string::npos);
                        }
                    }
                }

                const size_t bodyStart = headerEnd + 4;

                if (chunked == true) {
                    // The OTLP/HTTP exporter always sends a Content-Length; chunked bodies are not supported.
                    Respond(_T("411 Length Required"), _T("text/plain"), string());
                    _input.clear();
                    _continueSent = false;
                    queued = true;
                    done = true;
                } else if ((_input.size() - bodyStart) < contentLength) {
                    // Body not complete yet. If the client waits for permission, give it right away.
                    if ((expectContinue == true) && (_continueSent == false)) {
                        _output += _T("HTTP/1.1 100 Continue\r\n\r\n");
                        _continueSent = true;
                        queued = true;
                    }
                    done = true;
                } else {
                    const string body = _input.substr(bodyStart, contentLength);
                    _input.erase(0, bodyStart + contentLength);
                    _continueSent = false;

                    string status;
                    string contentType;
                    string responseBody;
                    _server.Parent().Handle(method, path, body, status, contentType, responseBody);
                    Respond(status, contentType, responseBody);
                    queued = true;
                }
            }
        }

        return (queued);
    }

    void OpenTelemetryCollector::Connection::Respond(const string& status, const string& contentType, const string& body)
    {
        _output += _T("HTTP/1.1 ") + status + _T("\r\n");
        _output += _T("Content-Type: ") + contentType + _T("\r\n");
        _output += _T("Cache-Control: no-store\r\n");
        _output += _T("Content-Length: ") + std::to_string(body.size()) + _T("\r\n");
        _output += _T("\r\n");
        _output += body;
    }

    // -------------------------------------------------------------------------------------------
    // Plugin
    // -------------------------------------------------------------------------------------------

    PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
    OpenTelemetryCollector::OpenTelemetryCollector()
        : _adminLock()
        , _server(*this)
        , _storagePath()
        , _store(true)
        , _print(true)
        , _history(0)
        , _traces{ 0, 0, 0, {} }
        , _logs{ 0, 0, 0, {} }
        , _metrics{ 0, 0, 0, {} }
    {
    }
    POP_WARNING()

    const string OpenTelemetryCollector::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        string message;
        Config config;
        config.FromString(service->ConfigLine());

        _store = config.Store.Value();
        _print = config.Print.Value();
        _history = config.History.Value();
        _storagePath = ((config.Path.IsSet() == true) && (config.Path.Value().empty() == false))
            ? Core::Directory::Normalize(config.Path.Value())
            : service->VolatilePath();

        _traces = { 0, 0, 0, {} };
        _logs = { 0, 0, 0, {} };
        _metrics = { 0, 0, 0, {} };

        if ((_store == true) && (Core::Directory(_storagePath.c_str()).CreatePath() == false)) {
            message = _T("Could not create storage path: ") + _storagePath;
        } else {
            const Core::NodeId listenNode(config.Binding.Value().c_str(), config.Port.Value());

            if (listenNode.IsValid() == false) {
                message = _T("Invalid binding: ") + config.Binding.Value();
            } else {
                _server.LocalNode(listenNode);

                if (_server.Open(1000) != Core::ERROR_NONE) {
                    message = _T("Could not open OTLP/HTTP collector on ") + listenNode.HostAddress() + _T(":") + std::to_string(config.Port.Value());
                } else {
                    fprintf(stderr, "[OTelCollector] Listening on http://%s:%u/v1/{traces,logs,metrics}, viewer on http://%s:%u/, storing to %s\n",
                        listenNode.HostAddress().c_str(), config.Port.Value(), listenNode.HostAddress().c_str(), config.Port.Value(), (_store ? _storagePath.c_str() : "<disabled>"));
                    fflush(stderr);
                }
            }
        }

        return (message);
    }

    void OpenTelemetryCollector::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        _server.Close(1000);
        _server.Cleanup();
    }

    string OpenTelemetryCollector::Information() const
    {
        return (Statistics());
    }

    string OpenTelemetryCollector::Statistics() const
    {
        _adminLock.Lock();
        const string result = _T("{")
            + StatisticsToJSON(_T("traces"), _traces.Requests, _traces.Bytes, _traces.Items) + _T(",")
            + StatisticsToJSON(_T("logs"), _logs.Requests, _logs.Bytes, _logs.Items) + _T(",")
            + StatisticsToJSON(_T("metrics"), _metrics.Requests, _metrics.Bytes, _metrics.Items)
            + _T("}");
        _adminLock.Unlock();

        return (result);
    }

    bool OpenTelemetryCollector::Received(const string& path, const string& body)
    {
        const TCHAR* signal = nullptr;
        Signal* statistics = nullptr;
        uint64_t items = 0;

        // Item counts are a cheap textual estimate; both OTLP JSON field name styles are accepted.
        if (EndsWith(path, _T("/v1/traces")) == true) {
            signal = _T("traces");
            statistics = &_traces;
            items = Count(body, _T("\"spanId\"")) + Count(body, _T("\"span_id\""));
        } else if (EndsWith(path, _T("/v1/logs")) == true) {
            signal = _T("logs");
            statistics = &_logs;
            items = Count(body, _T("\"observedTimeUnixNano\"")) + Count(body, _T("\"observed_time_unix_nano\""));
        } else if (EndsWith(path, _T("/v1/metrics")) == true) {
            signal = _T("metrics");
            statistics = &_metrics;
        }

        if (statistics != nullptr) {
            _adminLock.Lock();

            statistics->Requests++;
            statistics->Bytes += body.size();
            statistics->Items += items;

            if ((_history != 0) && (body.empty() == false)) {
                statistics->History.push_back(body);
                while (statistics->History.size() > _history) {
                    statistics->History.pop_front();
                }
            }

            if (_store == true) {
                std::ofstream file(_storagePath + signal + _T(".jsonl"), std::ios::out | std::ios::app);
                if (file.is_open() == true) {
                    file << body << '\n';
                }
            }

            _adminLock.Unlock();

            if (_print == true) {
                if (items != 0) {
                    fprintf(stderr, "[OTelCollector] %s: %zu bytes, %llu item(s)\n", signal, body.size(), static_cast<unsigned long long>(items));
                } else {
                    fprintf(stderr, "[OTelCollector] %s: %zu bytes\n", signal, body.size());
                }
                fflush(stderr);
            }
        }

        return (statistics != nullptr);
    }

    OpenTelemetryCollector::Signal* OpenTelemetryCollector::Find(const string& name, const TCHAR*& label)
    {
        Signal* result = nullptr;

        if (name == _T("traces")) {
            result = &_traces;
            label = _T("traces");
        } else if (name == _T("logs")) {
            result = &_logs;
            label = _T("logs");
        } else if (name == _T("metrics")) {
            result = &_metrics;
            label = _T("metrics");
        }

        return (result);
    }

    void OpenTelemetryCollector::Handle(const string& method, const string& fullPath, const string& requestBody, string& status, string& contentType, string& responseBody)
    {
        const string path = fullPath.substr(0, fullPath.find('?'));
        const string dataPrefix = _T("/data/");

        status = _T("200 OK");
        contentType = _T("application/json");

        if (method == _T("POST")) {
            if (path == _T("/clear")) {
                _adminLock.Lock();
                _traces = { 0, 0, 0, {} };
                _logs = { 0, 0, 0, {} };
                _metrics = { 0, 0, 0, {} };
                _adminLock.Unlock();
                responseBody = _T("{}");
            } else if (Received(path, requestBody) == true) {
                responseBody = _T("{}");
            } else {
                status = _T("404 Not Found");
            }
        } else if (method == _T("GET")) {
            const TCHAR* label = nullptr;

            if ((path == _T("/")) || (path == _T("/index.html"))) {
                contentType = _T("text/html; charset=utf-8");
                responseBody = ViewerPage;
            } else if (path == _T("/stats")) {
                responseBody = Statistics();
            } else if ((path.compare(0, dataPrefix.size(), dataPrefix) == 0) && (Find(path.substr(dataPrefix.size()), label) != nullptr)) {
                const Signal* signal = Find(path.substr(dataPrefix.size()), label);

                _adminLock.Lock();
                responseBody = _T("[");
                for (auto index = signal->History.cbegin(); index != signal->History.cend(); ++index) {
                    if (index != signal->History.cbegin()) {
                        responseBody += _T(",");
                    }
                    responseBody += *index;
                }
                responseBody += _T("]");
                _adminLock.Unlock();
            } else {
                status = _T("404 Not Found");
                contentType = _T("text/plain");
            }
        } else {
            status = _T("405 Method Not Allowed");
            contentType = _T("text/plain");
        }
    }

} // namespace Plugin
} // namespace Thunder
