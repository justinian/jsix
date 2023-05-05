#pragma once

#include <stdint.h>
#include <cpu/cpu_id.h>

class GDT;
class IDT;
class lapic;
struct TCB;
class TSS;

namespace obj {
    class process;
    class thread;
}

enum class cr0
{
    PE =  0,    // Protected mode enable
    MP =  1,    // Monitor co-processor
    EM =  2,    // (FPU) Emulation
    TS =  3,    // Task switched
    ET =  4,    // Extension type
    NE =  5,    // Numeric error
    WP = 16,    // (ring 0) Write protect
    CD = 30,    // Cache disable
    PG = 31,    // Paging
};

enum class cr4
{
    DE         =  3, // Debugging extensions
    PAE        =  5, // Physical address extension
    MCE        =  6, // Machine check exception
    PGE        =  7, // Page global enable
    OSXFSR     =  9, // OS supports FXSAVE
    OSXMMEXCPT = 10, // OS supports SIMD FP exceptions
    FSGSBASE   = 16, // Enable {RD|WR}{F|G}SBASE instructions
    PCIDE      = 17, // PCID enable
    OSXSAVE    = 18, // OS supports XSAVE and extended states
};

enum class xcr0
{
    X87,
    SSE,
    AVX,
    BINDREG,
    BINDCSR,
    OPMASK,
    ZMM_Hi256,
    ZMM_Hi16,
    PKRU = 9,

    J6_SUPPORTED = X87 | SSE | AVX | BINDREG | BINDCSR | OPMASK | ZMM_Hi16 | ZMM_Hi256,
};

enum class efer
{
    SCE   =  0, // System call extensions (SYSCALL/SYSRET)
    LME   =  8, // Long mode enable
    LMA   = 10, // Long mode active
    NXE   = 11, // No-execute enable
    FFXSR = 14, // Fast FXSAVE
};

enum class mxcsr
{
    IE  =  0, // Invalid operation flag
    DE  =  1, // Denormal flag
    ZE  =  2, // Divide by zero flag
    OE  =  3, // Overflow flag
    UE  =  4, // Underflow flag
    PE  =  5, // Precision flag
    DAZ =  6, // Denormals are zero
    IM  =  7, // Invalid operation mask
    DM  =  8, // Denormal mask
    ZM  =  9, // Divide by zero mask
    OM  = 10, // Overflow mask
    UM  = 11, // Underflow mask
    PM  = 12, // Precision mask
    RC0 = 13, // Rounding control bit 0
    RC1 = 14, // Rounding control bit 1
    FTZ = 15, // Flush to zero
};

struct cpu_state
{
    uint64_t r15, r14, r13, r12, r11, r10, r9,  r8;
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    uint64_t interrupt, errorcode;
    uint64_t rip, cs, rflags, rsp, ss;
};

/// Kernel-wide panic information
struct panic_data
{
    void const *symbol_data;
    cpu_state const *user_state;

    char const *message;
    char const *function;
    char const *file;
    uint32_t line;

    uint16_t cpus;
};

extern unsigned g_num_cpus;
extern panic_data *g_panic_data_p;

/// Per-cpu state data. If you change this, remember to update the assembly
/// version in 'tasking.inc'
struct cpu_data
{
    cpu_data *self;
    uint16_t id;
    uint16_t index;
    uint32_t reserved;
    uintptr_t rsp0;
    uintptr_t rsp3;
    uint64_t rflags3;
    TCB *tcb;
    obj::thread *thread;
    obj::process *process;
    IDT *idt;
    TSS *tss;
    GDT *gdt;

    // Members beyond this point do not appear in
    // the assembly version
    lapic *apic;
    panic_data *panic;
    cpu::features features;
};

extern "C" {
    uint32_t get_mxcsr();
    uint32_t set_mxcsr(uint32_t val);

    uint64_t get_xcr0();
    uint64_t set_xcr0(uint64_t val);

    cpu_data * _current_gsbase();
}

/// Do early initialization of the BSP CPU.
/// \returns  A pointer to the BSP cpu_data structure
cpu_data * bsp_early_init();

/// Do late initialization of the BSP CPU.
void bsp_late_init();

/// Create a new cpu_data struct with all requisite sub-objects.
/// \arg id    The ACPI specified id of the CPU
/// \arg index The kernel-specified initialization index of the CPU
/// \returns   The new cpu_data structure
cpu_data * cpu_create(uint16_t id, uint16_t index);

/// Set up the running CPU. This sets GDT, IDT, and necessary MSRs as well as creating
/// the cpu_data structure for this processor.
/// \arg cpu  The cpu_data structure for this CPU
/// \arg bsp  True if this CPU is the BSP
void cpu_init(cpu_data *cpu, bool bsp);

/// Get the cpu_data struct for the current executing CPU
inline cpu_data & current_cpu() { return *_current_gsbase(); }
