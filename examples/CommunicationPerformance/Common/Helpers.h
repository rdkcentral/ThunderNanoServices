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

namespace Thunder {
namespace Plugin {

namespace CommunicationPerformanceHelpers {

// Base case, no specialization, most genric, constant type
template<typename TYPE, size_t... N>
struct ConstexprArray
{};

// Partial specialization, constant type, splitting one element off
template<typename TYPE, size_t L, size_t... N>
struct ConstexprArray<TYPE, L, N...> : ConstexprArray<TYPE, L-1, L, N...>
{};

// Partial specialization, constant type, stop condition
template<typename TYPE, size_t... N>
struct ConstexprArray<TYPE, 0, N...>
{
    // Users should provide the definition
    constexpr static TYPE func(size_t n); 

    constexpr static std::array<TYPE, sizeof...(N)> values = {{ func(N)... }};
};

// Definition of in class declaration
template<typename TYPE, size_t... N>
constexpr /* static */ std::array<TYPE, sizeof...(N)> ConstexprArray<TYPE, 0, N...>::values;

} // namespace CommunicationPerformanceHelpers

} } // namespace Thunder::Plugin
