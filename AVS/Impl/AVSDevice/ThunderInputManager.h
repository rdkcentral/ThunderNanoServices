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

#include "TraceCategories.h"

#include <AVS/SampleApp/InteractionManager.h>
#include <interfaces/IAVSClient.h>

#include <atomic>

namespace Thunder {
namespace Plugin {

    /// Observes user input from the console and notifies the interaction manager of the user's intentions.
    class ThunderInputManager
        : public alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface,
          public alexaClientSDK::avsCommon::sdkInterfaces::CapabilitiesDelegateObserverInterface,
          public alexaClientSDK::registrationManager::RegistrationObserverInterface,
          public alexaClientSDK::avsCommon::sdkInterfaces::DialogUXStateObserverInterface {
    public:
        static std::unique_ptr<ThunderInputManager> create(std::shared_ptr<alexaClientSDK::sampleApp::InteractionManager> interactionManager);

        class AVSController : public Exchange::IAVSController {
        public:
            AVSController(const AVSController&) = delete;
            AVSController& operator=(const AVSController&) = delete;
            AVSController(ThunderInputManager* parent);
            ~AVSController();

            void NotifyDialogUXStateChanged(DialogUXState newState);

            // Exchange::IAVSController methods
            void Register(INotification* sink) override;
            void Unregister(const INotification* sink) override;
            uint32_t Mute(const bool mute) override;
            uint32_t Record(const bool start) override;

            BEGIN_INTERFACE_MAP(AVSController)
            INTERFACE_ENTRY(Exchange::IAVSController)
            END_INTERFACE_MAP

        private:
            ThunderInputManager& m_parent;
            std::list<Exchange::IAVSController::INotification*> m_notifications;
        };

        void onLogout() override;
        void onDialogUXStateChanged(DialogUXState newState) override;
        Exchange::IAVSController* Controller();

    private:
        ThunderInputManager(std::shared_ptr<alexaClientSDK::sampleApp::InteractionManager> interactionManager);

        void onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError) override;
        void onCapabilitiesStateChange(
                CapabilitiesDelegateObserverInterface::State newState,
                CapabilitiesDelegateObserverInterface::Error newError,
                const std::vector<std::string>& addedOrUpdatedEndpointIds,
                const std::vector<std::string>& deletedEndpointIds) override;


        std::atomic_bool m_limitedInteraction;
        std::shared_ptr<alexaClientSDK::sampleApp::InteractionManager> m_interactionManager;
        Core::ProxyType<AVSController> m_controller;
    };

} // namespace Plugin
} // namespace Thunder
