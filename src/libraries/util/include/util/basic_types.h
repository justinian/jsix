#pragma once
/// file basic_types.h
/// Type properties that would normally come from <type_traits>

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


} // namespace types
