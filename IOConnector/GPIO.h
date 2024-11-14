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
 
#ifndef __LINUX_GPIO_H__
#define __LINUX_GPIO_H__

#include "Module.h"
#include "TimedInput.h"

#include <interfaces/IExternalBase.h>
#include <interfaces/IInputPin.h>

namespace Thunder {

namespace GPIO {

    class Pin : public Exchange::ExternalBase, 
                public Exchange::IInputPin,
                public Core::IResource {
    public:
        using Exchange::ExternalBase::Register;
        using Exchange::ExternalBase::Unregister;

    private:
        using BaseClass = Exchange::ExternalBase;

        class TimedPin {
        private:
            typedef std::list<Exchange::IInputPin::INotification*> ObserverList;
            typedef std::map<uint32_t, ObserverList> MarkerMap;

        public:
            TimedPin() = delete;
            TimedPin(const TimedPin&) = delete;
            TimedPin& operator=(const TimedPin&) = delete;

            TimedPin(Pin* parent)
                : _parent(*parent)
                , _marker(~0)
                , _observerList()
                , _monitor()
                , _job(*this)
            {
                ASSERT(parent != nullptr);
            }
            ~TimedPin()
            {
                _job.Revoke();
            }

        public:
            void Register(IInputPin::INotification* sink) 
            {
                ASSERT(sink != nullptr);

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
                ASSERT(sink != nullptr);

                _parent.Lock();
                ObserverList::iterator index = std::find(_observerList.begin(), _observerList.end(), sink);
                ASSERT (index != _observerList.end());
                if (index != _observerList.end()) {
                    sink->Release();
                    _observerList.erase(index);
                }
                _parent.Unlock();
            }

            void Add(const uint32_t marker) 
            {
                _parent.Lock();

                _monitor.Add(marker);

                _parent.Unlock();
            }

            void Remove(const uint32_t marker) 
            {
                _parent.Lock();

                _monitor.Remove(marker);

                _parent.Unlock();
            }
            void Update(const bool pressed)
            {
                uint32_t marker;

                if (_monitor.Reached(pressed, marker) == true) {
 
                    _parent.Lock();

                    ASSERT(_marker == static_cast<uint32_t>(~0));
                    _marker = marker;
                    _job.Submit();
                    _parent.Unlock();
                }
            }

        private:
            friend Core::ThreadPool::JobType<TimedPin&>;
            void Dispatch()
            {
                TRACE(Trace::Information, (_T("TimedPin: job is dispatched")));
                _parent.Lock();

                uint32_t marker = _marker;

                ObserverList::const_iterator index(_observerList.cbegin());
                _marker = ~0;
                RecursiveCall(index, marker);
            }
            void RecursiveCall(ObserverList::const_iterator& position, const uint32_t marker)
            {
                if (position == _observerList.cend()) {
                    _parent.Unlock();
                } else {
                    IInputPin::INotification* client(*position);
                    client->AddRef();
                    position++;
                    RecursiveCall(position, marker);
                    client->Marker(marker);
                    client->Release();
                }
            }

        private:
            Pin& _parent;
            uint32_t _marker;
            ObserverList _observerList;
            TimedInput _monitor;

            Core::WorkerPool::JobType<TimedPin&> _job;
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
        Pin() = delete;
        Pin(const Pin&) = delete;
        Pin& operator=(const Pin&) = delete;

        Pin(const uint16_t id, const bool activeLow);
        ~Pin() override;

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

        void Evaluate() override;
        uint32_t Get(int32_t& value) const override;
        uint32_t Set(const int32_t value) override;
        string Metadata() const override { return (string()); }

        // IInput pin functionality. Get triggered by an IOPin if a marker has been reached
        // ---------------------------------------------------------------------------------
        void Register(IInputPin::INotification* sink) override;
        void Unregister(IInputPin::INotification* sink) override;

        void AddMarker(const uint32_t marker) override;
        void RemoveMarker(const uint32_t marker) override;

        BEGIN_INTERFACE_MAP(Pin)
            INTERFACE_RELAY(Exchange::IInputPin, this)
        NEXT_INTERFACE_MAP(Exchange::ExternalBase)

    private:
        Core::IResource::handle Descriptor() const override;
        uint16_t Events() override;
        void Handle(const uint16_t events) override;
        void Flush();

    private:
        const uint16_t _pin;
        uint8_t _activeLow;
        bool _lastValue;
        mutable int _descriptor;
        Core::ProxyObject<TimedPin> _timedPin;
    };
}
} // namespace Thunder::GPIO

#endif // __LINUX_GPIO_H__
