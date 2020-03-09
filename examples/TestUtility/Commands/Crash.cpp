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

class Crash : public TestCommandBase {
public:
    Crash(const Crash&) = delete;
    Crash& operator=(const Crash&) = delete;

public:
    using Parameter = JsonData::TestUtility::InputInfo;

    Crash()
        : TestCommandBase(
              TestCommandBase::DescriptionBuilder(_T("Cause segmenation fault resulting in crash")),
              TestCommandBase::SignatureBuilder(Parameter())
                  .InputParameter("crashDelay", JsonData::TestUtility::TypeType::NUMBER, "delay in ms before actual crash"))
        , _crashCore(CrashCore::Instance())
        , _name(_T("Crash"))
    {
        TestCore::TestCommandController::Instance().Announce(this);
    }

    virtual ~Crash() { TestCore::TestCommandController::Instance().Revoke(this); }

    BEGIN_INTERFACE_MAP(Crash)
    INTERFACE_ENTRY(Exchange::ITestUtility::ICommand)
    END_INTERFACE_MAP

public:
    virtual string /*JSON*/ Execute(const string& params) final
    {
        JsonData::TestUtility::RuncrashParamsData input;
        uint8_t crashDelay = CrashCore::DefaultCrashDelay;
        string responseString = _T("");

        if ((input.FromString(params) == true) && (input.Delay.IsSet() == true)) {
            crashDelay = input.Delay.Value();
        }

        responseString = _crashCore.Crash(crashDelay);

        return (_crashCore.ErrorResponse(responseString));
    }

    virtual string Name() const final { return _name; }
private:
    CrashCore& _crashCore;
    string _name;
};

static Crash* _singleton(Core::Service<Crash>::Create<Crash>());

} // namespace WPEFramework
