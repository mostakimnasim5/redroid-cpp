#include "VirtualPhonePro/CryptoUtils.hpp"
#include "openssl_stub.h"
#include "openssl_stub.h"
#include "openssl_stub.h"
#include "openssl_stub.h"
#include "openssl_stub.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <regex>
#include <algorithm>

namespace VirtualPhonePro {

std::string CryptoUtils::md5(const std::string& input) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), digest);
    
    std::stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
}

std::string CryptoUtils::sha256(const std::string& input) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), digest);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
}

std::string CryptoUtils::sha512(const std::string& input) {
    unsigned char digest[SHA512_DIGEST_LENGTH];
    SHA512(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), digest);
    
    std::stringstream ss;
    for (int i = 0; i < SHA512_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
}

std::string CryptoUtils::generateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    ss << std::hex;
    ss << std::setfill('0') << std::setw(8) << dis(gen) << dis(gen) << dis(gen) << dis(gen)
       << dis(gen) << dis(gen) << dis(gen) << dis(gen) << "-";
    ss << std::setw(4) << dis(gen) << dis(gen) << dis(gen) << dis(gen) << "-";
    ss << "4" << std::setw(3) << dis(gen) << dis(gen) << dis(gen) << "-";
    ss << std::hex << (dis2(gen)) << dis(gen) << dis(gen) << dis(gen) << "-";
    ss << std::setfill('0') << std::setw(12) << dis(gen) << dis(gen) << dis(gen) 
       << dis(gen) << dis(gen) << dis(gen);
    
    return ss.str();
}

std::string CryptoUtils::generateRandomString(int length) {
    const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
    
    std::string result;
    for (int i = 0; i < length; ++i) {
        result += chars[dis(gen)];
    }
    return result;
}

std::string CryptoUtils::generateRandomHex(int length) {
    const char chars[] = "0123456789ABCDEF";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
    
    std::string result;
    for (int i = 0; i < length; ++i) {
        result += chars[dis(gen)];
    }
    return result;
}

std::string CryptoUtils::base64Encode(const std::string& input) {
    static const char* base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int i = 0;
    unsigned char charArray3[3];
    unsigned char charArray4[4];
    
    for (size_t j = 0; j < input.length(); j++) {
        charArray3[i++] = input[j];
        if (i == 3) {
            charArray4[0] = (charArray3[0] & 0xfc) >> 2;
            charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
            charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
            charArray4[3] = charArray3[2] & 0x3f;
            
            for(i = 0; i < 4; i++)
                result += base64Chars[charArray4[i]];
            i = 0;
        }
    }
    
    if (i > 0) {
        for(int j = i; j < 3; j++)
            charArray3[j] = '\0';
        charArray4[0] = (charArray3[0] & 0xfc) >> 2;
        charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
        charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
        for(int j = 0; j < i + 1; j++)
            result += base64Chars[charArray4[j]];
        while(i++ < 3)
            result += '=';
    }
    
    return result;
}

std::string CryptoUtils::base64Decode(const std::string& input) {
    static const unsigned char base64DecodeTable[256] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
    };
    
    std::string result;
    unsigned char charArray4[4];
    int i = 0;
    unsigned char c;
    
    for (size_t k = 0; k < input.length(); k++) {
        c = static_cast<unsigned char>(input[k]);
        if (c == '=') break;
        if (base64DecodeTable[c] >= 64) continue;
        charArray4[i++] = c;
        if (i == 4) {
            result += static_cast<char>((charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4));
            result += static_cast<char>(((charArray4[1] & 0x0f) << 4) + ((charArray4[2] & 0x3c) >> 2));
            result += static_cast<char>(((charArray4[2] & 0x03) << 6) + charArray4[3]);
            i = 0;
        }
    }
    
    if (i > 0) {
        for (int k = i; k < 4; k++) charArray4[k] = 0;
        result += static_cast<char>((charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4));
        if (i > 1) result += static_cast<char>(((charArray4[1] & 0x0f) << 4) + ((charArray4[2] & 0x3c) >> 2));
        if (i > 2) result += static_cast<char>(((charArray4[2] & 0x03) << 6) + charArray4[3]);
    }
    
    return result;
}

std::string CryptoUtils::xorEncrypt(const std::string& input, const std::string& key) {
    std::string output = input;
    for (size_t i = 0; i < input.length(); ++i) {
        output[i] = input[i] ^ key[i % key.length()];
    }
    return output;
}

std::string CryptoUtils::xorDecrypt(const std::string& input, const std::string& key) {
    return xorEncrypt(input, key);
}

bool CryptoUtils::isValidHex(const std::string& hex) {
    return std::regex_match(hex, std::regex("^[0-9A-Fa-f]+$"));
}

bool CryptoUtils::isValidMAC(const std::string& mac) {
    std::regex macRegex("^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$");
    return std::regex_match(mac, macRegex);
}

bool CryptoUtils::isValidIPv4(const std::string& ip) {
    std::regex ipv4Regex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    return std::regex_match(ip, ipv4Regex);
}

bool CryptoUtils::isValidIPv6(const std::string& ip) {
    std::regex ipv6Regex("^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$");
    return std::regex_match(ip, ipv6Regex) || ip == "::1" || ip == "::";
}

std::string CryptoUtils::sanitizeString(const std::string& input) {
    std::string output;
    for (char c : input) {
        if (c >= 32 && c < 127) {
            output += c;
        }
    }
    return output;
}

std::string CryptoUtils::escapeString(const std::string& input) {
    std::string output;
    for (char c : input) {
        switch (c) {
            case '\\': output += "\\\\"; break;
            case '"': output += "\\\""; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c; break;
        }
    }
    return output;
}

std::vector<unsigned char> CryptoUtils::hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteStr = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoi(byteStr, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::string CryptoUtils::bytesToHex(const std::vector<unsigned char>& bytes) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char byte : bytes) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

}
