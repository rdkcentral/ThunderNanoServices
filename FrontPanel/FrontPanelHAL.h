#ifndef FRONTPANELHAL_H
#define FRONTPANELHAL_H

#include "Module.h"
extern "C" {
#include <dsFPD.h>
}

namespace WPEFramework {

class FrontPanelHAL {
private:
    FrontPanelHAL(const FrontPanelHAL&) = delete;
    FrontPanelHAL& operator=(const FrontPanelHAL&) = delete;

    const std::map<const dsFPDState_t, const string > stateLookup = {
        { dsFPD_STATE_OFF, "STATE_OFF" },
        { dsFPD_STATE_ON, "STATE_ON" }
    };

    const std::map<const dsFPDTimeFormat_t, const string > timeFormatLookup = {
        { dsFPD_TIME_12_HOUR, "12_HOUR" },
        { dsFPD_TIME_24_HOUR, "24_HOUR" },
        { dsFPD_TIME_STRING, "STRING" }
    };

    const std::map<const dsFPDColor_t, const string > colorLookup = {
        { dsFPD_COLOR_BLUE, "BLUE" },
        { dsFPD_COLOR_GREEN, "GREEN" },
        { dsFPD_COLOR_RED, "RED" },
        { dsFPD_COLOR_YELLOW, "YELLOW" },
        { dsFPD_COLOR_ORANGE, "ORANGE" },
        { dsFPD_COLOR_WHITE, "WHITE" },
        { dsFPD_COLOR_MAX, "MAXIMUM" }
    };

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
    bool GetFPState(uint32_t, string&);
    bool GetFPBrightness(uint32_t, uint32_t&);
    bool GetFPColor(uint32_t, string&);
    bool GetFPTextBrightness(uint32_t, uint32_t&);
    bool GetFPTimeFormat(string&);
    bool SetFPBlink(uint32_t, uint32_t, uint32_t);
    bool SetFPBrightness(uint32_t, uint32_t);
    bool SetFPState(uint32_t, bool);
    bool SetFPColor(uint32_t, uint32_t);
    bool SetFPTime(uint32_t, uint32_t, uint32_t);
    bool SetFPText(string);
    bool SetFPTextBrightness(uint32_t, uint32_t);
    bool FPEnableClockDisplay(bool);
    bool SetFPScroll(uint32_t, uint32_t, uint32_t);
    bool SetFPDBrightness(uint32_t, uint32_t, bool);
    bool SetFPDColor(uint32_t, uint32_t, bool);
    bool SetFPTimeFormat(uint32_t);

private:
    bool _isOperational;
};
}
#endif
