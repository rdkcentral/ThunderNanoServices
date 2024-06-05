#pragma once

#include "Module.h"

#include "Message.h"
#include <cec_device_adapter.h>
#include <simpleworker/SimpleWorker.h>

namespace Thunder {
namespace CEC {
    class LogicAddressClaimer {
    public:
        struct EXTERNAL ICallback {
            virtual ~ICallback() = default;
            virtual void Finished(const LogicAddressClaimer* claimer) = 0;
        };

        struct EXTERNAL IAdapter {
            virtual ~IAdapter() = default;
            virtual uint32_t Transmit(const cec_adapter_role_t initiator, const uint8_t follower, const uint8_t length, const uint8_t data[]) = 0;
        };

        LogicAddressClaimer(const LogicAddressClaimer&) = delete;
        LogicAddressClaimer& operator=(const LogicAddressClaimer&) = delete;

        LogicAddressClaimer(cec_adapter_role_t device, IAdapter* adapter, ICallback& callback)
            : _adapter(adapter)
            , _sink(*this)
            , _addresses(Convert(device))
            , _device(device)
            , _attemps(0)
            , _callback(callback)
        {
            TRACE(Trace::Information, ("%s(%d) build: %s", __FUNCTION__, this, __TIMESTAMP__));
            Core::SimpleWorker::Instance().Submit(&_sink);
        }
        ~LogicAddressClaimer()
        {
            Revoke();
            TRACE(Trace::Information, ("%s(%p) exit", __FUNCTION__, this));
        }

    private:
        class Sink : public Core::SimpleWorker::ICallback {
        public:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

            Sink(LogicAddressClaimer& parent)
                : _parent(parent)
            {
            }

            virtual ~Sink() = default;

        public:
            uint64_t Activity()
            {
                return _parent.Check();
            }

        private:
            LogicAddressClaimer& _parent;
        };

        uint64_t Check();

        uint32_t Revoke()
        {
            return Core::SimpleWorker::Instance().Revoke(&_sink) ? Core::ERROR_NONE : Core::ERROR_UNKNOWN_KEY;
        }

    public:
        cec_adapter_role_t Device() const
        {
            return _device;
        }

        logical_address_t LogicalAddress() const
        {
            return _addresses.empty() ? CEC_LOGICAL_ADDRESS_INVALID : _addresses.front();
        }

    private:
        IAdapter* _adapter;
        Sink _sink;
        logical_address_list_t _addresses;
        cec_adapter_role_t _device;
        uint8_t _attemps;
        ICallback& _callback;
    };

} // namespace CEC
} // namespace Thunder