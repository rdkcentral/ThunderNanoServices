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

namespace WPEFramework {

namespace GPIO {

    class TimedInput {
    private:
        const uint16_t BounceThreshold = 100;

    public:
        TimedInput(const TimedInput&) = delete;
        TimedInput& operator=(const TimedInput&) = delete;

        TimedInput()
            : _markers()
            , _pressedTime(0)
        {
        }
        ~TimedInput()
        {
        }

    public:
        void Clear() {
            _markers.clear();
        }
        void Add(const uint32_t marker) {
            std::list<uint32_t>::iterator index(_markers.begin());
            while ( (index != _markers.end()) && (marker > *index)) {
                index++;
            }
            if (index == _markers.end()) {
                _markers.push_back(marker);
            }
            else {
                // Do not set the same marker twice!!!
                ASSERT (marker != *index);
                _markers.insert(index, marker);
            }
        }
        void Remove(const uint32_t marker) {
            std::list<uint32_t>::iterator index(_markers.begin());
            while ( (index != _markers.end()) && (marker != *index)) {
                index++;
            }
            if (index != _markers.end()) {
                _markers.erase(index);
            }
        }
        bool Reached(bool pressed, uint32_t& marker) const
        {
            bool reached = false;
            uint64_t now = Core::Time::Now().Ticks();

            marker = ~0;

            if (pressed == true) {
                _pressedTime = now;
            }
            else if (_markers.size() == 0) {
                reached = true;
            } 
            else if (now > (_pressedTime + BounceThreshold)) {
                uint32_t elapsedTime = static_cast<uint32_t>( (now - _pressedTime) / Core::Time::TicksPerMillisecond );

                // See which marker we have reached..
                std::list<uint32_t>::const_iterator index (_markers.cbegin());
                while ( (index != _markers.cend()) && (elapsedTime > *index)) {
                    marker = *index;
                    index++;
                }

                // Now we know which marker we have reached, report it.
                reached = (index != _markers.cend());
            }

            return(reached);
        }

    private:
        std::list<uint32_t> _markers;
	mutable uint64_t _pressedTime;
    };

} // namespace Plugin
} // namespace WPEFramework
