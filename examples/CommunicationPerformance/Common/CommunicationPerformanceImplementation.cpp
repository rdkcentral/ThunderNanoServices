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

#include "CommunicationPerformance.h"

// Interface to provide an implementation for
#include <interfaces/IPerformance.h>

// Test data for Histogram2D
//#define _USE_TESTDATA
#ifdef _USE_TESTDATA
#include "TestData.h"
#endif

#include "Helpers.h"

#ifdef __cplusplus
extern "C"
{
#endif
#ifdef __cplusplus
}
#endif

#include <limits>
#include <list>
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
};

} // namespace CommunicationPerformanceHelpers

class Performance : public Exchange::IPerformance {
public :
    Performance(const Performance&) = delete;
    Performance(Performance&&) = delete;

    Performance& operator=(const Performance&) = delete;
    Performance& operator=(Performance&&) = delete;

    Performance()
        : Exchange::IPerformance()
    {}

    ~Performance() override = default;

    // IPerformance interface methods
    // ------------------------------

    // Register for COM_RPC notifications 
    uint32_t Register(VARIABLE_IS_NOT_USED IPerformance::INotification* sink) override
    {
        return Core::ERROR_UNAVAILABLE;
    }

    // Unregister for COM_RPC notifications 
    uint32_t Unregister(VARIABLE_IS_NOT_USED IPerformance::INotification* sink) override
    {
        return Core::ERROR_UNAVAILABLE;
    }

    uint32_t Send(VARIABLE_IS_NOT_USED const uint16_t sendSize, VARIABLE_IS_NOT_USED const uint8_t buffer[]) override
    {
        return Core::ERROR_NONE;
    }

    uint32_t Receive(VARIABLE_IS_NOT_USED uint16_t& bufferSize, VARIABLE_IS_NOT_USED uint8_t buffer[]) const override
    {
        return Core::ERROR_NONE;
    }

    uint32_t Exchange(VARIABLE_IS_NOT_USED uint16_t& bufferSize, VARIABLE_IS_NOT_USED uint8_t buffer[], VARIABLE_IS_NOT_USED const uint16_t maxBufferSize) override
    {
        // Keep it simple and just 'echo' the result
        return Core::ERROR_NONE;
    }

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
        ~Distribution2D()
        {
            Print();
            Clear();
        }

        bool Insert(const std::pair<A, B>& data)
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

        void Clear()
        {
            _lock.Lock();

            static_cast<DERIVED*>(this)->DoClear();

            _lock.Unlock();
        }

        // The average (mean) is not (always) equal to median
        const std::pair<A, B>& Average() const
        {
            return _average;
        }

        const std::pair<A, B>& Maximum() const
        {
            return _maximum;
        }

        const std::pair<A, B>& Minimum() const
        {
            return _minimum;
        }

        size_t Count() const
        {
            return _count;
        }

        void Print() const
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
    protected:
        Distribution2D()
            : _average{ std::pair<A, B>(0, 0) }
            , _remainder{ std::pair<A, B>(0, 0) }
            , _sign{ std::pair<bool, bool>(false, false) }
            , _maximum{ std::pair<A, B>(0, 0) }
            , _minimum{ std::pair<A, B>(std::numeric_limits<A>::max(), std::numeric_limits<B>::max()) }
            , _count{ 0 }
            , _lock{}
        {}
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
        Average(TYPE value, TYPE average, TYPE& remainder, bool& negative = false)
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
        void DoInsert(const std::pair<A, B>& data)
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

        void DoClear()
        {
            for(auto& item : _bins) {
                item.fill(0);
            }
        }

        void DoPrint() const
        {
            // Display contents

            uint64_t width = 1;

            for (auto& item : _bins) {
                for (auto value : item) {
                    width = std::max(width, value % 10);
                }
            }

            // setw only accepts int
            using common_t = std::common_type<int, uint64_t>::type;
            ASSERT((width + 2) <= static_cast<common_t>(std::numeric_limits<int>::max()));

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
    private:
        template<typename DISTRIBUTION>
        friend DISTRIBUTION& Measurements<A,B>::Instance();
        Histogram2D()
            : Distribution2D<Histogram2D<N,UPPER_BOUND_A, UPPER_BOUND_B, LOWER_BOUND_A, LOWER_BOUND_B>>{}
            , _bins{}
        {}

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
    static DISTRIBUTION& Instance()
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

private:
};

template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
class COMRPCClient : public Core::IReferenceCounted {
public:
    COMRPCClient() = delete;
    COMRPCClient(const COMRPCClient&) = delete;
    COMRPCClient(COMRPCClient&&) = delete;

