#include "CryptoEmulator.hpp"
#include "VirtualSecurityChip.hpp"
#include "Logger.hpp"
#include "openssl_stub.h"
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace AntiDetect {

CryptoEmulator::CryptoEmulator()
    : m_initialized(false)
    , m_deviceBrand("Samsung")
    , m_deviceModel("SM-G998B")
    , m_androidVersion("13")
    , m_securityPatch("2023-12-01")
{
}

CryptoEmulator::~CryptoEmulator() {
    shutdown();
}

CryptoEmulator& CryptoEmulator::getInstance() {
    static CryptoEmulator instance;
    return instance;
}

bool CryptoEmulator::initialize() {
    if (m_initialized) {
        Logger::getInstance().warning("CryptoEmulator already initialized");
        return true;
    }
    
    Logger::getInstance().info("Initializing CryptoEmulator...");
    
    // Initialize virtual security chip
    auto& vsc = VirtualSecurityChip::getInstance();
    if (!vsc.initialize()) {
        Logger::getInstance().error("Failed to initialize Virtual Security Chip");
        return false;
    }
    
    m_initialized = true;
    Logger::getInstance().info("CryptoEmulator initialized successfully");
    
    return true;
}

void CryptoEmulator::shutdown() {
    if (!m_initialized) return;
    
    Logger::getInstance().info("Shutting down CryptoEmulator...");
    
    m_certificates.clear();
    m_certificateChains.clear();
    m_keyPairs.clear();
    
    m_initialized = false;
}

bool CryptoEmulator::loadCertificate(const std::string& certData, const std::string& alias) {
    if (certData.empty() || alias.empty()) {
        Logger::getInstance().error("Invalid certificate or alias");
        return false;
    }
    
    m_certificates[alias] = certData;
    Logger::getInstance().info("Certificate loaded: " + alias);
    
    return true;
}

bool CryptoEmulator::loadCertificateChain(const std::vector<std::string>& certChain, 
                                         const std::string& alias) {
    if (certChain.empty()) {
        Logger::getInstance().error("Empty certificate chain");
        return false;
    }
    
    m_certificateChains[alias] = certChain;
    Logger::getInstance().info("Certificate chain loaded: " + alias + 
                              " (" + std::to_string(certChain.size()) + " certs)");
    
    return true;
}

std::string CryptoEmulator::getCertificate(const std::string& alias) const {
    auto it = m_certificates.find(alias);
    if (it != m_certificates.end()) {
        return it->second;
    }
    return "";
}

std::vector<std::string> CryptoEmulator::getCertificateChain(const std::string& alias) const {
    auto it = m_certificateChains.find(alias);
    if (it != m_certificateChains.end()) {
        return it->second;
    }
    return {};
}

bool CryptoEmulator::injectKeyPair(const std::string& privateKey, 
                                  const std::string& publicKey,
                                  const std::string& alias) {
    if (privateKey.empty() || publicKey.empty() || alias.empty()) {
        Logger::getInstance().error("Invalid key data or alias");
        return false;
    }
    
    auto& vsc = VirtualSecurityChip::getInstance();
    if (!vsc.injectHardwareKey(privateKey, alias)) {
        return false;
    }
    
    m_keyPairs[alias + "_public"] = publicKey;
    
    Logger::getInstance().info("Key pair injected: " + alias);
    return true;
}

bool CryptoEmulator::injectHardwareAttestationKey(const std::string& keyData,
                                                 const std::string& certChain,
                                                 const std::string& alias) {
    if (keyData.empty() || alias.empty()) {
        return false;
    }
    
    // Inject the key into virtual security chip
    auto& vsc = VirtualSecurityChip::getInstance();
    if (!vsc.injectHardwareKey(keyData, alias)) {
        return false;
    }
    
    // Parse and store certificate chain if provided
    if (!certChain.empty()) {
        std::vector<std::string> chains;
        std::string current;
        for (char c : certChain) {
            if (c == '|') {
                if (!current.empty()) {
                    chains.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            chains.push_back(current);
        }
        
        if (!chains.empty()) {
            m_certificateChains[alias] = chains;
        }
    }
    
    Logger::getInstance().info("Hardware attestation key injected: " + alias);
    return true;
}

std::string CryptoEmulator::generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    // Format: YYYY-MM-DDThh:mm:ssZ
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&time);
    
    std::stringstream ss;
    ss << std::put_time(tm, "%Y-%m-%dT%H:%M:%S");
    ss << "." << (millis % 1000) << "Z";
    
    return ss.str();
}

std::string CryptoEmulator::generateJwsHeader(const std::string& alg) {
    std::stringstream header;
    header << "{";
    header << "\"alg\":\"" << alg << "\",";
    header << "\"typ\":\"JWT\",";
    header << "\"x5c\":[";
    
    // Add certificate chain if available
    auto chains = getCertificateChain("attestation");
    for (size_t i = 0; i < chains.size(); i++) {
        if (i > 0) header << ",";
        header << "\"" << base64UrlEncode(chains[i]) << "\"";
    }
    
    header << "]}";
    return base64UrlEncode(header.str());
}

std::string CryptoEmulator::generateJwsPayload(const std::string& nonce,
                                              const std::string& packageName,
                                              const std::string& apkDigest) {
    std::stringstream payload;
    payload << "{";
    payload << "\"nonce\":\"" << base64UrlEncode(nonce) << "\",";
    payload << "\"timestamp\":{";
    payload << "\"millis\":" << std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    payload << "},";
    payload << "\"packageName\":\"" << packageName << "\",";
    payload << "\"apkPackageName\":\"" << packageName << "\",";
    payload << "\"apkDigestSha256\":\"" << apkDigest << "\",";
    payload << "\"basicIntegrity\":true,";
    payload << "\"ctsProfileMatch\":true,";
    payload << "\"deviceIntegrity\":[\"deviceProperties\",\"bootIntegrity\",\"hardwareProperties\"],";
    payload << "\"evaluationType\":\"EVALUATION_TYPE_BASIC_AND_HARDWARE_ATTESTATION\",";
    payload << "\"evaluationTypeBasic\":\"EVALUATION_TYPE_BASIC\",";
    payload << "\"evaluationTypeHardwareBacked\":\"EVALUATION_TYPE_HARDWARE_ATTESTATION\",";
    payload << "\"advice\":\"\",";
    payload << "\"verificationLog\":\"\",";
    
    // Device properties
    payload << "\"device\":{";
    payload << "\"brand\":\"" << m_deviceBrand << "\",";
    payload << "\"model\":\"" << m_deviceModel << "\",";
    payload << "\"manufacturer\":\"" << m_deviceBrand << "\",";
    payload << "\"device\":\"" << m_deviceModel << "\",";
    payload << "\"product\":\"" << m_deviceModel << "\",";
    payload << "\"osVersion\":\"" << m_androidVersion << "\",";
    payload << "\"securityPatch\":\"" << m_securityPatch << "\",";
    payload << "\"build\":{";
    payload << "\"ID\":\"" << m_securityPatch << "\",";
    payload << "\"FINGERPRINT\":\"" << m_deviceBrand << "/" << m_deviceModel 
           << "/" << m_deviceModel << ":" << m_androidVersion << "/" << m_securityPatch << "/"
           << "user/release-keys\"";
    payload << "}},";
    
    // Timestamp
    payload << "\"timestamp\":\"" << generateTimestamp() << "\"";
    
    payload << "}";
    return base64UrlEncode(payload.str());
}

std::string CryptoEmulator::createJwsSignature(const std::string& header,
                                              const std::string& payload) {
    auto& vsc = VirtualSecurityChip::getInstance();
    
    std::string dataToSign = header + "." + payload;
    
    // Sign with injected hardware key or generate simulated signature
    std::string signature;
    if (vsc.hasHardwareKey("attestation_key")) {
        signature = vsc.signData(dataToSign, "attestation_key");
    } else {
        // Generate simulated signature using SHA256
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(dataToSign.data()), 
               dataToSign.size(), hash);
        signature = base64UrlEncode(std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH));
    }
    
    return signature;
}

