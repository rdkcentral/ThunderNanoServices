/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Metrological
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

#include "SignallingServer.h"
#include "BluetoothAudioSink.h"
#include "BluetoothAudioSource.h"

#include <bluetooth/audio/bluetooth_audio.h>

namespace Thunder {

namespace Plugin {

    class BluetoothAudio : public PluginHost::IPlugin
                         , public PluginHost::JSONRPC {

        class ComNotificationSink : public PluginHost::IShell::ICOMLink::INotification {
        public:
            ComNotificationSink() = delete;
            ComNotificationSink(const ComNotificationSink&) = delete;
            ComNotificationSink& operator=(const ComNotificationSink&) = delete;
            ComNotificationSink(BluetoothAudio& parent)
                : _parent(parent)
            {
            }
            ~ComNotificationSink() = default;

        public:
            void Dangling(const Core::IUnknown* remote, const uint32_t interfaceId) override
            {
                Revoked(remote, interfaceId);
            }
            void Revoked(const Core::IUnknown* remote, const uint32_t interfaceId) override
            {
                ASSERT(remote != nullptr);

                _parent.Revoked(remote, interfaceId);
            }

        public:
            BEGIN_INTERFACE_MAP(ComNotificationSink)
            INTERFACE_ENTRY(PluginHost::IShell::ICOMLink::INotification)
            END_INTERFACE_MAP

        private:
            BluetoothAudio& _parent;
        }; // class ComNotificationSink

    public:
        BEGIN_INTERFACE_MAP(BluetoothAudioSink)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IBluetoothAudio::IStream, _sink)
            INTERFACE_AGGREGATE(Exchange::IBluetoothAudio::ISink, _sink)
            INTERFACE_AGGREGATE(Exchange::IBluetoothAudio::ISource, _source)
        END_INTERFACE_MAP

    public:
        BluetoothAudio(const BluetoothAudio&) = delete;
        BluetoothAudio& operator=(const BluetoothAudio&) = delete;

        BluetoothAudio()
            : _service(nullptr)
            , _comNotificationSink(*this)
            , _sink(nullptr)
            , _source(nullptr)
        {
        }

        ~BluetoothAudio()
        {
            if (_sink != nullptr) {
                _sink->Release();
                _sink = nullptr;
            }

            if (_source != nullptr) {
                _source->Release();
                _source = nullptr;
            }
        }

    private:
        void Revoked(const Core::IUnknown* remote, const uint32_t interfaceId)
        {
            if (_sink != nullptr) {
                _sink->Revoked(remote, interfaceId);
            }

            if (_source != nullptr) {
                _source->Revoked(remote, interfaceId);
            }
        }

    public:
        // IPlugin overrides
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override { return {}; }

    private:
        PluginHost::IShell* _service;
        Core::SinkType<ComNotificationSink> _comNotificationSink;
        BluetoothAudioSink* _sink;
        BluetoothAudioSource* _source;
    };

} // namespace Plugin

}
