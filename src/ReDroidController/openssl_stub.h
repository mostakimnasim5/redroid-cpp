/**
 * @file openssl_stub.h
 * @brief OpenSSL Stub for systems without OpenSSL
 * 
 * Provides cryptographic functions without requiring OpenSSL linkage.
 * Uses system crypto functions or provides fallback implementations.
 */

#pragma once

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

#endif // OPENSSL_STUB_H