    COMRPCClient& operator=(const COMRPCClient&) = delete;
    COMRPCClient& operator=(COMRPCClient&&) = delete;

    COMRPCClient(const std::string& remoteNode)
        : _remoteNode{ remoteNode.c_str() }
        , _messageHandler{}
        , _communicatorClient{}
        , _lock{}
        , _performanceImplementation{ nullptr }
        , _referenceCount{ 0 }
    {}

    ~COMRPCClient() // no override !
    {
        ASSERT(_referenceCount == 0);
        /* uint32_t result = */ Stop(Core::infinite);
    }

    uint32_t Start(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_GENERAL;

        if (_communicatorClient.IsValid() != true) {
            _messageHandler = Core::ProxyType<RPC::InvokeServerType<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>>::Create();

            if (_messageHandler.IsValid() != false) {
                _communicatorClient = Core::ProxyType<RPC::CommunicatorClient>::Create(_remoteNode, Core::ProxyType<Core::IIPCServer>(_messageHandler));
            }
        }

        if (_communicatorClient.IsValid() != false) {
            result = Open(waitTime);
        }

        return result;
    }

    uint32_t Stop(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_GENERAL;

        /* uint32_t */ Revoke(waitTime);

        ASSERT(_performanceImplementation == nullptr);

        if (   _communicatorClient.IsValid() != false
            && (result = Close(waitTime)) == Core::ERROR_NONE
           ) {
            result = _communicatorClient.Release();
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
        } 

        if (   _communicatorClient.IsValid() != true
            && _messageHandler.IsValid() != false
           ) {
            result = _messageHandler.Release();
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
        }

        return result;
    }

    // This implementation is asked for a handle to the requested interface
    Performance* Acquire(uint32_t waitTime)
    {
        _lock.Lock();

        ASSERT(_performanceImplementation != nullptr);

        _lock.Unlock();

        return _performanceImplementation;
    }

    // RPC::CommunicatorClient methods
    // -------------------------------

    // Offer the remote our implementation of the interface, e.g., inform a server this is implementing the given interface
    uint32_t Offer(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_GENERAL;

        // Replace any exisitng interface
        /* uint32_t */ Revoke(waitTime);
 
        _lock.Lock();

        ASSERT(_performanceImplementation == nullptr);

        // Implicit 2x AddRef (?)
        _performanceImplementation = Core::ServiceType<Performance>::Create<Performance>();

        if (   _performanceImplementation != nullptr
            && _communicatorClient.IsValid() != false
            ) {
            constexpr uint32_t version = static_cast<uint32_t>(~0);
            // Implicit AddRef
            result = _communicatorClient->Offer<Exchange::IPerformance>(_performanceImplementation, version, waitTime);
        }

        _lock.Unlock();

        return result;
    }

    // Revoke the available (earlier) offered interface
    uint32_t Revoke(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_GENERAL;

        _lock.Lock();

        if (   _communicatorClient.IsValid() != false
            && _performanceImplementation != nullptr
        ) {
            constexpr uint32_t version = static_cast<uint32_t>(~0);

            // No implicit Release
            result = _communicatorClient->Revoke<Exchange::IPerformance>(_performanceImplementation, version, waitTime);
            ASSERT(result == Core::ERROR_NONE);

            // The missing Release
            result = _performanceImplementation->Release();
            ASSERT((result == Core::ERROR_NONE) || (result == Core::ERROR_DESTRUCTION_SUCCEEDED));
        }

        // Release for the implicit AddRef of Create()
        if (_performanceImplementation != nullptr
            && (    result == Core::ERROR_DESTRUCTION_SUCCEEDED
                || (result = _performanceImplementation->Release()) == Core::ERROR_DESTRUCTION_SUCCEEDED
               )
           ) {
            _performanceImplementation = nullptr;
        }

        ASSERT(_performanceImplementation == nullptr);

        _lock.Unlock();

        return result;
    }

