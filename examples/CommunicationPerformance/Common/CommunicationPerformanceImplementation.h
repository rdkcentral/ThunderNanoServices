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

// Interface to provide an implementation for
#include <interfaces/IPerformance.h>
#include <interfaces/ISimplePlugin.h>

#include "Helpers.h"

#ifdef __cplusplus
extern "C"
{
#endif
#ifdef __cplusplus
}
#endif

#include <limits>
#include <forward_list>
#include <utility>
#include <iomanip>
#include <cstdlib>
#include <cstring>

namespace Thunder {
namespace Plugin {

namespace CommunicationPerformanceHelpers {

// User defined definition
template<typename TYPE, size_t... N>
constexpr TYPE ConstexprArray<TYPE, 0, N...>::func(size_t n)
{
    // (Integral) TYPE may not have the same range as size_t
    // Use overflow for arbitrary values

    return static_cast<TYPE>(n);
}

} // namespace CommunicationPerformanceHelpers

template<typename TYPE>
class Performance : public Exchange::IPerformance {
public :
    Performance(const Performance&) = delete;
    Performance(Performance&&) = delete;

    Performance& operator=(const Performance&) = delete;
    Performance& operator=(Performance&&) = delete;

    Performance();

    ~Performance() override = default;

    // IPerformance interface methods
    // ------------------------------

    // Register for COMRPC notifications 
    uint32_t Register(IPerformance::INotification* sink) override;

    // Unregister for COMRPC notifications
    uint32_t Unregister(IPerformance::INotification* sink) override;

    uint32_t Send(const uint16_t sendSize, const uint8_t buffer[]) override;

    uint32_t Receive(uint16_t& bufferSize, uint8_t buffer[]) const override;

    uint32_t Exchange(uint16_t& bufferSize, uint8_t buffer[], const uint16_t maxBufferSize) override;

    BEGIN_INTERFACE_MAP(Performance)
        INTERFACE_ENTRY(Exchange::IPerformance)
    END_INTERFACE_MAP
};

template<typename A, typename B>
class Measurements {
public:

    template<typename DERIVED>
    class Distribution2D {
    public:
        ~Distribution2D();

        bool Insert(const std::pair<A, B>& data);

        void Clear();

        // The average (mean) is not (always) equal to median
        const std::pair<A, B>& Average() const;

        const std::pair<A, B>& Maximum() const;

        const std::pair<A, B>& Minimum() const;

        size_t Count() const;

        void Print() const;
    protected:
        Distribution2D();
    private:
        std::pair<A, B> _average;
        std::pair<A, B> _remainder;
        std::pair<bool, bool> _sign;
        std::pair<A, B> _maximum;
        std::pair<A, B> _minimum;

        size_t _count;

        mutable Core::CriticalSection _lock;

        // Algorithm has only been analysed for unsigned integral types, in particular integer values in the range 0 to max
        template<typename TYPE>
        typename std::enable_if<std::is_unsigned<TYPE>::value && std::is_integral<TYPE>::value, TYPE>::type
        Average(TYPE value, TYPE average, TYPE& remainder, bool& negative = false);
    };

    template<size_t N, A UPPER_BOUND_A = std::numeric_limits<A>::max(), B UPPER_BOUND_B = std::numeric_limits<B>::max(), A LOWER_BOUND_A = 0, B LOWER_BOUND_B = 0>
    class Histogram2D : public Distribution2D<Histogram2D<N, UPPER_BOUND_A, UPPER_BOUND_B, LOWER_BOUND_A, LOWER_BOUND_B>> {
        friend Distribution2D<Histogram2D<N, UPPER_BOUND_A, UPPER_BOUND_B, LOWER_BOUND_A, LOWER_BOUND_B>>;
    public:
        Histogram2D(const Histogram2D&) = delete;
        Histogram2D(Histogram2D&&) = delete;

        Histogram2D& operator=(const Histogram2D&) = delete;
        Histogram2D& operator=(Histogram2D&&) = delete;

        ~Histogram2D() = default;

    protected:
        void DoInsert(const std::pair<A, B>& data);

        void DoClear();

        void DoPrint() const;
    private:
        template<typename DISTRIBUTION>
        friend DISTRIBUTION& Measurements<A,B>::Instance();
        Histogram2D();

        std::array<std::array<uint64_t, N>, N> _bins;
    };

    Measurements() = delete;

    Measurements(const Measurements&) = delete;
    Measurements(Measurements&&) = delete;

    Measurements& operator=(const Measurements&) = delete;
    Measurements& operator=(Measurements&&) = delete;

    ~Measurements() = default;

    // The 'creator'
    template<typename DISTRIBUTION>
    static DISTRIBUTION& Instance();
private:
};

// The 'out-of-process' part if supported and configured
template <typename DERIVED>
class SimplePluginImplementation : public Exchange::ISimplePlugin {
public :
    SimplePluginImplementation(const SimplePluginImplementation&) = delete;
    SimplePluginImplementation(SimplePluginImplementation&&) = delete;

    SimplePluginImplementation& operator=(const SimplePluginImplementation&) = delete;
    SimplePluginImplementation& operator=(SimplePluginImplementation&&) = delete;

    // SERVICE_REGISTRATION requires a constructor that takes no arguments
    SimplePluginImplementation();

    ~SimplePluginImplementation() override;

    // ISimplePlugin interface methods
    // -------------------------------

    // A client / user / notifyee implements ISimplePlugin::INotification::Event(..)
    uint32_t Register(ISimplePlugin::INotification* notifyee) override;

    uint32_t Unregister(ISimplePlugin::INotification* notifyee) override;

    uint32_t DoSomething() override;

    uint32_t ServiceStart(uint32_t waitTime) override;

    uint32_t ServiceStop(uint32_t waitTime) override;

    // IWorkerPool::JobType methods
    // ----------------------------

    void Dispatch();

    // Implement the QueryInterface
    BEGIN_INTERFACE_MAP(SimplePluginImplementation)
        // Notifees are called on operations but they have to register first
        INTERFACE_ENTRY(Exchange::ISimplePlugin)
    END_INTERFACE_MAP

protected :
    enum class STATE : uint8_t { IDLE, RUN, EXECUTE, STOP, ERROR };

private :

    mutable Core::CriticalSection _lock;

    // List of sinks / clients to be called on operations
    // SimplePluginCOMRPC only registers one which is itself
    std::forward_list<Exchange::ISimplePlugin::INotification*> _notifyees;

    STATE _state;

    Core::IWorkerPool::JobType<SimplePluginImplementation&> _job;

    // (Custom) class methods
    // ----------------------

    void NotifyAll(const std::string& eventDesscription, uint32_t eventValue) const;
};

} } // namespace Thunder::Plugin
