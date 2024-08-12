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

#include "PryonKeywordDetector.h"

#include "CompatibleAudioFormat.h"

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <memory>

namespace Thunder {
namespace Plugin {

    using namespace alexaClientSDK;
    using namespace alexaClientSDK::avsCommon;
    using namespace alexaClientSDK::avsCommon::avs;
    using namespace alexaClientSDK::avsCommon::sdkInterfaces;
    using namespace alexaClientSDK::avsCommon::utils::logger;

    static const size_t HERTZ_PER_KILOHERTZ = 1000;
    const std::chrono::milliseconds TIMEOUT_FOR_READ_CALLS = std::chrono::milliseconds(1000);

    static const char* DEFAULT_LOCALE = "en-US";
    static const std::string KEY_MODEL_LOCALES = "alexa";
    static constexpr const char* DETECTION_KEYWORD = "ALEXA";
    static constexpr const uint32_t DETECTION_TRESHOLD = 200;

    std::unique_ptr<PryonKeywordDetector> PryonKeywordDetector::create(
        std::shared_ptr<AudioInputStream> stream,
        utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>>
            keyWordDetectorStateObservers,
        const std::string& modelsFilePath,
        std::chrono::milliseconds msToPushPerIteration)
    {
        if (!stream) {
            TRACE_GLOBAL(AVSClient, (_T("Failed to create PryonKeywordDetector: stream is nullptr")));
            return nullptr;
        }

        if (!AudioFormatCompatibility::IsCompatible(audioFormat)) {
            return nullptr;
        }

        std::unique_ptr<PryonKeywordDetector> detector(new PryonKeywordDetector(
            stream, keyWordObservers, keyWordDetectorStateObservers, audioFormat, msToPushPerIteration));
        if (!detector->Initialize(modelsFilePath)) {
            TRACE_GLOBAL(AVSClient, (_T("Failed to initialize PryonKeywordDetector")));
            return nullptr;
        }

        return detector;
    }

    PryonKeywordDetector::~PryonKeywordDetector()
    {
        m_isShuttingDown = true;
        if (m_detectionThread.joinable()) {
            m_detectionThread.join();
        }

        PryonLiteError error = PryonLiteDecoder_Destroy(&m_decoder);
        if (error != PRYON_LITE_ERROR_OK) {

            TRACE(AVSClient, (_T("Failed to destroy PryonLiteDecoder")));
        }

        delete[] m_decoderBuffer;
        delete[] m_modelBuffer;
    }

    PryonKeywordDetector::PryonKeywordDetector(
        std::shared_ptr<AudioInputStream> stream,
        std::unordered_set<std::shared_ptr<KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers,
        utils::AudioFormat audioFormat,
        std::chrono::milliseconds msToPushPerIteration)
        : acsdkKWDImplementations::AbstractKeywordDetector(keyWordObservers, keyWordDetectorStateObservers)
        , m_isShuttingDown{ false }
        , m_stream{ stream }
        , m_streamReader{ nullptr }
        , m_detectionThread{}
        , m_maxSamplesPerPush((audioFormat.sampleRateHz / HERTZ_PER_KILOHERTZ) * msToPushPerIteration.count())
        , m_decoder{ nullptr }
        , m_config{}
        , m_sessionInfo{}
        , m_decoderBuffer{ nullptr }
        , m_modelBuffer{ nullptr }
    {
    }