    // Core::IReferenceCounted methods
    // -------------------------------
 
    uint32_t AddRef() const override
    {
        // Without locking it may spuriously fail

        uint32_t result = Core::ERROR_GENERAL;

        uint32_t expected = _referenceCount;

        if (    expected < std::numeric_limits<uint32_t>::max()
            && _referenceCount.compare_exchange_strong(expected, (expected + 1))
        ) {
            result = Core::ERROR_NONE;
        }

        return result;
    }

    uint32_t Release() const override
    {
        // Without locking it may spuriously fail

        uint32_t result = Core::ERROR_GENERAL;

        uint32_t expected = _referenceCount;

        if (    expected > 0
            && _referenceCount.compare_exchange_strong(expected, (expected - 1))
        ) {
            result = Core::ERROR_NONE;
        }
 
        return result;
    }

private:

    Core::NodeId _remoteNode;

    Core::ProxyType< RPC::InvokeServerType<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS> > _messageHandler;

    Core::ProxyType<RPC::CommunicatorClient> _communicatorClient;

    mutable Core::CriticalSection _lock;

    Performance* _performanceImplementation;

    mutable std::atomic<uint32_t> _referenceCount;

    uint32_t Open(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_NONE;

        if (_communicatorClient->IsOpen() != true) {
            if (   (result = _communicatorClient->Open(waitTime)) != Core::ERROR_NONE
                || _communicatorClient->IsOpen() != true
               ) {
                result = Core::ERROR_CONNECTION_CLOSED;
            }
        }

        return result;
    }

    uint32_t Close(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_NONE;

        if (_communicatorClient->IsOpen() != false) {
            if (   (result = _communicatorClient->Close(waitTime)) != Core::ERROR_NONE
                && _communicatorClient->IsOpen() != false
               ) {
                result = Core::ERROR_CLOSING_FAILED;
            }
        }

        return result;
    }
};

// A single server to serve one to N clients
template <const uint8_t THREADPOOLCOUNT, const uint32_t STACKSIZE, const uint32_t MESSAGESLOTS>
class COMRPCServer {
public :
    COMRPCServer() = delete;
    COMRPCServer(const COMRPCServer&) = delete;
    COMRPCServer(COMRPCServer&&) = delete;

    COMRPCServer& operator=(const COMRPCServer&) = delete;
    COMRPCServer& operator=(COMRPCServer&&) = delete;

    COMRPCServer(const std::string& remoteNode, const string& proxyStubPath, const std::string& sourceName)
        : _remoteNode{ static_cast<const TCHAR*>(remoteNode.c_str()) }
        , _proxyStubPath{ proxyStubPath }
        , _sourceName{ sourceName }
        , _messageHandler{}
        , _communicator{}
        , _lock{}
    {}

    ~COMRPCServer()
    {
        /* uint32_t result = */ Stop(Core::infinite);
    }

    uint32_t Start(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_GENERAL;

        if (_communicator.IsValid() != true) {
            _messageHandler = Core::ProxyType<RPC::InvokeServerType<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS>>::Create();

            if (_messageHandler.IsValid() != false) {
                _communicator = Core::ProxyType<Communicator>::Create(_remoteNode, _proxyStubPath, Core::ProxyType<Core::IIPCServer>(_messageHandler), static_cast<const TCHAR*>(_sourceName.c_str()));
            }
        }

        if (_communicator.IsValid() != false) {
            result = Open(waitTime);
        }

        return result;
    }

    uint32_t Stop(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_GENERAL;

        if (   _communicator.IsValid() != false
            && (result = Close(waitTime)) == Core::ERROR_NONE
           ) {
            _communicator->Revoke(static_cast<Core::IUnknown*>(_communicator->Acquire(_T(""), Exchange::IPerformance::ID, ~0)), Exchange::IPerformance::ID);

            result = _communicator.Release();
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
        }

        if (   _communicator.IsValid() != true
            && _messageHandler.IsValid() != false
           ) {
            result = _messageHandler.Release();
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
        }

        return result;
    }

