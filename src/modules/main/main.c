__attribute__((section(".text.entry")))
void kernel_main() {
    volatile int foo = 13;
    return;
}
