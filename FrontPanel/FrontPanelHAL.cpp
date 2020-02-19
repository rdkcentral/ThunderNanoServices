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

#include "FrontPanelHAL.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(FrontPanelHAL::Indicator){ FrontPanelHAL::MESSAGE, _TXT("MESSAGE") },
    { FrontPanelHAL::POWER, _TXT("POWER") },
    { FrontPanelHAL::RECORD, _TXT("RECORD") },
    { FrontPanelHAL::REMOTE, _TXT("REMOTE") },
    { FrontPanelHAL::RFBYPASS, _TXT("RFBYPASS") },
    { FrontPanelHAL::MAX_INDICATOR, _TXT("MAX_INDICATOR") },
    ENUM_CONVERSION_END(FrontPanelHAL::Indicator)

        ENUM_CONVERSION_BEGIN(FrontPanelHAL::TextDisplay){ FrontPanelHAL::TEXT, _TXT("TEXT") },
    { FrontPanelHAL::MAX_TEXTDISP, _TXT("MAX_TEXTDISP") },
    ENUM_CONVERSION_END(FrontPanelHAL::TextDisplay)

        ENUM_CONVERSION_BEGIN(FrontPanelHAL::State){ FrontPanelHAL::STATE_OFF, _TXT("STATE_OFF") },
    { FrontPanelHAL::STATE_ON, _TXT("STATE_ON") },
    ENUM_CONVERSION_END(FrontPanelHAL::State)

        ENUM_CONVERSION_BEGIN(FrontPanelHAL::TimeFormat){ FrontPanelHAL::TWELVE_HOUR, _TXT("TWELVE_HOUR") },
    { FrontPanelHAL::TWENTY_FOUR_HOUR, _TXT("TWENTY_FOUR_HOUR") },
    { FrontPanelHAL::STRING, _TXT("STRING") },
    ENUM_CONVERSION_END(FrontPanelHAL::TimeFormat)

        ENUM_CONVERSION_BEGIN(FrontPanelHAL::Color){ FrontPanelHAL::BLUE, _TXT("BLUE") },
    { FrontPanelHAL::GREEN, _TXT("GREEN") },
    { FrontPanelHAL::RED, _TXT("RED") },
    { FrontPanelHAL::YELLOW, _TXT("YELLOW") },
    { FrontPanelHAL::ORANGE, _TXT("ORANGE") },
    { FrontPanelHAL::WHITE, _TXT("WHITE") },
    { FrontPanelHAL::MAX_COLOR, _TXT("MAX_COLOR") },
    ENUM_CONVERSION_END(FrontPanelHAL::Color)

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

bool FrontPanelHAL::GetFPState(Indicator indicator, State& state)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDIndicator_t fpdIndicator = indicatorLookup.find(indicator)->second;
    dsFPDState_t fpdState;
    if (dsGetFPState(fpdIndicator, &fpdState) == dsERR_NONE) {
        const auto index = std::find_if(stateLookup.cbegin(), stateLookup.cend(),
            [fpdState](const std::pair<const State, const dsFPDState_t>& found) { return found.second == fpdState; });

        if (index != stateLookup.cend())
            state = index->first;

        return true;
    }
    return false;
}

bool FrontPanelHAL::GetFPBrightness(Indicator indicator, uint32_t& brightness)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDIndicator_t fpdIndicator = indicatorLookup.find(indicator)->second;
    if (dsGetFPBrightness(fpdIndicator, (dsFPDBrightness_t*)&brightness) == dsERR_NONE)
        return true;
    return false;
}

bool FrontPanelHAL::GetFPColor(Indicator indicator, Color& color)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDIndicator_t fpdIndicator = indicatorLookup.find(indicator)->second;
    dsFPDColor_t fpdColor;
    if (dsGetFPColor(fpdIndicator, &fpdColor) == dsERR_NONE) {
        const auto index = std::find_if(colorLookup.cbegin(), colorLookup.cend(),
            [fpdColor](const std::pair<const Color, const dsFPDColor_t>& found) { return found.second == fpdColor; });

        if (index != colorLookup.cend())
            color = index->first;

        return true;
    }
    return false;
}

bool FrontPanelHAL::GetFPTextBrightness(TextDisplay indicator, uint32_t& brightness)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDTextDisplay_t fpdIndicator = textDisplayLookup.find(indicator)->second;
    if (dsGetFPTextBrightness(fpdIndicator, (dsFPDBrightness_t*)&brightness) == dsERR_NONE)
        return true;
    return false;
}

bool FrontPanelHAL::GetFPTimeFormat(TimeFormat& timeFormat)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDTimeFormat_t fpdTimeFormat;
    if (dsGetFPTimeFormat(&fpdTimeFormat) == dsERR_NONE) {
        const auto index = std::find_if(timeFormatLookup.cbegin(), timeFormatLookup.cend(),
            [fpdTimeFormat](const std::pair<const TimeFormat, const dsFPDTimeFormat_t>& found) { return found.second == fpdTimeFormat; });

        if (index != timeFormatLookup.cend())
            timeFormat = index->first;

        return true;
    }
    return false;
}

