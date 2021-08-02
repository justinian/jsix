#pragma once
/// file basic_types.h
/// Type properties that would normally come from <type_traits>

namespace types {

template <bool B, typename T = void> struct enable_if {};
template <typename T> struct enable_if<true, T> { using type = T; };

template <typename T> struct is_integral           { static constexpr bool value = false; };
template <> struct is_integral<char>               { static constexpr bool value = true; };
template <> struct is_integral<unsigned char>      { static constexpr bool value = true; };
template <> struct is_integral<short>              { static constexpr bool value = true; };
template <> struct is_integral<unsigned short>     { static constexpr bool value = true; };
template <> struct is_integral<int>                { static constexpr bool value = true; };
template <> struct is_integral<unsigned int>       { static constexpr bool value = true; };
template <> struct is_integral<long>               { static constexpr bool value = true; };
template <> struct is_integral<unsigned long>      { static constexpr bool value = true; };
template <> struct is_integral<long long>          { static constexpr bool value = true; };
template <> struct is_integral<unsigned long long> { static constexpr bool value = true; };
 
template <typename E> struct integral           { using type = unsigned long long; };
template <> struct integral<char>               { using type = char; };
template <> struct integral<unsigned char>      { using type = unsigned char; };
template <> struct integral<short>              { using type = short; };
template <> struct integral<unsigned short>     { using type = unsigned short; };
template <> struct integral<int>                { using type = int; };
template <> struct integral<unsigned int>       { using type = unsigned int; };
template <> struct integral<long>               { using type = long; };
template <> struct integral<unsigned long>      { using type = unsigned long; };
template <> struct integral<long long>          { using type = long long; };
template <> struct integral<unsigned long long> { using type = unsigned long long; };

template <typename T, T v>
struct integral_constant
{
    using value_type = T;
    using type = integral_constant;
    static constexpr value_type value = v;
    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; }
};

using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;

template <bool B, typename T, typename F> struct conditional { using type = T; };
template <typename T, typename F> struct conditional<false, T, F> { using type = F; };

template<typename...> struct conjunction : true_type {};
template<typename B1> struct conjunction<B1> : B1 {};
template<typename B1, typename... Bn>
struct conjunction<B1, Bn...> : conditional<bool(B1::value), conjunction<Bn...>, B1>::type {};


} // namespace types
