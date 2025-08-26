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

//#include "CommunicationPerformance.h"

#include "CommunicationPerformanceImplementation.h"

namespace Thunder {
namespace Plugin {

template<typename TYPE>
Performance<TYPE>::Performance()
    : Exchange::IPerformance()
{}

template<typename TYPE>
uint32_t Performance<TYPE>::Register(VARIABLE_IS_NOT_USED IPerformance::INotification* sink)
{
    return Core::ERROR_UNAVAILABLE;
}

template<typename TYPE>
uint32_t Performance<TYPE>::Unregister(VARIABLE_IS_NOT_USED IPerformance::INotification* sink)
{
    return Core::ERROR_UNAVAILABLE;
}

template<typename TYPE>
uint32_t Performance<TYPE>::Send(VARIABLE_IS_NOT_USED const uint16_t sendSize, VARIABLE_IS_NOT_USED const uint8_t buffer[])
{
    return Core::ERROR_NONE;
}

template<typename TYPE>
uint32_t Performance<TYPE>::Receive(VARIABLE_IS_NOT_USED uint16_t& bufferSize, VARIABLE_IS_NOT_USED uint8_t buffer[]) const
{
    return Core::ERROR_NONE;
}

template<typename TYPE>
uint32_t Performance<TYPE>::Exchange(VARIABLE_IS_NOT_USED uint16_t& bufferSize, VARIABLE_IS_NOT_USED uint8_t buffer[], VARIABLE_IS_NOT_USED const uint16_t maxBufferSize)
{
    // Keep it simple and just 'echo' the result
    return Core::ERROR_NONE;
}


template<typename A, typename B>
template<typename DERIVED>
Measurements<A, B>::Distribution2D<DERIVED>::~Distribution2D()
{
    Print();
    Clear();
}

template<typename A, typename B>
template<typename DERIVED>
bool Measurements<A, B>::Distribution2D<DERIVED>::Insert(const std::pair<A, B>& data)
{
    _lock.Lock();

    bool result{ false };

    // For large n the remainder becomes dominant and may overflow (triggering the ASSERT) 
    if ( (result =    data.first <= ( std::numeric_limits<A>::max() - _remainder.first )
                   && data.second <= ( std::numeric_limits<B>::max() - _remainder.second )
                   && _count < std::numeric_limits<size_t>::max()
         ) != false
    ) {
        ++_count;

        _average = std::pair<A, B>(Average(data.first, _average.first, _remainder.first, _sign.first), Average(data.second, _average.second, _remainder.second, _sign.second));

        _maximum = std::pair<A, B>(std::max(data.first, _maximum.first), std::max(data.second, _maximum.second));

        _minimum = std::pair<A, B>(std::min(data.first, _minimum.first), std::min(data.second, _minimum.second));

        static_cast<DERIVED*>(this)->DoInsert(data);
    }

    _lock.Unlock();

    return result;
}

template<typename A, typename B>
template<typename DERIVED>
void Measurements<A, B>::Distribution2D<DERIVED>::Clear()
{
    _lock.Lock();

    static_cast<DERIVED*>(this)->DoClear();

    _lock.Unlock();
}

template<typename A, typename B>
template<typename DERIVED>
const std::pair<A, B>& Measurements<A, B>::Distribution2D<DERIVED>::Average() const
{
    return _average;
}

template<typename A, typename B>
template<typename DERIVED>
const std::pair<A, B>& Measurements<A, B>::Distribution2D<DERIVED>::Maximum() const
{
    return _maximum;
}

template<typename A, typename B>
template<typename DERIVED>
const std::pair<A, B>& Measurements<A, B>::Distribution2D<DERIVED>::Minimum() const
{
    return _minimum;
}

template<typename A, typename B>
template<typename DERIVED>
size_t Measurements<A, B>::Distribution2D<DERIVED>::Count() const
{
    return _count;
}

template<typename A, typename B>
template<typename DERIVED>
void Measurements<A, B>::Distribution2D<DERIVED>::Print() const
{
    _lock.Lock();

    std::cout << "Distribution2D :"
              << " - Average(" << Count() << ") = (" << Average().first << ", " << Average().second << ")"
              << " - Maximum(" << Count() << ") = (" << Maximum().first << ", " << Maximum().second << ")"
              << " - Minimum(" << Count() << ") = (" << Minimum().first << ", " << Minimum().second << ")"
              << " - Remainder(" << Count() << ") = (" << _remainder.first << ", " << _remainder.second << ")"
              << std::endl;

    static_cast<const DERIVED*>(this)->DoPrint();

    _lock.Unlock();
}
 
template<typename A, typename B>
template<typename DERIVED>
Measurements<A, B>::Distribution2D<DERIVED>::Distribution2D()
    : _average{ std::pair<A, B>(0, 0) }
    , _remainder{ std::pair<A, B>(0, 0) }
    , _sign{ std::pair<bool, bool>(false, false) }
    , _maximum{ std::pair<A, B>(0, 0) }
    , _minimum{ std::pair<A, B>(std::numeric_limits<A>::max(), std::numeric_limits<B>::max()) }
    , _count{ 0 }
    , _lock{}
{}
 
template<typename A, typename B>
template<typename DERIVED>
template<typename TYPE>
typename std::enable_if<std::is_unsigned<TYPE>::value && std::is_integral<TYPE>::value, TYPE>::type
Measurements<A, B>::Distribution2D<DERIVED>::Average(TYPE value, TYPE average, TYPE& remainder, bool& negative)
{
    ASSERT(_count > 0);

    // Recursive algorithm
    // a(n) = a[n-1] + 1/n * ( v(n) ) - a(n-1) )

    // Naive compensation per iteration, not cumulative 

    if (value == average) {
        /*
        condition
        v(n) == a(n-1)

        algorithm
        a(n) = a(n-1)

        check
        none
        */
    } else {
        // et(n-1), its sign is given by negative
        TYPE et = remainder;

        if (value < average) {
            /*
            condition
            v(n) < a(n-1)

            algorithm
            a(n) = a(n-1) - ( ( a(n-1) - v(n) ) / n )

            r(0) = 0
            r(n) = ( - ( a(n) - v(n) ) ) % n

            et(0) = 0
            et(n) = -r(n) / n + e(t(n-1)

            check
            a(n-1) < ( (a(n-1) - v(n) ) / n )
            */

            ASSERT(average >= ((average - value) / _count));

            // r(n), consider it as negative value
            remainder = (average - value) % _count;

            // From the current taken branch we know sign(r(n)) == -1 

            // Test sign(et(n-1))
            if (negative != false) {
                // sign(et(n-1)) == -1

                // et(n) = -r(n) - et(n-1)
                // et(n) = - ( r(n) + et(n-1) ) 

                // Possible overflow
                ASSERT(et <= (std::numeric_limits<TYPE>::max() - remainder));

                et = et + remainder;

                negative = true;
            } else {
                // sign(et(n-1)) == 1

                // et(n) = -r(n) + et(n-1)

                if (et <= remainder) {
                    // et(n) = - ( r(n) - et(n-1) )

                    et = remainder - et;

                    negative = true;
                } else { 
                    // et(n) = -r(n) + et(n-1)

                    et = et - remainder;

                    negative = false;
                }
            }

            // a(n) = a(n-1) - ( ( a(n-1) - v(n) ) / n )
            average = average - ( ( average - value) / _count );
        } 

        if (value > average) {
            /*
            condition
            v(n) > a(n-1)

            algorithm
            a(n) = a(n-1) + ( ( v(n) - a(n-1) ) / n )

            r(0) = 0
            r(n) = (v(n) - a(n) ) % n

            et(0) = 0
            et(n) = r(n) + et(n-1)

            check
            a(n-1) > max - ( ( v(n) - a(n-1) ) / n )
            */

            ASSERT(average <= ( std::numeric_limits<TYPE>::max()) - ( average + ( ( value - average ) / _count ) ) );

            // r(n), consider it as positive value
            remainder = (value - average) % _count;

            // From the current taken branch we know sign(r(n) == 1 

            // Test sign(et(n-1))
            if (negative != false) {
                // sign(et(n-1)) == -1

                // et(n) = r(n) - et(n-1) 

                if (et <= remainder) {
                    // et(n) = r(n) - et(n-1)

                    et = remainder - et;

                    negative = false;
                } else { 
                    // et(n) = r(n) - et(n-1)
                    // et(n) = - ( -r(n) + et(n-1) )

                    et = et - remainder;

                    negative = true;
                }
            } else {
                // sign(et(n-1)) == 1

                // et(n) = r(n) + et(n-1)

                // Possible overflow
                ASSERT(et <= ( std::numeric_limits<TYPE>::max() - remainder));

                et = et + remainder;

                negative = false;
            }

            // a(n) = a(n-1) + ( ( v(n) - a(n-1) ) / n )
            average = average + ( ( value - average ) / _count );
        }

        // Correct for the introduced error
        // Use et(n) / n or et(n) / (n-1)

#ifdef _RUNNINGAVERAGE_N 
        // Here et(n) / n 
        if (_count >= 1) {
            // Future et(n-1)
            remainder = et % _count;

            if (negative != false) {
                ASSERT(average >= (et / _count));

                average = average - (et / _count);
            } else {
                ASSERT(average >= 0);
                ASSERT(average <= (std::numeric_limits<TYPE>::max() - (et / _count)));

                average = average + (et / _count);
            }
        }
#else
        // Here et(n) / (n -1) 
        if (_count >= 2) {
            // Future et(n-1)
            remainder = et % (_count - 1);

            if (negative != false) {
                ASSERT(average >= (et / (_count - 1)));

                average = average - (et / (_count - 1));
            } else {
                ASSERT(average >= 0);
                ASSERT(average <= (std::numeric_limits<TYPE>::max() - (et / _count)));

                average = average + (et / (_count - 1));
            }
        }
#endif
    }

#undef _RUNNINGAVERAGE

    return average;
}

template<typename A, typename B>
template<size_t N, A UPPER_BOUND_A, B UPPER_BOUND_B, A LOWER_BOUND_A, B LOWER_BOUND_B>
void Measurements<A, B>::Histogram2D<N, UPPER_BOUND_A, UPPER_BOUND_B, LOWER_BOUND_A, LOWER_BOUND_B>::DoInsert(const std::pair<A, B>& data)
{
// TODO: accept negative values
    ASSERT(data.first >= 0);
    ASSERT(data.second >= 0);

    static_assert(   LOWER_BOUND_A >= 0
                  && LOWER_BOUND_B >= 0
                  && LOWER_BOUND_A < UPPER_BOUND_A
                  && LOWER_BOUND_B < UPPER_BOUND_B
                 , ""
    );

    constexpr A stepSizeA = (UPPER_BOUND_A - LOWER_BOUND_A) / N;
    constexpr B stepSizeB = (UPPER_BOUND_B - LOWER_BOUND_B) / N;

    // Bound to N, array indices are N - 1
    // The integral step size is truncated eg 'rounded' to zero

    const A indexA = data.first < stepSizeA ? 0 : data.first / stepSizeA;
    const B indexB = data.second < stepSizeB ? 0 : data.second / stepSizeB;

    ASSERT(indexA <= static_cast<A>(std::numeric_limits<size_t>::max()));
    ASSERT(indexB <= static_cast<B>(std::numeric_limits<size_t>::max()));

    _bins[static_cast<size_t>(indexA) < (N - 1) ? static_cast<size_t>(indexA) : N - 1][static_cast<size_t>(indexB) < (N -1) ? static_cast<size_t>(indexB) : N - 1]++;
}

template<typename A, typename B>
template<size_t N, A UPPER_BOUND_A, B UPPER_BOUND_B, A LOWER_BOUND_A, B LOWER_BOUND_B>
void Measurements<A, B>::Histogram2D<N, UPPER_BOUND_A, UPPER_BOUND_B, LOWER_BOUND_A, LOWER_BOUND_B>::DoClear()
{
    for(auto& item : _bins) {
        item.fill(0);
    }
}

template<typename A, typename B>
template<size_t N, A UPPER_BOUND_A, B UPPER_BOUND_B, A LOWER_BOUND_A, B LOWER_BOUND_B>
void Measurements<A, B>::Histogram2D<N, UPPER_BOUND_A, UPPER_BOUND_B, LOWER_BOUND_A, LOWER_BOUND_B>::DoPrint() const
{
    // Display contents

    uint64_t width = 1;

    for (auto& item : _bins) {
        for (auto value : item) {
            width = std::max(width, value % 10);
        }
    }

    // setw only accepts int
#ifndef NDEBUG
    using common_t = std::common_type<int, uint64_t>::type;
    ASSERT((width + 2) <= static_cast<common_t>(std::numeric_limits<int>::max()));
#endif
    constexpr A stepSizeA = (UPPER_BOUND_A - LOWER_BOUND_A) / N;
    constexpr B stepSizeB = (UPPER_BOUND_B - LOWER_BOUND_B) / N;

    std::cout << std::setw(width + 3) << "";

    for (size_t column = 0; column < N; column++) {
        std::cout << std::setw(width) << column * stepSizeB << ">= ";
    }

    std::cout << std::endl;

    size_t row = 0;

    for (auto& item : _bins) {
        std::cout << std::setw(width) << stepSizeA * row << ">= ";

        for (auto value : item) {
            std::cout << std::setw(width + 2) << value << " ";
        }

        std::cout << std::endl;

        ++row;
    }
}

template<typename A, typename B>
template<size_t N, A UPPER_BOUND_A, B UPPER_BOUND_B, A LOWER_BOUND_A, B LOWER_BOUND_B>
Measurements<A, B>::Histogram2D<N, UPPER_BOUND_A, UPPER_BOUND_B, LOWER_BOUND_A, LOWER_BOUND_B>::Histogram2D()
    : Distribution2D<Histogram2D<N,UPPER_BOUND_A, UPPER_BOUND_B, LOWER_BOUND_A, LOWER_BOUND_B>>{}
    , _bins{}
{}


template<typename A, typename B>
template<typename DISTRIBUTION>
/* static */ DISTRIBUTION& Measurements<A, B>::Instance()
{
    // A distribution should be able to 'accept' types even through conversion

    using a_t = decltype(std::declval<DISTRIBUTION>().Average().first);
    using b_t = decltype(std::declval<DISTRIBUTION>().Average().second);

    using common_a_t = typename std::common_type<a_t, A>::type;
    using common_b_t = typename std::common_type<b_t, B>::type;

    static_assert(   static_cast<common_a_t>(std::numeric_limits<A>::max()) <= static_cast<common_a_t>(std::numeric_limits<a_t>::max())
                  && static_cast<common_b_t>(std::numeric_limits<B>::max()) <= static_cast<common_b_t>(std::numeric_limits<b_t>::max())
                  , ""
    );

    static DISTRIBUTION distribution;

    return distribution;
}


template <typename DERIVED>
SimplePluginImplementation<DERIVED>::SimplePluginImplementation()
    : Exchange::ISimplePlugin{}
    , _lock{}
    , _notifyees{}
    , _state{ STATE::IDLE }
    , _job{ *this }
    // Load new (proxystub) libraries present in the proxyStubPath directory at runtime
{}
template <typename DERIVED>
SimplePluginImplementation<DERIVED>::~SimplePluginImplementation()
{
    /* uint32_t */ DeinitializeTask(1000);
}

template <typename DERIVED>
uint32_t SimplePluginImplementation<DERIVED>::Register(ISimplePlugin::INotification* notifyee)
{
    uint32_t result = Core::ERROR_INVALID_PARAMETER;

    _lock.Lock();

    // Avoid duplicates
    if (   notifyee != nullptr
        && std::find(_notifyees.begin(), _notifyees.end(), notifyee) == _notifyees.end()
    ) {
        result = notifyee->AddRef();
        ASSERT(result == Core::ERROR_NONE);

        /* void */ _notifyees.emplace_front(notifyee);
    }

    _lock.Unlock();

    return result;
}

template <typename DERIVED>
uint32_t SimplePluginImplementation<DERIVED>::Unregister(ISimplePlugin::INotification* notifyee)
{
    uint32_t result = Core::ERROR_INVALID_PARAMETER;

     _lock.Lock();

    std::forward_list<INotification*>::const_iterator item;

     _lock.Lock();

    // notifyee should be registered
    if (   notifyee != nullptr
        && (item = std::find(_notifyees.begin(), _notifyees.end(), notifyee)) != _notifyees.end()
    ) {
        /* void */ _notifyees.remove(*item);

        result = notifyee->Release();
        ASSERT(result == Core::ERROR_NONE);
    }

    _lock.Unlock();

    return result;
}

template <typename DERIVED>
uint32_t SimplePluginImplementation<DERIVED>::ExecuteTask()
{
    uint32_t result = Core::ERROR_NONE;

    NotifyAll("ExecuteTask", 0);

    return result;
}

template <typename DERIVED>
uint32_t SimplePluginImplementation<DERIVED>::InitializeTask(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (   _job.Submit() != false
        && (result = static_cast<DERIVED*>(this)->Start(waitTime)) == Core::ERROR_NONE
       ) {
        NotifyAll("InitializeTask successful", 0);
    } else {
        NotifyAll("InitializeTask incomplete", 0);
    }

    return result;
}

template <typename DERIVED>
uint32_t SimplePluginImplementation<DERIVED>::DeinitializeTask(const uint32_t waitTime)
{
    uint32_t result = Core::ERROR_GENERAL;

    /* void */ _job.Revoke();
    if ((result = static_cast<DERIVED*>(this)->Stop(waitTime)) == Core::ERROR_NONE) {
        NotifyAll("DeinitializeTask successful", 0);
    } else {
        NotifyAll("DeinitializeTask incomplete", 0);
    }

    return result;
}

template <typename DERIVED>
void SimplePluginImplementation<DERIVED>::Dispatch()
{
    // Go at lightning speed or slower after a callee update
    // Educated guess
    uint32_t waitTime = 0;

    if (static_cast<DERIVED*>(this)->Task(_state, waitTime) == Core::ERROR_NONE) {
        // Only known jobs can be rescheduled, eg submitted jobs
        VARIABLE_IS_NOT_USED bool result = _job.Reschedule(Core::Time::Now().Add(waitTime /* milliseconds */));
        ASSERT(result != true);
    } else {
// TODO:
        // Error condition
    }
}

template <typename DERIVED>
void SimplePluginImplementation<DERIVED>::NotifyAll(const std::string& eventDesscription, VARIABLE_IS_NOT_USED uint32_t eventValue) const
{
    _lock.Lock();

    for_each(_notifyees.begin(), _notifyees.end(), 
             [&](INotification* notifyee)
             {
                notifyee->ProcessEvent(eventDesscription);
             }
    );

    _lock.Unlock();
}


} } // namespace Thunder::Plugin
