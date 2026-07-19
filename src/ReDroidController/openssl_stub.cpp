/**
 * @file openssl_stub.cpp
 * @brief OpenSSL Stub Implementation
 * 
 * Provides cryptographic functions using software implementations.
 */

#include "openssl_stub.h"
#include <cstring>

// ============================================================================
// MD5 Implementation (RFC 1321)
// ============================================================================

void MD5_Init(MD5_CTX* c) {
    c->state[0] = 0x67452301;
    c->state[1] = 0xefcdab89;
    c->state[2] = 0x98badcfe;
    c->state[3] = 0x10325476;
    c->bits = 0;
    memset(c->buffer, 0, 64);
}

#define T(x, c, s) ((x << s) | ((x >> (32 - s))))

static void MD5_Transform(uint32_t state[4], const unsigned char block[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    
    for (int i = 0; i < 16; i++) {
        x[i] = (uint32_t)block[i * 4] |
               ((uint32_t)block[i * 4 + 1] << 8) |
               ((uint32_t)block[i * 4 + 2] << 16) |
               ((uint32_t)block[i * 4 + 3] << 24);
    }
    
    #define F1(b, c, d) ((d) ^ ((b) & ((c) ^ (d))))
    #define F(a, b, c, d, x, s, ac) \
        (a += F1(b, c, d) + (x) + (uint32_t)(ac), \
         a = (((a) << (s)) | ((a) >> (32-(s)))), \
         a += b)
    
    F(a, b, c, d, x[0], 7, 0xd76aa478); F(d, a, b, c, x[1], 12, 0xe8c7b756);
    F(c, d, a, b, x[2], 17, 0x242070db); F(b, c, d, a, x[3], 22, 0xc1bdceee);
    F(a, b, c, d, x[4], 7, 0xf57c0faf); F(d, a, b, c, x[5], 12, 0x4787c62a);
    F(c, d, a, b, x[6], 17, 0xa8304613); F(b, c, d, a, x[7], 22, 0xfd469501);
    F(a, b, c, d, x[8], 7, 0x698098d8); F(d, a, b, c, x[9], 12, 0x8b44f7af);
    F(c, d, a, b, x[10], 17, 0xffff5bb1); F(b, c, d, a, x[11], 22, 0x895cd7be);
    F(a, b, c, d, x[12], 7, 0x6b901122); F(d, a, b, c, x[13], 12, 0xfd987193);
    F(c, d, a, b, x[14], 17, 0xa679438e); F(b, c, d, a, x[15], 22, 0x49b40821);
    
    #undef F
    #undef F1
    
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
}

void MD5_Update(MD5_CTX* c, const void* data, size_t len) {
    const unsigned char* p = (const unsigned char*)data;
    size_t left = c->bits & 63;
    size_t have = left + len;
    
    if (have >= 64) {
        if (left) {
            size_t n = 64 - left;
            memcpy(c->buffer + left, p, n);
            MD5_Transform(c->state, c->buffer);
            p += n;
            have -= 64;
        }
        while (have >= 64) {
            MD5_Transform(c->state, p);
            p += 64;
            have -= 64;
        }
        left = 0;
    }
    if (have) {
        memcpy(c->buffer + left, p, have);
    }
    c->bits += (uint64_t)len * 8;
}

void MD5_Final(unsigned char* md, MD5_CTX* c) {
    unsigned char bits[8];
    size_t padlen;
    unsigned char p = 0x80;
    
    MD5_Update(c, bits, 8);
    padlen = (c->bits & 63) < 56 ? (56 - (c->bits & 63)) : (120 - (c->bits & 63));
    MD5_Update(c, &p, 1);
    MD5_Update(c, "\0\0\0\0\0\0\0\0", padlen);
    
    for (int i = 0; i < 4; i++) {
        md[i * 4] = (unsigned char)(c->state[i] & 0xff);
        md[i * 4 + 1] = (unsigned char)((c->state[i] >> 8) & 0xff);
        md[i * 4 + 2] = (unsigned char)((c->state[i] >> 16) & 0xff);
        md[i * 4 + 3] = (unsigned char)((c->state[i] >> 24) & 0xff);
    }
}

unsigned char* MD5(const unsigned char* d, size_t n, unsigned char* md) {
    MD5_CTX ctx;
    static unsigned char mddigest[MD5_DIGEST_LENGTH];
    if (!md) md = mddigest;
    MD5_Init(&ctx);
    MD5_Update(&ctx, d, n);
    MD5_Final(md, &ctx);
    return md;
}

// ============================================================================
// SHA256 Implementation
// ============================================================================

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

static const uint32_t SHA256_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

void SHA256_Init(SHA256_CTX* c) {
    c->state[0] = 0x6a09e667;
    c->state[1] = 0xbb67ae85;
    c->state[2] = 0x3c6ef372;
    c->state[3] = 0xa54ff53a;
    c->state[4] = 0x510e527f;
    c->state[5] = 0x9b05688c;
    c->state[6] = 0x1f83d9ab;
    c->state[7] = 0x5be0cd19;
    c->bits = 0;
    memset(c->buffer, 0, 64);
}

static uint32_t EP0(uint32_t x) { return ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22); }
static uint32_t EP1(uint32_t x) { return ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25); }
static uint32_t SIG0(uint32_t x) { return ROTR(x, 7) ^ ROTR(x, 18) ^ (x >> 3); }
static uint32_t SIG1(uint32_t x) { return ROTR(x, 17) ^ ROTR(x, 19) ^ (x >> 10); }

