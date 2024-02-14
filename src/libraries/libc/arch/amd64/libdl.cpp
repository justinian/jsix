
extern "C" {
    // Stub out these libdl functions for libunwind until
    // we have a real libdl
    int dladdr(const void *, void *) { return 0; }
    int dl_iterate_phdr(void *, void *) { return 0; }
}
