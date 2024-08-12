/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#include "../include/Module.h"

#include <CECMessage.h>

#include <interfaces/IKeyHandler.h>

namespace {

// TODO: handle functions via user control
// see:
// CEC 13.13.5 Other uses of <User Control Pressed>
bool IsFunction(usercontrol_code_t code)
{
    return (
        ((code >= CEC_USER_CONTROL_CODE_PLAY_FUNCTION) && (code <= CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION))
        || (code == CEC_USER_CONTROL_CODE_SELECT_BROADCAST_TYPE)
        || (code == CEC_USER_CONTROL_CODE_SELECT_SOUND_PRESENTATION));
}

class CECInput : public Exchange::IKeyProducer {
public:
    CECInput(const CECInput&) = delete;
    CECInput& operator=(const CECInput&) = delete;

    CECInput(LinuxDevice* parent)
        : _adminLock()
        , _callback(nullptr)
        , _inputHandler(nullptr)
        , _enabled(false)
        , _pressedKeys()
    {
        // Remotes::RemoteAdministrator::Instance().Announce(*this);
        //          _inputHandler = PluginHost::InputHandler::Handler();
        //  ASSERT(_inputHandler != nullptr);
    }
    virtual ~CECInput()
    {
        // Remotes::RemoteAdministrator::Instance().Revoke(*this);
    }
    string Name() const override
    {
        return (_T("CECUserControlInput"));
    }

    uint32_t Callback(Exchange::IKeyHandler* callback) override
    {
        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        ASSERT((callback == nullptr) ^ (_callback == nullptr));
        _callback = callback;
        return (Core::ERROR_NONE);
    }
    uint32_t Error() const override
    {
        return (Core::ERROR_NONE);
    }
    string MetaData() const override
    {
        return (Name());
    }

    void Configure(const string&) override
    {
        Pair();
    }

    void ProducerEvent(const Exchange::ProducerEvents event) override
    {
        if (_callback != nullptr) {
            _callback->ProducerEvent(Name(), event);
        }
    }

    bool Pair() override
    {
        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        ProducerEvent(Thunder::Exchange::ProducerEvents::PairingStarted);
        bool _enabled = true;

        ProducerEvent(Thunder::Exchange::ProducerEvents::PairingSuccess);

        return true;
    }
    bool Unpair(string bindingId) override
    {
        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        ProducerEvent(Thunder::Exchange::ProducerEvents::UnpairingStarted);
        bool _enabled = false;

        while (_pressedKeys.empty() == false) {
            ReleaseKey();
        }

        ProducerEvent(Thunder::Exchange::ProducerEvents::UnpairingSuccess);

        return true;
    }

    void ReleaseKey(usercontrol_code_t key)
    {
        if ((_callback != nullptr) && (_pressedKeys.empty() == false)) {
            usercontrol_code_t key = _pressedKeys.back();
            _pressedKeys.pop_back();
            _callback->KeyEvent(false, key, Name());
        }
    }

public:
    static CECInput& Instance()
    {
        return Core::SingletonType<CECInput>::Instance();
    }

    void Pressed(usercontrol_code_t code) override
    {
        ASSERT(_callback != nullptr);

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        auto index(_pressedKeys.find(code));

        if ((_enabled == true) && (_callback != nullptr) && (index != _pressedKeys.end())) {
            _pressedKeys.push_back(code);
            _callback->KeyEvent(true, code, Name());
        }
    }

    void Release() override
    {
        ASSERT(_callback != nullptr);
        ASSERT(_pressedKeys.empty() == false);

        Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

        ReleaseKey()
    }

    BEGIN_INTERFACE_MAP(CECInput)
    INTERFACE_ENTRY(Exchange::IKeyProducer)
    END_INTERFACE_MAP

private:
    mutable Core::CriticalSection _adminLock;
    Exchange::IKeyHandler* _callback;

    PluginHost::VirtualInput* _inputHandler;
    bool _enabled;
    std::list<usercontrol_code_t> _pressedKeys;
};
}

namespace Thunder {
namespace CEC {
    namespace Message {
        namespace Service {
            /**
             * @brief Used to indicate that the user pressed a remote control button or switched from one
             *        remote control button to another. Can also be used as a command that is not directly
             *        initiated by the user.
             *
             * @param usercontrol_code_t Required UI command, plus any necessary Additional Operands specified
             *                           in CEC Table 6 and CEC Table 7.
             */

            class UserControlPressed : public ServiceType<USER_CONTROL_PRESSED, NO_OPCODE, UserControlPressed, false> {
                friend ServiceType;

            public:
                UserControlPressed()
                    : ServiceType()
                {
                }

                ~UserControlPressed() override = default;

            private:
                uint8_t Process(const uint8_t length, uint8_t buffer[])
                {
                    ASSERT(length >= 1);

                    usercontrol_code_t code = reinterpet_cast<usercontrol_code_t>(buffer[0]);

                    // if (::IsFunction(code) == false) {
                    ::CECInput::Instance().Pressed(code);
                    //}
                }
            }; // class UserControlPressed

            /**
             * @brief Indicates that user released a remote control button (the last one indicated by the
             *        <User Control Pressed> message). Also used after a command that is not directly
             *        initiated by the user.
             **/
            class UserControlReleased : public ServiceType<USER_CONTROL_RELEASED, NO_OPCODE, UserControlReleased, false> {
                friend ServiceType;

            public:
                UserControlReleased()
                    : ServiceType()
                {
                }

                ~UserControlReleased() = default;

            private:
                uint8_t Process(const uint8_t length, uint8_t buffer[])
                {
                    ::CECInput::Instance().Release();
                }
            }; // class UserControlReleased

            static Service::UserControlPressed service_user_control_pressed();
            static Service::UserControlReleased service_user_control_released();

        } // namespace Message
    } // namespace CEC
} // namespace Thunder