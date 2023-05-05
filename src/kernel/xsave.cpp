#include <stdint.h>
#include <cpu/cpu_id.h>

#include "cpu.h"
#include "xsave.h"

uint64_t xcr0_val = 0;
static size_t xsave_size_val = 0;
const size_t &xsave_size = xsave_size_val;

void
xsave_init()
{
    cpu::cpu_id cpuid;
    const auto regs = cpuid.get(0x0d);
    const uint64_t cpu_supported = 
        (static_cast<uint64_t>(regs.edx) << 32) |
        static_cast<uint64_t>(regs.eax);

    xcr0_val = static_cast<uint64_t>(xcr0::J6_SUPPORTED) & cpu_supported;
    xsave_size_val = regs.ebx;
}

void
xsave_enable()
{
    const uint64_t rax = (xcr0_val & 0xFFFFFFFF);
    const uint64_t rdx = (xcr0_val >> 32);
    asm volatile ( "xsetbv" :: "c"(0), "d"(xcr0_val >> 32), "a"(xcr0_val) );
}
