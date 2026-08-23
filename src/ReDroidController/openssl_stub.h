/**
 * @file openssl_stub.h
 * @brief OpenSSL Stub for systems without OpenSSL
 * 
 * Provides cryptographic functions without requiring OpenSSL linkage.
 * Uses system crypto functions or provides fallback implementations.
 */


#ifndef OPENSSL_STUB_H
#define OPENSSL_STUB_H

#include <string>
#include <cstdint>
#include <cstring>

// OpenSSL compatibility definitions
#define MD5_DIGEST_LENGTH 16
#define SHA256_DIGEST_LENGTH 32
#define SHA384_DIGEST_LENGTH 48
#define SHA512_DIGEST_LENGTH 64

// Digest lengths
#define EVP_MAX_MD_SIZE 64

// OpenSSL types (simplified)
typedef struct {
    uint32_t state[8];
    uint64_t bits;
    unsigned char buffer[64];
    int usingHardware;
} SHA256_CTX;

typedef struct {
    uint32_t state[5];
    uint64_t bits;
    unsigned char buffer[64];
} MD5_CTX;

typedef struct {
    uint32_t state[8];
    uint64_t bits;
    unsigned char buffer[128];
} SHA512_CTX;

typedef struct {
    int version;
    int method;
    void* data;
    void* readAhead;
    int readAheadBuf;
} SSL;
typedef SSL SSL_CTX;

// MD5 Functions
void MD5_Init(MD5_CTX* c);
void MD5_Update(MD5_CTX* c, const void* data, size_t len);
void MD5_Final(unsigned char* md, MD5_CTX* c);
unsigned char* MD5(const unsigned char* d, size_t n, unsigned char* md);

// SHA256 Functions
void SHA256_Init(SHA256_CTX* c);
void SHA256_Update(SHA256_CTX* c, const void* data, size_t len);
void SHA256_Final(unsigned char* md, SHA256_CTX* c);
unsigned char* SHA256(const unsigned char* d, size_t n, unsigned char* md);

// SHA512 Functions
void SHA512_Init(SHA512_CTX* c);
void SHA512_Update(SHA512_CTX* c, const void* data, size_t len);
void SHA512_Final(unsigned char* md, SHA512_CTX* c);
unsigned char* SHA512(const unsigned char* d, size_t n, unsigned char* md);

// SSL-related (stub)
#define TLS1_2_VERSION 0x0303
#define TLS1_3_VERSION 0x0304


// RAND functions backed by the OS CSPRNG.
// Never fall back to rand()/mt19937 for key material: on total CSPRNG
// failure RAND_bytes returns 0 and callers must treat it as fatal.
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
inline int RAND_bytes(unsigned char* buf, int num) {
    if (num < 0) return 0;
    if (num == 0) return 1;
    return BCryptGenRandom(nullptr, buf, static_cast<ULONG>(num),
                           BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0 ? 1 : 0;
}
#else
#include <cstdio>
#include <cerrno>
#if defined(__linux__)
#include <sys/random.h>
#endif
inline int RAND_bytes(unsigned char* buf, int num) {
    if (num < 0) return 0;
    if (num == 0) return 1;
    size_t remaining = static_cast<size_t>(num);
    unsigned char* p = buf;
#if defined(__linux__)
    while (remaining > 0) {
        ssize_t n = getrandom(p, remaining, 0);
        if (n > 0) {
            p += n;
            remaining -= static_cast<size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        break;  // getrandom unavailable: fall through to /dev/urandom
    }
    if (remaining == 0) return 1;
#endif
    FILE* f = fopen("/dev/urandom", "rb");
    if (!f) return 0;
    size_t got = fread(p, 1, remaining, f);
    fclose(f);
    return got == remaining ? 1 : 0;
}
#endif
inline void RAND_seed(const void*, int) {}

#endif // OPENSSL_STUB_H
