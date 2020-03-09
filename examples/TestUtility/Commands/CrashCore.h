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
 
#pragma once

#include "../Module.h"

#include "../CommandCore/TraceCategories.h"
#include <fstream>

namespace WPEFramework {

class CrashCore {
public:
    CrashCore(const CrashCore&) = delete;
    CrashCore& operator=(const CrashCore&) = delete;

public:
    static constexpr const uint8_t DefaultCrashDelay = 3;
    static constexpr const char* PendingCrashFilepath = "/tmp/CrashCore.pending";

public:
    // ToDo: maybe error response should be generic and moved to TestCore
    class ErrorRes : public Core::JSON::Container {
    public:
        ErrorRes(const ErrorRes&) = delete;
        ErrorRes& operator=(const ErrorRes&) = delete;

    public:
        ErrorRes()
            : Core::JSON::Container()
            , ErrorMsg()
        {
            Add(_T("errorMsg"), &ErrorMsg);
        }

        ~ErrorRes() = default;

    public:
        Core::JSON::String ErrorMsg;
    };

public:
    CrashCore()
        : _lock()
    {
    }

    virtual ~CrashCore() = default;

    static CrashCore& Instance()
    {
        static CrashCore _singleton;
        return (_singleton);
    }

    string Crash(const uint8_t crashDelay) const
    {
        string response = EMPTY_STRING;

        TRACE(TestCore::TestOutput, (_T("Executing crash in %d seconds"), crashDelay));
        sleep(crashDelay);

        uint8_t* tmp = nullptr;
        *tmp = 3; // segmentaion fault

        response = _T("Function should never return");

        return response;
    }

    string CrashNTimes(const uint8_t crashCount)
    {
        string response = EMPTY_STRING;

        uint8_t pendingCrashCount = PendingCrashCount();
        if (pendingCrashCount != 0) {
            response = _T("Pending crash already in progress");
        } else {
            if (!SetPendingCrashCount(crashCount)) {
                response = _T("Failed to set new pending crash count");
            } else {
                ExecPendingCrash();
            }
        }

        return response;
    }

    string ExecPendingCrash(void)
    {
        string response = _T("");
        uint8_t pendingCrashCount = PendingCrashCount();
        if (pendingCrashCount > 0) {
            pendingCrashCount--;
            if (SetPendingCrashCount(pendingCrashCount)) {
                response = Crash(DefaultCrashDelay);
            } else {
                response = _T("Failed to set new pending crash count");
                TRACE(TestCore::TestOutput, ("%s", response.c_str()));
            }
        } else {
            response = _T("No pending crash");
            TRACE(TestCore::TestOutput, ("%s", response.c_str()));
        }

        return response;
    }

    // ToDo: maybe error response should be generic and moved to TestCore
    string ErrorResponse(const string& message) const
    {
        ErrorRes responseJSON;
        string responseString = _T("");

        if (!message.empty()) {
            responseJSON.ErrorMsg = message;
            responseJSON.ToString(responseString);
        }

        return responseString;
    }

private:
    uint8_t PendingCrashCount(void) const
    {
        uint8_t pendingCrashCount = 0;
        std::ifstream pendingCrashFile;

        _lock.Lock();
        pendingCrashFile.open(PendingCrashFilepath, std::fstream::binary);
        if (pendingCrashFile.is_open()) {
            uint8_t readVal = 0;

            pendingCrashFile >> readVal;
            if (pendingCrashFile.good()) {
                pendingCrashCount = readVal;
            } else {
                TRACE(TestCore::TestOutput, (_T("Failed to read value from pendingCrashFile")));
            }
        }
        _lock.Unlock();

        return pendingCrashCount;
    }

    bool SetPendingCrashCount(const uint8_t newCrashCount)
    {
        bool status = false;
        std::ofstream pendingCrashFile;

        _lock.Lock();
        pendingCrashFile.open(PendingCrashFilepath, std::fstream::binary | std::fstream::trunc);
        if (pendingCrashFile.is_open()) {
            pendingCrashFile << newCrashCount;

            if (pendingCrashFile.good()) {
                status = true;
            } else {
                TRACE(TestCore::TestOutput, (_T("Failed to write value to pendingCrashFile ")));
            }
            pendingCrashFile.close();
        }
        _lock.Unlock();

        return status;
    }

private:
    mutable Core::CriticalSection _lock;
};

} // namespace WPEFramework
