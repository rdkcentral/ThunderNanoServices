/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "Administrator.h"
#include <atomic>
#include <gst/gst.h>
#include <vector>

namespace WPEFramework {
namespace Player {
    namespace Implementation {

        namespace {

            class CENC : public IPlayerPlatform, public Core::Thread {
            public:
                CENC(const CENC&) = delete;
                CENC& operator=(const CENC&) = delete;

                CENC(const Exchange::IStream::streamtype streamType, const uint8_t index);

            public:
                struct PipelineData {
                    PipelineData()
                        : _playbin(nullptr)
                        , _mainLoop(nullptr)
                    {
                    }

                    GstElement* _playbin;
                    GMainLoop* _mainLoop;
                };

            public:
                // IPlayerPlatform methods
                // --------------------------------------------------------------
                uint32_t Setup() override;
                uint32_t Teardown() override;
                void Callback(ICallback* callback) override;
                string Metadata() const override;
                Exchange::IStream::streamtype Type() const override;
                Exchange::IStream::drmtype DRM() const override;
                Exchange::IStream::state State() const override;
                uint32_t Error() const override;
                uint8_t Index() const override;
                uint32_t Load(const string& uri) override;
                uint32_t AttachDecoder(const uint8_t) override;
                uint32_t DetachDecoder(const uint8_t) override;
                uint32_t Speed(const int32_t speed) override;
                int32_t Speed() const override;
                const std::vector<int32_t>& Speeds() const override;
                void Position(const uint64_t absoluteTime) override;
                uint64_t Position() const override;
                void TimeRange(uint64_t& begin, uint64_t& end) const override;
                const Rectangle& Window() const override;
                void Window(const Rectangle& rectangle) override;
                uint32_t Order() const override;
                void Order(const uint32_t order) override;
                const std::list<ElementaryStream>& Elements() const override;

                // Core::Thread method
                // ------------------------
                uint32_t Worker() override;

            private:
                // Gstreamer helper methods
                // ----------------------------------------------
                uint32_t SetupGstElements();
                uint32_t TeardownGstElements();
                void SendSeekEvent();
                GstEvent* CreateSeekEvent(const uint64_t position);

                // OCDM helper methods
                // -----------------------------------------------
                void initializeOcdm();

                uint8_t _index;
                int32_t _speed;
                std::vector<int32_t> _vecSpeeds;
                Exchange::IStream::streamtype _streamType;
                uint32_t _error;

                mutable Core::CriticalSection _adminLock;
                PipelineData _data;
                GstBus* _bus;

                // OpenCDMSystem* _system;
            };

            class Config : public Core::JSON::Container {
            public:
                Config(const Config&) = delete;
                Config& operator=(const Config&) = delete;

                Config()
                    : Core::JSON::Container()
                    , Speeds()
                {
                    Add(_T("speeds"), &Speeds);
                }

                Core::JSON::ArrayType<Core::JSON::DecSInt32> Speeds;
            } config;
        } // namespace

    } // namespace Implementation
} // namespace Player
}
