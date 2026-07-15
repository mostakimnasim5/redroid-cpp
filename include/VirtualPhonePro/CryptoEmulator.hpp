#ifndef CRYPTO_EMULATOR_HPP
#define CRYPTO_EMULATOR_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace VirtualPhonePro {

/**
 * CryptoEmulator - Cryptographic Certificate and Key Injection System
 * 
 * Provides ability to inject real hardware certificates and keys
 * for generating valid attestation responses.
 * 
 * Features:
 * - X.509 certificate parsing and storage
 * - Hardware key injection from real devices
 * - Attestation certificate chain management
 * - SafetyNet/Play Integrity response generation
 */
class CryptoEmulator {
public:
    static CryptoEmulator& getInstance();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Certificate Management
    bool loadCertificate(const std::string& certData, const std::string& alias);
    bool loadCertificateChain(const std::vector<std::string>& certChain, const std::string& alias);
    std::string getCertificate(const std::string& alias) const;
    std::vector<std::string> getCertificateChain(const std::string& alias) const;
    
    // Key Injection
    bool injectKeyPair(const std::string& privateKey, const std::string& publicKey, 
                      const std::string& alias);
    bool injectHardwareAttestationKey(const std::string& keyData,
                                     const std::string& certChain,
                                     const std::string& alias);
    
    // Attestation Response Generation
    std::string generateSafetyNetResponse(const std::string& nonce,
                                          const std::string& packageName,
                                          const std::string& apkDigest);
    
    std::string generatePlayIntegrityResponse(const std::string& nonce);
    
    std::string generateBasicIntegrityResponse();
    
    // Certificate Verification
    bool verifyCertificateChain(const std::vector<std::string>& chain) const;
    std::string extractPublicKeyHash(const std::string& certificate) const;
    
    // Device Identity
    void setDeviceBrand(const std::string& brand);
    void setDeviceModel(const std::string& model);
    void setAndroidVersion(const std::string& version);
    void setSecurityPatch(const std::string& patch);
    
    std::string getSignedAttestationResponse(const std::string& nonce,
                                           const std::string& packageName);
    
    // Reset
    void reset();

private:
    CryptoEmulator();
    ~CryptoEmulator();
    CryptoEmulator(const CryptoEmulator&) = delete;
    CryptoEmulator& operator=(const CryptoEmulator&) = delete;
    
    std::string generateTimestamp();
    std::string generateJwsHeader(const std::string& alg);
    std::string generateJwsPayload(const std::string& nonce,
                                   const std::string& packageName,
                                   const std::string& apkDigest);
    std::string createJwsSignature(const std::string& header,
                                   const std::string& payload);
    std::string base64UrlEncode(const std::string& data);
    
    // Stored certificates and keys
    std::map<std::string, std::string> m_certificates;
    std::map<std::string, std::vector<std::string>> m_certificateChains;
    std::map<std::string, std::string> m_keyPairs;
    
    // Device info for attestation
    bool m_initialized;
    std::string m_deviceBrand;
    std::string m_deviceModel;
    std::string m_androidVersion;
    std::string m_securityPatch;
    std::string m_packageName;
    std::string m_apkDigest;
};

} // namespace VirtualPhonePro

#endif // CRYPTO_EMULATOR_HPP
