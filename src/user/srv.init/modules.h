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

    module_iterator(const bootproto::modules_page *p, unsigned i, type t = type::none) :
        m_page {p}, m_idx {i}, m_type {t} {
            if ( t != type::none && p && deref()->type != t ) operator++();
    }

    const module * operator++();
    const module * operator++(int);

    bool operator==(const module* m) const { return m == deref(); }
    bool operator!=(const module* m) const { return m != deref(); }
    bool operator==(const module_iterator &i) const { return i.deref() == deref(); }
    bool operator!=(const module_iterator &i) const { return i.deref() != deref(); }

    const module & operator*() const { return *deref(); }
    operator const module & () const { return *deref(); }
    const module * operator->() const { return deref(); }

    // Allow iterators to be used in for(:) statments
    module_iterator & begin() { return *this; }
    module_iterator end() const { return {nullptr, 0}; }

private:
    inline const module * deref() const {
        if (m_page) {
            const module &m = m_page->modules[m_idx];
            if (m.type != type::none)
                return &m;
        }
        return nullptr;
    }

    unsigned m_idx;
    bootproto::modules_page const *m_page;
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

    iterator of_type(type t) const { return iterator {m_root, 0, t}; }

    iterator begin() const { return iterator {m_root, 0}; }
    iterator end() const { return {nullptr, 0}; }

private:
    using module = bootproto::module;

    modules(const bootproto::modules_page* root) : m_root {root} {}

    const bootproto::modules_page *m_root;
};
