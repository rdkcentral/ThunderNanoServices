#pragma once

#include <CECIAdapterAccessor.h>
#include <CECIDeviceAdapter.h>

namespace Thunder {
namespace CEC {
    class Adapter : public IAdapterAccesor {
    private:
        class Sink : public IDeviceAdapter::INotification {
        public:
            Sink() = delete;
            // Sink(const Sink&) = delete;
            // Sink& operator=(const Sink&) = delete;

            Sink(Adapter& parent)
                : _parent(parent)
            {
            }

            virtual ~Sink() = default;

        public:
            uint8_t Received(const cec_adapter_role_t follower VARIABLE_IS_NOT_USED, const logical_address_t initiator, const uint8_t length, const uint8_t data[]) override
            {
                return _parent.Received(initiator, length, data);
            }

        private:
            Adapter& _parent;
        };

    public:
        Adapter(cec_adapter_role_t initiator, const Core::ProxyType<IDeviceAdapter> deviceAdapter)
            : _initiator(initiator)
            , _deviceAdapter(deviceAdapter)
            , _sink(*this)
            , _awnserEvent(false, true)
            , _currentExchange(nullptr)
            , _currentFollower(CEC_LOGICAL_ADDRESS_INVALID)
        {
        }

        Adapter()
            : _initiator(CEC_DEVICE_UNKNOWN)
            , _deviceAdapter()
            , _sink(*this)
            , _awnserEvent(false, true)
            , _currentExchange(nullptr)
            , _currentFollower(CEC_LOGICAL_ADDRESS_INVALID)
        {
        }

        Adapter(const Adapter& copy)
            : _initiator(copy._initiator)
            , _deviceAdapter(copy._deviceAdapter)
            , _sink(copy._sink)
            , _awnserEvent(false, true)
            , _currentExchange(copy._currentExchange)
            , _currentFollower(copy._currentFollower)
        {
        }

        Adapter& operator=(const Adapter& rhs)
        {
            _initiator = rhs._initiator;
            _deviceAdapter = rhs._deviceAdapter;

            _currentExchange = rhs._currentExchange;
            _currentFollower = rhs._currentFollower;

            return (*this);
        }

        ~Adapter(){
            TRACE(Trace::Information, ("Exit %s", __FUNCTION__));
        }

        uint32_t Exchange(const logical_address_t follower, CEC::IExchange& exchange, const uint32_t waitTime = 1500) override;

        bool IsValid() const
        {
            return _deviceAdapter.IsValid();
        }

    private:
        inline bool WaitForAwnser(const uint32_t waitTime)
        {
            // Lock event until Dispatch() sets it.
            return (_awnserEvent.Lock(waitTime) == Core::ERROR_NONE);
        }

        uint8_t Received(const logical_address_t initiator, const uint8_t length, const uint8_t data[]);

    private:
        Core::CriticalSection _exchangeLock;
        cec_adapter_role_t _initiator;
        Core::ProxyType<IDeviceAdapter> _deviceAdapter;
        Sink _sink;
        Core::Event _awnserEvent;

        CEC::IExchange* _currentExchange;
        logical_address_t _currentFollower;
    }; // class Adapter
} // namespace CEC
} // namespace Thunder