    uint32_t Exchange(uint8_t buffer[], uint16_t bufferSize, uint16_t bufferMaxSize, uint64_t& duration) const
    {
        uint32_t result = Core::ERROR_GENERAL;

            Exchange::IPerformance* interface = nullptr;

        if (   _communicator.IsValid() != false
            // Cast to the interface type and NOT the derived class type!
            && (interface = static_cast<Exchange::IPerformance*>(_communicator->Acquire("", Exchange::IPerformance::ID, ~0))) != nullptr
           ) {
            Core::StopWatch timer;

            /* uint64_t */ timer.Reset();

            result = interface->Exchange(bufferSize, buffer, bufferMaxSize);

            duration = timer.Elapsed();
        } else {
            duration = 0;
        }

        return result;
    }

private :

    class Communicator : public RPC::Communicator {
    public :
        Communicator() = delete;
        Communicator(const Communicator&) = delete;
        Communicator(Communicator&&) = delete;

        Communicator& operator=(const Communicator&) = delete;
        Communicator& operator=(Communicator&&) = delete;

        Communicator(const Core::NodeId& remoteNode, const std::string& proxyStubPath, const Core::ProxyType<Core::IIPCServer>& messageHandler, const TCHAR* sourceName)
            : RPC::Communicator{ remoteNode, proxyStubPath, messageHandler, sourceName }
            , _lock{}
            , _performanceInterface{ nullptr }
        {}

        ~Communicator()
        {
            /* uint32_t */ Revoke(static_cast<Core::IUnknown*>(_performanceInterface), Exchange::IPerformance::ID);

            Lock();

            ASSERT(_performanceInterface == nullptr);

            Unlock();
        }

        void Lock() const
        {
            _lock.Lock();
        }

        void Unlock() const
        {
           _lock.Unlock();
        }

        // RPC::Communicator methods
        // -------------------------

        // This implementation is asked for a handle to the requested interface
        void* Acquire(VARIABLE_IS_NOT_USED const string& className, const uint32_t interfaceId, VARIABLE_IS_NOT_USED const uint32_t version) override
        {
            void* result = nullptr;

            if (interfaceId == Exchange::IPerformance::ID) {
                result = _performanceInterface;
            }

            return result;
        }

    private:

        friend uint32_t COMRPCServer::Stop(uint32_t);

        mutable Core::CriticalSection _lock;

        Exchange::IPerformance* _performanceInterface;

        // The remote offered an interface
        void Offer(Core::IUnknown* object, const uint32_t interfaceId) override 
        {
            if (interfaceId == Exchange::IPerformance::ID) {
                // Obtain the interface from the service object representing the remote side
                if (object != nullptr) {
                    // A new interface has been offered; drop the old one
                    // The remote could invoke it as well, re-use what is available
                    Revoke(object, interfaceId);

                    Lock();

                    _performanceInterface = object->QueryInterface<Exchange::IPerformance>();

                    ASSERT(_performanceInterface != nullptr);

                    Unlock();
                }
            }
        }

        // The remote revoked an interface
        void Revoke(const Core::IUnknown* object, const uint32_t interfaceId) override
        {
            // Performance the object representing the remote side before releasing the interface
            if (interfaceId == Exchange::IPerformance::ID) {
                Lock();

                if (   object != nullptr
                    // The examples show that not the same object as in Offer is an input parameter, but the result of QueryInterface
                    && object == _performanceInterface
                ) {
                    uint32_t result = _performanceInterface->Release();
                    ASSERT((result == Core::ERROR_NONE) || (result == Core::ERROR_DESTRUCTION_SUCCEEDED));

                    _performanceInterface = nullptr;
                }

                Unlock();
            }
        }

        // The remote probably forgot to revoke the interface 
        void Dangling(const Core::IUnknown* object, const uint32_t interfaceId) override
        {
            if (interfaceId == Exchange::IPerformance::ID) {
                Revoke(object, interfaceId);
            }
        }
    };

    Core::NodeId _remoteNode;

    const std::string _proxyStubPath;
    const std::string _sourceName;

    Core::ProxyType< RPC::InvokeServerType<THREADPOOLCOUNT, STACKSIZE, MESSAGESLOTS> > _messageHandler;

    Core::ProxyType<Communicator> _communicator;

    mutable Core::CriticalSection _lock;

