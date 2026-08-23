/**
 * @file ProfileGeneratorFactory.hpp
 * @brief Factory wiring DeviceProfileGenerator to real hardware identity
 * @version 1.0.0
 *
 * DeviceProfileGenerator is a deterministic engine:
 *     Master_Seed = HMAC-SHA256(HWID + License_Key, "PROFILE_" + Index)
 *
 * This factory supplies the two inputs the engine cannot know by itself:
 *  - HWID:        the stable per-machine identifier (HardwareId.hpp)
 *  - License_Key: the per-installation stable secret (HardwareId.hpp)
 *
 * It also owns the persistent profile-index counter so every new profile
 * receives a never-before-used index and seeds are never repeated.
 */

#ifndef VIRTUALPHONEPRO_PROFILE_GENERATOR_FACTORY_HPP
#define VIRTUALPHONEPRO_PROFILE_GENERATOR_FACTORY_HPP

#include "VirtualPhonePro/DeviceProfileGenerator.hpp"
#include "VirtualPhonePro/DeviceProfile.hpp"

#include <cstdint>
#include <memory>

class QString;

namespace VirtualPhonePro {

/**
 * @brief Create a DeviceProfileGenerator anchored to this machine + install.
 *
 * Equivalent to DeviceProfileGenerator(getStableHWID(), getInstallLicenseKey()).
 * Two generators built on the same machine/install produce identical output
 * for the same index; different machines or reinstalls never collide.
 */
std::unique_ptr<DeviceProfileGenerator> createHardwareAnchoredProfileGenerator();

/**
 * @brief Allocate the next unique profile index (persistent).
 *
 * Reads the last-issued index from the app config directory, increments it,
 * and atomically writes it back before returning. Values are monotonically
 * increasing within [MIN_PROFILE_INDEX, MAX_PROFILE_INDEX]; a corrupted or
 * missing counter file restarts at MIN_PROFILE_INDEX (fresh-install case).
 *
 * @return The newly allocated index, or 0 if the index space is exhausted.
 */
uint32_t allocateNextProfileIndex();

/**
 * @brief Last issued profile index without allocating a new one.
 * @return The persisted counter value, or 0 if none was ever issued.
 */
uint32_t lastIssuedProfileIndex();

/**
 * @brief Result of a registry-checked deterministic identity allocation.
 */
struct HardwareAnchoredIdentity {
    bool ok = false;                    ///< true when identity + index are usable
    uint32_t profileIndex = 0;          ///< the persisted index consumed
    DeviceIdentityProfile identity;     ///< all 20 derived units
};

/**
 * @brief Allocate a fresh, registry-unique hardware-anchored identity.
 *
 * Single allocation path shared by the GUI single-create dialog and the
 * batch/multi deploy path. Each attempt consumes a fresh persisted index
 * (allocateNextProfileIndex), derives all 20 units from the deterministic
 * engine, and rejects candidates colliding with the local uniqueness
 * registry (UniqueDeviceGenerator), retrying with the next index.
 *
 * @return HardwareAnchoredIdentity with ok=false only when the index space
 *         is exhausted, persistence fails, or 100 consecutive candidates
 *         collide with the registry.
 */
HardwareAnchoredIdentity generateUniqueHardwareAnchoredIdentity();

/**
 * @brief Map all 20 derived identity units onto a DeviceProfile.
 *
 * Shared by the GUI single-create path and the batch clone path so both
 * produce byte-identical DeviceProfile identity fields for the same
 * DeviceIdentityProfile. Only identity-bearing fields are touched;
 * caller-controlled fields (name, build model, ports, ...) are preserved.
 */
void applyIdentityToDeviceProfile(DeviceProfile& profile,
                                  const DeviceIdentityProfile& identity);

/**
 * @brief Record an issued identity in the local uniqueness registry.
 *
 * Registers IMEI/serial/androidId under the owning instance so future
 * allocations (GUI or batch) can detect collisions. Must be called once
 * per instance after applyIdentityToDeviceProfile().
 */
void registerIssuedIdentity(const QString& instanceId,
                            const DeviceProfile& profile);

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_PROFILE_GENERATOR_FACTORY_HPP
