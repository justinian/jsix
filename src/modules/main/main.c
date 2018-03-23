__attribute__((section(".text.entry")))
void kernel_main() {
    volatile register int foo = 0x1a1b1c10;
    volatile register int bar = 0;

    while(1)
        foo = foo | 0xfffffff0 + bar++ | 0xf;
}