    uint32_t Open(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_NONE;

        if (_communicator->IsListening() != true) {
            if (!(   (result = _communicator->Open(waitTime)) == Core::ERROR_NONE
                  && _communicator->IsListening()
                 )
            ) {
                result = Core::ERROR_CONNECTION_CLOSED;
            }
        }

        return result;
    }

    uint32_t Close(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_NONE;

        if (_communicator->IsListening() == true) {
            if (!(   (result = _communicator->Close(waitTime)) == Core::ERROR_NONE
                  && !_communicator->IsListening()
                )
            ) {
                result = Core::ERROR_CLOSING_FAILED;
            } else {
            }
        }

        return result;
    }
};

// The 'out-of-process' part if supported and configured
template <typename T>
class SimplePluginImplementation : public Exchange::ISimplePlugin, public Core::Thread {
public :
    SimplePluginImplementation(const SimplePluginImplementation&) = delete;
    SimplePluginImplementation(SimplePluginImplementation&&) = delete;

    SimplePluginImplementation& operator=(const SimplePluginImplementation&) = delete;
    SimplePluginImplementation& operator=(SimplePluginImplementation&&) = delete;

    // SERVICE_REGISTRATION requires a constructor that takes no arguments
    SimplePluginImplementation()
        : Exchange::ISimplePlugin{}
        , Core::Thread{ Core::Thread::DefaultStackSize(), "SimplePluginImplementation" }
        , _lock{}
        , _notifyees{}
        , _state{ STATE::IDLE }
        // Load new (proxystub) libraries present in the proxyStubPath directory at runtime
    {}

    ~SimplePluginImplementation() override
    {
        /* uint32_t */ ServiceStop(1000);
    }

    // ISimplePlugin interface methods
    // -------------------------------

