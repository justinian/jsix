#pragma once
/// \file profiler.h
/// The kernel profiling interface

#include <stdint.h>
#include <util/hash.h>

inline uint64_t rdtsc() {
    uint32_t high, low;
    asm ( "rdtsc" : "=a" (low), "=d" (high) );
    return (static_cast<uint64_t>(high) << 32) | low;
}

template<typename PC, size_t ID>
class profiler
{
    using class_t = PC;
    static constexpr size_t id = ID;

public:
    profiler(char const * const function = __builtin_FUNCTION()) : m_start(rdtsc()) {
        static_assert(id < class_t::count, "profiler out of bounds");
        class_t::call_counts[id]++;
        class_t::function_names[id] = function;
    }

    ~profiler() {
        uint64_t stop = rdtsc();
        class_t::call_durations[id] += (stop - m_start);
    }

private:
    uint64_t m_start;
};

template<typename T>
struct profile_class
{
    static volatile char const * function_names[];
    static volatile uint64_t call_counts[];
    static volatile uint64_t call_durations[];
};

#define DECLARE_PROFILE_CLASS(name, size) \
    struct name : public profile_class<name> { static constexpr size_t count = size; };

#define DEFINE_PROFILE_CLASS(name) \
    template<> volatile char const * profile_class<name>::function_names[name::count] = {0}; \
    template<> volatile uint64_t profile_class<name>::call_counts[name::count] = {0}; \
    template<> volatile uint64_t profile_class<name>::call_durations[name::count] = {0};
