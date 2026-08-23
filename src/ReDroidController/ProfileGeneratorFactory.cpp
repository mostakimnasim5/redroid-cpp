/**
 * @file ProfileGeneratorFactory.cpp
 * @brief Implementation of the hardware-anchored generator factory and
 *        the persistent profile-index counter
 */

#include "VirtualPhonePro/ProfileGeneratorFactory.hpp"
#include "VirtualPhonePro/HardwareId.hpp"
#include "VirtualPhonePro/UniqueDeviceGenerator.hpp"

#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>

namespace VirtualPhonePro {
namespace {

QString indexFilePath() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/profile_index.dat");
}

uint32_t readCounter(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }
    bool ok = false;
    const uint32_t value = QString::fromLatin1(file.readAll()).trimmed().toUInt(&ok);
    return ok ? value : 0;
}

bool writeCounterAtomic(const QString& path, uint32_t value) {
    const QString tmpPath = path + QStringLiteral(".tmp");
    {
        QFile tmp(tmpPath);
        if (!tmp.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        tmp.write(QByteArray::number(value));
        tmp.flush();
    }
    QFile::remove(path);
    return QFile::rename(tmpPath, path);
}

QMutex& counterMutex() {
    static QMutex mutex;
    return mutex;
}

} // namespace

std::unique_ptr<DeviceProfileGenerator> createHardwareAnchoredProfileGenerator() {
    return std::make_unique<DeviceProfileGenerator>(getStableHWIDStd(),
                                                    getInstallLicenseKeyStd());
}

uint32_t allocateNextProfileIndex() {
    QMutexLocker locker(&counterMutex());

    const QString path = indexFilePath();
    const uint32_t last = readCounter(path);

    uint32_t next = last + 1;
    if (next < MIN_PROFILE_INDEX) {
        next = MIN_PROFILE_INDEX;
    }
    if (next > MAX_PROFILE_INDEX) {
        return 0; // index space exhausted; never reuse a seed
    }

    if (!writeCounterAtomic(path, next)) {
        return 0;
    }
    return next;
}

uint32_t lastIssuedProfileIndex() {
    QMutexLocker locker(&counterMutex());
    return readCounter(indexFilePath());
}

HardwareAnchoredIdentity generateUniqueHardwareAnchoredIdentity() {
    HardwareAnchoredIdentity result;

    auto generator = createHardwareAnchoredProfileGenerator();
    if (!generator) {
        return result;
    }

    // Identity is derived from the hardware-anchored deterministic engine
    // (Master_Seed = HMAC-SHA256(HWID + License_Key, "PROFILE_" + Index)),
    // never from process-random sources. Each attempt consumes a fresh
    // persisted index; any candidate colliding with the local uniqueness
    // registry is rejected and retried with the next index.
    UniqueDeviceGenerator& registry = UniqueDeviceGenerator::instance();
    for (int attempt = 0; attempt < 100; ++attempt) {
        const uint32_t index = allocateNextProfileIndex();
        if (index == 0) {
            return result; // index space exhausted or persistence failure
        }

        DeviceIdentityProfile candidate = generator->generateProfile(index);
        const QString imei = QString::fromStdString(candidate.imei1);
        const QString serial = QString::fromStdString(candidate.serial_number);
        if (registry.isIMEIUnique(imei) && registry.isSerialUnique(serial)) {
            result.ok = true;
            result.profileIndex = index;
            result.identity = std::move(candidate);
            return result;
        }
    }
    return result;
}

void applyIdentityToDeviceProfile(DeviceProfile& profile,
                                  const DeviceIdentityProfile& identity) {
    // Full deterministic identity from the hardware-anchored engine.
    // All 20 derived units are mapped — nothing is dropped.
    profile.identity.imei = QString::fromStdString(identity.imei1);
    profile.identity.imei2 = QString::fromStdString(identity.imei2);
    profile.identity.serialNumber = QString::fromStdString(identity.serial_number);
    profile.identity.androidId = QString::fromStdString(identity.android_id);
    profile.identity.gsfId = QString::fromStdString(identity.gsf_id);
    profile.identity.advertisingId = QString::fromStdString(identity.advertising_id);
    profile.identity.deviceKey = QString::fromStdString(identity.device_key);
    profile.identity.authToken = QString::fromStdString(identity.auth_token);
    profile.identity.profileId = QString::fromStdString(identity.profile_id);

    profile.mac.wifiMac = QString::fromStdString(identity.wifi_mac);
    profile.mac.bluetoothMac = QString::fromStdString(identity.bluetooth_mac);
    profile.mac.bssid = QString::fromStdString(identity.bssid);

    profile.sim.iccid = QString::fromStdString(identity.iccid1);
    profile.sim.imsi = QString::fromStdString(identity.imsi1);
    profile.sim.iccid2 = QString::fromStdString(identity.iccid2);
    profile.sim.imsi2 = QString::fromStdString(identity.imsi2);
    profile.sim.phoneNumber1 = QString::fromStdString(identity.phone_number1);
    profile.sim.phoneNumber2 = QString::fromStdString(identity.phone_number2);

    profile.network.ipAddress = QString::fromStdString(identity.local_ip);

    profile.build.bootloader = QString::fromStdString(identity.bootloader_version);
    profile.build.radioVersion = QString::fromStdString(identity.radio_version);
}

void registerIssuedIdentity(const QString& instanceId,
                            const DeviceProfile& profile) {
    QJsonObject uniqueIds;
    uniqueIds[QStringLiteral("imei")] = profile.identity.imei;
    uniqueIds[QStringLiteral("serialNumber")] = profile.identity.serialNumber;
    uniqueIds[QStringLiteral("androidId")] = profile.identity.androidId;
    UniqueDeviceGenerator::instance().registerInstance(instanceId, uniqueIds);
}

} // namespace VirtualPhonePro
