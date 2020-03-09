/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FRONTPANELHAL_H
#define FRONTPANELHAL_H

#include "Module.h"
extern "C" {
#include <dsFPD.h>
}

namespace WPEFramework {

class FrontPanelHAL {
public:
    enum Indicator {
        MESSAGE,
        POWER,
        RECORD,
        REMOTE,
        RFBYPASS,
        MAX_INDICATOR
    };

    enum TextDisplay {
        TEXT,
        MAX_TEXTDISP
    };

    enum State {
        STATE_OFF,
        STATE_ON
    };

    enum TimeFormat {
        TWELVE_HOUR,
        TWENTY_FOUR_HOUR,
        STRING
    };

    enum Color {
        BLUE,
        GREEN,
        RED,
        YELLOW,
        ORANGE,
        WHITE,
        MAX_COLOR
    };

private:
    FrontPanelHAL(const FrontPanelHAL&) = delete;
    FrontPanelHAL& operator=(const FrontPanelHAL&) = delete;

    const std::map<const Indicator, const dsFPDIndicator_t> indicatorLookup = {
        { MESSAGE, dsFPD_INDICATOR_MESSAGE },
        { POWER, dsFPD_INDICATOR_POWER },
        { RECORD, dsFPD_INDICATOR_RECORD },
        { REMOTE, dsFPD_INDICATOR_REMOTE },
        { RFBYPASS, dsFPD_INDICATOR_RFBYPASS },
        { MAX_INDICATOR, dsFPD_INDICATOR_MAX }
    };

    const std::map<const TextDisplay, const dsFPDTextDisplay_t> textDisplayLookup = {
        { TEXT, dsFPD_TEXTDISP_TEXT },
        { MAX_TEXTDISP, dsFPD_TEXTDISP_MAX }
    };

    const std::map<const State, const dsFPDState_t> stateLookup = {
        { STATE_OFF, dsFPD_STATE_OFF },
        { STATE_ON, dsFPD_STATE_ON }
    };

    const std::map<const TimeFormat, const dsFPDTimeFormat_t> timeFormatLookup = {
        { TWELVE_HOUR, dsFPD_TIME_12_HOUR },
        { TWENTY_FOUR_HOUR, dsFPD_TIME_24_HOUR },
        { STRING, dsFPD_TIME_STRING }
    };

    const std::map<const Color, const dsFPDColor_t> colorLookup = {
        { BLUE, dsFPD_COLOR_BLUE },
        { GREEN, dsFPD_COLOR_GREEN },
        { RED, dsFPD_COLOR_RED },
        { YELLOW, dsFPD_COLOR_YELLOW },
        { ORANGE, dsFPD_COLOR_ORANGE },
        { WHITE, dsFPD_COLOR_WHITE },
        { MAX_COLOR, dsFPD_COLOR_MAX }
    };

public:
    FrontPanelHAL();
    virtual ~FrontPanelHAL();
    void Init();
    void Uninit();
    inline bool IsOperational() const
    {
        return (_isOperational);
    }
    bool GetFPState(Indicator, State&);
    bool GetFPBrightness(Indicator, uint32_t&);
    bool GetFPColor(Indicator, Color&);
    bool GetFPTextBrightness(TextDisplay, uint32_t&);
    bool GetFPTimeFormat(TimeFormat&);
    bool SetFPBlink(Indicator, uint32_t, uint32_t);
    bool SetFPBrightness(Indicator, uint32_t);
    bool SetFPState(Indicator, State);
    bool SetFPColor(Indicator, Color);
    bool SetFPTime(TimeFormat, uint32_t, uint32_t);
    bool SetFPText(string);
    bool SetFPTextBrightness(TextDisplay, uint32_t);
    bool FPEnableClockDisplay(bool);
    bool SetFPScroll(uint32_t, uint32_t, uint32_t);
    bool SetFPDBrightness(Indicator, uint32_t, bool);
    bool SetFPDColor(Indicator, Color, bool);
    bool SetFPTimeFormat(TimeFormat);

private:
    bool _isOperational;
};
}
#endif
