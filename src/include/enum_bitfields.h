#pragma once

#include "basic_types.h"

namespace bitfields {

template <typename E>
constexpr bool is_enum_bitfield(E) { return false; }

template <typename E>
struct enum_or_int {
	static constexpr bool value =
		is_enum_bitfield(E{}) || types::is_integral<E>::value;
};

template <typename E, typename F>
struct both_enum_or_int {
	static constexpr bool value =
		types::conjunction< enum_or_int<E>, enum_or_int<F> >::value;
};

template <typename E, typename F>
constexpr typename types::enable_if<both_enum_or_int<E, F>::value,E>::type&
operator |= (E &lhs, F rhs)
{
	return lhs = static_cast<E>(
		static_cast<typename types::integral<E>::type>(lhs) |
		static_cast<typename types::integral<F>::type>(rhs));
}

template <typename E, typename F>
constexpr typename types::enable_if<both_enum_or_int<E, F>::value,E>::type&
operator &= (E &lhs, F rhs)
{
	return lhs = static_cast<E>(
		static_cast<typename types::integral<E>::type>(lhs) &
		static_cast<typename types::integral<F>::type>(rhs));
}

template <typename E, typename F>
constexpr typename types::enable_if<both_enum_or_int<E, F>::value,E>::type&
operator ^= (E &lhs, F rhs)
{
	return lhs = static_cast<E>(
		static_cast<typename types::integral<E>::type>(lhs) ^
		static_cast<typename types::integral<F>::type>(rhs));
}

template <typename E, typename F>
constexpr typename types::enable_if<both_enum_or_int<E, F>::value,E>::type
operator & (E lhs, F rhs) { return lhs &= rhs; }

template <typename E, typename F>
constexpr typename types::enable_if<both_enum_or_int<E, F>::value,E>::type
operator | (E lhs, F rhs) { return lhs |= rhs; }

template <typename E, typename F>
constexpr typename types::enable_if<both_enum_or_int<E, F>::value,E>::type
operator ^ (E lhs, F rhs) { return lhs ^= rhs; }

template <typename E>
constexpr typename types::enable_if<is_enum_bitfield(E{}),E>::type
operator ~ (E rhs) { return static_cast<E>(~static_cast<typename types::integral<E>::type>(rhs)); }

template <typename E>
constexpr typename types::enable_if<is_enum_bitfield(E{}),bool>::type
operator ! (E rhs) { return static_cast<typename types::integral<E>::type>(rhs) == 0; }

/// Override logical-and to mean 'rhs contains all bits in lhs'
template <typename E>
constexpr typename types::enable_if<is_enum_bitfield(E{}),bool>::type
operator && (E rhs, E lhs) { return (rhs & lhs) == lhs; }

/// Generic 'has' for non-marked bitfields
template <typename E, typename F>
constexpr bool has(E set, F flags)
{
	return
		(static_cast<typename types::integral<E>::type>(set) &
		static_cast<typename types::integral<F>::type>(flags)) ==
		static_cast<typename types::integral<F>::type>(flags);
}

} // namespace bitfields

#define is_bitfield(name) \
	constexpr bool is_enum_bitfield(name) { return true; } \
	using namespace ::bitfields;

