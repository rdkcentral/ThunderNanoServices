#pragma once

#include "Module.h"
#include <interfaces/IPlayerInfo.h>
#include <interfaces/json/JsonData_PlayerInfo.h>

namespace WPEFramework {
namespace Plugin {

    class PlayerInfo : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    public:
        PlayerInfo(const PlayerInfo&) = delete;
        PlayerInfo& operator=(const PlayerInfo&) = delete;

        PlayerInfo()
            : _skipURL(0)
            , _connectionId(0)
            , _player(nullptr)
        {
            RegisterAll();
        }

        virtual ~PlayerInfo()
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(PlayerInfo)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        virtual void Inbound(Web::Request& request) override;
        virtual Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

    private:
        // JsonRpc
        void RegisterAll();
        void UnregisterAll();
        uint32_t get_playerinfo(JsonData::PlayerInfo::CodecsData&) const;

        void Info(JsonData::PlayerInfo::CodecsData&) const;

    private:
        uint8_t _skipURL;
        uint32_t _connectionId;
        Exchange::IPlayerProperties* _player;
    };

} // namespace Plugin
} // namespace WPEFramework
