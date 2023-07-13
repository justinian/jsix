#pragma once
/// file basic_types.h
/// Type properties that would normally come from <type_traits>

#include <stdint.h>

namespace types {

template <bool B, typename T = void> struct enable_if {};
template <typename T> struct enable_if<true, T> { using type = T; };

template <typename T> struct non_const          { using type = T; };
template <typename T> struct non_const<const T> { using type = T; };

template <typename T> struct is_integral           { static constexpr bool value = false; };

#define make_is_integral(n) \
    template <> struct is_integral< n >               { static constexpr bool value = true; }; \
    template <> struct is_integral<const n >          { static constexpr bool value = true; }; \
    template <> struct is_integral<unsigned n >       { static constexpr bool value = true; }; \
    template <> struct is_integral<const unsigned n > { static constexpr bool value = true; };

make_is_integral(char);
make_is_integral(short);
make_is_integral(int);
make_is_integral(long);
make_is_integral(long long);

template <typename E> struct integral           { using type = unsigned long long; };

#define make_integral(n) \
    template <> struct integral< n >               { using type = n ; }; \
    template <> struct integral<const n >          { using type = n ; }; \
    template <> struct integral<unsigned n >       { using type = unsigned n ; }; \
    template <> struct integral<const unsigned n > { using type = unsigned n ; };

make_integral(char);
make_integral(short);
make_integral(int);
make_integral(long);
make_integral(long long);

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

template <unsigned N> struct sized_uint_type {};
template <> struct sized_uint_type<8>  { using type = uint8_t; };
template <> struct sized_uint_type<16> { using type = uint16_t; };
template <> struct sized_uint_type<32> { using type = uint32_t; };
template <> struct sized_uint_type<64> { using type = uint64_t; };

template <unsigned N> struct sized_uint {
    static constexpr uint64_t mask = ((1<<N)-1);
    static constexpr unsigned bits = N;
    using type = typename sized_uint_type<N>::type;
};

template <typename T> struct remove_reference       { using type = T; };
template <typename T> struct remove_reference<T&>   { using type = T; };
template <typename T> struct remove_reference<T&&>  { using type = T; };

} // namespace types

namespace util {

template<typename T>
typename types::remove_reference<T>::type&&
move( T&& x ) { return (typename types::remove_reference<T>::type&&)x; }

template<typename T> T&& forward(typename types::remove_reference<T>::type&& param) { return static_cast<T&&>(param); }
template<typename T> T&& forward(typename types::remove_reference<T>::type&  param) { return static_cast<T&&>(param); }

template<typename T> void swap(T &t1, T &t2) { T tmp = move(t1); t1 = move(t2); t2 = move(tmp); }

} // namespace util
