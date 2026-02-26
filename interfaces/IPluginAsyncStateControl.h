/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IPluginAsyncStateControl : virtual public Core::IUnknown {

        enum { ID = ID_PLUGINASYNCSTATECONTROL };

        ~IPluginAsyncStateControl() override = default;

        struct EXTERNAL IActivationCallback : virtual public Core::IUnknown {
            enum { ID = ID_PLUGINASYNCSTATECONTROL_ACTIVATIONCALLBACK };
            ~IActivationCallback() override = default;

            enum class state : uint8_t {
                SUCCESS,
                FAILURE,
                ABORTED
            };

            // @brief callback called when an activation request has finished. Note this can be called while the Activate call has not yet returned
            // @param state result state of the activation request (ABORTED when AbortActivate was called AND the plugin did not reach activated state yet before the request was aborted, otherwise SUCCESS will be reported as a result of an AbortActivate request)
            // @param numberofretries Number of retries that happened the moment this callback was called
            virtual void Finished(const string& callsign, const state state, const uint8_t numberofretries) = 0;
        };

        // @brief Activate a plugin. Passed callbcak will be called on failure or success
        // @param callsign: callsign of plugin to activate
        // @param maxnumberretries: maximum number of retries to initialize the plugin (default used when 0 is passed)
        // @param delay: delay to be used (in ms) between initialization retries (default used when 0 is passed)
        // @param cb: callback interface called on success or failure
        // @retval ERROR_INPROGRESS Activation request is already in progress for this callsign
        // @retval ERROR_ILLEGAL_STATE Plugin with this callsign is in an invalid state for it to be able to be started (e.g. DESTROYED or UNAVAILABLE)
        // @retval ERROR_NOT_EXIST Plugin is unknown to Thunder (at this moment in case of Dynamic plugins)
        virtual Core::hresult Activate(const string& callsign, const uint8_t& maxnumberretries, const uint16_t& delay, IActivationCallback* const cb) = 0;

        // @brief Abort a previously started Activate request
        // @retval ERROR_NOT_EXIST There is no ongoing activation request 
        virtual Core::hresult AbortActivate(const string& callsign) = 0;

    };
}
}