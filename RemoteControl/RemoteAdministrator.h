/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include <interfaces/IKeyHandler.h>

namespace WPEFramework {
namespace Remotes {

    class RemoteAdministrator {
    private:
        RemoteAdministrator(const RemoteAdministrator&);
        RemoteAdministrator& operator=(const RemoteAdministrator&);
        RemoteAdministrator()
            : _adminLock()
            , _keyCallback(nullptr)
            , _wheelCallback(nullptr)
            , _pointerCallback(nullptr)
            , _touchCallback(nullptr)
            , _remotes()
            , _wheels()
            , _pointers()
            , _touchpanels()
        {
        }

    public:
        typedef Core::IteratorType<std::list<Exchange::IKeyProducer*>, Exchange::IKeyProducer*> KeyIterator;
        typedef Core::IteratorType<std::list<Exchange::IWheelProducer*>, Exchange::IWheelProducer*> WheelIterator;
        typedef Core::IteratorType<std::list<Exchange::IPointerProducer*>, Exchange::IPointerProducer*> PointerIterator;
        typedef Core::IteratorType<std::list<Exchange::ITouchProducer*>, Exchange::ITouchProducer*> TouchIterator;
        typedef KeyIterator Iterator;

        static RemoteAdministrator& Instance();
        ~RemoteAdministrator()
        {
        }

    public:
        inline Iterator Producers()
        {
            return (Iterator(_remotes));
        }
        inline KeyIterator KeyProducers()
        {
            return (KeyIterator(_remotes));
        }
        inline WheelIterator WheelProducers()
        {
            return (WheelIterator(_wheels));
        }
        inline PointerIterator PointerProducers()
        {
            return (PointerIterator(_pointers));
        }
        inline TouchIterator TouchProducers()
        {
            return (TouchIterator(_touchpanels));
        }
        uint32_t Error(const string& device)
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;
            bool found = false;

            _adminLock.Lock();

            std::list<Exchange::IKeyProducer*>::iterator index(_remotes.begin());

            while ((index != _remotes.end()) && (result == Core::ERROR_UNAVAILABLE)) {
                if (device == (*index)->Name()) {
                    result = (*index)->Error();
                    index = _remotes.end();
                    found = true;
                } else {
                    index++;
                }
            }

            if (found == false) {
                auto index(_wheels.begin());
                while ((index != _wheels.end()) && (result == Core::ERROR_UNAVAILABLE) && (found == false)) {
                    if (device == (*index)->Name()) {
                        result = (*index)->Error();
                        found = true;
                    } else {
                        index++;
                    }
                }
            }

            if (found == false) {
                auto index(_pointers.begin());
                while ((index != _pointers.end()) && (result == Core::ERROR_UNAVAILABLE) && (found == false)) {
                    if (device == (*index)->Name()) {
                        result = (*index)->Error();
                        found = true;
                    } else {
                        index++;
                    }
                }
            }

