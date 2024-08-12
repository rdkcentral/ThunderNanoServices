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

#include "CompatibleAudioFormat.h"
#include "TraceCategories.h"

#include <interfaces/IVoiceHandler.h>

#include <Audio/MicrophoneInterface.h>
#include <AVS/SampleApp/InteractionManager.h>
#if defined(ENABLE_SMART_SCREEN_SUPPORT)
#include <SampleApp/GUI/GUIManager.h>
#endif

#include <mutex>
#include <thread>

namespace Thunder {
namespace Plugin {

    /// Responsible for making an interaction on audio data
    template <typename MANAGER>
    class InteractionHandler {
    public:
        static std::unique_ptr<InteractionHandler> Create()
        {
            std::unique_ptr<InteractionHandler<MANAGER>> interactionHandler(new InteractionHandler<MANAGER>());
            if (!interactionHandler) {
                TRACE_GLOBAL(AVSClient, (_T("Failed to create a interaction handler!")));
                return nullptr;
            }

            return interactionHandler;
        }

        InteractionHandler(const InteractionHandler&) = delete;
        InteractionHandler& operator=(const InteractionHandler&) = delete;
        ~InteractionHandler() = default;

    private:
        InteractionHandler()
            : m_interactionManager{ nullptr }
        {
        }

    public:
        bool Initialize(std::shared_ptr<MANAGER> interactionManager)
        {
            bool status = false;
            if (interactionManager) {
                m_interactionManager = interactionManager;
                status = true;
            }
            return status;
        }

        bool Deinitialize()
        {
            bool status = false;
            if (m_interactionManager) {
                m_interactionManager.reset();
                status = true;
            }
            return status;
        }

        void HoldToTalk();

    private:
        std::shared_ptr<MANAGER> m_interactionManager;
    };

    template <>
    inline void InteractionHandler<alexaClientSDK::sampleApp::InteractionManager>::HoldToTalk()
    {
        if (m_interactionManager) {
            m_interactionManager->holdToggled();
        }
    }

#if defined(ENABLE_SMART_SCREEN_SUPPORT)
    template <>
    inline void InteractionHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>::HoldToTalk()
    {
        if (m_interactionManager) {
            m_interactionManager->handleHoldToTalk();
        }
    }
#endif

    // This class provides the audio input from Thunder
    template <typename MANAGER>
    class ThunderVoiceHandler : public alexaClientSDK::applicationUtilities::resources::audio::MicrophoneInterface {
    public:
        static std::unique_ptr<ThunderVoiceHandler> create(std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream, PluginHost::IShell* service, const string& callsign, std::shared_ptr<InteractionHandler<MANAGER>> interactionHandler, alexaClientSDK::avsCommon::utils::AudioFormat audioFormat)
        {
            if (!stream) {
                TRACE_GLOBAL(AVSClient, (_T("Invalid stream")));
                return nullptr;
            }

            if (!service) {
                TRACE_GLOBAL(AVSClient, (_T("Invalid service")));
                return nullptr;
            }

            if (!AudioFormatCompatibility::IsCompatible(audioFormat)) {
                TRACE_GLOBAL(AVSClient, (_T("Audio Format is not compatible")));
                return nullptr;
            }

            std::unique_ptr<ThunderVoiceHandler> thunderVoiceHandler(new ThunderVoiceHandler(stream, service, callsign, interactionHandler));
            if (!thunderVoiceHandler) {
                TRACE_GLOBAL(AVSClient, (_T("Failed to create a ThunderVoiceHandler!")));
                return nullptr;
            }

            if (!thunderVoiceHandler->Initialize()) {
                TRACE_GLOBAL(AVSClient, (_T("ThunderVoiceHandler is not initialized.")));
            }

            return thunderVoiceHandler;
        }

        bool stopStreamingMicrophoneData() override
        {
            TRACE_L1(_T("stopStreamingMicrophoneData()"));

            return true;
        }

        bool startStreamingMicrophoneData() override
        {
            TRACE_L1(_T("startStreamingMicrophoneData()"));

            return true;
        }

        void stateChange(PluginHost::IShell* audiosource)
        {
            if (audiosource->State() == PluginHost::IShell::ACTIVATED) {
                bool status = Initialize();
                if (!status) {
                    TRACE(AVSClient, (_T("Failed to initialize ThunderVoiceHandlerWraper")));
                    return;
                }
            }

            if (audiosource->State() == PluginHost::IShell::DEACTIVATED) {
                bool status = Deinitialize();
                if (!status) {
                    TRACE(AVSClient, (_T("Failed to deinitialize ThunderVoiceHandlerWraper")));
                    return;
                }
            }
        }

        ~ThunderVoiceHandler()
        {
            if (m_service != nullptr) {
                m_service->Release();
            }
        }

