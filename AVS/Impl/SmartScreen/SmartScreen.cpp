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

#include "SmartScreen.h"

#if defined(KWD_PRYON)
#include "PryonKeywordDetector.h"
#endif
#include "ThunderLogger.h"
#include "ThunderVoiceHandler.h"
#include "TraceCategories.h"

#include <ACL/Transport/HTTP2TransportFactory.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPut.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <Alerts/Storage/SQLiteAlertStorage.h>
#include <Audio/AudioFactory.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <ContextManager/ContextManager.h>
#include <Notifications/SQLiteNotificationsStorage.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>

#include <SmartScreen/Communication/WebSocketServer.h>

#include <SmartScreen/SampleApp/AplCoreEngineSDKLogBridge.h>
#include <SmartScreen/SampleApp/AplCoreGuiRenderer.h>
#include <SmartScreen/SampleApp/JsonUIManager.h>
#include <SmartScreen/SampleApp/KeywordObserver.h>
#include <SmartScreen/SampleApp/LocaleAssetsManager.h>
#include <SmartScreen/SampleApp/PortAudioMicrophoneWrapper.h>

#include <cctype>
#include <fstream>

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(SmartScreen, 1, 0);

    using namespace alexaClientSDK;

    // Alexa Client Config keys
    static const std::string SAMPLE_APP_CONFIG_KEY("sampleApp");
    static const std::string FIRMWARE_VERSION_KEY("firmwareVersion");
    static const std::string ENDPOINT_KEY("endpoint");

    // Share Data stream Configuraiton
    static const size_t MAX_READERS = 10;
    static const size_t WORD_SIZE = 2;
    static const unsigned int SAMPLE_RATE_HZ = 16000;
    static const unsigned int NUM_CHANNELS = 1;
    static const std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER = std::chrono::seconds(15);
    static const size_t BUFFER_SIZE_IN_SAMPLES = (SAMPLE_RATE_HZ)*AMOUNT_OF_AUDIO_DATA_IN_BUFFER.count();

    // Thunder voice handler
    static constexpr const char* PORTAUDIO_CALLSIGN("PORTAUDIO");

    // smart screein
    static const std::string WEBSOCKET_INTERFACE_KEY("websocketInterface");
    static const std::string WEBSOCKET_PORT_KEY("websocketPort");
    static const std::string DEFAULT_WEBSOCKET_INTERFACE = "127.0.0.1";
    static const int DEFAULT_WEBSOCKET_PORT = 8933;

    bool SmartScreen::Initialize(PluginHost::IShell* service, const string& configuration)
    {
        TRACE(AVSClient, (_T("Initializing SmartScreen...")));

        Config config;
        bool status = true;

        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);
        _service = service;

        config.FromString(configuration);
        const std::string logLevel = config.LogLevel.Value();
        if (logLevel.empty() == true) {
            TRACE(AVSClient, (_T("Missing log level")));
            status = false;
        } else {
            status = InitSDKLogs(logLevel);
        }

        const std::string alexaClientConfig = config.AlexaClientConfig.Value();
        if ((status == true) && (alexaClientConfig.empty() == true)) {
            TRACE(AVSClient, (_T("Missing AlexaClient config file")));
            status = false;
        }

        const std::string smartScreenConfig = config.SmartScreenConfig.Value();
        if ((status == true) && (alexaClientConfig.empty() == true)) {
            TRACE(AVSClient, (_T("Missing SmartScreenConfig config file")));
            status = false;
        }

        const std::string pathToInputFolder = config.KWDModelsPath.Value();
        if ((status == true) && (pathToInputFolder.empty() == true)) {
            TRACE(AVSClient, (_T("Missing KWD models path")));
            status = false;
        }

        const std::string audiosource = config.Audiosource.Value();
        if ((status == true) && (audiosource.empty() == true)) {
            TRACE(AVSClient, (_T("Missing audiosource")));
            status = false;
        }

        const bool enableKWD = config.EnableKWD.Value();
        if (enableKWD == true) {
#if !defined(KWD_PRYON)
            TRACE(AVSClient, (_T("Requested KWD, but it is not compiled in")));
            status = false;
#endif
        }

        std::vector<std::shared_ptr<std::istream>> configJsonStreams;
        if ((status == true) && (JsonConfigToStream(configJsonStreams, alexaClientConfig) == false)) {
            TRACE(AVSClient, (_T("Failed to load alexaClientConfig")));
            status = false;
        }
        if ((status == true) && (JsonConfigToStream(configJsonStreams, smartScreenConfig) == false)) {
            TRACE(AVSClient, (_T("Failed to load smartScreenConfig")));
            status = false;
        }
