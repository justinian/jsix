"""Python implementation of FNV hashes."""

_FNV1_prime32 = 16777619
_FNV1_prime64 = 1099511628211
_FNV1_offset32 = 2166136261
_FNV1_offset64 = 14695981039346656037

def _fnv1a(offset, prime, mask):
    def hashfunc(data):
        h = offset
        for b in data:
            h = ((h ^ b) * prime) & mask
        return h
    return hashfunc

hash64 = _fnv1a(_FNV1_offset64, _FNV1_prime64, (2**64)-1)
hash32 = _fnv1a(_FNV1_offset32, _FNV1_prime32, (2**32)-1)
