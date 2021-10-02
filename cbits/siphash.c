/* Almost a verbatim copy of the reference implementation. */

#include "siphash.h"
#include <stddef.h>

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define SIPROUND                                                               \
  do {                                                                         \
    v0 += v1;                                                                  \
    v1 = ROTL(v1, 13);                                                         \
    v1 ^= v0;                                                                  \
    v0 = ROTL(v0, 32);                                                         \
    v2 += v3;                                                                  \
    v3 = ROTL(v3, 16);                                                         \
    v3 ^= v2;                                                                  \
    v0 += v3;                                                                  \
    v3 = ROTL(v3, 21);                                                         \
    v3 ^= v0;                                                                  \
    v2 += v1;                                                                  \
    v1 = ROTL(v1, 17);                                                         \
    v1 ^= v2;                                                                  \
    v2 = ROTL(v2, 32);                                                         \
  } while (0)

#if defined(__i386)
#define _siphash24 plain_siphash24
#endif

static inline uint64_t odd_read(const u8 *p, int count, uint64_t val,
                                int shift) {
  switch (count) {
  case 7:
    val |= ((uint64_t)p[6]) << (shift + 48);
  case 6:
    val |= ((uint64_t)p[5]) << (shift + 40);
  case 5:
    val |= ((uint64_t)p[4]) << (shift + 32);
  case 4:
    val |= ((uint64_t)p[3]) << (shift + 24);
  case 3:
    val |= ((uint64_t)p[2]) << (shift + 16);
  case 2:
    val |= ((uint64_t)p[1]) << (shift + 8);
  case 1:
    val |= ((uint64_t)p[0]) << shift;
  }
  return val;
}

static inline uint64_t _siphash_chunk
    ( const int c
    , const int d
    , uint64_t v[4] // this mutates, allowing you to keep on hashing
    , const u8 *str
    , const size_t len
){

  uint64_t v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];
  const u8 *p;
  const u8* end;
  for (p = str, end = str + (len & ~7); p < end; p += 8) {
    uint64_t m = peek_uint64_tle((uint64_t *)p);
    v3 ^= m;
    if (c == 2) {
      SIPROUND;
      SIPROUND;
    } else {
      for (int i = 0; i < c; i++)
        SIPROUND;
    }
    v0 ^= m;
  }

  uint64_t b = odd_read(p, len & 7, ((uint64_t)len) << 56, 0);

  v3 ^= b;
  if (c == 2) {
    SIPROUND;
    SIPROUND;
  } else {
    for (int i = 0; i < c; i++)
      SIPROUND;
  }
  v0 ^= b;

  v2 ^= 0xff;
  if (d == 4) {
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
  } else {
    for (int i = 0; i < d; i++)
      SIPROUND;
  }
  return v0 ^ v1 ^ v2 ^ v3;
}

static inline uint64_t _siphash(const int c, const int d, const uint64_t k0, const uint64_t k1,
                                const u8 *str, size_t len) {
  // intialize state
  uint64_t state[4] = { 0x736f6d6570736575ull ^ k0
         , 0x646f72616e646f6dull ^ k1
         , 0x6c7967656e657261ull ^ k0
         , 0x7465646279746573ull ^ k1
         };
  return _siphash_chunk(c, d, state, str, len);

}

static inline uint64_t _siphash24(uint64_t k0, uint64_t k1, const u8 *str,
                                  size_t len) {
  return _siphash(2, 4, k0, k1, str, len);
}

#if defined(__i386)
#undef _siphash24

static uint64_t (*_siphash24)(uint64_t k0, uint64_t k1, const u8 *, size_t);

static void maybe_use_sse() __attribute__((constructor));

static void maybe_use_sse() {
  uint32_t eax = 1, ebx, ecx, edx;

  __asm volatile("mov %%ebx, %%edi;" /* 32bit PIC: don't clobber ebx */
                 "cpuid;"
                 "mov %%ebx, %%esi;"
                 "mov %%edi, %%ebx;"
                 : "+a"(eax), "=S"(ebx), "=c"(ecx), "=d"(edx)
                 :
                 : "edi");

#if defined(HAVE_SSE2)
  if (edx & (1 << 26))
    _siphash24 = hashable_siphash24_sse2;
#if defined(HAVE_SSE41)
  else if (ecx & (1 << 19))
    _siphash24 = hashable_siphash24_sse41;
#endif
  else
#endif
    _siphash24 = plain_siphash24;
}

#endif

/* ghci's linker fails to call static initializers. */
static inline void ensure_sse_init() {
#if defined(__i386)
  if (_siphash24 == NULL)
    maybe_use_sse();
#endif
}

uint64_t hashable_siphash(int c, int d, uint64_t k0, uint64_t k1, const u8 *str,
                          size_t len) {
  return _siphash(c, d, k0, k1, str, len);
}

uint64_t hashable_siphash24(uint64_t k0, uint64_t k1, const u8 *str,
                            size_t len) {
  ensure_sse_init();
  return _siphash24(k0, k1, str, len);
}

/* Used for ByteArray#s. We can't treat them like pointers in
   native Haskell, but we can in unsafe FFI calls.
 */
uint64_t hashable_siphash24_offset(uint64_t k0, uint64_t k1, const u8 *str,
                                   size_t off, size_t len) {
  ensure_sse_init();
  return _siphash24(k0, k1, str + off, len);
}

void hashable_siphash_init(uint64_t k0, uint64_t k1, uint64_t *v) {
  v[0] = 0x736f6d6570736575ull ^ k0;
  v[1] = 0x646f72616e646f6dull ^ k1;
  v[2] = 0x6c7967656e657261ull ^ k0;
  v[3] = 0x7465646279746573ull ^ k1;
}

int hashable_siphash24_chunk(uint64_t v[4], const u8 *str,
                             size_t len) {
  return _siphash_chunk(2, 4, v, str, len);
}

/*
 * Used for ByteArray#.
 */
int hashable_siphash24_chunk_offset(uint64_t v[4], const u8 *str,
                                    size_t off, size_t len) {
  return _siphash_chunk(2, 4, v, str + off, len);
}