#if defined(KWD_PRYON)
        if (enableKWD) {
            if ((status == true) && (JsonConfigToStream(configJsonStreams, pathToInputFolder + "/localeToModels.json") == false)) {
                TRACE(AVSClient, (_T("Failed to load localeToModels.json")));
                status = false;
            }
        }
#endif
        if ((status == true) && (avsCommon::avs::initialization::AlexaClientSDKInit::initialize(configJsonStreams) == false)) {
            TRACE(AVSClient, (_T("Failed to initialize SDK!")));
            return false;
        }

        if (status == true) {
            status = Init(audiosource, enableKWD, pathToInputFolder);
        }

        return status;
    }

    bool SmartScreen::Init(const std::string& audiosource, const bool enableKWD, const std::string& pathToInputFolder VARIABLE_IS_NOT_USED)
    {
        auto config = avsCommon::utils::configuration::ConfigurationNode::getRoot();

        std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo = avsCommon::utils::DeviceInfo::create(config);
        if (!deviceInfo) {
            TRACE(AVSClient, (_T("Failed to create deviceInfo")));
            return false;
        }
        int firmwareVersion = static_cast<int>(avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION);
        config[SAMPLE_APP_CONFIG_KEY].getInt(FIRMWARE_VERSION_KEY, &firmwareVersion, firmwareVersion);

        auto customerDataManager = std::make_shared<registrationManager::CustomerDataManager>();
        if (!customerDataManager) {
            TRACE(AVSClient, (_T("Failed to create customerDataManager")));
            return false;
        }

        // speakers and media players
        auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();

        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> speakSpeaker;
        std::tie(m_speakMediaPlayer, speakSpeaker) = createApplicationMediaPlayer(
            httpContentFetcherFactory,
            false,
            avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
            "SpeakMediaPlayer");
        if (!m_speakMediaPlayer || !speakSpeaker) {
            TRACE(AVSClient, (_T("Failed to create SpeakMediaPlayer")));
            return false;
        }

        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> audioSpeaker;
        std::tie(m_audioMediaPlayer, audioSpeaker) = createApplicationMediaPlayer(
            httpContentFetcherFactory,
            false,
            avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
            "AudioMediaPlayer");
        if (!m_audioMediaPlayer || !audioSpeaker) {
            TRACE(AVSClient, (_T("Failed to create AudioMediaPlayer")));
            return false;
        }

        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> alertsSpeaker;
        std::tie(m_alertsMediaPlayer, alertsSpeaker) = createApplicationMediaPlayer(
            httpContentFetcherFactory,
            false,
            avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_ALERTS_VOLUME,
            "AlertsMediaPlayer");
        if (!m_alertsMediaPlayer || !alertsSpeaker) {
            TRACE(AVSClient, (_T("Failed to create AlertsMediaPlayer")));
            return false;
        }

        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> notificationsSpeaker;
        std::tie(m_notificationsMediaPlayer, notificationsSpeaker) = createApplicationMediaPlayer(
            httpContentFetcherFactory,
            false,
            avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_ALERTS_VOLUME,
            "NotificationsMediaPlayer");
        if (!m_notificationsMediaPlayer || !notificationsSpeaker) {
            TRACE(AVSClient, (_T("Failed to create NotificationsMediaPlayer")));
            return false;
        }

        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> bluetoothSpeaker;
        std::tie(m_bluetoothMediaPlayer, bluetoothSpeaker) = createApplicationMediaPlayer(
            httpContentFetcherFactory,
            false,
            avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
            "BluetoothMediaPlayer");
        if (!m_bluetoothMediaPlayer || !bluetoothSpeaker) {
            TRACE(AVSClient, (_T("Failed to create BluetoothMediaPlayer")));
            return false;
        }

        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> ringtoneSpeaker;
        std::tie(m_ringtoneMediaPlayer, ringtoneSpeaker) = createApplicationMediaPlayer(
            httpContentFetcherFactory,
            false,
            avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
            "RingtoneMediaPlayer");
        if (!m_ringtoneMediaPlayer || !ringtoneSpeaker) {
            TRACE(AVSClient, (_T("Failed to create RingtoneMediaPlayer")));
            return false;
        }

        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> systemSoundSpeaker;
        std::tie(m_systemSoundMediaPlayer, systemSoundSpeaker) = createApplicationMediaPlayer(
            httpContentFetcherFactory,
            false,
            avsCommon::sdkInterfaces::SpeakerInterface::Type::AVS_SPEAKER_VOLUME,
            "SystemSoundMediaPlayer");
        if (!m_systemSoundMediaPlayer || !systemSoundSpeaker) {
            TRACE(AVSClient, (_T("Failed to create SystemSoundMediaPlayer")));
            return false;
        }

        auto audioFactory = std::make_shared<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory>();
        if (!audioFactory) {
            TRACE(AVSClient, (_T("Failed to create audioFactory")));
            return false;
        }

        // storage
        auto authDelegateStorage = authorization::cblAuthDelegate::SQLiteCBLAuthDelegateStorage::create(config);

        auto alertStorage = alexaClientSDK::capabilityAgents::alerts::storage::SQLiteAlertStorage::create(config, audioFactory->alerts());
        if (!alertStorage) {
            TRACE(AVSClient, (_T("Failed to create alertStorage")));
            return false;
        }

        auto messageStorage = alexaClientSDK::certifiedSender::SQLiteMessageStorage::create(config);
        if (!messageStorage) {
            TRACE(AVSClient, (_T("Failed to create messageStorage")));
            return false;
        }

        auto notificationsStorage = alexaClientSDK::capabilityAgents::notifications::SQLiteNotificationsStorage::create(config);
        if (!notificationsStorage) {
            TRACE(AVSClient, (_T("Failed to create notificationsStorage")));
            return false;
        }

        auto deviceSettingsStorage = alexaClientSDK::settings::storage::SQLiteDeviceSettingStorage::create(config);
        if (!deviceSettingsStorage) {
            TRACE(AVSClient, (_T("Failed to create deviceSettingsStorage")));
            return false;
        }

        std::shared_ptr<storage::sqliteStorage::SQLiteMiscStorage> miscStorage = storage::sqliteStorage::SQLiteMiscStorage::create(config);
        if (!miscStorage) {
            TRACE(AVSClient, (_T("Failed to create deviceSettingsStorage")));
            return false;
        }

        // Asset Manager
        auto localeAssetsManager = alexaSmartScreenSDK::sampleApp::LocaleAssetsManager::create(enableKWD);
        if (!localeAssetsManager) {
            TRACE(AVSClient, (_T("Failed to create localeAssetsManager")));
            return false;
        }

        // SmartScreen connection
        std::string websocketInterface;
        int websocketPortNumber;
        config.getString(WEBSOCKET_INTERFACE_KEY, &websocketInterface, DEFAULT_WEBSOCKET_INTERFACE);
        config.getInt(WEBSOCKET_PORT_KEY, &websocketPortNumber, DEFAULT_WEBSOCKET_PORT);
        auto webSocketServer = std::make_shared<alexaSmartScreenSDK::communication::WebSocketServer>(websocketInterface, websocketPortNumber);
        if (!webSocketServer) {
            TRACE(AVSClient, (_T("Failed to create webSocketServer")));
            return false;
        }

        // GUI
        m_guiClient = alexaSmartScreenSDK::sampleApp::gui::GUIClient::create(webSocketServer, miscStorage);
        if (!m_guiClient) {
            TRACE(AVSClient, (_T("Failed to create m_guiClient")));
            return false;
        }

        auto aplCoreConnectionManager = std::make_shared<alexaSmartScreenSDK::sampleApp::AplCoreConnectionManager>(m_guiClient);
        auto aplCoreGuiRenderer = std::make_shared<alexaSmartScreenSDK::sampleApp::AplCoreGuiRenderer>(aplCoreConnectionManager, httpContentFetcherFactory);

        m_guiClient->setAplCoreConnectionManager(aplCoreConnectionManager);
        m_guiClient->setAplCoreGuiRenderer(aplCoreGuiRenderer);
        if (!m_guiClient->start()) {
            TRACE(AVSClient, (_T("Failed to start m_guiClient")));
            return false;
        }

        auto userInterfaceManager = std::make_shared<alexaSmartScreenSDK::sampleApp::JsonUIManager>(
            std::static_pointer_cast<alexaSmartScreenSDK::smartScreenSDKInterfaces::GUIClientInterface>(m_guiClient), deviceInfo);
        if (!userInterfaceManager) {
            TRACE(AVSClient, (_T("Failed to create userInterfaceManager")));
            return false;
        }
        m_guiClient->setObserver(userInterfaceManager);
        std::string APLVersion = m_guiClient->getMaxAPLVersion();

        // Context
        auto contextManager = contextManager::ContextManager::create();
        if (!contextManager) {
            TRACE(AVSClient, (_T("Failed to create contextManager")));
            return false;
        }

        // AVS Authorization
        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate = authorization::cblAuthDelegate::CBLAuthDelegate::create(
            config, customerDataManager, std::move(authDelegateStorage), userInterfaceManager, nullptr, deviceInfo);
        if (!authDelegate) {
            TRACE(AVSClient, (_T("Failed to create authDelegate")));
            return false;
        }
        authDelegate->addAuthObserver(userInterfaceManager);

        // AVS Connection
        std::shared_ptr<avsCommon::utils::libcurlUtils::HttpPut> httpPut = avsCommon::utils::libcurlUtils::HttpPut::create();
        m_capabilitiesDelegate = alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate::create(
            authDelegate, miscStorage, httpPut, customerDataManager, config, deviceInfo);
        if (!m_capabilitiesDelegate) {
            TRACE(AVSClient, (_T("Failed to create m_capabilitiesDelegate")));
            return false;
        }
        m_capabilitiesDelegate->addCapabilitiesObserver(userInterfaceManager);

        auto postConnectSynchronizerFactory = acl::PostConnectSynchronizerFactory::create(contextManager);
        if (!postConnectSynchronizerFactory) {
            TRACE(AVSClient, (_T("Failed to create postConnectSynchronizerFactory")));
            return false;
        }

        auto internetConnectionMonitor = avsCommon::utils::network::InternetConnectionMonitor::create(httpContentFetcherFactory);
        if (!internetConnectionMonitor) {
            TRACE(AVSClient, (_T("Failed to create internetConnectionMonitor")));
            return false;
        }

        auto transportFactory = std::make_shared<acl::HTTP2TransportFactory>(
            std::make_shared<avsCommon::utils::libcurlUtils::LibcurlHTTP2ConnectionFactory>(),
            postConnectSynchronizerFactory);
        if (!transportFactory) {
            TRACE(AVSClient, (_T("Failed to create transportFactory")));
            return false;
        }

        // MAIN CLIENT
        std::shared_ptr<alexaSmartScreenSDK::smartScreenClient::SmartScreenClient> client = alexaSmartScreenSDK::smartScreenClient::SmartScreenClient::create(
            deviceInfo,
            customerDataManager,
            m_externalMusicProviderMediaPlayersMap,
            m_externalMusicProviderSpeakersMap,
            m_adapterToCreateFuncMap,
            m_speakMediaPlayer,
            m_audioMediaPlayer,
            m_alertsMediaPlayer,
            m_notificationsMediaPlayer,
            m_bluetoothMediaPlayer,
            m_ringtoneMediaPlayer,
            m_systemSoundMediaPlayer,
            speakSpeaker,
            audioSpeaker,
            alertsSpeaker,
            notificationsSpeaker,
            bluetoothSpeaker,
            ringtoneSpeaker,
            systemSoundSpeaker,
            std::vector<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>>(),
            nullptr,
            audioFactory,
            authDelegate,
            std::move(alertStorage),
            std::move(messageStorage),
            std::move(notificationsStorage),
            std::move(deviceSettingsStorage),
            nullptr,
            std::move(miscStorage),
            { userInterfaceManager },
            { userInterfaceManager },
            std::move(internetConnectionMonitor),
            m_capabilitiesDelegate,
            contextManager,
            transportFactory,
            localeAssetsManager,
            nullptr,
            firmwareVersion,
            true,
            nullptr,
            nullptr,
            m_guiManager,
            APLVersion);

        if (!client) {
            TRACE(AVSClient, (_T("Failed to create default SDK client")));
            return false;
        }

        client->addSpeakerManagerObserver(userInterfaceManager);
        client->addNotificationsObserver(userInterfaceManager);

        // Shared Data stream
        size_t bufferSize = alexaClientSDK::avsCommon::avs::AudioInputStream::calculateBufferSize(
            BUFFER_SIZE_IN_SAMPLES, WORD_SIZE, MAX_READERS);
        auto buffer = std::make_shared<alexaClientSDK::avsCommon::avs::AudioInputStream::Buffer>(bufferSize);
        std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream = alexaClientSDK::avsCommon::avs::AudioInputStream::create(buffer, WORD_SIZE, MAX_READERS);
        if (!sharedDataStream) {
            TRACE(AVSClient, (_T("Failed to create sharedDataStream")));
            return false;
        }

        // Audio providers
        alexaClientSDK::avsCommon::utils::AudioFormat compatibleAudioFormat;
        compatibleAudioFormat.sampleRateHz = SAMPLE_RATE_HZ;
        compatibleAudioFormat.sampleSizeInBits = WORD_SIZE * CHAR_BIT;
        compatibleAudioFormat.numChannels = NUM_CHANNELS;
        compatibleAudioFormat.endianness = alexaClientSDK::avsCommon::utils::AudioFormat::Endianness::LITTLE;
        compatibleAudioFormat.encoding = alexaClientSDK::avsCommon::utils::AudioFormat::Encoding::LPCM;

        alexaClientSDK::capabilityAgents::aip::AudioProvider tapToTalkAudioProvider(
            sharedDataStream,
            compatibleAudioFormat,
            alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
            true, // alwaysReadable
            true, // canOverride
            true); // canBeOverridden

        alexaClientSDK::capabilityAgents::aip::AudioProvider holdToTalkAudioProvider(
            sharedDataStream,
            compatibleAudioFormat,
            alexaClientSDK::capabilityAgents::aip::ASRProfile::CLOSE_TALK,
            false, // alwaysReadable
            true, // canOverride
            false); // canBeOverridden

        alexaClientSDK::capabilityAgents::aip::AudioProvider wakeWordAudioProvider(capabilityAgents::aip::AudioProvider::null());
#if defined(KWD_PRYON)
        if (enableKWD) {
            wakeWordAudioProvider = alexaClientSDK::capabilityAgents::aip::AudioProvider(sharedDataStream,
                compatibleAudioFormat,
                alexaClientSDK::capabilityAgents::aip::ASRProfile::NEAR_FIELD,
                true, // alwaysReadable
                false, // canOverride
                true); // canBeOverridden
        }
#endif

        // Audio input
        std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> aspInput = nullptr;
        std::shared_ptr<InteractionHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>> aspInputInteractionHandler = nullptr;

        if (audiosource == PORTAUDIO_CALLSIGN) {
#if defined(PORTAUDIO)
            aspInput = alexaSmartScreenSDK::sampleApp::PortAudioMicrophoneWrapper::create(sharedDataStream);
#else
            TRACE(AVSClient, (_T("PortAudio support is not compiled in")));
            return false;
#endif
        } else {
            aspInputInteractionHandler = InteractionHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>::Create();
            if (!aspInputInteractionHandler) {
                TRACE(AVSClient, (_T("Failed to create aspInputInteractionHandler")));
                return false;
            }

            m_thunderVoiceHandler = ThunderVoiceHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>::create(sharedDataStream, _service, audiosource, aspInputInteractionHandler, compatibleAudioFormat);
            aspInput = m_thunderVoiceHandler;
            aspInput->startStreamingMicrophoneData();
        }
        if (!aspInput) {
            TRACE(AVSClient, (_T("Failed to create aspInput")));
            return false;
        }

        // Key Word Detection
#if defined(KWD_PRYON)
        if (enableKWD) {
            auto keywordObserver = std::make_shared<alexaSmartScreenSDK::sampleApp::KeywordObserver>(client, wakeWordAudioProvider);
            m_keywordDetector = PryonKeywordDetector::create(
                sharedDataStream,
                compatibleAudioFormat,
                { keywordObserver },
                std::unordered_set<
                    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>(),
                pathToInputFolder);
            if (!m_keywordDetector) {
                TRACE(AVSClient, (_T("Failed to create m_keywordDetector")));
                return false;
            }
        }
#endif
        // GUI manager / Interaction manager
        m_guiManager = alexaSmartScreenSDK::sampleApp::gui::GUIManager::create(
            m_guiClient,
            holdToTalkAudioProvider,
            tapToTalkAudioProvider,
            aspInput,
            wakeWordAudioProvider);

        // Interaction Manager
        if (audiosource != PORTAUDIO_CALLSIGN) {
            if (aspInputInteractionHandler) {
                // register interactions that ThunderVoiceHandler may initiate
                if (!aspInputInteractionHandler->Initialize(m_guiManager)) {
                    TRACE(AVSClient, (_T("Failed to initialize aspInputInteractionHandle")));
                    return false;
                }
            }
        }

        client->addSpeakerManagerObserver(userInterfaceManager);
        client->addNotificationsObserver(userInterfaceManager);
        client->addTemplateRuntimeObserver(m_guiManager);
        client->addAlexaPresentationObserver(m_guiManager);
        client->addAlexaDialogStateObserver(m_guiManager);
        client->addAudioPlayerObserver(m_guiManager);

        client->getRegistrationManager()->addObserver(m_guiClient);

        m_guiManager->setClient(client);
        m_guiClient->setGUIManager(m_guiManager);
        authDelegate->addAuthObserver(m_guiClient);

        m_capabilitiesDelegate->addCapabilitiesObserver(m_guiClient);
        m_capabilitiesDelegate->addCapabilitiesObserver(client);

        // START
        std::string endpoint;
        config.getString(ENDPOINT_KEY, &endpoint);
        client->connect(m_capabilitiesDelegate, endpoint);

        return true;
    }

    bool SmartScreen::InitSDKLogs(const string& logLevel)
    {
        bool status = true;
        std::shared_ptr<avsCommon::utils::logger::Logger> thunderLogger = avsCommon::utils::logger::getThunderLogger();
        avsCommon::utils::logger::Level logLevelValue = avsCommon::utils::logger::Level::UNKNOWN;
        string logLevelUpper(logLevel);

        std::transform(logLevelUpper.begin(), logLevelUpper.end(), logLevelUpper.begin(), [](unsigned char c) { return std::toupper(c); });
        if (logLevelUpper.empty() == false) {
            logLevelValue = avsCommon::utils::logger::convertNameToLevel(logLevelUpper);
            if (avsCommon::utils::logger::Level::UNKNOWN == logLevelValue) {
                TRACE(AVSClient, (_T("Unknown log level")));
                status = false;
            }
        } else {
            status = false;
        }

        if (status == true) {
            TRACE(AVSClient, (_T("Running app with log level: %s"), avsCommon::utils::logger::convertLevelToName(logLevelValue).c_str()));
            thunderLogger->setLevel(logLevelValue);
            avsCommon::utils::logger::LoggerSinkManager::instance().initialize(thunderLogger);
        }

        if (status == true) {
            apl::LoggerFactory::instance().initialize(std::make_shared<alexaSmartScreenSDK::sampleApp::AplCoreEngineSDKLogBridge>(alexaSmartScreenSDK::sampleApp::AplCoreEngineSDKLogBridge()));
        }

        return status;
    }

    bool SmartScreen::JsonConfigToStream(std::vector<std::shared_ptr<std::istream>>& streams, const std::string& configFile)
    {
        if (configFile.empty()) {
            TRACE(AVSClient, (_T("Config filename is empty!")));
            return false;
        }

        auto configStream = std::shared_ptr<std::ifstream>(new std::ifstream(configFile));
        if (!configStream->good()) {
            TRACE(AVSClient, (_T("Failed to read config file %s"), configFile.c_str()));
            return false;
        }

        streams.push_back(configStream);
        return true;
    }

    bool SmartScreen::Deinitialize()
    {
        TRACE(AVSClient, (_T("Deinitialize()")));

        return true;
    }

    Exchange::IAVSController* SmartScreen::Controller()
    {
        // Smart screen is controlled by websocket API and Controller is not needed
        return nullptr;
    }

    void SmartScreen::StateChange(PluginHost::IShell* audiosource)
    {
        if (m_thunderVoiceHandler) {
            m_thunderVoiceHandler->stateChange(audiosource);
        }
    }
}
}
