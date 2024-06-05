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

#include <acsdkAlerts/Storage/SQLiteAlertStorage.h>
#include <acsdkAudioInputStream/AudioInputStreamFactory.h>
#include <acsdkAudioInputStream/CompatibleAudioFormat.h>
#include <acsdkBluetooth/BasicDeviceConnectionRule.h>
#include <acsdkEqualizerImplementations/MiscDBEqualizerStorage.h>
#include <acsdkEqualizerImplementations/SDKConfigEqualizerConfiguration.h>
#include <acsdkKWDProvider/KWDProvider/KeywordDetectorProvider.h>
#include <acsdkNotifications/SQLiteNotificationsStorage.h>

#include <ACL/Transport/HTTP2TransportFactory.h>
#include <ACL/Transport/PostConnectSequencerFactory.h>
#include <APLClient/AplCoreEngineLogBridge.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/AVS/Initialization/InitializationParametersBuilder.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPut.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlHTTP2ConnectionFactory.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <AVSGatewayManager/AVSGatewayManager.h>
#include <AVSGatewayManager/Storage/AVSGatewayManagerStorage.h>
#include <Audio/AudioFactory.h>
#include <CapabilitiesDelegate/Storage/SQLiteCapabilitiesDelegateStorage.h>
#include <CBLAuthDelegate/CBLAuthDelegate.h>
#include <CBLAuthDelegate/SQLiteCBLAuthDelegateStorage.h>
#include <Communication/WebSocketServer.h>
#include <ContextManager/ContextManager.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>
#include <SQLiteStorage/SQLiteMiscStorage.h>
#include <SynchronizeStateSender/SynchronizeStateSenderFactory.h>

#include <AVS/SampleApp/GuiRenderer.h>
#include <AVSSDK/SampleApp/ExternalCapabilitiesBuilder.h>
#include <AVSSDK/SampleApp/JsonUIManager.h>
#include <AVSSDK/SampleApp/KeywordObserver.h>
#include <AVSSDK/SampleApp/LocaleAssetsManager.h>
#ifdef PORTAUDIO
#include <AVSSDK/SampleApp/PortAudioMicrophoneWrapper.h>
#endif
#include <AVSSDK/SampleApp/SampleApplicationComponent.h>
#include <AVSSDK/SampleApp/SampleEqualizerModeController.h>
#include <AVSSDK/SampleApp/SmartScreenCaptionPresenter.h>

#if defined(KWD_PRYON)
#include "PryonKeywordDetector.h"
#endif
#include "ThunderLogger.h"
#include "ThunderVoiceHandler.h"
#include "TraceCategories.h"

#include <cctype>
#include <fstream>

namespace Thunder {
namespace Plugin {

    SERVICE_REGISTRATION(SmartScreen, 1, 0)

    using namespace alexaClientSDK;

    // Alexa Client Config keys
    static const std::string SAMPLE_APP_CONFIG_KEY("sampleApp");
    static const std::string FIRMWARE_VERSION_KEY("firmwareVersion");
    static const std::string ENDPOINT_KEY("endpoint");
    static const std::string AUDIO_MEDIAPLAYER_POOL_SIZE_KEY("audioMediaPlayerPoolSize");
    static const std::string CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS_KEY("contentCacheReusePeriodInSeconds");
    static const std::string CONTENT_CACHE_MAX_SIZE_KEY("contentCacheMaxSize");
    static const std::string DEFAULT_CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS("600");
    static const std::string DEFAULT_CONTENT_CACHE_MAX_SIZE("50");
    static const std::string MAX_NUMBER_OF_CONCURRENT_DOWNLOAD_CONFIGURATION_KEY = "maxNumberOfConcurrentDownloads";
    static const std::string MISC_STORAGE_APP_COMPONENT_NAME("SmartScreenSampleApp");
    static const std::string MISC_STORAGE_APL_TABLE_NAME("APL");
    static const std::string APL_MAX_VERSION_DB_KEY("APLMaxVersion");

    static const int DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD = 5;
    static const unsigned int AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT = 2;

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

