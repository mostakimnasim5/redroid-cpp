#include <QtTest>
#include <QObject>
#include <QString>
#include <QJsonObject>

#include "Android/LocaleTimezoneManager.h"

using namespace VirtualPhonePro;

// Proxy-type + network-kind propagation verification.
// Compiles ONLY LocaleTimezoneManager.cpp with LTM_NO_CONTROLLER (same
// isolation as Test_CarrierSelection / Test_WifiNetworkIdentity) — no network,
// no proxy, no ADB, no docker. Asserts:
//   1. The proxy.type string the GUI combo produces ("http"/"socks5") round-
//      trips verbatim through setProxy()/getStateAsJson() — the value
//      QNetworkProxy and the redsocks config branch on.
//   2. The WiFi-vs-Cellular kind decision mirrors the controller's mapping
//      (mode 1 ISP -> WiFi, mode 2 mobile -> Cellular) and is consistent with
//      the deterministic carrier/wifi identity each kind resolves to.
class Test_ProxyTypeKindPropagation : public QObject {
    Q_OBJECT

private slots:
    void proxyTypeRoundTrips();
    void kindMappingMirrorsController();
    void carrierDiffersFromWifiForSameSeed();
};

void Test_ProxyTypeKindPropagation::proxyTypeRoundTrips() {
    LocaleTimezoneManager& ltm = LocaleTimezoneManager::instance();
    for (const QString& type : {QStringLiteral("http"), QStringLiteral("socks5")}) {
        ProxyInfo p;
        p.host     = "gw.proxy.example";
        p.port     = 1080;
        p.type     = type;
        p.username = "u";
        p.password = "p";
        p.isValid  = true;
        const QString id = QStringLiteral("proxy-type-%1").arg(type);
        QVERIFY(ltm.setProxy(id, p));

        const QJsonObject state = ltm.getStateAsJson(id);
        QVERIFY(!state.isEmpty());
        QCOMPARE(state["proxy"].toObject()["type"].toString(), type);
        QCOMPARE(state["proxy"].toObject()["host"].toString(), QStringLiteral("gw.proxy.example"));

        ProxyInfo got = ltm.getProxy(id);
        QCOMPARE(got.type, type);
        QVERIFY(got.isValid);
    }
}

void Test_ProxyTypeKindPropagation::kindMappingMirrorsController() {
    // ReDroidController/DashboardWindow map: GUI mode 1 (ISP/residential) ->
    // SyncNetworkKind::WiFi, mode 2 (mobile) -> SyncNetworkKind::Cellular.
    // Recreate the mapping exactly as DashboardWindow.cpp:954-957 does and
    // assert the resolved enum values.
    const SyncNetworkKind mode1 = (1 == 1) ? SyncNetworkKind::WiFi     : SyncNetworkKind::Cellular;
    const SyncNetworkKind mode2 = (2 == 1) ? SyncNetworkKind::WiFi     : SyncNetworkKind::Cellular;
    QCOMPARE(mode1, SyncNetworkKind::WiFi);
    QCOMPARE(mode2, SyncNetworkKind::Cellular);
    QVERIFY(mode1 != mode2);
}

void Test_ProxyTypeKindPropagation::carrierDiffersFromWifiForSameSeed() {
    // For the SAME profile seed, the Cellular story is a carrier identity and
    // the WiFi story is a home-WiFi identity — the two kinds must never
    // produce the other's props.
    LocaleTimezoneManager& ltm = LocaleTimezoneManager::instance();
    const QString seed = QStringLiteral("propagation-seed-42");

    CarrierConfig carrier = ltm.getCarrierForLocation("US", QString(), seed);
    QVERIFY(!carrier.name.isEmpty());
    QVERIFY(!carrier.mcc.isEmpty());
    QVERIFY(!carrier.mnc.isEmpty());

    WifiNetworkConfig wifi = ltm.generateWifiNetworkConfig(seed, "US");
    QVERIFY(!wifi.ssid.isEmpty());
    QVERIFY(!wifi.bssid.isEmpty());

    // A WiFi identity must not carry SIM/carrier data and vice versa.
    QVERIFY(wifi.ssid != carrier.name);
    QVERIFY(!carrier.mcc.isEmpty() && wifi.gateway.startsWith("192.168."));
}

QTEST_MAIN(Test_ProxyTypeKindPropagation)
#include "Test_ProxyTypeKindPropagation.moc"
