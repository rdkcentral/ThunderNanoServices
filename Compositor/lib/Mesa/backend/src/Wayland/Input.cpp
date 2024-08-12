#include "Input.h"

namespace Thunder {
namespace Compositor {
    namespace Backend {
        Input::Input()
            : _wlSeat(nullptr)
            , _seatId()
            , _wlTouch(nullptr)
            , _wlPointer(nullptr)
            , _wlKeyboard(nullptr)
        {
        }

        Input::~Input()
        {
            if (_wlSeat != nullptr) {
                // wl_seat_destroy(_wlSeat);
            }
        }

        void Input::onRegister(struct wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
        {
            _wlSeat = static_cast<wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, 4));
            wl_seat_add_listener(_wlSeat, &seatListener, this);
        }

        void Input::onUnregister(uint32_t name) {}

        /*static*/ void Input::onSeatCapabilities(void* data, struct wl_seat* seat, uint32_t capabilities)
        {
            Input* implementation = static_cast<Input*>(data);

            if (implementation != nullptr) {
                TRACE_GLOBAL(Trace::Backend, ("Seat capabilities %p [0x%04X]", seat, capabilities));

                if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && (implementation->_wlPointer == nullptr)) {
                    TRACE_GLOBAL(Trace::Backend, ("Seat %p offered pointer", seat));
                }
                if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) && (implementation->_wlPointer != nullptr)) {
                    TRACE_GLOBAL(Trace::Backend, ("Seat %p dropped pointer", seat));
                }

                if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && (implementation->_wlKeyboard == nullptr)) {
                    TRACE_GLOBAL(Trace::Backend, ("Seat %p offered keyboard", seat));
                }
                if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && (implementation->_wlKeyboard != nullptr)) {
                    TRACE_GLOBAL(Trace::Backend, ("Seat %p dropped keyboard", seat));
                }

                if ((capabilities & WL_SEAT_CAPABILITY_TOUCH) && (implementation->_wlTouch == nullptr)) {
                    TRACE_GLOBAL(Trace::Backend, ("Seat %p offered touch", seat));
                }
                if (!(capabilities & WL_SEAT_CAPABILITY_TOUCH) && (implementation->_wlTouch != nullptr)) {
                    TRACE_GLOBAL(Trace::Backend, ("Seat %p dropped touch", seat));
                }
            }
        }

        /*static*/ void Input::onSeatName(void* data, struct wl_seat* seat, const char* name)
        {
            Input* implementation = static_cast<Input*>(data);

            if (implementation != nullptr) {
                TRACE_GLOBAL(Trace::Backend, ("Seat Name %s", name));
                implementation->_seatId = (name != nullptr) ? name : "";
            }
        }
    } //    namespace Backend
} //    namespace Compositor
} //   namespace Thunder