            if (found == false) {
                auto index(_touchpanels.begin());
                while ((index != _touchpanels.end()) && (result == Core::ERROR_UNAVAILABLE) && (found == false)) {
                    if (device == (*index)->Name()) {
                        result = (*index)->Error();
                        found = true;
                    } else {
                        index++;
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        bool Pair(const string& device)
        {
            bool result = true;

            _adminLock.Lock();

            std::list<Exchange::IKeyProducer*>::iterator index(_remotes.begin());

            while (index != _remotes.end()) {
                if (device.empty() == true) {
                    result = (*index)->Pair() && result;
                    index++;
                } else if (device == (*index)->Name()) {
                    result = (*index)->Pair();
                    index = _remotes.end();
                } else {
                    index++;
                }
            }

            // Leave out non-key producers

            _adminLock.Unlock();

            return (result);
        }

        bool Unpair(const string& device, string bindingId)
        {
            bool result = true;

            _adminLock.Lock();

            std::list<Exchange::IKeyProducer*>::iterator index(_remotes.begin());

            while (index != _remotes.end()) {
                if (device.empty() == true) {
                    result = (*index)->Unpair(bindingId) && result;
                    index++;
                } else if (device == (*index)->Name()) {
                    result = (*index)->Unpair(bindingId);
                    index = _remotes.end();
                } else {
                    index++;
                }
            }

            // Leave out non-key producers

            _adminLock.Unlock();

            return (result);
        }

        string MetaData(const string& device)
        {
            string result;

            _adminLock.Lock();

            std::list<Exchange::IKeyProducer*>::iterator index(_remotes.begin());

            while (index != _remotes.end()) {
                string entry;

                if (device.empty() == true) {
                    entry = '\"' + string((*index)->Name()) + _T("\":\"") + (*index)->MetaData() + '\"';
                    index++;
                } else if (device == (*index)->Name()) {
                    entry = '\"' + string((*index)->Name()) + _T("\":\"") + (*index)->MetaData() + '\"';
                    index = _remotes.end();
                } else {
                    index++;
                }

                if (result.empty() == true) {
                    result = '{' + entry;
                } else {
                    result += ',' + entry;
                }
            }

            // TODO: pick up non-keyproducers as well

            _adminLock.Unlock();

            if (result.empty() == false) {
                result += '}';
            }

            return (result);
        }
        void Announce(Exchange::IKeyProducer& remoteControl)
        {
            _adminLock.Lock();

            std::list<Exchange::IKeyProducer*>::iterator index(std::find(_remotes.begin(), _remotes.end(), &remoteControl));

            // Announce a remote only once.
            ASSERT(index == _remotes.end());

            if (index == _remotes.end()) {
                _remotes.push_back(&remoteControl);

                if (_keyCallback != nullptr) {
                    remoteControl.Callback(_keyCallback);
                }
            }

            _adminLock.Unlock();
        }
        void Announce(Exchange::IWheelProducer& wheel)
        {
            _adminLock.Lock();

            auto index(std::find(_wheels.begin(), _wheels.end(), &wheel));
            ASSERT(index == _wheels.end());

            if (index == _wheels.end()) {
                _wheels.push_back(&wheel);

                if (_wheelCallback != nullptr) {
                    wheel.Callback(_wheelCallback);
                }
            }

            _adminLock.Unlock();
        }
        void Announce(Exchange::IPointerProducer& pointer)
        {
            _adminLock.Lock();

            auto index(std::find(_pointers.begin(), _pointers.end(), &pointer));
            ASSERT(index == _pointers.end());

            if (index == _pointers.end()) {
                _pointers.push_back(&pointer);

                if (_pointerCallback != nullptr) {
                    pointer.Callback(_pointerCallback);
                }
            }

            _adminLock.Unlock();
        }
        void Announce(Exchange::ITouchProducer& touchPanel)
        {
            _adminLock.Lock();

            auto index(std::find(_touchpanels.begin(), _touchpanels.end(), &touchPanel));
            ASSERT(index == _touchpanels.end());

            if (index == _touchpanels.end()) {
                _touchpanels.push_back(&touchPanel);

                if (_touchCallback != nullptr) {
                    touchPanel.Callback(_touchCallback);
                }
            }

            _adminLock.Unlock();
        }
        void Revoke(Exchange::IKeyProducer& remoteControl)
        {
            _adminLock.Lock();

            std::list<Exchange::IKeyProducer*>::iterator index(std::find(_remotes.begin(), _remotes.end(), &remoteControl));

            // Only revoke remotes you subscribed !!!!
            ASSERT(index != _remotes.end());

            if (index != _remotes.end()) {
                _remotes.erase(index);

                if (_keyCallback != nullptr) {
                    remoteControl.Callback(nullptr);
                }
            }

            _adminLock.Unlock();
        }
        void Revoke(Exchange::IWheelProducer& wheel)
        {
            _adminLock.Lock();

            auto index(std::find(_wheels.begin(), _wheels.end(), &wheel));
            ASSERT(index != _wheels.end());

            if (index != _wheels.end()) {
                _wheels.erase(index);

                if (_wheelCallback != nullptr) {
                    wheel.Callback(nullptr);
                }
            }

            _adminLock.Unlock();
        }
        void Revoke(Exchange::IPointerProducer& pointer)
        {
            _adminLock.Lock();

            auto index(std::find(_pointers.begin(), _pointers.end(), &pointer));
            ASSERT(index != _pointers.end());

            if (index != _pointers.end()) {
                _pointers.erase(index);

                if (_pointerCallback != nullptr) {
                    pointer.Callback(nullptr);
                }
            }

            _adminLock.Unlock();
        }
        void Revoke(Exchange::ITouchProducer& touchpanel)
        {
            _adminLock.Lock();

            auto index(std::find(_touchpanels.begin(), _touchpanels.end(), &touchpanel));
            ASSERT(index != _touchpanels.end());

            if (index != _touchpanels.end()) {
                _touchpanels.erase(index);

                if (_touchCallback != nullptr) {
                    touchpanel.Callback(nullptr);
                }
            }

            _adminLock.Unlock();
        }
        void RevokeAll()
        {
            _adminLock.Lock();

            {
                auto index(_remotes.begin());
                while (index != _remotes.end()) {
                    if (_keyCallback != nullptr) {
                        (*index)->Callback(nullptr);
                    }
                    index++;
                }
                _remotes.clear();
            }

            {
                auto index(_wheels.begin());
                while (index != _wheels.end()) {
                    if (_wheelCallback != nullptr) {
                        (*index)->Callback(nullptr);
                    }
                    index++;
                }
                _wheels.clear();
            }

            {
                auto index(_pointers.begin());
                while (index != _pointers.end()) {
                    if (_pointerCallback != nullptr) {
                        (*index)->Callback(nullptr);
                    }
                    index++;
                }
                _pointers.clear();
            }

            {
                auto index(_touchpanels.begin());
                while (index != _touchpanels.end()) {
                    if (_touchCallback != nullptr) {
                        (*index)->Callback(nullptr);
                    }
                    index++;
                }
                _touchpanels.clear();
            }

            _adminLock.Unlock();
        }
        void Callback(Exchange::IKeyHandler* callback)
        {
            _adminLock.Lock();

            ASSERT((_keyCallback == nullptr) ^ (callback == nullptr));

            auto index(_remotes.begin());
            _keyCallback = callback;

            while (index != _remotes.end()) {
                uint32_t result = (*index)->Callback(callback);

                if (result != Core::ERROR_NONE) {
                    if (callback == nullptr) {
                        SYSLOG(Logging::Startup, (_T("Failed to initialize %s, error [%d]"), (*index)->Name(), result));
                    } else {
                        SYSLOG(Logging::Shutdown, (_T("Failed to deinitialize %s, error [%d]"), (*index)->Name(), result));
                    }
                }

                index++;
            }

            _adminLock.Unlock();
        }
        void Callback(Exchange::IWheelHandler* callback)
        {
            _adminLock.Lock();

            ASSERT((_wheelCallback == nullptr) ^ (callback == nullptr));

            auto index(_wheels.begin());
            _wheelCallback = callback;

            while (index != _wheels.end()) {
                uint32_t result = (*index)->Callback(callback);

                if (result != Core::ERROR_NONE) {
                    if (callback == nullptr) {
                        SYSLOG(Logging::Startup, (_T("Failed to initialize %s, error [%d]"), (*index)->Name(), result));
                    } else {
                        SYSLOG(Logging::Shutdown, (_T("Failed to deinitialize %s, error [%d]"), (*index)->Name(), result));
                    }
                }

                index++;
            }

            _adminLock.Unlock();
        }
        void Callback(Exchange::IPointerHandler* callback)
        {
            _adminLock.Lock();

            ASSERT((_pointerCallback == nullptr) ^ (callback == nullptr));

            auto index(_pointers.begin());
            _pointerCallback = callback;

            while (index != _pointers.end()) {
                uint32_t result = (*index)->Callback(callback);

                if (result != Core::ERROR_NONE) {
                    if (callback == nullptr) {
                        SYSLOG(Logging::Startup, (_T("Failed to initialize %s, error [%d]"), (*index)->Name(), result));
                    } else {
                        SYSLOG(Logging::Shutdown, (_T("Failed to deinitialize %s, error [%d]"), (*index)->Name(), result));
                    }
                }

                index++;
            }

            _adminLock.Unlock();
        }
        void Callback(Exchange::ITouchHandler* callback)
        {
            _adminLock.Lock();

            ASSERT((_touchCallback == nullptr) ^ (callback == nullptr));

            auto index(_touchpanels.begin());
            _touchCallback = callback;

            while (index != _touchpanels.end()) {
                uint32_t result = (*index)->Callback(callback);

                if (result != Core::ERROR_NONE) {
                    if (callback == nullptr) {
                        SYSLOG(Logging::Startup, (_T("Failed to initialize %s, error [%d]"), (*index)->Name(), result));
                    } else {
                        SYSLOG(Logging::Shutdown, (_T("Failed to deinitialize %s, error [%d]"), (*index)->Name(), result));
                    }
                }

                index++;
            }

            _adminLock.Unlock();
        }

    private:
        Core::CriticalSection _adminLock;
        Exchange::IKeyHandler* _keyCallback;
        Exchange::IWheelHandler* _wheelCallback;
        Exchange::IPointerHandler* _pointerCallback;
        Exchange::ITouchHandler* _touchCallback;
        std::list<Exchange::IKeyProducer*> _remotes;
        std::list<Exchange::IWheelProducer*> _wheels;
        std::list<Exchange::IPointerProducer*> _pointers;
        std::list<Exchange::ITouchProducer*> _touchpanels;
    };
}
}