bool FrontPanelHAL::SetFPBlink(Indicator indicator, uint32_t uBlinkDuration, uint32_t blinkIterations)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDIndicator_t fpdIndicator = indicatorLookup.find(indicator)->second;
    if (dsSetFPBlink(fpdIndicator, uBlinkDuration, blinkIterations) == dsERR_NONE) {
        TRACE(Trace::Information, (_T("Set Front Panel Blink.")));
        return true;
    }
    return false;
}

bool FrontPanelHAL::SetFPBrightness(Indicator indicator, uint32_t brightness)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDIndicator_t fpdIndicator = indicatorLookup.find(indicator)->second;
    if (dsSetFPBrightness(fpdIndicator, (dsFPDBrightness_t)brightness) == dsERR_NONE) {
        TRACE(Trace::Information, (_T("Set Front Panel Brightness.")));
        return true;
    }
    return false;
}

bool FrontPanelHAL::SetFPState(Indicator indicator, State state)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDIndicator_t fpdIndicator = indicatorLookup.find(indicator)->second;
    dsFPDState_t fpdState = stateLookup.find(state)->second;
    if (dsSetFPState(fpdIndicator, fpdState) == dsERR_NONE) {
        TRACE(Trace::Information, (_T("Set Front Panel State.")));
        return true;
    }
    return false;
}

bool FrontPanelHAL::SetFPColor(Indicator indicator, Color color)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDIndicator_t fpdIndicator = indicatorLookup.find(indicator)->second;
    dsFPDColor_t fpdColor = colorLookup.find(color)->second;
    if (dsSetFPColor(fpdIndicator, fpdColor) == dsERR_NONE) {
        TRACE(Trace::Information, (_T("Set Front Panel Color.")));
        return true;
    }
    return false;
}

bool FrontPanelHAL::SetFPTime(TimeFormat timeFormat, uint32_t hour, uint32_t minutes)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDTimeFormat_t fpdTimeFormat = timeFormatLookup.find(timeFormat)->second;
    if (dsSetFPTime(fpdTimeFormat, hour, minutes) == dsERR_NONE) {
        TRACE(Trace::Information, (_T("Set Front Panel Time.")));
        return true;
    }
    return false;
}

bool FrontPanelHAL::SetFPText(string text)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    if (dsSetFPText(text.c_str()) == dsERR_NONE) {
        TRACE(Trace::Information, (_T("Set Front Panel Text.")));
        return true;
    }
    return false;
}

bool FrontPanelHAL::SetFPTextBrightness(TextDisplay indicator, uint32_t brightness)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDTextDisplay_t fpdIndicator = textDisplayLookup.find(indicator)->second;
    if (dsSetFPTextBrightness(fpdIndicator, (dsFPDBrightness_t)brightness) == dsERR_NONE) {
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

bool FrontPanelHAL::SetFPScroll(uint32_t scrollHoldOnDur, uint32_t horzScrollIterations, uint32_t vertScrollIterations)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    if (dsSetFPScroll(scrollHoldOnDur, horzScrollIterations, vertScrollIterations) == dsERR_NONE) {
        TRACE(Trace::Information, (_T("Set Front Panel Scroll.")));
        return true;
    }
    return false;
}

bool FrontPanelHAL::SetFPDBrightness(Indicator indicator, uint32_t brightness, bool toPersist)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDIndicator_t fpdIndicator = indicatorLookup.find(indicator)->second;
    if (dsSetFPDBrightness(fpdIndicator, (dsFPDBrightness_t)brightness, toPersist) == dsERR_NONE) {
        TRACE(Trace::Information, (_T("Set Front Panel Display Brightness.")));
        return true;
    }
    return false;
}

bool FrontPanelHAL::SetFPDColor(Indicator indicator, Color color, bool toPersist)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDIndicator_t fpdIndicator = indicatorLookup.find(indicator)->second;
    dsFPDColor_t fpdColor = colorLookup.find(color)->second;
    if (dsSetFPDColor(fpdIndicator, fpdColor, toPersist) == dsERR_NONE) {
        TRACE(Trace::Information, (_T("Set Front Panel Display Color.")));
        return true;
    }
    return false;
}

bool FrontPanelHAL::SetFPTimeFormat(TimeFormat timeFormat)
{
    TRACE(Trace::Information, (_T("%s"), __func__));
    dsFPDTimeFormat_t fpdTimeFormat = timeFormatLookup.find(timeFormat)->second;
    if (dsSetFPTimeFormat(fpdTimeFormat) == dsERR_NONE) {
        TRACE(Trace::Information, (_T("Set Front Panel Time Format.")));
        return true;
    }
    return false;
}
}
