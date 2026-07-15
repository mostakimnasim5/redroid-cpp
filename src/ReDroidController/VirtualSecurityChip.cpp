#include "VirtualSecurityChip.hpp"
#include "CryptoUtils.hpp"
#include "Logger.hpp"
#include "openssl_stub.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstring>

namespace VirtualPhonePro {

VirtualSecurityChip::VirtualSecurityChip()
    : m_initialized(false)
    , m_bootState("green")
    , m_securityLevel("STRONG_BOX")
{
    m_startTime = std::chrono::steady_clock::now();
}

VirtualSecurityChip::~VirtualSecurityChip() {
    shutdown();
}

VirtualSecurityChip& VirtualSecurityChip::getInstance() {
    static VirtualSecurityChip instance;
    return instance;
}

bool VirtualSecurityChip::initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        Logger::getInstance().warning("VirtualSecurityChip already initialized");
        return true;
    }
    
    Logger::getInstance().info("Initializing Virtual Security Chip (Software TEE)...");
    
    // Generate unique device ID
    m_deviceUniqueId = generateSecureDeviceId();
    
    // Initialize boot chain
    m_bootCertificate = buildVerifiedBootChain();
    
    // Set default security level
    m_securityLevel = "STRONG_BOX";
    
    // Initialize attestation chain
    m_attestationChain = {
        "GTS_ROOT_R1",      // Google Trust Services Root
        "GTS_INTERMEDIATE_R1", // Google Trust Services Intermediate
        "DEVICE_ATTESTATION"    // Device attestation certificate
    };
    
    m_initialized = true;
    Logger::getInstance().info("Virtual Security Chip initialized successfully");
    Logger::getInstance().info("Device ID: " + m_deviceUniqueId.substr(0, 16) + "...");
    Logger::getInstance().info("Security Level: " + m_securityLevel);
    
    return true;
}

void VirtualSecurityChip::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) return;
    
    Logger::getInstance().info("Shutting down Virtual Security Chip...");
    
    // Securely clear keys
    m_secureKeys.clear();
    m_deviceUniqueId.clear();
    m_boundProfileId.clear();
    
    m_initialized = false;
    Logger::getInstance().info("Virtual Security Chip shutdown complete");
}

bool VirtualSecurityChip::isInitialized() const {
    return m_initialized;
}

std::string VirtualSecurityChip::generateRandomBytes(size_t length) {
    std::string result;
    result.reserve(length);
    
    // Use OpenSSL RAND_bytes or fallback
    unsigned char* buffer = new unsigned char[length];
    
    // Initialize random seed
    RAND_seed(__FILE__, sizeof(__FILE__));
    
    if (RAND_bytes(buffer, static_cast<int>(length)) != 1) {
        // Fallback to std::random if OpenSSL fails
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (size_t i = 0; i < length; i++) {
            buffer[i] = static_cast<unsigned char>(dis(gen));
        }
    }
    
    result.assign(reinterpret_cast<char*>(buffer), length);
    delete[] buffer;
    
    return result;
}

std::string VirtualSecurityChip::calculateSHA256(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);
    return std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
}

std::string VirtualSecurityChip::calculateSHA512(const std::string& data) {
    unsigned char hash[SHA512_DIGEST_LENGTH];
    SHA512(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);
    return std::string(reinterpret_cast<char*>(hash), SHA512_DIGEST_LENGTH);
}

