#ifndef RTSPSESSIONINFO_H
#define RTSPSESSIONINFO_H

#include <string>

namespace WPEFramework {
namespace Plugin {

struct ServerInfo {
    std::string name;
    std::string address;
    uint16_t port;
};

class RtspSessionInfo
{
    public:
        RtspSessionInfo();
        void reset();
        ServerInfo srm;
        ServerInfo pump;

        std::string sessionId, ctrlSessionId;
        bool bSrmIsRtspProxy;
        int sessionTimeout, ctrlSessionTimeout;
        int defaultSessionTimeout, defaultCtrlSessionTimeout;
        unsigned int frequency;
        unsigned int programNum;
        unsigned int modulation;
        unsigned int symbolRate;
        int duration;
        float bookmark;
        float npt;
        float scale;
};

}} // WPEFramework::Plugin

#endif
