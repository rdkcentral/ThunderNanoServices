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

#include "ThunderInputManager.h"

namespace Thunder {
namespace Plugin {

    using namespace alexaClientSDK::avsCommon::sdkInterfaces;
    using namespace Exchange;

    std::unique_ptr<ThunderInputManager> ThunderInputManager::create(std::shared_ptr<alexaClientSDK::sampleApp::InteractionManager> interactionManager)
    {
        if (!interactionManager) {
            TRACE_GLOBAL(AVSClient, (_T("Invalid InteractionManager passed to ThunderInputManager")));
            return nullptr;
        }
        return std::unique_ptr<ThunderInputManager>(new ThunderInputManager(interactionManager));
    }

    ThunderInputManager::ThunderInputManager(std::shared_ptr<alexaClientSDK::sampleApp::InteractionManager> interactionManager)
        : m_limitedInteraction{ false }
        , m_interactionManager{ interactionManager }
        , m_controller{ Core::ProxyType<AVSController>::Create(this) }
    {
    }

    void ThunderInputManager::onDialogUXStateChanged(DialogUXState newState)
    {
        if (m_controller.IsValid()) {
            m_controller->NotifyDialogUXStateChanged(newState);
        }
    }

    ThunderInputManager::AVSController::AVSController(ThunderInputManager* parent)
        : m_parent(*parent)
        , m_notifications()
    {
    }

    ThunderInputManager::AVSController::~AVSController()
    {
        for (auto* notification : m_notifications) {
            notification->Release();
        }
        m_notifications.clear();
    }

    void ThunderInputManager::AVSController::NotifyDialogUXStateChanged(DialogUXState newState)
    {
        IAVSController::INotification::dialoguestate dialoguestate;
        bool isStateHandled = true;

        switch (newState) {
        case DialogUXState::IDLE:
            dialoguestate = IAVSController::INotification::IDLE;
            break;
        case DialogUXState::LISTENING:
            dialoguestate = IAVSController::INotification::LISTENING;
            break;
        case DialogUXState::EXPECTING:
            dialoguestate = IAVSController::INotification::EXPECTING;
            break;
        case DialogUXState::THINKING:
            dialoguestate = IAVSController::INotification::THINKING;
            break;
        case DialogUXState::SPEAKING:
            dialoguestate = IAVSController::INotification::SPEAKING;
            break;
        case DialogUXState::FINISHED:
            TRACE(AVSClient, (_T("Unmapped Dialog state (%d)"), newState));
            isStateHandled = false;
            break;
        default:
            TRACE(AVSClient, (_T("Unknown State (%d)"), newState));
            isStateHandled = false;
        }

        if (isStateHandled == true) {
            for (auto* notification : m_notifications) {
                notification->DialogueStateChange(dialoguestate);
            }
        }
    }

    void ThunderInputManager::AVSController::Register(INotification* notification)
    {
        ASSERT(notification != nullptr);
        auto item = std::find(m_notifications.begin(), m_notifications.end(), notification);
        ASSERT(item == m_notifications.end());
        if (item == m_notifications.end()) {
            notification->AddRef();
            m_notifications.push_back(notification);
        }
    }

    void ThunderInputManager::AVSController::Unregister(const INotification* notification)
    {
        ASSERT(notification != nullptr);

        auto item = std::find(m_notifications.begin(), m_notifications.end(), notification);
        ASSERT(item != m_notifications.end());
        if (item != m_notifications.end()) {
            m_notifications.erase(item);
            (*item)->Release();
        }
    }

    uint32_t ThunderInputManager::AVSController::Mute(const bool mute)
    {
        if (m_parent.m_limitedInteraction) {
            return static_cast<uint32_t>(Core::ERROR_GENERAL);
        }

        if (!m_parent.m_interactionManager) {
            return static_cast<uint32_t>(Core::ERROR_UNAVAILABLE);
        }

        m_parent.m_interactionManager->setMute(ChannelVolumeInterface::Type::AVS_SPEAKER_VOLUME, mute);
        m_parent.m_interactionManager->setMute(ChannelVolumeInterface::Type::AVS_ALERTS_VOLUME, mute);

        return static_cast<uint32_t>(Core::ERROR_NONE);
    }

    uint32_t ThunderInputManager::AVSController::Record(const bool start VARIABLE_IS_NOT_USED)
    {
        if (m_parent.m_limitedInteraction) {
            return static_cast<uint32_t>(Core::ERROR_GENERAL);
        }

        m_parent.m_interactionManager->tap();

        return static_cast<uint32_t>(Core::ERROR_NONE);
    }

    Exchange::IAVSController* ThunderInputManager::Controller()
    {
        return (&(*m_controller));
    }

    void ThunderInputManager::onLogout()
    {
        m_limitedInteraction = true;
    }

    void ThunderInputManager::onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError VARIABLE_IS_NOT_USED)
    {
        m_limitedInteraction = m_limitedInteraction || (newState == AuthObserverInterface::State::UNRECOVERABLE_ERROR);
    }

    void ThunderInputManager::onCapabilitiesStateChange(
        CapabilitiesDelegateObserverInterface::State newState,
        CapabilitiesDelegateObserverInterface::Error newError VARIABLE_IS_NOT_USED,
        const std::vector<std::string>& addedOrUpdatedEndpointIds VARIABLE_IS_NOT_USED,
        const std::vector<std::string>& deletedEndpointIds VARIABLE_IS_NOT_USED)

    {
        m_limitedInteraction = m_limitedInteraction || (newState == CapabilitiesDelegateObserverInterface::State::FATAL_ERROR);
    }

} // namespace Plugin
} // namespace Thunder
