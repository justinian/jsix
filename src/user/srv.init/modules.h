#pragma once
/// \file modules.h
/// Routines for loading initial argument modules

#include <bootproto/init.h>
#include <j6/types.h>
#include <util/pointers.h>


class module_iterator
{
public:
    using type = bootproto::module_type;
    using module = bootproto::module;

    module_iterator(const module *m, type t = type::none) :
        m_mod {m}, m_type {t} {}

    const module * operator++();
    const module * operator++(int);

    bool operator==(const module* m) const { return m == m_mod; }
    bool operator!=(const module* m) const { return m != m_mod; }
    bool operator==(const module_iterator &i) const { return i.m_mod == m_mod; }
    bool operator!=(const module_iterator &i) const { return i.m_mod != m_mod; }

    const module & operator*() const { return *m_mod; }
    operator const module & () const { return *m_mod; }
    const module * operator->() const { return m_mod; }

    // Allow iterators to be used in for(:) statments
    module_iterator & begin() { return *this; }
    module_iterator end() const { return nullptr; }

private:
    module const * m_mod;
    type m_type;
};

class modules
{
public:
    using type = bootproto::module_type;
    using iterator = module_iterator;

    static modules load_modules(
        uintptr_t address,
        j6_handle_t system,
        j6_handle_t self);

    iterator of_type(type t) const { return iterator {m_root, t}; }

    iterator begin() const { return iterator {m_root}; }
    iterator end() const { return nullptr; }

private:
    using module = bootproto::module;

    modules(const module* root) : m_root {root} {}

    const module *m_root;
};
