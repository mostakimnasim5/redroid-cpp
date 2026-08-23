/**
 * @file HardwareId.cpp
 * @brief Implementation of stable per-machine HWID and per-install license key
 */

#include "VirtualPhonePro/HardwareId.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QRandomGenerator>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QSysInfo>
#include <QUuid>

#if defined(Q_OS_WIN)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#elif defined(Q_OS_MAC)
    #include <sys/sysctl.h>
#endif

namespace VirtualPhonePro {
namespace {

QString configFilePath(const QString& name) {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + name;
}

QString readSmallFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromLatin1(file.read(256)).trimmed();
}

void writePrivateFile(const QString& path, const QByteArray& data) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }
    file.write(data);
    file.close();
    // Owner-only: the fallback HWID / license key must not leak to other users.
    file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

bool looksValid(const QString& s) {
    return s.size() >= 8 && !s.contains(QLatin1Char(' '));
}

#if defined(Q_OS_WIN)
QString windowsMachineGuid() {
    const QSettings reg(QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Cryptography"),
                        QSettings::NativeFormat);
    return reg.value(QStringLiteral("MachineGuid")).toString().trimmed();
}

QString windowsVolumeSerial() {
    DWORD serial = 0;
    if (GetVolumeInformationW(L"C:\\", nullptr, 0, &serial, nullptr, nullptr, nullptr, 0)) {
        return QString::number(serial, 16);
    }
    return {};
}
#endif

QString collectPrimarySource() {
#if defined(Q_OS_WIN)
    QStringList parts;
    const QString guid = windowsMachineGuid();
    if (looksValid(guid)) {
        parts << guid;
    }
    const QString vol = windowsVolumeSerial();
    if (!vol.isEmpty()) {
        parts << vol;
    }
    return parts.join(QLatin1Char('|'));
#elif defined(Q_OS_LINUX)
    QString id = readSmallFile(QStringLiteral("/etc/machine-id"));
    if (!looksValid(id)) {
        id = readSmallFile(QStringLiteral("/var/lib/dbus/machine-id"));
    }
    return looksValid(id) ? id : QString{};
#elif defined(Q_OS_MAC)
    char buf[128] = {0};
    size_t len = sizeof(buf);
    if (sysctlbyname("kern.hostuuid", buf, &len, nullptr, 0) == 0) {
        const QString id = QString::fromLatin1(buf, static_cast<int>(len)).trimmed();
        if (looksValid(id)) {
            return id;
        }
    }
    return {};
#else
    return {};
#endif
}

QString loadOrCreatePersistedId(const QString& fileName) {
    const QString path = configFilePath(fileName);
    const QString existing = readSmallFile(path);
    if (looksValid(existing)) {
        return existing;
    }
    const QString generated = QUuid::createUuid().toString(QUuid::WithoutBraces);
    writePrivateFile(path, generated.toLatin1());
    return generated;
}

QString computeStableHWID() {
    const QString primary = collectPrimarySource();

    QStringList sources;
    if (!primary.isEmpty()) {
        sources << primary;
    }

    // Qt's portable fallback (on Windows this is also registry-derived, on
    // Linux machine-id); including it adds entropy when available.
    const QByteArray qtId = QSysInfo::machineUniqueId();
    if (!qtId.isEmpty()) {
        sources << QString::fromLatin1(qtId);
    }

    if (sources.isEmpty()) {
        // Last resort: random UUID persisted on first run so the machine keeps
        // a stable identity even without any OS-provided identifier.
        sources << loadOrCreatePersistedId(QStringLiteral("hwid_fallback.dat"));
    }

    const QByteArray combined = sources.join(QLatin1Char('|')).toUtf8();
    return QString::fromLatin1(
        QCryptographicHash::hash(combined, QCryptographicHash::Sha256).toHex());
}

QString computeInstallLicenseKey() {
    const QString path = configFilePath(QStringLiteral("install_license.key"));
    const QString existing = readSmallFile(path);
    if (existing.size() == 64) {
        return existing;
    }
    // 32 bytes of system entropy -> 64 hex chars, generated once per install.
    QByteArray raw(32, 0);
    QRandomGenerator::system()->fillRange(
        reinterpret_cast<quint32*>(raw.data()), raw.size() / 4);
    const QString key = QString::fromLatin1(
        QCryptographicHash::hash(raw, QCryptographicHash::Sha256).toHex());
    writePrivateFile(path, key.toLatin1());
    return key;
}

} // namespace

QString getStableHWID() {
    // Function-local static: computed once, thread-safe since C++11.
    static const QString hwid = computeStableHWID();
    return hwid;
}

std::string getStableHWIDStd() {
    return getStableHWID().toStdString();
}

QString getInstallLicenseKey() {
    static const QString key = computeInstallLicenseKey();
    return key;
}

std::string getInstallLicenseKeyStd() {
    return getInstallLicenseKey().toStdString();
}

} // namespace VirtualPhonePro