    private:
        ThunderVoiceHandler(std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream, PluginHost::IShell* service, const string& callsign, std::shared_ptr<InteractionHandler<MANAGER>> interactionHandler)
            : m_audioInputStream{ stream }
            , m_callsign{ callsign }
            , m_service{ service }
            , m_voiceProducer{ nullptr }
            , m_isInitialized{ false }
            , m_interactionHandler{ interactionHandler }
            , m_voiceHandler{ Core::ProxyType<VoiceHandler>::Create(this) }
        {
            m_service->AddRef();
        }

        /// Initializes ThunderVoiceHandler.
        bool Initialize()
        {
            const std::lock_guard<std::mutex> lock{ m_mutex };
            bool error = false;

            if (m_isInitialized) {
                error = true;
            }

            if (error != true) {
                m_writer = m_audioInputStream->createWriter(alexaClientSDK::avsCommon::avs::AudioInputStream::Writer::Policy::NONBLOCKABLE);
                if (m_writer == nullptr) {
                    TRACE(AVSClient, (_T("Failed to create stream writer")));
                    error = true;
                }
            }

            if (error != true) {
                m_voiceProducer = m_service->QueryInterfaceByCallsign<Exchange::IVoiceProducer>(m_callsign);
                if (m_voiceProducer == nullptr) {
                    TRACE(AVSClient, (_T("Failed to obtain VoiceProducer interface!")));
                    error = true;
                } else {
                    m_voiceProducer->Callback((&(*m_voiceHandler)));
                }
            }

            if (error != true) {
                m_isInitialized = true;
            }

            return m_isInitialized;
        }

        bool Deinitialize()
        {
            const std::lock_guard<std::mutex> lock{ m_mutex };

            if (m_writer) {
                m_writer.reset();
            }

            if (m_voiceProducer) {
                m_voiceProducer->Release();
            }

            m_isInitialized = false;
            return true;
        }

        bool isStreaming() override
        {
            return m_voiceHandler->IsStreaming();
        }

    private:
        ///  Responsible for getting audio data from Thunder
        class VoiceHandler : public Exchange::IVoiceHandler {
        public:
            VoiceHandler(ThunderVoiceHandler* parent)
                : m_profile{ nullptr }
                , m_parent{ parent }
                , m_isStarted{ false }
            {
            }

            // A callback method that is invoked with each start of the audio transmission.
            // The profile should stay the same between Start() and Stop().
            void Start(const Exchange::IVoiceProducer::IProfile* profile) override
            {
                TRACE_L1(_T("ThunderVoiceHandler::VoiceHandler::Start()"));

                if (m_isStarted == true) {
                    TRACE(AVSClient, (_T("The audiotransmission is already started. Skipping...")));
                } else {
                    m_isStarted = true;
                    m_profile = profile;
                    if (m_profile) {
                        m_profile->AddRef();
                    }

                    if (m_parent && m_parent->m_interactionHandler) {
                        m_parent->m_interactionHandler->HoldToTalk();
                    }
                }
            }

            // A callback method that is invoked with each stop of the audio transmission.
            // The profile should stay the same between Start() and Stop().
            void Stop() override
            {
                TRACE_L1(_T("ThunderVoiceHandler::VoiceHandler::Stop()"));

                if (m_profile) {
                    m_profile->Release();
                    m_profile = nullptr;
                }

                if (m_parent && m_parent->m_interactionHandler) {
                    m_parent->m_interactionHandler->HoldToTalk();
                }

                m_isStarted = false;
            }

            void Data(const uint32_t sequenceNo VARIABLE_IS_NOT_USED, const uint8_t data[], const uint16_t length) override
            {
                TRACE_L1(_T("ThunderVoiceHandler::VoiceHandler::Data()"));

                if (m_parent && m_parent->m_writer) {
                    // incoming data length = number of bytes
                    size_t nWords = length / m_parent->m_writer->getWordSize();
                    ssize_t rc = m_parent->m_writer->write(data, nWords);
                    if (rc <= 0) {
                        TRACE(AVSClient, (_T("Failed to write to stream with rc = %d"), rc));
                    }
                }
            }
            bool IsStreaming()
            {
                return (m_isStarted);
            }

            BEGIN_INTERFACE_MAP(VoiceHandler)
            INTERFACE_ENTRY(Exchange::IVoiceHandler)
            END_INTERFACE_MAP

        private:
            const Exchange::IVoiceProducer::IProfile* m_profile;
            ThunderVoiceHandler* m_parent;
            bool m_isStarted;
        };

    private:
        const std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> m_audioInputStream;
        string m_callsign;
        PluginHost::IShell* m_service;
        Exchange::IVoiceProducer* m_voiceProducer;
        bool m_isInitialized;
        std::shared_ptr<InteractionHandler<MANAGER>> m_interactionHandler;
        std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream::Writer> m_writer;

        Core::ProxyType<VoiceHandler> m_voiceHandler;

        std::mutex m_mutex;
    };

} // namespace Plugin
} // namespace Thunder
