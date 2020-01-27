#include "DIALServer.h"

#include "interfaces/ISwitchBoard.h"

namespace WPEFramework {
namespace DIALHandlers {

    class Netflix : public Plugin::DIALServer::Default {
    private:
        Netflix() = delete;
        Netflix(const Netflix&) = delete;
        Netflix& operator=(const Netflix&) = delete;

    public:
        Netflix(PluginHost::IShell* service, const Plugin::DIALServer::Config::App& config, Plugin::DIALServer *parent)
            : Default(service, config, parent)
        {
        }
        virtual ~Netflix()
        {
        }

    };

    static Plugin::DIALServer::ApplicationRegistrationType<Netflix> _netflixHandler;
}
}