    // A client / user / notifyee implements ISimplePlugin::INotification::Event(..)
    uint32_t Register(ISimplePlugin::INotification* notifyee) override
    {
        uint32_t result = Core::ERROR_INVALID_PARAMETER;

        _lock.Lock();

        // Avoid duplicates
        if (   notifyee != nullptr
            && std::find(_notifyees.begin(), _notifyees.end(), notifyee) == _notifyees.end()
        ) {
            result = notifyee->AddRef();
            ASSERT(result == Core::ERROR_NONE);

            _notifyees.push_back(notifyee);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t Unregister(ISimplePlugin::INotification* notifyee) override
    {
        uint32_t result = Core::ERROR_INVALID_PARAMETER;

         _lock.Lock();
// TODO: forward_list
        std::list<INotification*>::const_iterator item;

         _lock.Lock();

        // notifyee should be registered
        if (   notifyee != nullptr
            && (item = std::find(_notifyees.begin(), _notifyees.end(), notifyee)) != _notifyees.end()
        ) {
            /* 'iterator' */ _notifyees.erase(item);

            result = notifyee->Release();
            ASSERT(result == Core::ERROR_NONE);
        }

        _lock.Unlock();

        return result;
    }

    uint32_t DoSomething() override
    {
        uint32_t result = Core::ERROR_NONE;

        NotifyAll("DoSomeThing", 0);

        return result;
    }

    uint32_t ServiceStart(uint32_t waitTime) override
    {
        uint32_t result = Core::ERROR_GENERAL;

        if ((result = static_cast<T*>(this)->Start(waitTime)) == Core::ERROR_NONE) {
            Core::Thread::Run();

            while(!Core::Thread::Wait(Core::Thread::RUNNING, waitTime)) {
            }

            NotifyAll("ServiceStart successful", 0);
        } else {
            NotifyAll("ServiceStart incomplete", 0);
        }

        return result;
    }

    uint32_t ServiceStop(uint32_t waitTime) override
    {
        uint32_t result = Core::ERROR_GENERAL;

        Core::Thread::Stop();

        while(!Core::Thread::Wait(Thread::STOPPED | Core::Thread::BLOCKED, waitTime)) {
        }

        if ((result = static_cast<T*>(this)->Stop(waitTime)) == Core::ERROR_NONE) {
            NotifyAll("ServiceStop successful", 0);
        } else {
            NotifyAll("ServiceStop incomplete", 0);
        }

        return result;
   }

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
    std::list<Exchange::ISimplePlugin::INotification*> _notifyees;

    STATE _state;

    // (Custom) class methods
    // ----------------------

    void NotifyAll(const std::string& eventDesscription, VARIABLE_IS_NOT_USED uint32_t eventValue) const
    {
        _lock.Lock();

        for_each(_notifyees.begin(), _notifyees.end(), 
                 [&](INotification* notifyee)
                 {
                    notifyee->LifeChangingEvent(eventDesscription);
                 }
        );

        _lock.Unlock();
    }

    // Core::Thread methods
    // --------------------

    uint32_t Worker() override
    {
        // Go at lightning speed or slower after a callee update, in 'blocking' mode
        uint32_t waitTime = 0;

        VARIABLE_IS_NOT_USED uint32_t result = static_cast<T*>(this)->Task(_state, waitTime);

        // Threads 'wait' until a 'runnable' state or timeout on the given value, effectively delaying execution
        if (_state == STATE::STOP || _state == STATE::ERROR) {
            Block();
        }

        // Schedule next iteration at this interval
        return waitTime;
    }
};

class SimplePluginCOMRPCClientImplementation : public SimplePluginImplementation<SimplePluginCOMRPCClientImplementation> {
public :
    SimplePluginCOMRPCClientImplementation(const SimplePluginCOMRPCClientImplementation&) = delete;
    SimplePluginCOMRPCClientImplementation(SimplePluginCOMRPCClientImplementation&&) = delete;

    SimplePluginCOMRPCClientImplementation& operator=(const SimplePluginCOMRPCClientImplementation&) = delete;
    SimplePluginCOMRPCClientImplementation& operator=(SimplePluginCOMRPCClientImplementation&&) = delete;

    // SERVICE_REGISTRATION requires a constructor that takes no arguments
    SimplePluginCOMRPCClientImplementation()
        : _client{ _T("127.0.0.1:8080") /* remoteNode */ }
    {}

    ~SimplePluginCOMRPCClientImplementation()
    {
        /* uint32_t */ Stop(Core::infinite);
    }

    uint32_t Start(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_GENERAL;

        if ((result = _client.Start(waitTime)) != Core::ERROR_NONE) {
            /* uint32_t */ Stop(waitTime);
        } else {
            result = _client.Offer(waitTime);
        }

        return result;
    }

    uint32_t Stop(uint32_t waitTime)
    {
        uint32_t result = Core ::ERROR_GENERAL;

       if ((result = _client.Revoke(waitTime)) != Core::ERROR_NONE) {
            /* uint32_t */ _client.Stop(waitTime);
       } else {

// probably destruction_succeeded

            result = _client.Stop(waitTime);
       }

        return result;
    }

private :
// TODO:
    // Some arbitrary values
    COMRPCClient<1, 0, 4> _client;

    friend SimplePluginImplementation;
    uint32_t Task(VARIABLE_IS_NOT_USED STATE& state, VARIABLE_IS_NOT_USED uint32_t& waitTime)
    {
        return Core::ERROR_NONE;
    }
};

class SimplePluginCOMRPCServerImplementation : public SimplePluginImplementation<SimplePluginCOMRPCServerImplementation> {
public :
    SimplePluginCOMRPCServerImplementation(const SimplePluginCOMRPCServerImplementation&) = delete;
    SimplePluginCOMRPCServerImplementation(SimplePluginCOMRPCServerImplementation&&) = delete;

    SimplePluginCOMRPCServerImplementation& operator=(const SimplePluginCOMRPCServerImplementation&) = delete;
    SimplePluginCOMRPCServerImplementation& operator=(SimplePluginCOMRPCServerImplementation&&) = delete;

    // SERVICE_REGISTRATION requires a constructor that takes no arguments
    SimplePluginCOMRPCServerImplementation()
        : _server{ _T("127.0.0.1:8080") /* listeningNode */, _T("") /* proxyStubPath */, _T("") /* sourceName */}
    {}

    ~SimplePluginCOMRPCServerImplementation()
    {
        /* uint32_t */ Stop(Core::infinite);
    }

    uint32_t Start(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_GENERAL;

        if ((result = _server.Start(waitTime)) != Core::ERROR_NONE) {
            /* uint32_t */ Stop(waitTime);
        }

        static_assert(  std::is_integral<decltype(std::time(nullptr))>::value
                      , "'std::srand' requires an integral type as seed"
        );

        std::srand(std::time(nullptr));

        return result;
    }

    uint32_t Stop(uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_GENERAL;

        if ((result = _server.Stop(waitTime)) == Core::ERROR_NONE) {
        }

        return result;
    }

private :
// TODO:
    // Some arbitrary values
    COMRPCServer<1, 0, 4> _server;

#ifdef _USE_TESTDATA
    // Select series 1, 2, 3, 4 or 5
    TestData<1> _data;
#endif

    friend SimplePluginImplementation;
    uint32_t Task(STATE& state, VARIABLE_IS_NOT_USED uint32_t& waitTime)
    {
        uint32_t result = Core::ERROR_GENERAL;

PUSH_WARNING(DISABLE_WARNING_IMPLICIT_FALLTHROUGH);
        switch (state) {
        case STATE::IDLE    :   state = STATE::RUN;
        case STATE::RUN     :   state = STATE::EXECUTE;
        case STATE::EXECUTE :   {
                                // Set to a low value for quick builds
                                //constexpr uint16_t bufferMaxSize = 9999;
                                constexpr uint16_t bufferMaxSize = 999;

                                constexpr size_t numberOfBins = 10;

#ifndef _USE_TESTDATA
                                std::array<uint8_t, bufferMaxSize> buffer = CommunicationPerformanceHelpers::ConstexprArray<uint8_t, bufferMaxSize>::values;

                                // Educated guess, system dependent, required for distribution
                                constexpr uint64_t upperBoundDuration = 500;
 
                                // Round trip time
                                uint64_t duration = 0;

                                // Add some randomness

                                using common_t = std::common_type<int, uint16_t>::type;
                                const uint16_t bufferSize = static_cast<uint16_t>(static_cast<common_t>(std::rand()) % static_cast<common_t>(bufferMaxSize));

                                // With no mistakes this always holds 
                                ASSERT(bufferSize <= bufferMaxSize);

                                // This fails if the client is missing so it is not typically an error
                                if (_server.Exchange(buffer.data(), bufferSize, bufferMaxSize, duration) == Core::ERROR_NONE) {
                                    using measurements_t = Measurements<uint16_t, uint64_t>;
                                    using distribution_t = measurements_t::Histogram2D<numberOfBins, bufferMaxSize, upperBoundDuration>;

                                    distribution_t& measurements = measurements_t::Instance<distribution_t>();

                                    // Do not continue if values cannot be inserted without error (remainder too large, numerical instability)
                                    if (/* data corruption
                                        || */ measurements.Insert(std::pair<uint16_t, uint64_t>(bufferSize, duration)) != true) {
                                        state = STATE::STOP;
                                    }
                                }
#else
                                std::pair<uint16_t, uint64_t> element{ 0, 0 };

                                if (_data.Next(element)!= false) {
                                    uint16_t& bufferSize = element.first;
                                    uint64_t& duration = element.second;

                                    using measurements_t = Measurements<uint16_t, uint64_t>;
                                    // Educated guess
                                    constexpr uint64_t upperBoundDuration = 500;
                                    using distribution_t = measurements_t::Histogram2D<numberOfBins, bufferMaxSize, upperBoundDuration>;

                                    distribution_t& measurements = measurements_t::Instance<distribution_t>();

                                    if (measurements.Insert(std::pair<uint16_t, uint64_t>(bufferSize, duration)) != true) {
                                        state = STATE::STOP;
                                    }
                                }
#endif
                                }
                                break;
        case STATE::STOP    :
        case STATE::ERROR   :   waitTime = Core::infinite;
        default             :   ;
        }
POP_WARNING();

        return result;
    }
};

// Inform Thunder this out-of-service (module) implements this service (class)
// Arguments are module (service) name, major minor and optional patch version
// Use after the service has been defined / declared
SERVICE_REGISTRATION(SimplePluginCOMRPCServerImplementation, 1, 0);
SERVICE_REGISTRATION(SimplePluginCOMRPCClientImplementation, 1, 0);

} } // namespace Thunder::Plugin
