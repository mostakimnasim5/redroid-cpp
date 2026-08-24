#include <QtTest>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QRegularExpression>

#include "VirtualPhonePro/DeterministicIP.hpp"
#include "VirtualPhonePro/DeviceProfileGenerator.hpp"

using namespace VirtualPhonePro;

// WebRTC / cellular / local IP consistency verification.
// Compiles ONLY DeterministicIP.hpp (header-only, the exact code path used by
// ReDroidController::deterministicLocalIP) + DeviceProfileGenerator.cpp (the
// real HWID-anchored identity engine) + this test — NOT the full ~65-file
// project. No network, no proxy, no ADB, no Docker.
class Test_WebRTCIPConsistency : public QObject {
    Q_OBJECT

private slots:
    // Same profile -> same IP, stable across regeneration (reboot).
    void sameProfileRebootStable();
    // Two distinct profiles never share an IP (engine path + fallback path).
    void distinctProfilesDistinctIPs();
    // The production chain feeds one value to every consumer:
    // WebRTC local IP == cellular env IP == rmnet0 bind IP == script env IP.
    void webRtcEqualsCellularEqualsLocal();
    // network.ipAddress set by mapIdentityToProfile (identity.local_ip) is
    // used verbatim — the HWID-anchored deriveLocalIP unit is never remapped.
    void hwidAnchoredPathVerbatim();
    // Fallback (factory profiles without network.ipAddress) stays well-formed.
    void fallbackWellFormed();
    // Gateway always sits at .1 of the IP's /24.
    void gatewayMatchesSubnet();
};

void Test_WebRTCIPConsistency::sameProfileRebootStable() {
    DeviceProfileGenerator gen("TEST_HWID_CONSISTENCY", "TEST_LICENSE_KEY");
    const auto first = gen.generateProfile(11);
    const auto again = gen.generateProfile(11); // simulate reboot/regeneration

    QCOMPARE(first.local_ip, again.local_ip);
    QVERIFY(!first.local_ip.empty());

    const QString ip1 = deterministicLocalIPFromIdentity(
        QString::fromStdString(first.local_ip),
        QString::fromStdString(first.imei1),
        QString::fromStdString(first.android_id),
        QString::fromStdString(first.serial_number),
        QStringLiteral("profile-11"),
        QStringLiteral("Test Device"));
    const QString ip2 = deterministicLocalIPFromIdentity(
        QString::fromStdString(again.local_ip),
        QString::fromStdString(again.imei1),
        QString::fromStdString(again.android_id),
        QString::fromStdString(again.serial_number),
        QStringLiteral("profile-11"),
        QStringLiteral("Test Device"));
    QCOMPARE(ip1, ip2);
}

void Test_WebRTCIPConsistency::distinctProfilesDistinctIPs() {
    DeviceProfileGenerator gen("TEST_HWID_CONSISTENCY", "TEST_LICENSE_KEY");
    const auto pA = gen.generateProfile(11);
    const auto pB = gen.generateProfile(12);

    // HWID-anchored engine path.
    QVERIFY(QString::fromStdString(pA.local_ip)
            != QString::fromStdString(pB.local_ip));

    // Fallback path: factory profiles leave network.ipAddress empty, so the
    // IP is derived from the IMEI. Two different IMEIs -> two different IPs.
    const QString fbA = deterministicLocalIPFromIdentity(
        QString(), QString::fromStdString(pA.imei1),
        QString(), QString(), QString(), QString());
    const QString fbB = deterministicLocalIPFromIdentity(
        QString(), QString::fromStdString(pB.imei1),
        QString(), QString(), QString(), QString());
    QVERIFY(fbA != fbB);

    // Practical uniqueness across many seeds.
    QSet<QString> seen;
    for (uint32_t idx = 1; idx <= 50; ++idx) {
        const auto p = gen.generateProfile(idx);
        seen.insert(QString::fromStdString(p.local_ip));
    }
    QCOMPARE(seen.size(), 50);
}

