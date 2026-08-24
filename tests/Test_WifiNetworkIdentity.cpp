#include <QtTest>
#include <QObject>
#include <QString>
#include <QSet>
#include <QRegularExpression>

#include "Android/LocaleTimezoneManager.h"

using namespace VirtualPhonePro;

// WiFi network identity (ISP/residential proxy mode) verification.
// Compiles ONLY LocaleTimezoneManager.cpp (LTM_NO_CONTROLLER) + this test —
// NOT the full ~65-file project. No network, no proxy, no ADB. Only the pure
// generateWifiNetworkConfig() is exercised; applyWifiNetwork() is excluded by
// LTM_NO_CONTROLLER by design.
class Test_WifiNetworkIdentity : public QObject {
    Q_OBJECT

private slots:
    // Same seed -> identical identity (reboot-stable, profile-anchored).
    void deterministicPerSeed();
    // Two different profiles -> two different identities.
    void distinctPerSeed();
    // SSID / BSSID / gateway / band / speed / signal are all well-formed and
    // within realistic home-WiFi ranges.
    void wellFormedIdentity();
    // Country does not vary the identity (it is per-profile, not per-country).
    void countryIndependent();
};

void Test_WifiNetworkIdentity::deterministicPerSeed() {
    LocaleTimezoneManager& ltm = LocaleTimezoneManager::instance();
    const QString seed = "1234-profile-abc";
    WifiNetworkConfig a = ltm.generateWifiNetworkConfig(seed, "US");
    WifiNetworkConfig b = ltm.generateWifiNetworkConfig(seed, "US");
    QCOMPARE(a.ssid, b.ssid);
    QCOMPARE(a.bssid, b.bssid);
    QCOMPARE(a.gateway, b.gateway);
    QCOMPARE(a.frequency, b.frequency);
    QCOMPARE(a.linkSpeedMbps, b.linkSpeedMbps);
    QCOMPARE(a.signalDbm, b.signalDbm);
}

void Test_WifiNetworkIdentity::distinctPerSeed() {
    LocaleTimezoneManager& ltm = LocaleTimezoneManager::instance();
    WifiNetworkConfig a = ltm.generateWifiNetworkConfig("profile-A", "US");
    WifiNetworkConfig b = ltm.generateWifiNetworkConfig("profile-B", "US");
    QVERIFY2(a.ssid != b.ssid, "two profiles must not share an SSID");
    QVERIFY2(a.bssid != b.bssid, "two profiles must not share a BSSID");
}

void Test_WifiNetworkIdentity::wellFormedIdentity() {
    LocaleTimezoneManager& ltm = LocaleTimezoneManager::instance();
    const QRegularExpression ssidRe("^Home_WiFi_[0-9]{4}$");
    const QRegularExpression bssidRe("^02(:[0-9A-F]{2}){5}$");
    const QRegularExpression gwRe("^192\\.168\\.[0-9]{1,3}\\.1$");
    for (int i = 0; i < 50; ++i) {
        WifiNetworkConfig w = ltm.generateWifiNetworkConfig(
            QString("seed-%1").arg(i), "US");
        QVERIFY2(ssidRe.match(w.ssid).hasMatch(), qPrintable(w.ssid));
        QVERIFY2(bssidRe.match(w.bssid).hasMatch(), qPrintable(w.bssid));
        QVERIFY2(gwRe.match(w.gateway).hasMatch(), qPrintable(w.gateway));
        QVERIFY(w.frequency == "5 GHz" || w.frequency == "2.4 GHz");
        QVERIFY(w.linkSpeedMbps >= 72 && w.linkSpeedMbps <= 733);
        QVERIFY(w.signalDbm >= -65 && w.signalDbm <= -51);
        QCOMPARE(w.dns1, w.gateway);
        QCOMPARE(w.dns2, QStringLiteral("8.8.8.8"));
    }
}

void Test_WifiNetworkIdentity::countryIndependent() {
    LocaleTimezoneManager& ltm = LocaleTimezoneManager::instance();
    const QString seed = "same-profile";
    WifiNetworkConfig us = ltm.generateWifiNetworkConfig(seed, "US");
    WifiNetworkConfig jp = ltm.generateWifiNetworkConfig(seed, "JP");
    QCOMPARE(us.ssid, jp.ssid);
    QCOMPARE(us.bssid, jp.bssid);
}

QTEST_MAIN(Test_WifiNetworkIdentity)
#include "Test_WifiNetworkIdentity.moc"
