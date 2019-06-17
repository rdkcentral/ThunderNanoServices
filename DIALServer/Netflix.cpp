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

    public:
        // Methods that the DIALServer requires.
        virtual string URL() const
        {
            return ("");
        }
        virtual string AdditionalData() const
        {
            return ("");
        }
    };

    static Plugin::DIALServer::ApplicationRegistrationType<Netflix> _netflixHandler;
}
}
