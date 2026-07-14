#pragma once

#ifndef VIRTUALPHONEPRO_SAFETYNET_SPOOFER_H
#define VIRTUALPHONEPRO_SAFETYNET_SPOOFER_H

#include <QString>
#include <QMap>
#include <QByteArray>
#include <QJsonObject>

namespace VirtualPhonePro {

/**
 * @brief SafetyNet/Play Integrity Spoofing Module
 * 
 * Provides methods to bypass SafetyNet, Play Integrity, and other
 * hardware attestation checks used by banking and security-sensitive apps.
 */
class SafetyNetSpoofer {
public:
    static SafetyNetSpoofer& instance();
    
    // =========================================================================
    // SafetyNet Attestation
    // =========================================================================
    
    /**
     * @briefspoofSafetyNetResponse - Spoof SafetyNet attestation response
     * @param instanceId Target instance
     * @return true if spoofed successfully
     */
    bool spoofSafetyNetResponse(const QString& instanceId);
    
    /**
     * @briefspoofPlayIntegrity - Spoof Play Integrity API response
     * @param instanceId Target instance
     * @return true if spoofed successfully
     */
    bool spoofPlayIntegrity(const QString& instanceId);
    
    /**
     * @briefspoofBasicIntegrity - Spoof basic integrity check
     * @param instanceId Target instance
     * @return true if spoofed successfully
     */
    bool spoofBasicIntegrity(const QString& instanceId);
    
    /**
     * @briefspoofDeviceIntegrity - Spoof device integrity flags
     * @param instanceId Target instance
     * @return true if spoofed successfully
     */
    bool spoofDeviceIntegrity(const QString& instanceId);
    
    /**
     * @briefspoofGooglePlayServices - Spoof Play Services compatibility
     * @param instanceId Target instance
     * @return true if spoofed successfully
     */
    bool spoofGooglePlayServices(const QString& instanceId);
    
    // =========================================================================
     // Integrity Check Configuration
    // =========================================================================
    
    /**
     * @briefspoofctsProfileMatch - Spoof CTS profile match
     * @param instanceId Target instance
     * @param shouldMatch true = pass, false = fail
     */
    bool spoofCtsProfileMatch(const QString& instanceId, bool shouldMatch = true);
    
    /**
     * @briefspoofBasicIntegrityCheck - Spoof basic integrity
     * @param instanceId Target instance
     * @param isSecure true = pass, false = fail
     */
    bool spoofBasicIntegrityCheck(const QString& instanceId, bool isSecure = true);
    
    /**
     * @briefspoofStrongIntegrity - Spoof strong integrity
     * @param instanceId Target instance
     * @param isSecure true = pass, false = fail
     */
    bool spoofStrongIntegrity(const QString& instanceId, bool isSecure = true);
    
    /**
     * @briefspoofMeetsDeviceIntegrity - Spoof meetsDeviceIntegrity
     * @param instanceId Target instance
     * @param meets true = pass, false = fail
     */
    bool spoofMeetsDeviceIntegrity(const QString& instanceId, bool meets = true);
    
    /**
     * @briefspoofMeetsStrongIntegrity - Spoof meetsStrongIntegrity
     * @param instanceId Target instance
     * @param meets true = pass, false = fail
     */
    bool spoofMeetsStrongIntegrity(const QString& instanceId, bool meets = true);
    
    // =========================================================================
    // Verification Helpers
    // =========================================================================
    
    /**
     * @briefverifySafetyNet - Verify SafetyNet check
     * @param instanceId Target instance
     * @return true if check would pass
     */
    bool verifySafetyNet(const QString& instanceId);
    
    /**
     * @briefverifyPlayIntegrity - Verify Play Integrity check
     * @param instanceId Target instance
     * @return true if check would pass
     */
    bool verifyPlayIntegrity(const QString& instanceId);
    
    /**
     * @briefgetIntegrityStatus - Get current integrity status
     * @param instanceId Target instance
     * @return Status object
     */
    QJsonObject getIntegrityStatus(const QString& instanceId);
    
    // =========================================================================
    // Certificate & Signature Spoofing
    // =========================================================================
    
    /**
     * @briefinstallSpoofedGmsCore - Install spoofed GMS Core with SafetyNet
     * @param instanceId Target instance
     * @return true if installed successfully
     */
    bool installSpoofedGmsCore(const QString& instanceId);
    
    /**
     * @briefpatchSafetyNetAttestation - Patch SafetyNet attestation library
     * @param instanceId Target instance
     * @return true if patched successfully
     */
    bool patchSafetyNetAttestation(const QString& instanceId);
    
    /**
     * @briefsetPlayProtectionRating - Set Play Protection rating
     * @param instanceId Target instance
     * @param rating Rating level (MEETS_SECURITY_LABELS, etc.)
     */
    bool setPlayProtectionRating(const QString& instanceId, const QString& rating);

private:
    SafetyNetSpoofer() = default;
    
    // Internal helpers
    bool executeCommand(const QString& instanceId, const QString& command);
    QString executeCommandSync(const QString& instanceId, const QString& command);
    bool pushFile(const QString& instanceId, const QString& local, const QString& remote);
    bool installApk(const QString& instanceId, const QString& apk);
    
    // Certificate patching
    bool patchCertDatabase(const QString& instanceId);
    bool installCACerts(const QString& instanceId);
    
    // SafetyNet response generation
    QByteArray generateSafetyNetJws(const QString& instanceId);
    QJsonObject generatePlayIntegrityResponse(const QString& instanceId);
    
    // Debug flags
    bool setDebugFlags(const QString& instanceId);
    bool clearRootFlags(const QString& instanceId);
    bool setSecureFlags(const QString& instanceId);
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_SAFETYNET_SPOOFER_H
