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

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AVSCommon/SDKInterfaces/KeyWordDetectorStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/KeyWordObserverInterface.h>
#include <AVSCommon/Utils/AudioFormat.h>
#include <KWD/AbstractKeywordDetector.h>

#include "pryon_lite.h"

#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>

namespace WPEFramework {
namespace Plugin {

    class PryonKeywordDetector : public alexaClientSDK::kwd::AbstractKeywordDetector {
    public:
        static std::unique_ptr<PryonKeywordDetector> create(
            std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream,
            alexaClientSDK::avsCommon::utils::AudioFormat audioFormat,
            std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
            std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
            const std::string& modelFilePath,
            std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

        ~PryonKeywordDetector() override;

    private:
        PryonKeywordDetector(
            std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream,
            std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
            std::unordered_set<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
            alexaClientSDK::avsCommon::utils::AudioFormat audioFormat,
            std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(10));

        bool Initialize(const std::string& modelFilePath);
        void DetectionLoop();
        static void DetectionCallback(PryonLiteDecoderHandle handle, const PryonLiteResult* result);
        static void VadCallback(PryonLiteDecoderHandle handle, const PryonLiteVadEvent* vadEvent);

        std::atomic<bool> m_isShuttingDown;
        const std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> m_stream;
        std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream::Reader> m_streamReader;
        std::thread m_detectionThread;
        const size_t m_maxSamplesPerPush;
        PryonLiteDecoderHandle m_decoder;
        PryonLiteDecoderConfig m_config;
        PryonLiteSessionInfo m_sessionInfo;
        char* m_decoderBuffer;
        uint8_t* m_modelBuffer;
    };

} // namespace Plugin
} // namespace WPEFramework