std::string VirtualSecurityChip::base64Encode(const std::string& data) {
    static const char encodeTable[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
    int i = 0;
    int j = 0;
    unsigned char charArray3[3];
    unsigned char charArray4[4];
    int inLen = static_cast<int>(data.size());
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(data.data());
    
    while (inLen--) {
        charArray3[i++] = *(bytes++);
        if (i == 3) {
            charArray4[0] = (charArray3[0] & 0xfc) >> 2;
            charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
            charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
            charArray4[3] = charArray3[2] & 0x3f;
            
            for(i = 0; i < 4; i++) {
                result += encodeTable[charArray4[i]];
            }
            i = 0;
        }
    }
    
    if (i) {
        for(j = i; j < 3; j++) {
            charArray3[j] = '\0';
        }
        
        charArray4[0] = (charArray3[0] & 0xfc) >> 2;
        charArray4[1] = ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
        charArray4[2] = ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
        
        for (j = 0; j < i + 1; j++) {
            result += encodeTable[charArray4[j]];
        }
        
        while((i++ < 3)) {
            result += '=';
        }
    }
    
    return result;
}

std::string VirtualSecurityChip::base64Decode(const std::string& data) {
    static const unsigned char decodeTable[256] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0, 63, 52, 53, 54, 55, 56, 57,
        58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 0,
        0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
        44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0
    };
    
    std::string result;
    int inLen = static_cast<int>(data.size());
    int i = 0, j = 0;
    int in = 0;
    unsigned char charArray4[4], charArray3[3];
    
    while (inLen-- && (data[in] != '=') && isalnum(data[in])) {
        charArray4[i++] = data[in]; in++;
        if (i == 4) {
            for (i = 0; i < 4; i++) {
                charArray4[i] = decodeTable[charArray4[i]];
            }
            charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
            charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
            charArray3[2] = ((charArray4[2] & 0x3) << 6) + charArray4[3];
            
            for (i = 0; i < 3; i++) {
                result += charArray3[i];
            }
            i = 0;
        }
    }
    
    if (i) {
        for (int j = 0; j < i; j++) {
            charArray4[j] = decodeTable[charArray4[j]];
        }
        charArray3[0] = (charArray4[0] << 2) + ((charArray4[1] & 0x30) >> 4);
        charArray3[1] = ((charArray4[1] & 0xf) << 4) + ((charArray4[2] & 0x3c) >> 2);
        
        for (int j = 0; j < i - 1; j++) {
            result += charArray3[j];
        }
    }
    
    return result;
}

std::string VirtualSecurityChip::generateSecureDeviceId() {
    std::string timestamp = generateSecureTimestamp();
    std::string random = generateRandomBytes(16);
    std::string combined = timestamp + random + "VSC"; // VSC = Virtual Security Chip
    
    return calculateSHA256(combined);
}

std::string VirtualSecurityChip::generateGSFId() {
    // GSF ID format: 16 digits starting with "af-"
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);
    
    std::stringstream ss;
    ss << "af-";
    for (int i = 0; i < 12; i++) {
        ss << dis(gen);
    }
    
    return ss.str();
}

std::string VirtualSecurityChip::generateIMEI(const std::string& TAC) {
    // TAC (Type Allocation Code) is first 8 digits
    // IMEI = TAC + 6 digits + Luhn check digit
    std::string baseIMEI = TAC;
    
    while (baseIMEI.length() < 14) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 9);
        baseIMEI += std::to_string(dis(gen));
    }
    
    baseIMEI = baseIMEI.substr(0, 14);
    baseIMEI += std::to_string(calculateLuhnCheckDigit(baseIMEI));
    
    return baseIMEI;
}

std::string VirtualSecurityChip::generateSerialNumber() {
    // Generate realistic serial number format
    // Format: 2 letters + 10 alphanumeric
    static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(chars) - 2);
    
    std::stringstream ss;
    // Add manufacturer code (2 letters)
    ss << chars[dis(gen)] << chars[dis(gen)];
    ss << "-";
    // Add 10 alphanumeric characters
    for (int i = 0; i < 10; i++) {
        ss << chars[dis(gen)];
    }
    
    return ss.str();
}

