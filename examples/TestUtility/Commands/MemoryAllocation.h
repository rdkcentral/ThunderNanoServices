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

namespace WPEFramework {

class MemoryAllocation {
public:
    MemoryAllocation(const MemoryAllocation&) = delete;
    MemoryAllocation& operator=(const MemoryAllocation&) = delete;

    MemoryAllocation()
        : _lock()
        , _process()
        , _startSize(0)
        , _startResident(0)
        , _currentMemoryAllocation(0)
        , _memory()
    {
        DisableOOMKill();
        _startSize = static_cast<uint32_t>(_process.Allocated() >> 10);
        _startResident = static_cast<uint32_t>(_process.Resident() >> 10);
    }

public:
    static MemoryAllocation& Instance()
    {
        static MemoryAllocation _singleton;
        return (_singleton);
    }

    ~MemoryAllocation() = default;

public:
    // Memory Allocation methods
    void Malloc(uint32_t size) // size in Kb
    {
        uint32_t noOfBlocks = 0;
        uint32_t blockSize = (32 * (getpagesize() >> 10)); // 128kB block size
        uint32_t runs = (uint32_t)size / blockSize;

        _lock.Lock();
        for (noOfBlocks = 0; noOfBlocks < runs; ++noOfBlocks) {
            _memory.push_back(malloc(static_cast<size_t>(blockSize << 10)));
            if (!_memory.back()) {
                SYSLOG(Logging::Fatal, (_T("*** Failed allocation !!! ***")));
                break;
            }

            for (uint32_t index = 0; index < (blockSize << 10); index++) {
                static_cast<unsigned char*>(_memory.back())[index] = static_cast<unsigned char>(rand() & 0xFF);
            }
        }
        _currentMemoryAllocation += (noOfBlocks * blockSize);
        _lock.Unlock();
    }

    bool Free(void)
    {
        bool status = false;

        if (!_memory.empty()) {
            for (auto const& memoryBlock : _memory) {
                free(memoryBlock);
            }
            _memory.clear();
            status = true;
        }

        _lock.Lock();
        _currentMemoryAllocation = 0;
        _lock.Unlock();

        return status;
    }

    void Statm(uint32_t& allocated, uint32_t& size, uint32_t& resident)
    {
        _lock.Lock();
        allocated = _currentMemoryAllocation;
        _lock.Unlock();

        size = static_cast<uint32_t>(_process.Allocated() >> 10);
        resident = static_cast<uint32_t>(_process.Resident() >> 10);

        LogMemoryUsage();
    }

    string /*JSON*/ CreateResponse()
    {
        string jsonResponse = EMPTY_STRING;

        JsonData::TestUtility::RunmemoryResultData response;
        uint32_t allocated, size, resident;

        Statm(allocated, size, resident);

        response.Allocated = allocated;
        response.Resident = resident;
        response.Size = size;
        response.ToString(jsonResponse);

        return jsonResponse;
    }

private:
    void DisableOOMKill(void)
    {
        int8_t oomNo = -17;
        _process.OOMAdjust(oomNo);
    }

    void LogMemoryUsage(void)
    {
        SYSLOG(Logging::Notification, (_T("*** Current allocated: %lu Kb ***"), _currentMemoryAllocation));
        SYSLOG(Logging::Notification, (_T("*** Initial Size:     %lu Kb ***"), _startSize));
        SYSLOG(Logging::Notification, (_T("*** Initial Resident: %lu Kb ***"), _startResident));
        SYSLOG(Logging::Notification, (_T("*** Size:     %lu Kb ***"), static_cast<uint32_t>(_process.Allocated() >> 10)));
        SYSLOG(Logging::Notification, (_T("*** Resident: %lu Kb ***"), static_cast<uint32_t>(_process.Resident() >> 10)));
    }

private:
    Core::CriticalSection _lock;
    Core::ProcessInfo _process;
    uint32_t _startSize;
    uint32_t _startResident;
    uint32_t _currentMemoryAllocation; // size in Kb
    std::list<void*> _memory;
};
} // namespace WPEFramework
