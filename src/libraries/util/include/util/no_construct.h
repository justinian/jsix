#pragma once
/// \file no_construct.h
/// Tools for creating objects witout running constructors

namespace util {

/// Helper template for creating objects witout running constructors
template <typename T>
union no_construct
{
    T value;
    no_construct() {}
    ~no_construct() {}
};

} // namespace util
