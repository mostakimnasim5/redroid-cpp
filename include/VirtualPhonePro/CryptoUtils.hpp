#pragma once

#include <string>
#include <vector>

namespace AntiDetect {

class CryptoUtils {
public:
    static std::string md5(const std::string& input);
    static std::string sha256(const std::string& input);
    static std::string sha512(const std::string& input);
    
    static std::string generateUUID();
    static std::string generateRandomString(int length);
    static std::string generateRandomHex(int length);
    
    static std::string base64Encode(const std::string& input);
    static std::string base64Decode(const std::string& input);
    
    static std::string xorEncrypt(const std::string& input, const std::string& key);
    static std::string xorDecrypt(const std::string& input, const std::string& key);
    
    static bool isValidHex(const std::string& hex);
    static bool isValidMAC(const std::string& mac);
    static bool isValidIPv4(const std::string& ip);
    static bool isValidIPv6(const std::string& ip);
    
    static std::string sanitizeString(const std::string& input);
    static std::string escapeString(const std::string& input);
    
    static std::vector<unsigned char> hexToBytes(const std::string& hex);
    static std::string bytesToHex(const std::vector<unsigned char>& bytes);
};

}
