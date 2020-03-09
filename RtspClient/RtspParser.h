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

#ifndef RTSPPARSER_H
#define RTSPPARSER_H

#include <map>
#include <string>

#include "RtspCommon.h"
#include "RtspSessionInfo.h"

namespace WPEFramework {
namespace Plugin {

    typedef std::map<std::string, std::string> NAMED_ARRAY;

    class RtspParser {
    public:
        RtspParser(RtspSessionInfo& sessionInfo);
        RtspMessagePtr BuildSetupRequest(const std::string& server, const std::string& assetId);
        RtspMessagePtr BuildPlayRequest(float scale = 1.0, uint32_t position = 0);
        RtspMessagePtr BuildGetParamRequest(bool bSRM);
        RtspMessagePtr BuildTeardownRequest(int reason);
        RtspMessagePtr BuildResponse(int seq, bool bSRM);

        void ProcessSetupResponse(const std::string& response);
        void ProcessPlayResponse(const std::string& response);
        void ProcessGetParamResponse(const std::string& response);
        void ProcessTeardownResponse(const std::string& response);

        void Parse(const std::string& str, NAMED_ARRAY& contents, const string& sep1, const string& sep2);
        RtspMessagePtr ParseResponse(const std::string str);
        RtspMessagePtr ParseAnnouncement(const std::string& response, bool bSRM);

        static void HexDump(const char* label, const std::string& msg, uint16_t charsPerLine = 32);

    private:
        void UpdateNPT(NAMED_ARRAY& playMap);
        int Split(const string& str, const string& delim, std::vector<string>& tokens);

    public:
        RtspSessionInfo& _sessionInfo;

    private:
        static constexpr const char* const RtspLineTerminator = "\r\n";
        static unsigned int _sequence;
    };
}
} // WPEFramework::Plugin

#endif
