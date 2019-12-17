#include "DIALServer.h"

#include "interfaces/IBrowser.h"
#include "interfaces/ISwitchBoard.h"

namespace WPEFramework {
namespace DIALHandlers {

    class YouTube : public Plugin::DIALServer::Default {
    private:
        YouTube() = delete;
        YouTube(const YouTube&) = delete;
        YouTube& operator=(const YouTube&) = delete;

    public:
        YouTube(PluginHost::IShell* service, const Plugin::DIALServer::Config::App& config, Plugin::DIALServer* parent)
            : Default(service, config, parent)
        {
        }
        virtual ~YouTube()
        {
        }

    public:
        virtual void Started(const string& data)
        {
            Exchange::IBrowser* browser = QueryInterface<Exchange::IBrowser>();
            if (browser != nullptr) {
                browser->SetURL(data);
                browser->Release();
            }
        }
        virtual void Stopped(const string& data)
        {
        }
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

    static Plugin::DIALServer::ApplicationRegistrationType<YouTube> _youTubeHandler;
}
}
