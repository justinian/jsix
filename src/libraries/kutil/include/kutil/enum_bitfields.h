#pragma once

#include <type_traits>

template<typename E>
struct is_enum_bitfield { static constexpr bool value = false; };

#define IS_BITFIELD(name) \
	template<> struct ::is_enum_bitfield<name> {static constexpr bool value=true;}

template <typename E>
struct enum_or_int {
	static constexpr bool value =
		std::disjunction< is_enum_bitfield<E>, std::is_integral<E> >::value;
};

template <typename E, typename F>
struct both_enum_or_int {
	static constexpr bool value =
		std::conjunction< enum_or_int<E>, enum_or_int<F> >::value;
};

template <typename E>
struct integral { using type = typename std::underlying_type<E>::type; };

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

template <typename E, typename F>
constexpr typename std::enable_if<both_enum_or_int<E, F>::value,E>::type
operator & (E lhs, F rhs)
{
	return static_cast<E> (
		static_cast<typename integral<E>::type>(lhs) &
		static_cast<typename integral<F>::type>(rhs));
}

template <typename E, typename F>
constexpr typename std::enable_if<both_enum_or_int<E, F>::value,E>::type
operator | (E lhs, F rhs)
{
	return static_cast<E> (
		static_cast<typename integral<E>::type>(lhs) |
		static_cast<typename integral<F>::type>(rhs));
}

template <typename E, typename F>
constexpr typename std::enable_if<both_enum_or_int<E, F>::value,E>::type
operator ^ (E lhs, F rhs)
{
	return static_cast<E> (
		static_cast<typename integral<E>::type>(lhs) ^
		static_cast<typename integral<F>::type>(rhs));
}

template <typename E>
constexpr typename std::enable_if<is_enum_bitfield<E>::value,E>::type
operator ~ (E rhs)
{
	return static_cast<E>(~static_cast<typename std::underlying_type<E>::type>(rhs));
}

template <typename E, typename F>
constexpr typename std::enable_if<both_enum_or_int<E, F>::value,E>::type&
operator |= (E &lhs, F rhs)
{
	lhs = static_cast<E>(
		static_cast<typename integral<E>::type>(lhs) |
		static_cast<typename integral<F>::type>(rhs));

	return lhs;
}

template <typename E, typename F>
constexpr typename std::enable_if<both_enum_or_int<E, F>::value,E>::type&
operator &= (E &lhs, F rhs)
{
	lhs = static_cast<E>(
		static_cast<typename integral<E>::type>(lhs) &
		static_cast<typename integral<F>::type>(rhs));

	return lhs;
}

template <typename E, typename F>
constexpr typename std::enable_if<both_enum_or_int<E, F>::value,E>::type&
operator ^= (E &lhs, F rhs)
{
	lhs = static_cast<E>(
		static_cast<typename integral<E>::type>(lhs) ^
		static_cast<typename integral<F>::type>(rhs));

	return lhs;
}

template <typename E, typename F>
constexpr typename std::enable_if<both_enum_or_int<E, F>::value,E>::type&
operator -= (E &lhs, F rhs)
{
	lhs = static_cast<E>(
		static_cast<typename integral<E>::type>(lhs) &
		~static_cast<typename integral<F>::type>(rhs));

	return lhs;
}

template <typename E, typename F>
constexpr typename std::enable_if<both_enum_or_int<E, F>::value,E>::type&
operator += (E &lhs, F rhs)
{
	lhs = static_cast<E>(
		static_cast<typename integral<E>::type>(lhs) |
		static_cast<typename integral<F>::type>(rhs));

	return lhs;
}

template <typename E>
constexpr typename std::enable_if<is_enum_bitfield<E>::value,bool>::type
operator ! (E rhs)
{
	return static_cast<typename std::underlying_type<E>::type>(rhs) == 0;
}

template <typename E>
constexpr typename std::enable_if<is_enum_bitfield<E>::value,bool>::type
bitfield_has(E set, E flag)
{
	return (set & flag) == flag;
}

// Overload the logical-and operator to be 'bitwise-and, bool-cast'
template <typename E>
constexpr typename std::enable_if<is_enum_bitfield<E>::value,bool>::type
operator && (E set, E flag)
{
	return (set & flag) == flag;
}
