#include "FrontPanelHAL.h"

namespace WPEFramework {

    FrontPanelHAL::FrontPanelHAL()
        : _isOperational(false)
    {
        Init();
    }

    FrontPanelHAL::~FrontPanelHAL()
    {
        Uninit();
    }

    void FrontPanelHAL::Init()
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if (dsFPInit() == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Initialized.")));
            _isOperational = true;
        }
    }

    void FrontPanelHAL::Uninit()
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if (dsFPTerm() == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Uninitialized.")));
            _isOperational = false;
        }
    }

    bool FrontPanelHAL::GetFPState(uint32_t eIndicator, string& state)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        dsFPDState_t fpdState;
        if (dsGetFPState((dsFPDIndicator_t)eIndicator, &fpdState) == dsERR_NONE) {
            state = stateLookup.find(fpdState)->second;
            return true;
        }
        return false;
    }

    bool FrontPanelHAL::GetFPBrightness(uint32_t eIndicator, uint32_t& brightness)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if (dsGetFPBrightness((dsFPDIndicator_t)eIndicator, (dsFPDBrightness_t*)&brightness) == dsERR_NONE)
            return true;
        return false;
    }

    bool FrontPanelHAL::GetFPColor(uint32_t eIndicator, string& color)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        dsFPDColor_t fpdColor;
        if (dsGetFPColor((dsFPDIndicator_t)eIndicator, &fpdColor) == dsERR_NONE) {
            color = colorLookup.find(fpdColor)->second;
            return true;
        }
        return false;

    }

    bool FrontPanelHAL::GetFPTextBrightness(uint32_t eIndicator, uint32_t& brightness)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        dsFPDTextDisplay_t fpdIndicator = static_cast<dsFPDTextDisplay_t>(eIndicator);
        if (dsGetFPTextBrightness(fpdIndicator, (dsFPDBrightness_t*)&brightness) == dsERR_NONE)
            return true;
        return false;
    }

    bool FrontPanelHAL::GetFPTimeFormat(string& timeFormat)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        dsFPDTimeFormat_t fpdTimeFormat;
        if (dsGetFPTimeFormat(&fpdTimeFormat) == dsERR_NONE) {
            timeFormat = timeFormatLookup.find(fpdTimeFormat)->second;
            return true;
        }
        return false;
    }

    bool FrontPanelHAL::SetFPBlink(uint32_t eIndicator, uint32_t uBlinkDuration, uint32_t uBlinkIterations)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if (dsSetFPBlink((dsFPDIndicator_t)eIndicator, uBlinkDuration, uBlinkIterations) == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Set Front Panel Blink.")));
            return true;
        }
        return false;
    }

    bool FrontPanelHAL::SetFPBrightness(uint32_t eIndicator, uint32_t eBrightness)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if (dsSetFPBrightness((dsFPDIndicator_t)eIndicator, (dsFPDBrightness_t)eBrightness)  == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Set Front Panel Brightness.")));
            return true;
        }
        return false;
    }

    bool FrontPanelHAL::SetFPState(uint32_t eIndicator, bool state)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        dsFPDState_t fpdState = state ? dsFPD_STATE_ON : dsFPD_STATE_OFF;
        if (dsSetFPState((dsFPDIndicator_t)eIndicator, fpdState)  == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Set Front Panel State.")));
            return true;
        }
        return false;
    }

    bool FrontPanelHAL::SetFPColor(uint32_t eIndicator, uint32_t eColor)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if (dsSetFPColor((dsFPDIndicator_t)eIndicator, (dsFPDColor_t)eColor) == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Set Front Panel Color.")));
            return true;
        }
        return false;
    }

    bool FrontPanelHAL::SetFPTime(uint32_t eTimeFormat, uint32_t uHour, uint32_t uMinutes)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if (dsSetFPTime((dsFPDTimeFormat_t)eTimeFormat, uHour, uMinutes) == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Set Front Panel Time.")));
            return true;
        }
        return false;
    }

    bool FrontPanelHAL::SetFPText(string pText)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if (dsSetFPText(pText.c_str()) == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Set Front Panel Text.")));
            return true;
        }
        return false;
    }

    bool FrontPanelHAL::SetFPTextBrightness(uint32_t eIndicator, uint32_t eBrightness)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        dsFPDTextDisplay_t fpdIndicator = static_cast<dsFPDTextDisplay_t>(eIndicator);
        if (dsSetFPTextBrightness(fpdIndicator, (dsFPDBrightness_t)eBrightness) == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Set Front Panel Text Brightness.")));
            return true;
        }
        return false;
    }

    bool FrontPanelHAL::FPEnableClockDisplay(bool enable)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if (dsFPEnableCLockDisplay((int)enable) == dsERR_NONE)
            return true;
        return false;
    }

    bool FrontPanelHAL::SetFPScroll(uint32_t uScrollHoldOnDur, uint32_t uHorzScrollIterations, uint32_t uVertScrollIterations)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if (dsSetFPScroll(uScrollHoldOnDur, uHorzScrollIterations, uVertScrollIterations) == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Set Front Panel Scroll.")));
            return true;
        }
        return false;
    }

    bool FrontPanelHAL::SetFPDBrightness(uint32_t eIndicator, uint32_t eBrightness, bool toPersist)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if(dsSetFPDBrightness((dsFPDIndicator_t)eIndicator, (dsFPDBrightness_t)eBrightness, toPersist) == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Set Front Panel Display Brightness.")));
            return true;
        }
        return false;
    }

    bool FrontPanelHAL::SetFPDColor(uint32_t eIndicator, uint32_t eColor, bool toPersist)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if (dsSetFPDColor((dsFPDIndicator_t)eIndicator, (dsFPDColor_t)eColor, toPersist) == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Set Front Panel Display Color.")));
            return true;
        }
        return false;
    }

    bool FrontPanelHAL::SetFPTimeFormat(uint32_t eTimeFormat)
    {
        TRACE(Trace::Information, (_T("%s"), __func__));
        if (dsSetFPTimeFormat((dsFPDTimeFormat_t)eTimeFormat) == dsERR_NONE) {
            TRACE(Trace::Information, (_T("Set Front Panel Time Format.")));
            return true;
        }
        return false;
    }
}