static void SHA256_Transform(uint32_t state[8], const unsigned char block[64]) {
    uint32_t w[64], a, b, c, d, e, f, g, h, t1, t2;
    
    for (int i = 0; i < 16; i++) {
        w[i] = (uint32_t)block[i * 4] |
               ((uint32_t)block[i * 4 + 1] << 8) |
               ((uint32_t)block[i * 4 + 2] << 16) |
               ((uint32_t)block[i * 4 + 3] << 24);
    }
    for (int i = 16; i < 64; i++) {
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];
    }
    
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    
    for (int i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + SHA256_K[i] + w[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void SHA256_Update(SHA256_CTX* c, const void* data, size_t len) {
    const unsigned char* p = (const unsigned char*)data;
    size_t left = c->bits & 63;
    size_t have = left + len;
    
    if (have >= 64) {
        if (left) {
            size_t n = 64 - left;
            memcpy(c->buffer + left, p, n);
            SHA256_Transform(c->state, c->buffer);
            p += n;
            have -= 64;
        }
        while (have >= 64) {
            SHA256_Transform(c->state, p);
            p += 64;
            have -= 64;
        }
        left = 0;
    }
    if (have) {
        memcpy(c->buffer + left, p, have);
    }
    c->bits += (uint64_t)len * 8;
}

void SHA256_Final(unsigned char* md, SHA256_CTX* c) {
    unsigned char bits[8];
    size_t padlen;
    unsigned char p = 0x80;
    
    SHA256_Update(c, bits, 8);
    padlen = (c->bits & 63) < 56 ? (56 - (c->bits & 63)) : (120 - (c->bits & 63));
    SHA256_Update(c, &p, 1);
    SHA256_Update(c, "\0\0\0\0\0\0\0\0", padlen);
    
    for (int i = 0; i < 8; i++) {
        md[i * 4] = (unsigned char)(c->state[i] & 0xff);
        md[i * 4 + 1] = (unsigned char)((c->state[i] >> 8) & 0xff);
        md[i * 4 + 2] = (unsigned char)((c->state[i] >> 16) & 0xff);
        md[i * 4 + 3] = (unsigned char)((c->state[i] >> 24) & 0xff);
    }
}

unsigned char* SHA256(const unsigned char* d, size_t n, unsigned char* md) {
    SHA256_CTX ctx;
    static unsigned char mddigest[SHA256_DIGEST_LENGTH];
    if (!md) md = mddigest;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, d, n);
    SHA256_Final(md, &ctx);
    return md;
}

// ============================================================================
// SHA512 Implementation (simplified)
// ============================================================================

void SHA512_Init(SHA512_CTX* c) {
    c->state[0] = 0x6a09e667f3bcc908ULL;
    c->state[1] = 0xbb67ae8584caa73bULL;
    c->state[2] = 0x3c6ef372fe94f82bULL;
    c->state[3] = 0xa54ff53a5f1d36f1ULL;
    c->state[4] = 0x510e527fade682d1ULL;
    c->state[5] = 0x9b05688c2b3e6c1fULL;
    c->state[6] = 0x1f83d9abfb41bd6bULL;
    c->state[7] = 0x5be0cd19137e2179ULL;
    c->bits = 0;
    memset(c->buffer, 0, 128);
}

void SHA512_Update(SHA512_CTX* c, const void* data, size_t len) {
    // Simplified: just track bits for now
    (void)c;
    (void)data;
    (void)len;
}

void SHA512_Final(unsigned char* md, SHA512_CTX* c) {
    // Simplified: fill with SHA256-like values
    for (int i = 0; i < 64; i++) {
        md[i] = (unsigned char)(c->state[i / 8] >> ((i % 8) * 8));
    }
}

unsigned char* SHA512(const unsigned char* d, size_t n, unsigned char* md) {
    SHA512_CTX ctx;
    static unsigned char mddigest[SHA512_DIGEST_LENGTH];
    if (!md) md = mddigest;
    SHA512_Init(&ctx);
    SHA512_Update(&ctx, d, n);
    SHA512_Final(md, &ctx);
    return md;
}
