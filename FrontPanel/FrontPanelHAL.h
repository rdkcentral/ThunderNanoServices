#ifndef FRONTPANELHAL_H
#define FRONTPANELHAL_H

#include "Module.h"

namespace WPEFramework {
namespace WPASupplicant {   // XXX: Change ???

class FrontPanelHAL {
private:
    FrontPanelHAL(const FrontPanelHAL&) = delete;
    FrontPanelHAL& operator=(const FrontPanelHAL&) = delete;

public:
    static Core::ProxyType<FrontPanelHAL> Create () {
       return (Core::ProxyType<FrontPanelHAL>::Create());
    }

    FrontPanelHAL();
    virtual ~FrontPanelHAL();
    void Init();
    void Uninit();
    inline bool IsOperational () const {
        return (_isOperational);
    }
    bool GetFPState(uint32_t, bool&);
    bool GetFPBrightness(uint32_t, uint32_t&);
    bool GetFPColor(uint32_t, uint32_t&);
    bool GetFPTextBrightness(uint32_t, uint32_t&);
    bool GetFPTimeFormat(uint32_t&);
    bool SetFPBlink(uint32_t, uint32_t, uint32_t);
    bool SetFPBrightness(uint32_t, uint32_t);
    bool SetFPState(uint32_t, bool);
    bool SetFPColor(uint32_t, uint32_t);
    bool SetFPTime(uint32_t, uint32_t, uint32_t);
    bool SetFPText(string);
    bool SetFPTextBrightness(uint32_t, uint32_t);
    bool FPEnableClockDisplay(uint32_t);
    bool SetFPScroll(uint32_t, uint32_t, uint32_t);
    bool SetFPDBrightness(uint32_t, uint32_t, bool);
    bool SetFPDColor(uint32_t, uint32_t, bool);
    bool SetFPTimeFormat(uint32_t);

private:
    bool _isOperational;
};

}
}

#endif
