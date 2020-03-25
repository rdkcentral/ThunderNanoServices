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
 
#ifndef __LINUX_GPIO_H__
#define __LINUX_GPIO_H__

#include "Module.h"
#include "TimedInput.h"

#include <interfaces/IExternalBase.h>
#include <interfaces/IInputPin.h>

namespace WPEFramework {

namespace GPIO {

    class Pin : public Exchange::ExternalBase<Exchange::IExternal::GPIO>, 
                public Exchange::IInputPin,
                public Core::IResource {
    private:
        Pin() = delete;
        Pin(const Pin&) = delete;
        Pin& operator=(const Pin&) = delete;

        typedef Exchange::ExternalBase<Exchange::IExternal::GPIO> BaseClass;

        class TimedPin  : public Core::IDispatch {
        private:
            TimedPin() = delete;
            TimedPin(const TimedPin&) = delete;
            TimedPin& operator=(const TimedPin&) = delete;

            typedef std::list<Exchange::IInputPin::INotification*> ObserverList;
            typedef std::map<uint32_t, ObserverList> MarkerMap;

        public:
            TimedPin(Pin* parent)
                : _parent(*parent)
                , _marker(~0)
                , _job()
                , _observerList()
                , _markerMap()
                , _monitor()
            {
                ASSERT(parent != nullptr);
            }
            virtual ~TimedPin()
            {
            }

        public:
            inline void AddReference()
            {
                _job = Core::ProxyType<Core::IDispatch>(static_cast<Core::IDispatch&>(*this));
            }

            inline void DropReference()
            {
                _parent.Revoke(_job);
                _job.Release();
            }

            void Register(IInputPin::INotification* sink) 
            {
                _parent.Lock();
                ObserverList::iterator index = std::find(_observerList.begin(), _observerList.end(), sink);
                ASSERT (index == _observerList.end());
                if (index == _observerList.end()) {
                    sink->AddRef();
                    _observerList.push_back(sink);
                }
                _parent.Unlock();
            }

            void Unregister(IInputPin::INotification* sink) 
            {
                _parent.Lock();
                MarkerMap::iterator loop (_markerMap.begin());
                while (loop != _markerMap.end()) {
                    ObserverList::iterator found = std::find(loop->second.begin(), loop->second.end(), sink);
                    if (found != loop->second.end()) {
                        loop->second.erase(found);
                        if (loop->second.size() == 0) {
                            _monitor.Remove(loop->first);
                            loop = _markerMap.erase(loop);
                            continue;
                        }
                    }
                    loop++;
                }
                ObserverList::iterator index = std::find(_observerList.begin(), _observerList.end(), sink);
                ASSERT (index != _observerList.end());
                if (index != _observerList.end()) {
                    sink->Release();
                    _observerList.erase(index);
                }
                _parent.Unlock();
            }

            uint32_t Add(const IInputPin::INotification* sink, const uint32_t marker) 
            {
                uint32_t result = Core::ERROR_UNKNOWN_KEY;

                _parent.Lock();

                ObserverList::iterator index = std::find(_observerList.begin(), _observerList.end(), sink);

                if (index != _observerList.end()) {
                    result = Core::ERROR_NONE;
                    MarkerMap::iterator loop (_markerMap.find(marker));
                    if (loop == _markerMap.end()) {
                        auto newEntry = _markerMap.emplace(std::piecewise_construct, std::forward_as_tuple(marker), std::forward_as_tuple());
                        newEntry.first->second.push_back(*index);
                        _monitor.Add(marker);
                    }
                    else {
                        ObserverList::iterator found = std::find(loop->second.begin(), loop->second.end(), sink);

                        if (found != loop->second.end()) {
                            result = Core::ERROR_DUPLICATE_KEY;
                        }
                        else {
                            loop->second.push_back(*index);
                        }
                    }
                }

                _parent.Unlock();

                return (result);
            }

            uint32_t Remove(const IInputPin::INotification* sink, const uint32_t marker) 
            {
                uint32_t result = Core::ERROR_UNKNOWN_KEY;

                _parent.Lock();

                MarkerMap::iterator loop (_markerMap.find(marker));
                if (loop != _markerMap.end()) {
                    ObserverList::iterator found = std::find(loop->second.begin(), loop->second.end(), sink);

                    if (found != loop->second.end()) {
                        result = Core::ERROR_NONE;

                        loop->second.erase(found);
                        if (loop->second.size() == 0) {
                            _monitor.Remove(loop->first);
                            _markerMap.erase(loop);
                        }
                    }
                }

                _parent.Unlock();

                return (result);

            }
            void Update(const bool pressed)
            {
                uint32_t marker;

                if (_monitor.Reached(pressed, marker) == true) {
 
                    _parent.Lock();

                    ASSERT(_marker == static_cast<uint32_t>(~0));
                    _marker = marker;
                    _parent.Schedule(Core::Time(), _job);
                    _parent.Unlock();
                }
            }

        private:
            virtual void Dispatch()
            {
                _parent.Lock();

                uint32_t marker = _marker;

                MarkerMap::iterator loop (_markerMap.find(marker));
                if (loop != _markerMap.end()) {
                    ObserverList::const_iterator index(loop->second.cbegin());
                    _marker = ~0;
                    RecursiveCall(loop->second, index, marker);
                }
            }
            void RecursiveCall(const ObserverList& list, ObserverList::const_iterator& position, const uint32_t marker)
            {
                if (position == list.cend()) {
                    _parent.Unlock();
                } else {
                    IInputPin::INotification* client(*position);
                    client->AddRef();
                    position++;
                    RecursiveCall(list, position, marker);
                    client->Marker(marker);
                    client->Release();
                }
            }

        private:
            Pin& _parent;
            uint32_t _marker;
            Core::ProxyType<Core::IDispatch> _job;
            ObserverList _observerList;
            MarkerMap _markerMap;
            TimedInput _monitor;
        };

    public:
        enum pin_mode {
            INPUT,
            OUTPUT,
            PWM_TONE,
            PWM,
            CLOCK
        };

        enum pull_mode {
            OFF = 0,
            DOWN = 1,
            UP = 2
        };

        enum trigger_mode {
            NONE = 0x00,
            FALLING = 0x01,
            RISING = 0x02,
            BOTH = 0x03,
            HIGH = 0x04,
            LOW = 0x08
        };

    public:
        Pin(const uint16_t id, const bool activeLow);
        virtual ~Pin();

    public:
        bool Get() const;
        void Set(const bool value);

        void Trigger(const trigger_mode mode);
        void Mode(const pin_mode mode);
        void Pull(const pull_mode mode);

        bool HasChanged() const;
        void Align();

        inline void Subscribe(Exchange::IExternal::INotification* sink)
        {
            BaseClass::Register(sink);
            Core::ResourceMonitor::Instance().Register(*this);
        }
        inline void Unsubscribe(Exchange::IExternal::INotification* sink)
        {
            Core::ResourceMonitor::Instance().Unregister(*this);
            BaseClass::Unregister(sink);
        }

        virtual void Trigger() override;
        virtual uint32_t Get(int32_t& value) const override;
        virtual uint32_t Set(const int32_t value) override;

        // IInput pin functionality. Get triggered by an IOPin if a marker has been reached
        // ---------------------------------------------------------------------------------
        void Register(IInputPin::INotification* sink) override;
        void Unregister(IInputPin::INotification* sink) override;

        uint32_t AddMarker(const IInputPin::INotification* sink, const uint32_t marker) override;
        uint32_t RemoveMarker(const IInputPin::INotification* sink, const uint32_t marker) override;

        BEGIN_INTERFACE_MAP(Pin)
            INTERFACE_RELAY(Exchange::IInputPin, this)
        NEXT_INTERFACE_MAP(Exchange::ExternalBase<Exchange::IExternal::GPIO>)

    private:
        virtual Core::IResource::handle Descriptor() const override;
        virtual uint16_t Events() override;
        virtual void Handle(const uint16_t events) override;

        virtual void Schedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job) override;
        virtual void Revoke(const Core::ProxyType<Core::IDispatch>& job) override;

        void Flush();

    private:
        const uint16_t _pin;
        uint8_t _activeLow;
        bool _lastValue;
        mutable int _descriptor;
        Core::ProxyObject<TimedPin> _timedPin;
    };
}
} // namespace WPEFramework::GPIO

#endif // __LINUX_GPIO_H__
