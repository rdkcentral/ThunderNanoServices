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
            printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
            Exchange::IBrowser* browser = QueryInterface<Exchange::IBrowser>();
            if (browser != nullptr) {
                printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
                browser->SetURL(data);
                browser->Release();
            } else
                printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
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
