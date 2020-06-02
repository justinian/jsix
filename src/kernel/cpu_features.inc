CPU_FEATURE_OPT(pcid,       0x00000001, 0, ecx, 17)
CPU_FEATURE_OPT(x2apic,     0x00000001, 0, ecx, 21)
CPU_FEATURE_OPT(in_hv,      0x00000001, 0, ecx, 31)

CPU_FEATURE_REQ(fpu,        0x00000001, 0, edx,  0)
CPU_FEATURE_REQ(pse,        0x00000001, 0, edx,  3)
CPU_FEATURE_OPT(tsc,        0x00000001, 0, edx,  4)
CPU_FEATURE_REQ(msr,        0x00000001, 0, edx,  5)
CPU_FEATURE_REQ(apic,       0x00000001, 0, edx,  9)
CPU_FEATURE_REQ(pge,        0x00000001, 0, edx, 13)
CPU_FEATURE_OPT(pat,        0x00000001, 0, edx, 16)
CPU_FEATURE_REQ(fxsr,       0x00000001, 0, edx, 24)

CPU_FEATURE_OPT(fsgsbase,   0x00000007, 0, ebx,  0)
CPU_FEATURE_OPT(invpcid,    0x00000007, 0, ebx, 10)

CPU_FEATURE_OPT(pku,        0x00000007, 0, ecx,  3)

CPU_FEATURE_OPT(extapic,    0x80000001, 0, ecx,  3)

CPU_FEATURE_REQ(syscall,    0x80000001, 0, edx, 11)
CPU_FEATURE_REQ(pdpe1gb,    0x80000001, 0, edx, 26)

// vim: et
