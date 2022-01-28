#pragma once
/// \file smp.h
/// Multi-core / multi-processor startup code

struct cpu_data;

namespace smp {

/// Start all APs and have them wait for the BSP to
/// call smp::ready().
/// \arg bsp    The cpu_data struct representing the BSP
/// \arg kpml4  The kernel PML4
unsigned start(cpu_data &bsp, void *kpml4);

/// Unblock all APs and let them start their schedulers.
void ready();

} // namespace smp
