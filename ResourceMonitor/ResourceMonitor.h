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

#include "Module.h"
#include <interfaces/IMemory.h>
#include <interfaces/IResourceMonitor.h>

namespace Thunder
{
    namespace Plugin
    {
        class ResourceMonitor : public PluginHost::IPlugin, public PluginHost::IWeb {
        private:
            class Notification : public RPC::IRemoteConnection::INotification {
            public:
                Notification() = delete;
                Notification(const Notification&) = delete;
                Notification& operator=(const Notification&) = delete;

                explicit Notification(ResourceMonitor& parent)
                    : _parent(parent) {
                }
                ~Notification() override = default;

            public:
                void Activated(RPC::IRemoteConnection* /* connection */) override {
                }
                void Deactivated(RPC::IRemoteConnection* connection) override {
                    _parent.Deactivated(connection);
                }
                void Terminated(RPC::IRemoteConnection* /* connection */) override {
                }

                BEGIN_INTERFACE_MAP(Notification)
                    INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                END_INTERFACE_MAP

            private:
                ResourceMonitor& _parent;
            };

        public:
            ResourceMonitor(const ResourceMonitor &) = delete;
            ResourceMonitor &operator=(const ResourceMonitor &) = delete;
            ResourceMonitor()
                : _service(nullptr), _monitor(nullptr), _connectionId(0), _notification(*this)
            {
            }

            ~ResourceMonitor() override = default;

            void Inbound(Web::Request&) override
            {
            }

            Core::ProxyType<Web::Response> Process(const Web::Request &request) override
            {
                Core::ProxyType<Web::Response> result = PluginHost::IFactories::Instance().Response();
                result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
                result->Message = string(_T("Unknown request path specified."));

                if (request.Verb == Web::Request::HTTP_GET)
                {
                    Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');

                    // Move to request name (shoudl be "history")
                    index.Next();

                    if (index.IsValid() == true && index.Next() == true)
                    {
                        const string requestStr = index.Current().Text();
                        if (requestStr == "history")
                        {
                            // Asked for history csv
                            result->ErrorCode = Web::STATUS_OK;
                            result->ContentType = Web::MIMETypes::MIME_TEXT;
                            result->Message = _T("OK");
                            Core::ProxyType<Web::TextBody> body(webBodyFactory.Element());
                            *body = _monitor->CompileMemoryCsv();
                            result->Body(body);
                        }
                    }
                }

                return result;
            }

            BEGIN_INTERFACE_MAP(ResourceMonitor)
            INTERFACE_ENTRY(IPlugin)
            INTERFACE_ENTRY(PluginHost::IWeb)
            INTERFACE_AGGREGATE(Exchange::IResourceMonitor, _monitor)
            END_INTERFACE_MAP

        public:
            const string Initialize(PluginHost::IShell *service) override;
            void Deinitialize(PluginHost::IShell *service) override;
            string Information() const override;

        private:
            void Deactivated(RPC::IRemoteConnection* connection);

        private:
            PluginHost::IShell *_service;
            Exchange::IResourceMonitor *_monitor;
            uint32_t _connectionId;
            static Core::ProxyPoolType<Web::TextBody> webBodyFactory;
            uint32_t _skipURL;
            Core::SinkType<Notification> _notification;
        };
    } // namespace Plugin
} // namespace Thunder