    bool PryonKeywordDetector::Initialize(const std::string& modelFilePath)
    {
        m_streamReader = m_stream->createReader(AudioInputStream::Reader::Policy::BLOCKING);
        if (!m_streamReader) {
            TRACE(AVSClient, (_T("Failed to initialize PryonKeywordDetector: m_streamReader is nullptr")));
            return false;
        }

        m_config = PryonLiteDecoderConfig_Default;

        std::set<std::string> localePaths;
        auto localeToModelsConfig = alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot()[KEY_MODEL_LOCALES];
        bool isLocaleFound = localeToModelsConfig.getStringValues(DEFAULT_LOCALE, &localePaths);
        if (!isLocaleFound) {
            TRACE(AVSClient, (_T("Failed to get default locale from config")));
            return false;
        }

        std::string localizedModelFilepath = "";
        for (auto it = localePaths.cbegin(); it != localePaths.cend(); ++it) {
            if (!it->empty()) {
                localizedModelFilepath = modelFilePath + "/" + (*it) + ".bin";
            }
        }

        Core::File modelFile(localizedModelFilepath);
        if (modelFile.Open()) {
            m_config.sizeofModel = modelFile.Size();
            m_modelBuffer = new uint8_t[m_config.sizeofModel]();
            uint32_t nRead = modelFile.Read(m_modelBuffer, m_config.sizeofModel);
            if (nRead != m_config.sizeofModel) {
                TRACE(AVSClient, (_T("Failed to read model file")));
                return false;
            }
        } else {
            TRACE(AVSClient, (_T("Failed to open model file")));
            return false;
        }
        modelFile.Close();

        m_config.model = m_modelBuffer;

        // Query for the size of instance memory required by the decoder
        PryonLiteModelAttributes modelAttributes;
        PryonLiteError error = PryonLite_GetModelAttributes(m_config.model, m_config.sizeofModel, &modelAttributes);
        if (error) {
            TRACE(AVSClient, (_T("Failed to get model attributes from config")));
            return false;
        }

        m_decoderBuffer = new char[modelAttributes.requiredDecoderMem]();
        m_config.decoderMem = m_decoderBuffer;
        m_config.sizeofDecoderMem = modelAttributes.requiredDecoderMem;
        m_config.userData = reinterpret_cast<void*>(this);
        m_config.detectThreshold = DETECTION_TRESHOLD;
        m_config.resultCallback = DetectionCallback;
        m_config.vadCallback = VadCallback;
        m_config.useVad = 1;

        error = PryonLiteDecoder_Initialize(&m_config, &m_sessionInfo, &m_decoder);
        if (error) {
            TRACE(AVSClient, (_T("Failed to initialize PryonLiteDecoder")));
            return false;
        }

        error = PryonLiteDecoder_SetDetectionThreshold(m_decoder, DETECTION_KEYWORD, m_config.detectThreshold);
        if (error) {
            TRACE(AVSClient, (_T("Failed to set detection treshold")));
            return false;
        }

        m_isShuttingDown = false;
        m_detectionThread = std::thread(&PryonKeywordDetector::DetectionLoop, this);
        return true;
    }

    void PryonKeywordDetector::DetectionLoop()
    {
        std::vector<int16_t> audioDataToPush(m_maxSamplesPerPush);
        ssize_t wordsRead;
        bool didErrorOccur = false;
        PryonLiteError writeStatus = PRYON_LITE_ERROR_OK;

        notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);

        while (!m_isShuttingDown) {
            wordsRead = readFromStream(
                m_streamReader,
                m_stream,
                audioDataToPush.data(),
                audioDataToPush.size(),
                TIMEOUT_FOR_READ_CALLS,
                &didErrorOccur);

            if (didErrorOccur) {
                TRACE(AVSClient, (_T("Overrun in detection loop")));
                break;
            } else if (wordsRead > 0) {
                writeStatus = PryonLiteDecoder_PushAudioSamples(m_decoder, audioDataToPush.data(), audioDataToPush.size());
                if (writeStatus) {
                    TRACE(AVSClient, (_T("Error (%d) in detection loop"), writeStatus));
                    notifyKeyWordDetectorStateObservers(KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ERROR);
                    break;
                }
            } else {
                TRACE(AVSClient, (_T("Unhandled error in detection loop")));
            }
        }

        m_streamReader->close();
        TRACE_L1(_T("End of detection thread"));
    }

    /* static */ void PryonKeywordDetector::DetectionCallback(PryonLiteDecoderHandle handle VARIABLE_IS_NOT_USED, const PryonLiteResult* result)
    {
        TRACE_L1(_T("DetectionCallback()"));

        if (!result) {
            TRACE_GLOBAL(AVSClient, (_T("Result is nullptr")));
            return;
        }

        const PryonKeywordDetector* pryonKWD = reinterpret_cast<PryonKeywordDetector*>(result->userData);
        if (!pryonKWD) {
            TRACE_GLOBAL(AVSClient, (_T("User data is nullptr")));
            return;
        }

        auto sampleLen = result->endSampleIndex - result->beginSampleIndex;
        TRACE_GLOBAL(AVSClient, (_T("Detection Callback Results:\n"
                     "confidenence = %d, beginSampleIndex = %d, endSampleIndex = %d, m_streamReader->tell() = %d, sampleLen = %d, keyword = %s"),
            result->confidence, result->beginSampleIndex, result->endSampleIndex, pryonKWD->m_streamReader->tell(), sampleLen, result->keyword));

        pryonKWD->notifyKeyWordObservers(
            pryonKWD->m_stream,
            result->keyword,
            pryonKWD->m_streamReader->tell() - sampleLen,
            pryonKWD->m_streamReader->tell());
    }

    /* static */ void PryonKeywordDetector::VadCallback(PryonLiteDecoderHandle handle VARIABLE_IS_NOT_USED, const PryonLiteVadEvent* vadEvent VARIABLE_IS_NOT_USED)
    {
        TRACE_L1(_T("VadCallback()"));
    }

} // namespace Plugin
} // namespace Thunder