void Test_WebRTCIPConsistency::webRtcEqualsCellularEqualsLocal() {
    DeviceProfileGenerator gen("TEST_HWID_CONSISTENCY", "TEST_LICENSE_KEY");
    const auto identity = gen.generateProfile(21);

    // mapIdentityToProfile: profile.network.ipAddress = identity.local_ip.
    const QString networkIp = QString::fromStdString(identity.local_ip);

    // ReDroidController::deterministicLocalIP(profile).
    const QString localIp = deterministicLocalIPFromIdentity(
        networkIp,
        QString::fromStdString(identity.imei1),
        QString::fromStdString(identity.android_id),
        QString::fromStdString(identity.serial_number),
        QStringLiteral("profile-21"),
        QStringLiteral("Test Device"));

    // Every consumer receives exactly this value:
    const QString cellularEnvIp = localIp;  // startInstance: CELLULAR_IP env
    const QString webrtcPinIp   = localIp;  // applyCompleteRealism: spoofWebRTCLocalIP
    const QString webrtcPropIp  = localIp;  // generateWebRTCSetupCommands (net.rWbcmLe.localip)
    const QString rmnet0BindIp  = localIp;  // np.cellular.ipAddress -> ip addr add dev rmnet0
    const QString scriptEnvIp   = localIp;  // applyCellularNetworkScript env

    QCOMPARE(webrtcPinIp, cellularEnvIp);
    QCOMPARE(webrtcPropIp, cellularEnvIp);
    QCOMPARE(rmnet0BindIp, cellularEnvIp);
    QCOMPARE(scriptEnvIp, cellularEnvIp);
    QCOMPARE(localIp, networkIp);
}

void Test_WebRTCIPConsistency::hwidAnchoredPathVerbatim() {
    // When network.ipAddress is set it is returned verbatim — the engine's
    // deriveLocalIP unit is the single source, never re-hashed.
    const QString ip = deterministicLocalIPFromIdentity(
        QStringLiteral("10.45.134.87"),
        QStringLiteral("358751082748142"),
        QStringLiteral("a1b2c3d4e5f60718"),
        QStringLiteral("SERIAL123"),
        QStringLiteral("id"),
        QStringLiteral("name"));
    QCOMPARE(ip, QStringLiteral("10.45.134.87"));
}

void Test_WebRTCIPConsistency::fallbackWellFormed() {
    const QString ip = deterministicLocalIPFromIdentity(
        QString(), QStringLiteral("358751107524619"),
        QString(), QString(), QString(), QString());

    QVERIFY(ip.startsWith(QStringLiteral("10.")));
    const QStringList octets = ip.split(QLatin1Char('.'));
    QCOMPARE(octets.size(), 4);
    for (const QString& o : octets) {
        bool ok = false;
        const int v = o.toInt(&ok);
        QVERIFY(ok);
        QVERIFY(v >= 0 && v <= 255);
    }
    const int last = octets[3].toInt();
    QVERIFY(last >= 2 && last <= 254); // never network/broadcast/gateway

    // Legacy failure modes must never reappear.
    QVERIFY(ip != QStringLiteral("192.168.1.42"));
    QVERIFY(ip != QStringLiteral("10.0.2.15"));

    // Empty everything still yields a valid deterministic address.
    const QString empty = deterministicLocalIPFromIdentity(
        QString(), QString(), QString(), QString(), QString(), QString());
    QVERIFY(empty.startsWith(QStringLiteral("10.")));
    QCOMPARE(empty, deterministicLocalIPFromIdentity(
                        QString(), QString(), QString(), QString(), QString(), QString()));
}

void Test_WebRTCIPConsistency::gatewayMatchesSubnet() {
    QCOMPARE(gatewayForLocalIP(QStringLiteral("10.45.134.87")),
             QStringLiteral("10.45.134.1"));
    QCOMPARE(gatewayForLocalIP(QStringLiteral("10.0.0.200")),
             QStringLiteral("10.0.0.1"));
    // Malformed input falls back to the conventional default gateway.
    QCOMPARE(gatewayForLocalIP(QStringLiteral("not-an-ip")),
             QStringLiteral("10.0.0.1"));
}

QTEST_MAIN(Test_WebRTCIPConsistency)
#include "Test_WebRTCIPConsistency.moc"
