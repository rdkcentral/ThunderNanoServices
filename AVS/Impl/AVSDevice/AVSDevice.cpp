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

#include "AVSDevice.h"

#if defined(KWD_PRYON)
#include "PryonKeywordDetector.h"
#endif
#include "ThunderLogger.h"

#include <acsdkAudioInputStream/AudioInputStreamFactory.h>
#include <acsdkAlerts/Storage/SQLiteAlertStorage.h>
#include <acsdkAudioInputStream/CompatibleAudioFormat.h>
#include <acsdkEqualizerImplementations/MiscDBEqualizerStorage.h>
#include <acsdkEqualizerImplementations/SDKConfigEqualizerConfiguration.h>
#include <acsdkNotifications/SQLiteNotificationsStorage.h>
#include <Audio/AudioFactory.h>
#include <ACL/Transport/HTTP2TransportFactory.h>
#include <ACL/Transport/PostConnectSequencerFactory.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/AVS/Initialization/InitializationParametersBuilder.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPut.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>
#include <AVSCommon/Utils/Logger/ConsoleLogger.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <AVSGatewayManager/AVSGatewayManager.h>
#include <AVSGatewayManager/Storage/AVSGatewayManagerStorage.h>
#include <AVS/SampleApp/CaptionPresenter.h>
#include <AVS/SampleApp/ExternalCapabilitiesBuilder.h>
#if defined(KWD_PRYON)
#include <AVS/SampleApp/KeywordObserver.h>
#endif
#include <AVS/SampleApp/LocaleAssetsManager.h>
#include <AVS/SampleApp/PortAudioMicrophoneWrapper.h>
#include <AVS/SampleApp/SampleApplicationComponent.h>
#include <AVS/SampleApp/SampleEqualizerModeController.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <ContextManager/ContextManager.h>
#include <CapabilitiesDelegate/Storage/SQLiteCapabilitiesDelegateStorage.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>
#include <SynchronizeStateSender/SynchronizeStateSenderFactory.h>

#include <cctype>
#include <fstream>

namespace Thunder {
namespace Plugin {

    SERVICE_REGISTRATION(AVSDevice, 1, 0)

    using namespace alexaClientSDK;

    static const unsigned int AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT = 2;

    // Alexa Client Config keys
    static const std::string SAMPLE_APP_CONFIG_KEY("sampleApp");
    static const std::string FIRMWARE_VERSION_KEY("firmwareVersion");
    static const std::string ENDPOINT_KEY("endpoint");
    static const std::string DISPLAY_CARD_KEY("displayCardsSupported");
    static const std::string AUDIO_MEDIAPLAYER_POOL_SIZE_KEY("audioMediaPlayerPoolSize");

    // Share Data stream Configuraiton
    static const size_t MAX_READERS = 10;
    static const size_t WORD_SIZE = 2;
    static const unsigned int SAMPLE_RATE_HZ = 16000;
    static const unsigned int NUM_CHANNELS = 1;
    static const std::chrono::seconds AMOUNT_OF_AUDIO_DATA_IN_BUFFER = std::chrono::seconds(15);
    static const size_t BUFFER_SIZE_IN_SAMPLES = (SAMPLE_RATE_HZ)*AMOUNT_OF_AUDIO_DATA_IN_BUFFER.count();

    // Thunder voice handler
    static constexpr const char* PORTAUDIO_CALLSIGN("PORTAUDIO");

