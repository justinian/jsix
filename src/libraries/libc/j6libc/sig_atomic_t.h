#pragma once
/* Type definition: sig_atomic_t */

#if __is_identifier(sig_atomic_t)
#if __SIG_ATOMIC_WIDTH__ == 16
typedef int16_t sig_atomic_t;
#elif __SIG_ATOMIC_WIDTH__ == 32
typedef int32_t sig_atomic_t;
#elif __SIG_ATOMIC_WIDTH__ == 64
typedef int64_t sig_atomic_t;
#else
#error "Unknown size of sig_atomic_t" __SIG_ATOMIC_WIDTH__
#endif
#endif
