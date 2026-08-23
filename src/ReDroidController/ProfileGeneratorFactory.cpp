/**
 * @file ProfileGeneratorFactory.cpp
 * @brief Implementation of the hardware-anchored generator factory and
 *        the persistent profile-index counter
 */

#include "VirtualPhonePro/ProfileGeneratorFactory.hpp"
#include "VirtualPhonePro/HardwareId.hpp"

#include <QDir>
#include <QFile>
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

} // namespace VirtualPhonePro
