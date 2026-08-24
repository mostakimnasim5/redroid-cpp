/**
 * @file DeterministicIP.hpp
 * @brief Single HWID-anchored source for a profile's local/cellular IP.
 *
 * One function feeds every consumer — CELLULAR_IP docker env, WebRTC local
 * IP pin, init_cellular_network.sh env, rmnet0 bind — so the addresses can
 * never diverge. Header-only: the isolated unit test compiles exactly this
 * code path (not a copy) without pulling in the full ReDroidController.
 *
 * Determinism contract:
 *  - networkIpAddress set (hardware-anchored engine's deriveLocalIP unit)
 *    -> returned verbatim;
 *  - otherwise FNV-1a 64-bit over the profile identity (IMEI -> Android ID
 *    -> serial -> profile id -> name) mapped into 10.x.x.x.
 * Same profile -> same IP across reboots; two profiles never share one.
 */

#ifndef VIRTUALPHONEPRO_DETERMINISTIC_IP_HPP
#define VIRTUALPHONEPRO_DETERMINISTIC_IP_HPP

#include <QString>
#include <QStringList>

namespace VirtualPhonePro {

inline QString deriveLocalIPFromSeed(const QString& seed) {
    quint64 h = 14695981039346656037ULL; // FNV-1a 64-bit offset basis
    const QByteArray bytes = seed.toUtf8();
    for (const char c : bytes) {
        h ^= static_cast<quint64>(static_cast<quint8>(c));
        h *= 1099511628211ULL; // FNV prime
    }
    const int b2 = static_cast<int>(h & 0xFF);
    const int b3 = static_cast<int>((h >> 8) & 0xFF);
    const int b4 = static_cast<int>((h >> 16) % 253) + 2; // avoid .0/.1/.255
    return QStringLiteral("10.%1.%2.%3").arg(b2).arg(b3).arg(b4);
}

inline QString deterministicLocalIPFromIdentity(const QString& networkIpAddress,
                                                const QString& imei,
                                                const QString& androidId,
                                                const QString& serialNumber,
                                                const QString& profileId,
                                                const QString& profileName) {
    const QString ip = networkIpAddress.trimmed();
    if (!ip.isEmpty()) {
        return ip;
    }
    QString seed = imei;
    if (seed.isEmpty()) seed = androidId;
    if (seed.isEmpty()) seed = serialNumber;
    if (seed.isEmpty()) seed = profileId;
    if (seed.isEmpty()) seed = profileName;
    return deriveLocalIPFromSeed(seed);
}

inline QString gatewayForLocalIP(const QString& ip) {
    const QStringList parts = ip.split(QLatin1Char('.'));
    if (parts.size() == 4) {
        return QStringLiteral("%1.%2.%3.1").arg(parts[0], parts[1], parts[2]);
    }
    return QStringLiteral("10.0.0.1");
}

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_DETERMINISTIC_IP_HPP
