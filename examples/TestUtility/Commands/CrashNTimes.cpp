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
 
#include "../CommandCore/TestCommandBase.h"
#include "../CommandCore/TestCommandController.h"

#include "CrashCore.h"

namespace WPEFramework {

class CrashNTimes : public TestCommandBase {
public:
    CrashNTimes(const CrashNTimes&) = delete;
    CrashNTimes& operator=(const CrashNTimes&) = delete;

public:
    using Parameter = JsonData::TestUtility::InputInfo;

    CrashNTimes()
        : TestCommandBase(TestCommandBase::DescriptionBuilder(_T("Cause segmenation fault N times in a row")),
              TestCommandBase::SignatureBuilder(Parameter())
                  .InputParameter("crashCount", JsonData::TestUtility::TypeType::NUMBER, "how many times Crash will be executed consecutively"))
        , _crashCore(CrashCore::Instance())
        , _name(_T("CrashNTimes"))
    {
        TestCore::TestCommandController::Instance().Announce(this);

        _crashCore.ExecPendingCrash();
    }

    virtual ~CrashNTimes() { TestCore::TestCommandController::Instance().Revoke(this); }

    BEGIN_INTERFACE_MAP(CrashNTimes)
    INTERFACE_ENTRY(Exchange::ITestUtility::ICommand)
    END_INTERFACE_MAP

public:
    virtual string Execute(const string& params) final
    {
        JsonData::TestUtility::RuncrashParamsData input;
        string responseString = _T("");
        uint8_t crashCount = 0;

        if ((input.FromString(params) == true) && (input.Count.IsSet() == true)) {
            crashCount = input.Count.Value();
        }

        if (crashCount > 0) {
            responseString = _crashCore.CrashNTimes(crashCount);
        } else {
            responseString = _T("crashCount must be greater than 0");
        }

        return (_crashCore.ErrorResponse(responseString));
    }

    virtual string Name() const final { return _name; }

private:
    CrashCore& _crashCore;
    string _name;
};

static CrashNTimes* _singleton(Core::Service<CrashNTimes>::Create<CrashNTimes>());

} // namespace WPEFramework
