#pragma once
/// \file no_construct.h
/// Tools for creating objects witout running constructors

namespace kutil {

/// Helper template for creating objects witout running constructors
template <typename T>
union no_construct
{
	T value;
	no_construct() {}
};

} // namespace kutil
