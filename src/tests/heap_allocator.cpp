#include <chrono>
#include <random>
#include <vector>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>

#include "kutil/memory.h"
#include "kutil/heap_allocator.h"
#include "catch.hpp"

using namespace kutil;

const size_t hs = 0x10; // header size
const size_t max_block = 1 << 22;

int signalled = 0;
void *signalled_at = nullptr;

void *mem_base = nullptr;
size_t mem_size = 4 * max_block;

extern bool ASSERT_EXPECTED;
extern bool ASSERT_HAPPENED;

std::vector<size_t> sizes = {
    16000, 8000, 4000, 4000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 150,
    150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 150, 48, 48, 48, 13 };

void segfault_handler(int signum, siginfo_t *info, void *ctxp)
{
    uintptr_t start = reinterpret_cast<uintptr_t>(mem_base);
    uintptr_t end = start + mem_size;
    uintptr_t addr = reinterpret_cast<uintptr_t>(info->si_addr);

    if (addr < start || addr >= end) {
        CAPTURE( start );
        CAPTURE( end );
        CAPTURE( addr );
        FAIL("Segfaulted outside memory area");
    }

    signalled_at = info->si_addr;
    signalled += 1;
    if (mprotect(signalled_at, max_block, PROT_READ|PROT_WRITE)) {
        perror("mprotect");
        exit(100);
    }
}

TEST_CASE( "Buddy blocks tests", "[memory buddy]" )
{
    using clock = std::chrono::system_clock;
    unsigned seed = clock::now().time_since_epoch().count();
    std::default_random_engine rng(seed);

    mem_base = aligned_alloc(max_block, mem_size);

    // Catch segfaults so we can track memory access
    struct sigaction sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_NODEFER|SA_SIGINFO;
    sigact.sa_sigaction = segfault_handler;
    sigaction(SIGSEGV, &sigact, nullptr);

    // Protect our memory arena so we trigger out fault handler
    REQUIRE( mprotect(mem_base, max_block*4, PROT_NONE) == 0 );

    heap_allocator mm(
            reinterpret_cast<uintptr_t>(mem_base),
            max_block * 4);

    // Initial creation should not have allocated
    CHECK( signalled == 0 );
    signalled = 0;

    // Allocating too much should assert
    ASSERT_EXPECTED = true;
    void *p = mm.allocate(max_block - hs + 1);
    REQUIRE( ASSERT_HAPPENED );
    ASSERT_HAPPENED = false;

    // Allocating should signal just at the first page.
    p = mm.allocate(max_block - hs);
    CHECK( p == offset_pointer(mem_base, hs) );
    CHECK( signalled == 1 );
    CHECK( signalled_at == mem_base );
    signalled = 0;

    // Freeing and allocating should not allocate
    mm.free(p);
    p = mm.allocate(max_block - hs);
    CHECK( p == offset_pointer(mem_base, hs) );
    CHECK( signalled == 0 );
    signalled = 0;

    mm.free(p);
    CHECK( signalled == 0 );
    signalled = 0;

    // Blocks should be:
    // 22: 0-4M

    std::vector<void *> allocs(6);
    for (int i = 0; i < 6; ++i)
        allocs[i] = mm.allocate(150); // size 8

    // Should not have grown
    CHECK( signalled == 0 );
    signalled = 0;

    // Blocks should be:
    // 22: [0-4M]
    // 21: [0-2M], 2-4M
    // 20: [0-1M], 1-2M
    // 19: [0-512K], 512K-1M
    // 18: [0-256K], 256-512K
    // 17: [0-128K], 128-256K
    // 16: [0-64K], 64-128K
    // 15: [0-32K], 32K-64K
    // 14: [0-16K], 16K-32K
    // 13: [0-8K], 8K-16K
    // 12: [0-4K], 4K-8K
    // 11: [0-2K], 2K-4K
    // 10: [0-1K, 1-2K]
    //  9: [0, 512, 1024], 1536
    //  8: [0, 256, 512, 768, 1024, 1280]

    // We have free memory at 1526 and 2K, but we should get 4K
    void *big = mm.allocate(4000); // size 12

    CHECK( signalled == 0 );
    signalled = 0;

    REQUIRE( big == offset_pointer(mem_base, 4096 + hs) );
    mm.free(big);

    // free up 512
    mm.free(allocs[3]);
    mm.free(allocs[4]);

    // Blocks should be:
    // ...
    //  9: [0, 512, 1024], 1536
    //  8: [0, 256, 512], 768, 1024, [1280]

    // A request for a 512-block should not cross the buddy divide
    big = mm.allocate(500); // size 9
    REQUIRE( big >= offset_pointer(mem_base, 1536 + hs) );
    mm.free(big);

    mm.free(allocs[0]);
    mm.free(allocs[1]);
    mm.free(allocs[2]);
    mm.free(allocs[5]);
    allocs.clear();

    std::shuffle(sizes.begin(), sizes.end(), rng);

    allocs.reserve(sizes.size());
    for (size_t size : sizes)
        allocs.push_back(mm.allocate(size));

    std::shuffle(allocs.begin(), allocs.end(), rng);
    for (void *p: allocs)
        mm.free(p);
    allocs.clear();

    big = mm.allocate(max_block / 2 + 1);

    // If everything was freed / joined correctly, that should not have allocated
    CHECK( signalled == 0 );
    signalled = 0;

    // And we should have gotten back the start of memory
    CHECK( big == offset_pointer(mem_base, hs) );

    // Allocating again should signal at the next page.
    void *p2 = mm.allocate(max_block - hs);
    CHECK( p2 == offset_pointer(mem_base, max_block + hs) );
    CHECK( signalled == 1 );
    CHECK( signalled_at == offset_pointer(mem_base, max_block) );
    signalled = 0;

    mm.free(p2);
    CHECK( signalled == 0 );
    signalled = 0;

    free(mem_base);
}

