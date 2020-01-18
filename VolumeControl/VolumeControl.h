#pragma once

#include "Module.h"
#include <interfaces/IVolumeControl.h>
#include <interfaces/json/JsonData_VolumeControl.h>

namespace WPEFramework {
namespace Plugin {

    class VolumeControl : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    public:
        VolumeControl(const VolumeControl&) = delete;
        VolumeControl& operator=(const VolumeControl&) = delete;

        VolumeControl()
            : _implementation(nullptr)
            , _connectionId(0)
            , _service(nullptr)
            , _connectionNotification(this)
            , _volumeNotification(this)
        {
            RegisterAll();
        }

        ~VolumeControl() override
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(VolumeControl)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::IVolumeControl, _implementation)
        END_INTERFACE_MAP

        //   IPlugin methods
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    private:
        class ConnectionNotification : public RPC::IRemoteConnection::INotification {
        public:
            explicit ConnectionNotification(VolumeControl* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            ~ConnectionNotification() override
            {
            }

            ConnectionNotification() = delete;
            ConnectionNotification(const ConnectionNotification&) = delete;
            ConnectionNotification& operator=(const ConnectionNotification&) = delete;

            void Activated(RPC::IRemoteConnection*) override
            {
            }

            void Deactivated(RPC::IRemoteConnection* connection) override
            {
                _parent.Deactivated(connection);
            }

            BEGIN_INTERFACE_MAP(ConnectionNotification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

        private:
            VolumeControl& _parent;
        };

        class VolumeNotification : public Exchange::IVolumeControl::INotification {
        public:
            explicit VolumeNotification(VolumeControl* parent)
                : _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            ~VolumeNotification() override
            {
            }

            VolumeNotification() = delete;
            VolumeNotification(const VolumeNotification&) = delete;
            VolumeNotification& operator=(const VolumeNotification&) = delete;

            void Volume(const uint8_t volume) override
            {
                _parent.VolumeChanged(volume);
            }

            void Muted(const bool muted) override
            {
                _parent.MutedChanged(muted);
            }

            BEGIN_INTERFACE_MAP(VolumeNotification)
                INTERFACE_ENTRY(Exchange::IVolumeControl::INotification)
            END_INTERFACE_MAP

        private:
            VolumeControl& _parent;
        };

        void Deactivated(RPC::IRemoteConnection* connection);
        void VolumeChanged(const uint8_t volume)
        {
            event_volume(volume);
        }

        void MutedChanged(const bool muted)
        {
            event_muted(muted);
        }

        void RegisterAll();
        void UnregisterAll();
        uint32_t get_volume(Core::JSON::DecUInt8& response) const;
        uint32_t set_volume(const Core::JSON::DecUInt8& param);
        uint32_t get_muted(Core::JSON::Boolean& response) const;
        uint32_t set_muted(const Core::JSON::Boolean& param);
        void event_volume(const uint8_t& volume);
        void event_muted(const bool& muted);

        Exchange::IVolumeControl* _implementation;
        uint32_t _connectionId;
        PluginHost::IShell* _service;
        Core::Sink<ConnectionNotification> _connectionNotification;
        Core::Sink<VolumeNotification> _volumeNotification;

    };

} // namespace Plugin
} // namespace WPEFramework
