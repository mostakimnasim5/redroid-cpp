/**
 * @file HardwareId.hpp
 * @brief Cross-platform stable hardware identifier (HWID) collection
 * @version 1.0.0
 *
 * Provides a per-machine stable identifier used to anchor deterministic
 * profile generation (DeviceProfileGenerator) to real hardware.
 *
 * Resolution order:
 *  1. Platform-primary source
 *     - Windows: HKLM\\SOFTWARE\\Microsoft\\Cryptography\\MachineGuid
 *                + system volume serial number
 *     - Linux:   /etc/machine-id (fallback /var/lib/dbus/machine-id)
 *     - macOS:   kern.hostuuid via sysctlbyname
 *  2. QSysInfo::machineUniqueId()
 *  3. Persisted random UUID file (created once, reused forever)
 *
 * The raw platform value is never exposed: sources are combined and hashed
 * with SHA-256, yielding a uniform 64-char lowercase hex HWID.
 */

#ifndef VIRTUALPHONEPRO_HARDWARE_ID_HPP
#define VIRTUALPHONEPRO_HARDWARE_ID_HPP

#include <QString>
#include <string>

namespace VirtualPhonePro {

/**
 * @brief Stable, per-machine unique hardware identifier.
 *
 * The value is computed once per process and cached; repeated calls return
 * the identical string. Guaranteed non-empty: if every OS source fails, a
 * random UUID is generated once and persisted to the app config directory,
 * so subsequent runs on the same machine still agree.
 *
 * @return 64-character lowercase hex string (SHA-256 of combined sources)
 */
QString getStableHWID();

/// std::string convenience overload of getStableHWID().
std::string getStableHWIDStd();

/**
 * @brief Per-installation stable license key.
 *
 * Returns the license key provisioned for this installation. If none exists
 * yet, a cryptographically random key is generated once and persisted, so
 * every subsequent call returns the same value. Combined with the HWID this
 * anchors profile seeds to (machine, installation).
 *
 * @return 64-character lowercase hex string
 */
QString getInstallLicenseKey();

/// std::string convenience overload of getInstallLicenseKey().
std::string getInstallLicenseKeyStd();

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_HARDWARE_ID_HPP
