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
#include "MemoryAllocation.h"

namespace WPEFramework {

class Malloc : public TestCommandBase {
public:
    Malloc(const Malloc&) = delete;
    Malloc& operator=(const Malloc&) = delete;

public:
    using Parameter = JsonData::TestUtility::InputInfo;

    Malloc()
        : TestCommandBase(TestCommandBase::DescriptionBuilder("Allocates desired kB in memory and holds it"),
              TestCommandBase::SignatureBuilder("memory",JsonData::TestUtility::TypeType::NUMBER, "memory statistics in KB")
                  .InputParameter("size", JsonData::TestUtility::TypeType::NUMBER, "memory in kB for allocation"))
        , _memoryAdmin(MemoryAllocation::Instance())
    {
        TestCore::TestCommandController::Instance().Announce(this);
    }

    virtual ~Malloc()
    {
        TestCore::TestCommandController::Instance().Revoke(this);
    }

public:
    // ICommand methods
    string Execute(const string& params) final
    {
        JsonData::TestUtility::RunmemoryParamsData input;
        uint32_t size;

        if ((input.FromString(params) == true) && (input.Size.IsSet())) {
            size = input.Size.Value();
            _memoryAdmin.Malloc(size);
        }
        return _memoryAdmin.CreateResponse();
    }

    string Name() const final
    {
        return _name;
    }

private:
    BEGIN_INTERFACE_MAP(Malloc)
    INTERFACE_ENTRY(Exchange::ITestUtility::ICommand)
    END_INTERFACE_MAP

private:
    MemoryAllocation& _memoryAdmin;
    const string _name = _T("Malloc");
};

static Malloc* _singleton(Core::Service<Malloc>::Create<Malloc>());

} // namespace WPEFramework
