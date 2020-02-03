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
 
        bool Reached(bool pressed, uint32_t& marker)
        {
            bool reached = false;
            uint64_t now = Core::Time::Now().Ticks();
            if (pressed == true) {
                if (now > (_pressedTime + BounceThreshold)) {
                    _pressedTime = now;
                }
            }
            // Filter out contact bounce here.
            else if ((_pressedTime + BounceThreshold) > now) {
                uint32_t elapsedTime = static_cast<uint32_t>(now - _pressedTime);

                // See which marker we have reached..
                std::list<uint32_t>::const_iterator index;
                while ( (index != _markers.end()) && (elapsedTime <= *index)) {
                    marker = *index;
                    index++;
                }

                // Now we know which marker we have reached, report it.
                reached = true;
            }

            return(reached);
        }

    private:
        std::list<uint32_t> _markers;
	uint64_t _pressedTime;
    };

} // namespace Plugin
} // namespace WPEFramework