        auto configJsonStreams = std::make_shared<std::vector<std::shared_ptr<std::istream>>>();
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
        if (status == true) {
            status = Init(service, audiosource, enableKWD, pathToInputFolder, configJsonStreams);
        }

        return status;
    }

    bool SmartScreen::Init(PluginHost::IShell* service, const std::string& audiosource, const bool enableKWD, const std::string& pathToInputFolder VARIABLE_IS_NOT_USED, const std::shared_ptr<std::vector<std::shared_ptr<std::istream>>>& configJsonStreams)
    {
        auto builder = avsCommon::avs::initialization::InitializationParametersBuilder::create();
        if (!builder) {
            TRACE(AVSClient, (_T("createInitializeParamsFailed reason: nullBuilder")));
            return false;
        }

        builder->withJsonStreams(configJsonStreams);
        auto initParams = builder->build();
        if (!initParams) {
            TRACE(AVSClient,(_T("Failed to get initParams")));
            return false;
        }

        auto sampleAppComponent = alexaSmartScreenSDK::sampleApp::getComponent(std::move(initParams), m_shutdownRequiredList);
        auto manufactory = acsdkManufactory::Manufactory<
            std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>,
            std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
            std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>,
            std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
            std::shared_ptr<avsCommon::utils::DeviceInfo>,
            std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
            std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>::create(sampleAppComponent);

        auto metricRecorder = manufactory->get<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>();

        auto configPtr = manufactory->get<std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>>();
        if (!configPtr) {
            TRACE(AVSClient,(_T("Failed to acquire the configuration")));
            return false;
        }
        auto& config = *configPtr;

        auto sampleAppConfig = config[SAMPLE_APP_CONFIG_KEY];

        // speakers and media players
        auto httpContentFetcherFactory = std::make_shared<avsCommon::utils::libcurlUtils::HTTPContentFetcherFactory>();

        // Creating the misc DB object to be used by various components.
        std::shared_ptr<alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage> miscStorage =
            alexaClientSDK::storage::sqliteStorage::SQLiteMiscStorage::create(config);

        if (!miscStorage->isOpened() && !miscStorage->open()) {
            TRACE(AVSClient,(_T("openStorage : Couldn't open misc database. Creating.")));

            if (!miscStorage->createDatabase()) {
                TRACE(AVSClient,(_T("openStorageFailed: Could not create misc database.")));
                return false;
            }
        }

        auto speakerMediaInterfaces = createApplicationMediaPlayer(httpContentFetcherFactory, false, "SpeakMediaPlayer");
        if (!speakerMediaInterfaces) {
            TRACE(AVSClient, (_T("Failed to create SpeakMediaPlayer")));
            return false;
        }
        m_speakMediaPlayer = speakerMediaInterfaces->mediaPlayer;


        int poolSize = AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT;
        sampleAppConfig.getInt(AUDIO_MEDIAPLAYER_POOL_SIZE_KEY, &poolSize, AUDIO_MEDIAPLAYER_POOL_SIZE_DEFAULT);
        std::vector<std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>> audioSpeakers;

        for (int index = 0; index < poolSize; index++) {
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> mediaPlayer;
            std::shared_ptr<avsCommon::sdkInterfaces::SpeakerInterface> speaker;
            std::shared_ptr<acsdkEqualizerInterfaces::EqualizerInterface> equalizer;

            auto audioMediaInterfaces =
                createApplicationMediaPlayer(httpContentFetcherFactory, false, "AudioMediaPlayer");
            if (!audioMediaInterfaces) {
                TRACE(AVSClient,(_T("Failed to create application media interfaces for audio!")));
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
            TRACE(AVSClient,(_T("Failed to create media player factory for content!")));
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


        auto alertsMediaInterfaces = createApplicationMediaPlayer(httpContentFetcherFactory, false, "AlertsMediaPlayer");
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
        auto alertStorage = alexaClientSDK::acsdkAlerts::storage::SQLiteAlertStorage::create(
                config, audioFactory->alerts(), metricRecorder);
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

        // Asset Manager
        auto localeAssetsManager = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>>();
        if (!localeAssetsManager) {
            TRACE(AVSClient, (_T("Failed to create localeAssetsManager")));
            return false;
        }

        // SmartScreen connection
        std::string websocketInterface;
        int websocketPortNumber = DEFAULT_WEBSOCKET_PORT;
        sampleAppConfig.getString(WEBSOCKET_INTERFACE_KEY, &websocketInterface, DEFAULT_WEBSOCKET_INTERFACE);
        sampleAppConfig.getInt(WEBSOCKET_PORT_KEY, &websocketPortNumber, DEFAULT_WEBSOCKET_PORT);

        // Create the websocket server that handles communications with websocket clients

#ifdef UWP_BUILD
        auto webSocketServer = std::make_shared<NullSocketServer>();
#else
        auto webSocketServer = std::make_shared<alexaSmartScreenSDK::communication::WebSocketServer>(websocketInterface, websocketPortNumber);
#endif  // UWP_BUILD

        if (!webSocketServer) {
            TRACE(AVSClient, (_T("Failed to create webSocketServer")));
            return false;
        }

        /*
         * Creating customerDataManager which will be used by the registrationManager and all classes that extend
         * CustomerDataHandler
         */
        auto customerDataManager = manufactory->get<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>();
        if (!customerDataManager) {
            TRACE(AVSClient,(_T("Failed to get CustomerDataManager!")));
            return false;
        }

        // GUI
        m_guiClient = alexaSmartScreenSDK::sampleApp::gui::GUIClient::create(webSocketServer, miscStorage, customerDataManager);
        if (!m_guiClient) {
            TRACE(AVSClient, (_T("Failed to create m_guiClient")));
            return false;
        }

        std::string cachePeriodInSeconds;
        std::string maxCacheSize;

        sampleAppConfig.getString(
            CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS_KEY,
            &cachePeriodInSeconds,
            DEFAULT_CONTENT_CACHE_REUSE_PERIOD_IN_SECONDS);
        sampleAppConfig.getString(CONTENT_CACHE_MAX_SIZE_KEY, &maxCacheSize, DEFAULT_CONTENT_CACHE_MAX_SIZE);

        auto contentDownloadManager = std::make_shared<alexaSmartScreenSDK::sampleApp::CachingDownloadManager>(
            httpContentFetcherFactory,
            std::stol(cachePeriodInSeconds),
            std::stol(maxCacheSize),
            miscStorage,
            customerDataManager);

        int maxNumberOfConcurrentDownloads = DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD;
        sampleAppConfig.getInt(
            MAX_NUMBER_OF_CONCURRENT_DOWNLOAD_CONFIGURATION_KEY,
            &maxNumberOfConcurrentDownloads,
            DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD);

        if (1 > maxNumberOfConcurrentDownloads) {
            maxNumberOfConcurrentDownloads = DEFAULT_MAX_NUMBER_OF_CONCURRENT_DOWNLOAD;
            TRACE(AVSClient,(_T("Invalid values for maxNumberOfConcurrentDownloads")));
        }
        auto parameters = alexaSmartScreenSDK::sampleApp::AplClientBridgeParameter{maxNumberOfConcurrentDownloads};
        m_aplClientBridge = alexaSmartScreenSDK::sampleApp::AplClientBridge::create(contentDownloadManager, m_guiClient, parameters);
        std::string APLVersion = m_aplClientBridge->getMaxSupportedAPLVersion();

        bool aplTableExists = false;
        if (!miscStorage->tableExists(MISC_STORAGE_APP_COMPONENT_NAME, MISC_STORAGE_APL_TABLE_NAME, &aplTableExists)) {
            TRACE(AVSClient,(_T("openStorageFailed: Could not get apl table information from misc database.")));
            return false;
        }

        if (!aplTableExists) {
            TRACE(AVSClient,(_T("openStorage apl table doesn't exist %d"), MISC_STORAGE_APL_TABLE_NAME));
            if (!miscStorage->createTable(
                MISC_STORAGE_APP_COMPONENT_NAME,
                MISC_STORAGE_APL_TABLE_NAME,
                avsCommon::sdkInterfaces::storage::MiscStorageInterface::KeyType::STRING_KEY,
                avsCommon::sdkInterfaces::storage::MiscStorageInterface::ValueType::STRING_VALUE)) {
                TRACE(AVSClient,(_T("openStorageFailed reason Could not create table, table: %d, component %d"),
                                   MISC_STORAGE_APL_TABLE_NAME, MISC_STORAGE_APP_COMPONENT_NAME));
                return false;
            }
        }

        std::string APLDBVersion;
        if (!miscStorage->get(
                                       MISC_STORAGE_APP_COMPONENT_NAME, MISC_STORAGE_APL_TABLE_NAME, APL_MAX_VERSION_DB_KEY, &APLDBVersion)) {
            TRACE(AVSClient,(_T("getAPLMaxVersionFromStorageFailed reason : storage failure")));
        }
        TRACE(AVSClient,(_T("APLDBVersion"), APLDBVersion));
        if (APLDBVersion.empty()) {
            TRACE(AVSClient,(_T("no APL version saved reason: couldn't find saved APLDBVersion")));
        }

        bool aplVersionChanged = false;
        if (APLVersion != APLDBVersion) {
            // Empty value does not constitute version change, and should not result in app reset.
            if (!APLDBVersion.empty()) {
                aplVersionChanged = true;
            }
            if (!miscStorage->put(
                    MISC_STORAGE_APP_COMPONENT_NAME, MISC_STORAGE_APL_TABLE_NAME, APL_MAX_VERSION_DB_KEY, APLVersion)) {
                TRACE(AVSClient,(_T("saveAPLMaxVersionInStorage : Could not set new value")));
            } else {
                TRACE(AVSClient,(_T("saveAPLMaxVersionInStorage : Succeeded")));
            }
        }

         m_guiClient->setAplClientBridge(m_aplClientBridge, aplVersionChanged);

        /*
         * Create the presentation layer for the captions.
         */
        auto captionPresenter = std::make_shared<alexaSmartScreenSDK::sampleApp::SmartScreenCaptionPresenter>(m_guiClient);

        /*
         * Creating the deviceInfo object
         */
        auto deviceInfo = manufactory->get<std::shared_ptr<avsCommon::utils::DeviceInfo>>();
        if (!deviceInfo) {
            TRACE(AVSClient,(_T("Creation of DeviceInfo failed!")));
            return false;
        }

        // Creating the UI component that observes various components and prints to the console accordingly.

        auto userInterfaceManager = std::make_shared<alexaSmartScreenSDK::sampleApp::JsonUIManager>(
            std::static_pointer_cast<alexaSmartScreenSDK::smartScreenSDKInterfaces::GUIClientInterface>(m_guiClient),
            deviceInfo);
        m_guiClient->setObserver(userInterfaceManager);
        /*
         * Supply a SALT for UUID generation, this should be as unique to each individual device as possible
         */
        alexaClientSDK::avsCommon::utils::uuidGeneration::setSalt(
            deviceInfo->getClientId() + deviceInfo->getDeviceSerialNumber());

        auto authDelegateStorage =
        authorization::cblAuthDelegate::SQLiteCBLAuthDelegateStorage::createCBLAuthDelegateStorageInterface(
            configPtr, nullptr, nullptr);

        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate =
            authorization::cblAuthDelegate::CBLAuthDelegate::create(
            config, customerDataManager, std::move(authDelegateStorage), userInterfaceManager, nullptr, deviceInfo);

        if (!authDelegate) {
            TRACE(AVSClient, (_T("Failed to create authDelegate")));
            return false;
        }
        /*
         * Creating the CapabilitiesDelegate - This component provides the client with the ability to send messages to the
         * Capabilities API.
         */
        auto capabilitiesDelegateStorage =
            alexaClientSDK::capabilitiesDelegate::storage::SQLiteCapabilitiesDelegateStorage::create(config);

        m_capabilitiesDelegate = alexaClientSDK::capabilitiesDelegate::CapabilitiesDelegate::create(
            authDelegate, std::move(capabilitiesDelegateStorage), customerDataManager);

        if (!m_capabilitiesDelegate) {
            TRACE(AVSClient,(_T("Creation of CapabilitiesDelegate failed!")));
            return false;
        }

        m_shutdownRequiredList.push_back(m_capabilitiesDelegate);
        authDelegate->addAuthObserver(userInterfaceManager);
        m_capabilitiesDelegate->addCapabilitiesObserver(userInterfaceManager);
        // INVALID_FIRMWARE_VERSION is passed to @c getInt() as a default in case FIRMWARE_VERSION_KEY is not found.
        int firmwareVersion = static_cast<int>(avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION);
        sampleAppConfig.getInt(FIRMWARE_VERSION_KEY, &firmwareVersion, firmwareVersion);

        /*
         * Creating the InternetConnectionMonitor that will notify observers of internet connection status changes.
         */
        auto internetConnectionMonitor =
            avsCommon::utils::network::InternetConnectionMonitor::create(httpContentFetcherFactory);
        if (!internetConnectionMonitor) {
            TRACE(AVSClient,(_T("Failed to create InternetConnectionMonitor")));
            return false;
        }

        /*
         * Creating the Context Manager - This component manages the context of each of the components to update to AVS.
         * It is required for each of the capability agents so that they may provide their state just before any event is
         * fired off.
         */
        auto contextManager = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>();
        if (!contextManager) {
            TRACE(AVSClient,(_T("Creation of ContextManager failed.")));
            return false;
        }

        auto avsGatewayManagerStorage = avsGatewayManager::storage::AVSGatewayManagerStorage::create(miscStorage);
        if (!avsGatewayManagerStorage) {
            TRACE(AVSClient,(_T("Creation of AVSGatewayManagerStorage failed")));
            return false;
        }
        auto avsGatewayManager = avsGatewayManager::AVSGatewayManager::create(
            std::move(avsGatewayManagerStorage), customerDataManager, config, authDelegate);
        if (!avsGatewayManager) {
            TRACE(AVSClient,(_T("Creation of AVSGatewayManager failed")));
            return false;
        }

        auto synchronizeStateSenderFactory = synchronizeStateSender::SynchronizeStateSenderFactory::create(contextManager);
        if (!synchronizeStateSenderFactory) {
            TRACE(AVSClient,(_T("Creation of SynchronizeStateSenderFactory failed")));
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

        std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::ProtocolTracerInterface> deviceProtocolTracer;
        /*
         * Create a factory to create objects that establish a connection with AVS.
         */
        auto transportFactory = std::make_shared<acl::HTTP2TransportFactory>(
            std::make_shared<avsCommon::utils::libcurlUtils::LibcurlHTTP2ConnectionFactory>(),
            postConnectSequencerFactory,
            nullptr,
            deviceProtocolTracer);

        /*
         * Create the BluetoothDeviceManager to communicate with the Bluetooth stack.
         */
        std::unique_ptr<avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceManagerInterface> bluetoothDeviceManager;

        /*
         * Create the connectionRules to communicate with the Bluetooth stack.
         */
        std::unordered_set<
           std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::bluetooth::BluetoothDeviceConnectionRuleInterface>>
           enabledConnectionRules;
        enabledConnectionRules.insert(alexaClientSDK::acsdkBluetooth::BasicDeviceConnectionRule::create());

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

        // Creating wake word audio provider, if necessary
        alexaClientSDK::capabilityAgents::aip::AudioProvider wakeWordAudioProvider(capabilityAgents::aip::AudioProvider::null());
#if defined(KWD_PRYON)
        if (enableKWD) {
            wakeWordAudioProvider =
                alexaClientSDK::capabilityAgents::aip::AudioProvider::WakeAudioProvider(
                    sharedDataStream,
                    *compatibleAudioFormat);
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

            m_thunderVoiceHandler = ThunderVoiceHandler<alexaSmartScreenSDK::sampleApp::gui::GUIManager>::create(sharedDataStream, service, audiosource, aspInputInteractionHandler, *compatibleAudioFormat);
            if (!m_thunderVoiceHandler) {
                TRACE(AVSClient, (_T("Failed to create m_thunderVoiceHandler")));
                return false;
            }
            aspInput = m_thunderVoiceHandler;
            aspInput->startStreamingMicrophoneData();
        }
        if (!aspInput) {
            TRACE(AVSClient, (_T("Failed to create aspInput")));
            return false;
        }

        // If wake word is not enabled, then creating the gui manager without a wake word audio provider.
        m_guiManager = alexaSmartScreenSDK::sampleApp::gui::GUIManager::create(
            m_guiClient,
            holdToTalkAudioProvider,
            tapToTalkAudioProvider,
            aspInput,
            wakeWordAudioProvider);
        /*
         * Creating the SmartScreenClient - this component serves as an out-of-box default object that instantiates and
         * "glues" together all the modules.
         */
        m_client = alexaSmartScreenSDK::smartScreenClient::SmartScreenClient::create(
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
            {userInterfaceManager},
            {userInterfaceManager},
            std::move(internetConnectionMonitor),
            m_capabilitiesDelegate,
            contextManager,
            transportFactory,
            avsGatewayManager,
            localeAssetsManager,
            enabledConnectionRules,
            /* systemTimezone*/ nullptr,
            firmwareVersion,
            true,
            nullptr,
            std::move(bluetoothDeviceManager),
            metricRecorder,
            nullptr,
            nullptr,
            std::make_shared<alexaSmartScreenSDK::sampleApp::ExternalCapabilitiesBuilder>(deviceInfo),
            std::make_shared<alexaClientSDK::capabilityAgents::speakerManager::DefaultChannelVolumeFactory>(),
            true,
            std::make_shared<alexaClientSDK::acl::MessageRouterFactory>(),
            nullptr,
            capabilityAgents::aip::AudioProvider::null(),
            m_guiManager,
            APLVersion);
        if (!m_client) {
            TRACE(AVSClient,(_T("Failed to create default SDK client!")));
            return false;
        }

        m_client->addSpeakerManagerObserver(userInterfaceManager);

        m_client->addNotificationsObserver(userInterfaceManager);

        m_client->addTemplateRuntimeObserver(m_guiManager);
        m_client->addAlexaPresentationObserver(m_guiManager);

        m_client->addAlexaDialogStateObserver(m_guiManager);
        m_client->addAudioPlayerObserver(m_guiManager);
        m_client->addAudioPlayerObserver(m_aplClientBridge);
        m_client->addExternalMediaPlayerObserver(m_aplClientBridge);
        m_client->addCallStateObserver(m_guiManager);
        m_client->addDtmfObserver(m_guiManager);
        m_client->addFocusManagersObserver(m_guiManager);
        m_client->addAudioInputProcessorObserver(m_guiManager);
        m_guiManager->setClient(m_client);
        m_guiClient->setGUIManager(m_guiManager);

#if defined(KWD_PRYON)
        if (enableKWD) {
            auto keywordObserver = std::make_shared<alexaSmartScreenSDK::sampleApp::KeywordObserver>(m_client, wakeWordAudioProvider);
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
        }
#endif

        authDelegate->addAuthObserver(m_guiClient);
        m_client->addRegistrationObserver(m_guiClient);
        m_capabilitiesDelegate->addCapabilitiesObserver(m_guiClient);

        m_guiManager->setDoNotDisturbSettingObserver(m_guiClient);
        m_guiManager->configureSettingsNotifications();

        if (!m_guiClient->start()) {
            return false;
        }

        m_client->connect();

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
            apl::LoggerFactory::instance().initialize(std::make_shared<APLClient::AplCoreEngineLogBridge>(APLClient::AplCoreEngineLogBridge(m_aplClientBridge)));
        }

        return status;
    }

    bool SmartScreen::JsonConfigToStream(std::shared_ptr<std::vector<std::shared_ptr<std::istream>>>& streams, const std::string& configFile)
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
