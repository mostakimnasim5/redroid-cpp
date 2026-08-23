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

#include <cstdint>
#include <memory>

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

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_PROFILE_GENERATOR_FACTORY_HPP