std::string CryptoEmulator::base64UrlEncode(const std::string& data) {
    static const char encodeTable[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    
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

std::string CryptoEmulator::generateSafetyNetResponse(const std::string& nonce,
                                                      const std::string& packageName,
                                                      const std::string& apkDigest) {
    std::string jwsHeader = generateJwsHeader("RS256");
    std::string jwsPayload = generateJwsPayload(nonce, packageName, apkDigest);
    std::string jwsSignature = createJwsSignature(jwsHeader, jwsPayload);
    
    std::string jws = jwsHeader + "." + jwsPayload + "." + jwsSignature;
    
    Logger::getInstance().debug("Generated SafetyNet JWS response");
    return jws;
}

std::string CryptoEmulator::generatePlayIntegrityResponse(const std::string& nonce) {
    std::string jwsHeader = generateJwsHeader("ES256");
    std::string jwsPayload = generateJwsPayload(nonce, m_packageName, m_apkDigest);
    std::string jwsSignature = createJwsSignature(jwsHeader, jwsPayload);
    
    return jwsHeader + "." + jwsPayload + "." + jwsSignature;
}

std::string CryptoEmulator::generateBasicIntegrityResponse() {
    std::stringstream response;
    response << "{";
    response << "\"basicIntegrity\":true,";
    response << "\"ctsProfileMatch\":true,";
    response << "\"deviceIntegrity\":[\"deviceProperties\"],";
    response << "\"evaluationType\":\"EVALUATION_TYPE_BASIC\"";
    response << "}";
    return response.str();
}

bool CryptoEmulator::verifyCertificateChain(const std::vector<std::string>& chain) const {
    // Simplified verification - in production, verify each certificate
    // signs the next one in the chain
    if (chain.empty()) {
        return false;
    }
    
    // Check if chain is properly ordered
    return chain.size() >= 2;
}

std::string CryptoEmulator::extractPublicKeyHash(const std::string& certificate) const {
    if (certificate.empty()) {
        return "";
    }
    
    // Calculate SHA-256 hash of the certificate
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(certificate.data()),
           certificate.size(), hash);
    
    std::string hashStr(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
    
    // Convert to hex
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(hash[i]);
    }
    
    return ss.str();
}

void CryptoEmulator::setDeviceBrand(const std::string& brand) {
    m_deviceBrand = brand;
}

void CryptoEmulator::setDeviceModel(const std::string& model) {
    m_deviceModel = model;
}

void CryptoEmulator::setAndroidVersion(const std::string& version) {
    m_androidVersion = version;
}

void CryptoEmulator::setSecurityPatch(const std::string& patch) {
    m_securityPatch = patch;
}

std::string CryptoEmulator::getSignedAttestationResponse(const std::string& nonce,
                                                          const std::string& packageName) {
    // Generate default APK digest if empty
    std::string apkDigest = m_apkDigest;
    if (apkDigest.empty()) {
        apkDigest = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
    }
    
    return generateSafetyNetResponse(nonce, packageName, apkDigest);
}

void CryptoEmulator::reset() {
    m_certificates.clear();
    m_certificateChains.clear();
    m_keyPairs.clear();
    m_initialized = false;
    
    auto& vsc = VirtualSecurityChip::getInstance();
    vsc.reset();
    
    Logger::getInstance().info("CryptoEmulator reset");
}

} // namespace AntiDetect