bool VirtualSecurityChip::injectHardwareKey(const std::string& keyData, const std::string& keyAlias) {
    if (!m_initialized) {
        Logger::getInstance().error("Cannot inject key: Security chip not initialized");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    SecureKey key;
    key.keyData = keyData;
    key.keyId = calculateSHA256(keyAlias + keyData);
    key.creationTime = generateSecureTimestamp();
    key.hardwareBacked = true; // Simulating hardware-backed
    key.keyType = "INJECTED";
    key.attributes["injected"] = "true";
    key.attributes["origin"] = "EXTRACTED";
    
    m_secureKeys[keyAlias] = key;
    
    Logger::getInstance().info("Hardware key injected: " + keyAlias);
    return true;
}

bool VirtualSecurityChip::hasHardwareKey(const std::string& keyAlias) const {
    return m_secureKeys.find(keyAlias) != m_secureKeys.end();
}

std::string VirtualSecurityChip::getHardwareKeyId(const std::string& keyAlias) const {
    auto it = m_secureKeys.find(keyAlias);
    if (it != m_secureKeys.end()) {
        return it->second.keyId;
    }
    return "";
}

bool VirtualSecurityChip::verifyLuhn(const std::string& number) const {
    int sum = 0;
    bool alternate = false;
    
    for (int i = static_cast<int>(number.length()) - 1; i >= 0; i--) {
        int n = number[i] - '0';
        if (alternate) {
            n *= 2;
            if (n > 9) n -= 9;
        }
        sum += n;
        alternate = !alternate;
    }
    
    return (sum % 10 == 0);
}

int VirtualSecurityChip::calculateLuhnCheckDigit(const std::string& baseNumber) const {
    int sum = 0;
    bool alternate = true;
    
    for (int i = static_cast<int>(baseNumber.length()) - 1; i >= 0; i--) {
        int n = baseNumber[i] - '0';
        if (alternate) {
            n *= 2;
            if (n > 9) n -= 9;
        }
        sum += n;
        alternate = !alternate;
    }
    
    return (10 - (sum % 10)) % 10;
}

std::string VirtualSecurityChip::generateKeyAttestation(const std::string& challenge) {
    if (!m_initialized) {
        Logger::getInstance().error("Cannot attest: Security chip not initialized");
        return "";
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Build attestation statement
    std::stringstream attestation;
    
    // Header
    attestation << "{\"attestation\":{";
    attestation << "\"version\":1,";
    attestation << "\"challenge\":\"" << base64Encode(challenge) << "\",";
    attestation << "\"timestamp\":\"" << generateSecureTimestamp() << "\",";
    attestation << "\"deviceUniqueId\":\"" << m_deviceUniqueId << "\",";
    
    // Security level
    attestation << "\"securityLevel\":\"" << m_securityLevel << "\",";
    attestation << "\"keymasterVersion\":4,";
    attestation << "\"attestationChallenge\":\"" << base64Encode(challenge) << "\",";
    
    // Boot state
    attestation << "\"bootState\":\"" << m_bootState << "\",";
    attestation << "\"verifiedBootHash\":\"" << calculateSHA256(m_bootCertificate) << "\",";
    
    // Software info
    attestation << "\"software\":{";
    attestation << "\"version\":1,";
    attestation << "\"preview\":false,";
    attestation << "\"oemMachine\":\"" << m_deviceUniqueId.substr(0, 8) << "\",";
    attestation << "\"oemLocked\":true";
    attestation << "},";
    
    // Key claims
    attestation << "\"keyClaims\":[";
    bool first = true;
    for (const auto& kv : m_secureKeys) {
        if (!first) attestation << ",";
        attestation << "{\"alias\":\"" << kv.first << "\",";
        attestation << "\"keyId\":\"" << kv.second.keyId << "\",";
        attestation << "\"hardwareBacked\":" << (kv.second.hardwareBacked ? "true" : "false") << "}";
        first = false;
    }
    attestation << "],";
    
    // Attestation chain
    attestation << "\"attestationChain\":[";
    first = true;
    for (const auto& cert : m_attestationChain) {
        if (!first) attestation << ",";
        attestation << "\"" << cert << "\"";
        first = false;
    }
    attestation << "]},";
    
    // Signature
    std::string toSign = attestation.str() + challenge;
    std::string signature = base64Encode(calculateSHA512(toSign));
    attestation << "\"signature\":\"" << signature << "\"}}";
    
    return attestation.str();
}

std::string VirtualSecurityChip::generateAttestationStatement(const std::string& challenge) {
    return generateKeyAttestation(challenge);
}

std::map<std::string, std::string> VirtualSecurityChip::getAttestationClaims() {
    std::map<std::string, std::string> claims;
    
    claims["deviceId"] = m_deviceUniqueId;
    claims["securityLevel"] = m_securityLevel;
    claims["bootState"] = m_bootState;
    claims["bootCertificate"] = m_bootCertificate;
    claims["timestamp"] = generateSecureTimestamp();
    claims["keymasterVersion"] = "4";
    claims["keyCount"] = std::to_string(m_secureKeys.size());
    
    return claims;
}

std::string VirtualSecurityChip::signData(const std::string& data, const std::string& keyAlias) {
    if (!hasHardwareKey(keyAlias)) {
        Logger::getInstance().error("Cannot sign: Key not found - " + keyAlias);
        return "";
    }
    
    // Simulate hardware-backed signing
    std::string toSign = data + m_secureKeys[keyAlias].keyData + generateSecureTimestamp();
    return base64Encode(calculateSHA512(toSign));
}

bool VirtualSecurityChip::verifySignature(const std::string& data, const std::string& signature,
                                         const std::string& keyAlias) {
    if (!hasHardwareKey(keyAlias)) {
        return false;
    }
    
    std::string expectedSignature = signData(data, keyAlias);
    return signature == expectedSignature;
}

bool VirtualSecurityChip::createHardwareBackedKey(const std::string& keyAlias, int keySize) {
    if (!m_initialized) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    SecureKey key;
    key.keyData = generateRandomBytes(keySize / 8);
    key.keyId = calculateSHA256(keyAlias + key.keyData);
    key.creationTime = generateSecureTimestamp();
    key.hardwareBacked = true;
    key.keyType = "GENERATED";
    key.attributes["algorithm"] = "EC_P256";
    key.attributes["keySize"] = std::to_string(keySize);
    
    m_secureKeys[keyAlias] = key;
    
    Logger::getInstance().info("Hardware-backed key created: " + keyAlias);
    return true;
}

bool VirtualSecurityChip::isHardwareBackedKey(const std::string& keyAlias) const {
    auto it = m_secureKeys.find(keyAlias);
    if (it != m_secureKeys.end()) {
        return it->second.hardwareBacked;
    }
    return false;
}

void VirtualSecurityChip::setVerifiedBootState(const std::string& state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (state == "green" || state == "yellow" || state == "orange" || state == "red") {
        m_bootState = state;
        m_bootCertificate = buildVerifiedBootChain();
        Logger::getInstance().info("Boot state set to: " + state);
    }
}

std::string VirtualSecurityChip::getVerifiedBootState() const {
    return m_bootState;
}

std::string VirtualSecurityChip::getBootCertificate() const {
    return m_bootCertificate;
}

std::string VirtualSecurityChip::buildVerifiedBootChain() {
    std::stringstream chain;
    chain << "VB2|";
    chain << "kernel:";
    chain << calculateSHA256("kernel-" + m_deviceUniqueId) << "|";
    chain << "boot:";
    chain << calculateSHA256("boot-" + m_deviceUniqueId) << "|";
    chain << "verified:" << m_deviceUniqueId.substr(0, 16) << "|";
    chain << "lock:" << calculateSHA256(m_deviceUniqueId);
    
    return base64Encode(chain.str());
}

std::string VirtualSecurityChip::buildAttestationChain() {
    std::stringstream chain;
    chain << "ATTEST|";
    
    for (size_t i = 0; i < m_attestationChain.size(); i++) {
        if (i > 0) chain << "|";
        chain << m_attestationChain[i];
    }
    
    chain << "|";
    chain << "sig:" << calculateSHA256(chain.str());
    
    return base64Encode(chain.str());
}

std::string VirtualSecurityChip::generateSecureTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    std::stringstream ss;
    ss << millis;
    
    return ss.str();
}

std::string VirtualSecurityChip::calculateIntegrityToken(const std::string& data) {
    std::stringstream token;
    token << base64Encode(data) << ".";
    token << base64Encode(calculateSHA256(data + m_deviceUniqueId)) << ".";
    token << base64Encode(calculateSHA512(data + generateSecureTimestamp()));
    
    return token.str();
}

std::string VirtualSecurityChip::getSecurityLevel() const {
    return m_securityLevel;
}

bool VirtualSecurityChip::bindToProfile(const std::string& profileId) {
    if (!m_initialized) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_boundProfileId = profileId;
    
    Logger::getInstance().info("Security chip bound to profile: " + profileId);
    return true;
}

std::string VirtualSecurityChip::getBoundProfileId() const {
    return m_boundProfileId;
}

void VirtualSecurityChip::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_secureKeys.clear();
    m_deviceUniqueId.clear();
    m_boundProfileId.clear();
    m_bootState = "green";
    m_bootCertificate.clear();
    m_initialized = false;
    
    Logger::getInstance().info("Virtual Security Chip reset");
}

} // namespace VirtualPhonePro