    bool AVSDevice::Initialize(PluginHost::IShell* service, const string& configuration)
    {
        TRACE(AVSClient, (_T("Initializing AVSDevice...")));

        Config config;
        bool status = true;

        ASSERT(service != nullptr);

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

        auto configJsonStreams = std::make_shared<std::vector<std::shared_ptr<std::istream>>>();
        if ((status == true) && (JsonConfigToStream(configJsonStreams, alexaClientConfig) == false)) {
            TRACE(AVSClient, (_T("Failed to load alexaClientConfig")));
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

        if (status == true) {
            status = Init(service, audiosource, enableKWD, pathToInputFolder, configJsonStreams);
        }

        return status;
    }

    bool AVSDevice::Init(PluginHost::IShell* service, const std::string& audiosource, const bool enableKWD, const std::string& pathToInputFolder VARIABLE_IS_NOT_USED, const std::shared_ptr<std::vector<std::shared_ptr<std::istream>>>& configJsonStreams)
    {
        auto builder = avsCommon::avs::initialization::InitializationParametersBuilder::create();
        if (!builder) {
            TRACE(AVSClient, (_T("createInitializeParamsFailed reason nullBuilder")));
            return false;
        }

        builder->withJsonStreams(configJsonStreams);

        auto initParams = builder->build();
        if (!initParams) {
            TRACE(AVSClient,(_T("Failed to get initParams")));
            return false;
        }

        acsdkSampleApplication::SampleApplicationComponent sampleAppComponent =
            acsdkSampleApplication::getComponent(std::move(initParams), m_shutdownRequiredList);

        auto manufactory = acsdkManufactory::Manufactory<
            std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>,
            std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>,
            std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
            std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>,
            std::shared_ptr<avsCommon::utils::DeviceInfo>,
            std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
            std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>,
            std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
            std::shared_ptr<acsdkCryptoInterfaces::CryptoFactoryInterface>,
            std::shared_ptr<acsdkCryptoInterfaces::KeyStoreInterface>,
            std::shared_ptr<sampleApp::UIManager>>::create(sampleAppComponent);

        auto metricRecorder = manufactory->get<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>();
        m_sdkInit = manufactory->get<std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>>();
        if (!m_sdkInit) {
            TRACE(AVSClient,(_T("Failed to get SDKInit!")));
            return false;
        }

        auto configPtr = manufactory->get<std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>>();
        if (!configPtr) {
            TRACE(AVSClient, (_T("Failed to create config")));
            return false;
        }
        auto& config = *configPtr;
        auto sampleAppConfig = config[SAMPLE_APP_CONFIG_KEY];

        auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();

        std::shared_ptr<storage::sqliteStorage::SQLiteMiscStorage> miscStorage = storage::sqliteStorage::SQLiteMiscStorage::create(config);
        if (!miscStorage) {
            TRACE(AVSClient, (_T("Failed to create deviceSettingsStorage")));
            return false;
        }

        auto customerDataManager = manufactory->get<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>();
        if (!customerDataManager) {
            TRACE(AVSClient, (_T("Failed to create customerDataManager")));
            return false;
        }

        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate;

#ifdef AUTH_MANAGER
        m_authManager = acsdkAuthorization::AuthorizationManager::create(miscStorage, customerDataManager);
        if (!m_authManager) {
            TRACE(AVSClient, (_T("Failed to create AuthorizationManager!")));
            return false;
        }

        m_shutdownRequiredList.push_back(m_authManager);
        authDelegate = m_authManager;
#else
        /*
         * Creating the AuthDelegate - this component takes care of LWA and authorization of the client.
         */
        authDelegate = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>>();
#endif

        if (!authDelegate) {
            TRACE(AVSClient, (_T("Creation of AuthDelegate failed!")));
            return false;
        }

        auto speakerMediaInterfaces =
            createApplicationMediaPlayer(httpContentFetcherFactory, false, "SpeakMediaPlayer");
        if (!speakerMediaInterfaces) {
            TRACE(AVSClient, (_T("Failed to create SpeakMediaPlayer")));
            return false;
        }
        m_speakMediaPlayer = speakerMediaInterfaces->mediaPlayer;

        int poolSize = AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT;
        sampleAppConfig.getInt(AUDIO_MEDIAPLAYER_POOL_SIZE_KEY, &poolSize, AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT);
        std::vector<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>> audioSpeakers;

        for (int index = 0; index < poolSize; index++) {
            std::shared_ptr<MediaPlayerInterface> mediaPlayer;
            std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker;
            std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface> equalizer;

            auto audioMediaInterfaces =
                createApplicationMediaPlayer(httpContentFetcherFactory, false, "AudioMediaPlayer");
            if (!audioMediaInterfaces) {
                TRACE(AVSClient, (_T("Failed to create AudioMediaPlayer")));
                return false;
            }
            m_audioMediaPlayerPool.push_back(audioMediaInterfaces->mediaPlayer);
            audioSpeakers.push_back(audioMediaInterfaces->speaker);
        }

        avsCommon::utils::Optional<avsCommon::utils::mediaPlayer::Fingerprint> fingerprint =
        (*(m_audioMediaPlayerPool.begin()))->getFingerprint();
        auto audioMediaPlayerFactory = std::unique_ptr<mediaPlayer::PooledMediaPlayerFactory>();
        if (fingerprint.hasValue()) {
            audioMediaPlayerFactory =
                mediaPlayer::PooledMediaPlayerFactory::create(m_audioMediaPlayerPool, fingerprint.value());
        } else {
            audioMediaPlayerFactory = mediaPlayer::PooledMediaPlayerFactory::create(m_audioMediaPlayerPool);
        }
        if (!audioMediaPlayerFactory) {
            TRACE(AVSClient, (_T("Failed to create media player factory for content!")));
            return false;
        }

        auto notificationMediaInterfaces =
            createApplicationMediaPlayer(httpContentFetcherFactory, false, "NotificationsMediaPlayer");
        if (!notificationMediaInterfaces) {
            TRACE(AVSClient, (_T("Failed to create NotificationsMediaPlayer")));
            return false;
        }
        m_notificationsMediaPlayer = notificationMediaInterfaces->mediaPlayer;

        auto bluetoothMediaInterfaces =
            createApplicationMediaPlayer(httpContentFetcherFactory, false, "BluetoothMediaPlayer");
        if (!bluetoothMediaInterfaces) {
            TRACE(AVSClient, (_T("Failed to create BluetoothMediaPlayer")));
            return false;
        }
        m_bluetoothMediaPlayer = bluetoothMediaInterfaces->mediaPlayer;

        auto ringtoneMediaInterfaces =
            createApplicationMediaPlayer(httpContentFetcherFactory, false, "RingtoneMediaPlayer");
        if (!ringtoneMediaInterfaces) {
            TRACE(AVSClient, (_T("Failed to create RingtoneMediaPlayer")));
            return false;
        }
        m_ringtoneMediaPlayer = ringtoneMediaInterfaces->mediaPlayer;

        auto alertsMediaInterfaces =
            createApplicationMediaPlayer(httpContentFetcherFactory, false, "AlertsMediaPlayer");
        if (!alertsMediaInterfaces) {
            TRACE(AVSClient, (_T("Failed to create AlertsMediaPlayer")));
            return false;
        }
        m_alertsMediaPlayer = alertsMediaInterfaces->mediaPlayer;

        auto systemSoundMediaInterfaces =
            createApplicationMediaPlayer(httpContentFetcherFactory, false, "SystemSoundMediaPlayer");
        if (!systemSoundMediaInterfaces) {
            TRACE(AVSClient, (_T("Failed to create SystemSoundMediaPlayer")));
            return false;
        }
        m_systemSoundMediaPlayer = systemSoundMediaInterfaces->mediaPlayer;

        auto audioFactory = std::make_shared<alexaClientSDK::applicationUtilities::resources::audio::AudioFactory>();
        if (!audioFactory) {
            TRACE(AVSClient, (_T("Failed to create audioFactory")));
            return false;
        }

        // storage

        auto alertStorage = alexaClientSDK::acsdkAlerts::storage::SQLiteAlertStorage::create(config, audioFactory->alerts(), metricRecorder);
        if (!alertStorage) {
            TRACE(AVSClient, (_T("Failed to create alertStorage")));
            return false;
        }

        auto messageStorage = alexaClientSDK::certifiedSender::SQLiteMessageStorage::create(config);
        if (!messageStorage) {
            TRACE(AVSClient, (_T("Failed to create messageStorage")));
            return false;
        }

        auto notificationsStorage = alexaClientSDK::acsdkNotifications::SQLiteNotificationsStorage::create(config);
        if (!notificationsStorage) {
            TRACE(AVSClient, (_T("Failed to create notificationsStorage")));
            return false;
        }

        auto deviceSettingsStorage = alexaClientSDK::settings::storage::SQLiteDeviceSettingStorage::create(config);
        if (!deviceSettingsStorage) {
            TRACE(AVSClient, (_T("Failed to create deviceSettingsStorage")));
            return false;
        }

        /*
         * Create sample locale asset manager.
         */
        auto localeAssetsManager = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>>();
        if (!localeAssetsManager) {
            TRACE(AVSClient, (_T("Failed to create localeAssetsManager")));
            return false;
        }

        /*
         * Creating the UI component that observes various components and prints to the console accordingly.
         */
        auto userInterfaceManager = manufactory->get<std::shared_ptr<alexaClientSDK::sampleApp::UIManager>>();
        if (!userInterfaceManager) {
            TRACE(AVSClient, (_T("Failed to create userInterfaceManager")));
            return false;
        }

        /*
         * Create the presentation layer for the captions.
         */
        auto captionPresenter = std::make_shared<alexaClientSDK::sampleApp::CaptionPresenter>();

        std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo = avsCommon::utils::DeviceInfo::create(config);
        if (!deviceInfo) {
            TRACE(AVSClient, (_T("Failed to create deviceInfo")));
            return false;
        }

        /*
         * Supply a SALT for UUID generation, this should be as unique to each individual device as possible
         */
        alexaClientSDK::avsCommon::utils::uuidGeneration::setSalt(
        deviceInfo->getClientId() + deviceInfo->getDeviceSerialNumber());

        /*
         * Creating the CapabilitiesDelegate - This component provides the client with the ability to send messages to the
         * Capabilities API.
         */
        auto capabilitiesDelegateStorage =
            alexaClientSDK::capabilitiesDelegate::storage::SQLiteCapabilitiesDelegateStorage::create(config);

        m_capabilitiesDelegate = alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate::create(
            authDelegate, std::move(capabilitiesDelegateStorage), customerDataManager);
        if (!m_capabilitiesDelegate) {
            TRACE(AVSClient, (_T("Failed to create m_capabilitiesDelegate")));
            return false;
        }

        m_shutdownRequiredList.push_back(m_capabilitiesDelegate);
        authDelegate->addAuthObserver(userInterfaceManager);
        m_capabilitiesDelegate->addCapabilitiesObserver(userInterfaceManager);

        int firmwareVersion = static_cast<int>(avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION);
        sampleAppConfig.getInt(FIRMWARE_VERSION_KEY, &firmwareVersion, firmwareVersion);

        /*
         * Check to see if displayCards is supported on the device. The default is supported unless specified otherwise in
         * the configuration.
         */
        bool displayCardsSupported;
        sampleAppConfig.getBool(DISPLAY_CARD_KEY, &displayCardsSupported, true);


        auto internetConnectionMonitor = avsCommon::utils::network::InternetConnectionMonitor::create(httpContentFetcherFactory);
        if (!internetConnectionMonitor) {
            TRACE(AVSClient, (_T("Failed to create internetConnectionMonitor")));
            return false;
        }

        // Context
        auto contextManager = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>();
        if (!contextManager) {
            TRACE(AVSClient, (_T("Failed to create contextManager")));
            return false;
        }

        auto avsGatewayManagerStorage = avsGatewayManager::storage::AVSGatewayManagerStorage::create(miscStorage);
        if (!avsGatewayManagerStorage) {
            TRACE(AVSClient, (_T("Creation of AVSGatewayManagerStorage failed")));
            return false;
        }
        auto avsGatewayManager = avsGatewayManager::AVSGatewayManager::create(
        std::move(avsGatewayManagerStorage), customerDataManager, config, authDelegate);
        if (!avsGatewayManager) {
            TRACE(AVSClient, (_T("Creation of AVSGatewayManager failed")));
            return false;
        }

        auto synchronizeStateSenderFactory = synchronizeStateSender::SynchronizeStateSenderFactory::create(contextManager);
        if (!synchronizeStateSenderFactory) {
            TRACE(AVSClient, (_T("Creation of SynchronizeStateSenderFactory failed")));
            return false;
        }

        std::vector<std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationProviderInterface>> providers;
        providers.push_back(synchronizeStateSenderFactory);
        providers.push_back(avsGatewayManager);
        providers.push_back(m_capabilitiesDelegate);

        /*
         * Create a factory for creating objects that handle tasks that need to be performed right after establishing
         * a connection to AVS.
         */
        auto postConnectSequencerFactory = acl::PostConnectSequencerFactory::create(providers);

        /*
         * Create a factory to create objects that establish a connection with AVS.
         */
        auto transportFactory = std::make_shared<acl::HTTP2TransportFactory>(
            std::make_shared<avsCommon::utils::libcurlUtils::LibcurlHTTP2ConnectionFactory>(),
            postConnectSequencerFactory,
            nullptr,
            nullptr);
        if (!transportFactory) {
            TRACE(AVSClient, (_T("Failed to create transportFactory")));
            return false;
        }

        /*
         * Creating the buffer (Shared Data Stream) that will hold user audio data. This is the main input into the SDK.
         */
        std::shared_ptr<alexaClientSDK::avsCommon::utils::AudioFormat> compatibleAudioFormat =
            alexaClientSDK::acsdkAudioInputStream::CompatibleAudioFormat::getCompatibleAudioFormat();

        std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream =
            alexaClientSDK::acsdkAudioInputStream::AudioInputStreamFactory::createAudioInputStream(
                compatibleAudioFormat, WORD_SIZE, MAX_READERS, AMOUNT_OF_AUDIO_DATA_IN_BUFFER);

        if (!sharedDataStream) {
            TRACE(AVSClient,(_T("Failed to create shared data stream!")));
            return false;
        }

        /*
         * Creating each of the audio providers. An audio provider is a simple package of data consisting of the stream
         * of audio data, as well as metadata about the stream. For each of the three audio providers created here, the same
         * stream is used since this sample application will only have one microphone.
         */

        // Creating tap to talk audio provider
        alexaClientSDK::capabilityAgents::aip::AudioProvider tapToTalkAudioProvider =
            alexaClientSDK::capabilityAgents::aip::AudioProvider::AudioProvider::TapAudioProvider(
                sharedDataStream, *compatibleAudioFormat);

        // Creating hold to talk audio provider
        alexaClientSDK::capabilityAgents::aip::AudioProvider holdToTalkAudioProvider =
            alexaClientSDK::capabilityAgents::aip::AudioProvider::AudioProvider::HoldAudioProvider(
                sharedDataStream, *compatibleAudioFormat);


        // MAIN CLIENT
        std::shared_ptr<alexaClientSDK::defaultClient::DefaultClient> client = alexaClientSDK::defaultClient::DefaultClient::create(
            deviceInfo,
            customerDataManager,
            m_externalMusicProviderMediaPlayersMap,
            m_externalMusicProviderSpeakersMap,
            m_adapterToCreateFuncMap,
            m_speakMediaPlayer,
            std::move(audioMediaPlayerFactory),
            m_alertsMediaPlayer,
            m_notificationsMediaPlayer,
            m_bluetoothMediaPlayer,
            m_ringtoneMediaPlayer,
            m_systemSoundMediaPlayer,
            speakerMediaInterfaces->speaker,
            audioSpeakers,
            alertsMediaInterfaces->speaker,
            notificationMediaInterfaces->speaker,
            bluetoothMediaInterfaces->speaker,
            ringtoneMediaInterfaces->speaker,
            systemSoundMediaInterfaces->speaker,
            {},
            nullptr,
            audioFactory,
            authDelegate,
            std::move(alertStorage),
            std::move(messageStorage),
            std::move(notificationsStorage),
            std::move(deviceSettingsStorage),
            nullptr,
            miscStorage,
            { userInterfaceManager },
            { userInterfaceManager },
            std::move(internetConnectionMonitor),
            displayCardsSupported,
            m_capabilitiesDelegate,
            contextManager,
            transportFactory,
            avsGatewayManager,
            localeAssetsManager,
            {},
            nullptr,
            firmwareVersion,
            true,
            nullptr,
            nullptr,
            metricRecorder,
            nullptr,
            nullptr,
            std::make_shared<alexaClientSDK::sampleApp::ExternalCapabilitiesBuilder>(deviceInfo),
            std::make_shared<alexaClientSDK::capabilityAgents::speakerManager::DefaultChannelVolumeFactory>(),
            true,
            std::make_shared<alexaClientSDK::acl::MessageRouterFactory>(),
            nullptr,
            tapToTalkAudioProvider);

        if (!client) {
            TRACE(AVSClient, (_T("Failed to create default SDK client")));
            return false;
        }

        client->addSpeakerManagerObserver(userInterfaceManager);
        client->addNotificationsObserver(userInterfaceManager);
        client->addBluetoothDeviceObserver(userInterfaceManager);

        m_shutdownManager = client->getShutdownManager();
        if (!m_shutdownManager) {
            TRACE(AVSClient,(_T("Failed to get ShutdownManager!")));
            return false;
        }

        userInterfaceManager->configureSettingsNotifications(client->getSettingsManager());

        /*
         * Add GUI Renderer as an observer if display cards are supported.
         */
        if (displayCardsSupported) {
            m_guiRenderer = std::make_shared<sampleApp::GuiRenderer>();
            client->addTemplateRuntimeObserver(m_guiRenderer);
        }

        // Audio input
        std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> aspInput = nullptr;
        std::shared_ptr<InteractionHandler<alexaClientSDK::sampleApp::InteractionManager>> aspInputInteractionHandler = nullptr;
        if (audiosource == PORTAUDIO_CALLSIGN) {
#if defined(PORTAUDIO)
            aspInput = sampleApp::PortAudioMicrophoneWrapper::create(sharedDataStream);
#else
            TRACE(AVSClient, (_T("PortAudio support is not compiled in")));
            return false;
#endif
        } else {
            aspInputInteractionHandler = InteractionHandler<alexaClientSDK::sampleApp::InteractionManager>::Create();
            if (!aspInputInteractionHandler) {
                TRACE(AVSClient, (_T("Failed to create aspInputInteractionHandler")));
                return false;
            }

            m_thunderVoiceHandler = ThunderVoiceHandler<alexaClientSDK::sampleApp::InteractionManager>::create(sharedDataStream, service, audiosource, aspInputInteractionHandler, *compatibleAudioFormat);
            if (!m_thunderVoiceHandler) {
                TRACE(AVSClient, (_T("Failed to create m_thunderVoiceHandler")));
                return false;
            }

            aspInput = m_thunderVoiceHandler;
            if (aspInput) {
                aspInput->startStreamingMicrophoneData();
            }
        }

        if (!aspInput) {
            TRACE(AVSClient, (_T("Failed to create aspInput")));
            return false;
        }

        // Creating wake word audio provider, if necessary
        alexaClientSDK::capabilityAgents::aip::AudioProvider wakeWordAudioProvider = capabilityAgents::aip::AudioProvider::null();

#if defined(KWD_PRYON)
        if (enableKWD) {
            wakeWordAudioProvider = alexaClientSDK::capabilityAgents::aip::AudioProvider::WakeAudioProvider(
                   sharedDataStream, *compatibleAudioFormat);

            auto keywordObserver = std::make_shared<alexaClientSDK::sampleApp::KeywordObserver>(client, wakeWordAudioProvider);
            m_keywordDetector = PryonKeywordDetector::create(
                sharedDataStream,
                *compatibleAudioFormat,
                { keywordObserver },
                std::unordered_set<
                   std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>(),
                pathToInputFolder);
            if (!m_keywordDetector) {
                TRACE(AVSClient,(_T("Failed to create keyword detector!")));
                return false;
            }

            // This observer is notified any time a keyword is detected and notifies the DefaultClient to start recognizing.
//            auto keywordObserver = sampleApp::KeywordObserver::create(client, wakeWordAudioProvider, std::move(m_keywordDetector));

        }
#endif
        // If wake word is enabled, then creating the interaction manager with a wake word audio provider.
        m_interactionManager = std::make_shared<alexaClientSDK::sampleApp::InteractionManager>(
            client,
            aspInput,
            userInterfaceManager,
            holdToTalkAudioProvider,
            tapToTalkAudioProvider,
            m_guiRenderer,
            wakeWordAudioProvider,
            nullptr,
            nullptr);

        if (!m_interactionManager) {
            TRACE(AVSClient, (_T("Failed to create InteractionManager")));
            return false;
        }

        m_shutdownRequiredList.push_back(m_interactionManager);
        client->addAlexaDialogStateObserver(m_interactionManager);
        client->addCallStateObserver(m_interactionManager);

        if (audiosource != PORTAUDIO_CALLSIGN) {
            if (aspInputInteractionHandler) {
                // register interactions that ThunderVoiceHandler may initiate
                if (!aspInputInteractionHandler->Initialize(m_interactionManager)) {
                    TRACE(AVSClient, (_T("Failed to initialize aspInputInteractionHandle")));
                    return false;
                }
            }
        }

        // Thunder Input Manager
        m_thunderInputManager = ThunderInputManager::create(m_interactionManager);
        if (!m_thunderInputManager) {
            TRACE(AVSClient, (_T("Failed to create m_thunderInputManager")));
            return false;
        }

        authDelegate->addAuthObserver(m_thunderInputManager);
        client->addRegistrationObserver(m_thunderInputManager);
        m_capabilitiesDelegate->addCapabilitiesObserver(m_thunderInputManager);

        // START
        client->connect();

        return true;
    }

    bool AVSDevice::InitSDKLogs(const string& logLevel)
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

        return status;
    }

    bool AVSDevice::JsonConfigToStream(std::shared_ptr<std::vector<std::shared_ptr<std::istream>>>& streams, const std::string& configFile)
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

        streams->push_back(configStream);
        return true;
    }

    bool AVSDevice::Deinitialize()
    {
        TRACE(AVSClient, (_T("Deinitialize()")));
        m_sdkInit.reset();
        return true;
    }

    Exchange::IAVSController* AVSDevice::Controller()
    {
        if (m_thunderInputManager) {
            return m_thunderInputManager->Controller();
        } else {
            return nullptr;
        }
    }

    void AVSDevice::StateChange(PluginHost::IShell* audiosource)
    {
        if (m_thunderVoiceHandler) {
            m_thunderVoiceHandler->stateChange(audiosource);
        }
    }
}
}
