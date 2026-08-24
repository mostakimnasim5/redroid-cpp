#include <QtTest>
#include <QObject>
#include <QString>
#include <QSet>
#include <QRegularExpression>

#include "Android/LocaleTimezoneManager.h"

using namespace VirtualPhonePro;

// Multi-carrier selection verification.
// Compiles ONLY LocaleTimezoneManager.cpp (LTM_NO_CONTROLLER) + this test —
// NOT the full ~65-file project. No network, no proxy, no ADB.
class Test_CarrierSelection : public QObject {
    Q_OBJECT

private slots:
    // Every country returns a well-formed, realistic MCC/MNC pair.
    void validMccMncForAllCountries();
    // Same seed -> same carrier (deterministic, profile-anchored).
    void deterministicPerSeed();
    // A multi-carrier country yields >1 distinct carrier across seeds.
    void multiCarrierVariety();
    // Explicitly documented countries resolve to documented operators.
    void documentedCarriers();
    // Unknown country falls back to a safe placeholder (never crash/garbage).
    void unknownCountryFallback();
};

void Test_CarrierSelection::validMccMncForAllCountries() {
    LocaleTimezoneManager& ltm = LocaleTimezoneManager::instance();
    const QStringList codes = {"US","GB","DE","FR","JP","KR","IN","BD","CN","AU",
                               "CA","BR","RU","AE","SG","PK","SA","MX","IT","ES",
                               "NL","SE","NO","DK","FI","PL","TH","VN","MY","ID",
                               "PH","TW","HK","NZ","ZA","EG","NG","KE","AR","CL","CO","PE"};
    const QRegularExpression mccRe("^[0-9]{3}$");
    const QRegularExpression mncRe("^[0-9]{2,3}$");
    for (const QString& c : codes) {
        CarrierConfig cfg = ltm.getCarrierForLocation(c, "", "seed-" + c);
        QVERIFY2(!cfg.name.isEmpty(), qPrintable("empty name for " + c));
        QVERIFY2(mccRe.match(cfg.mcc).hasMatch(),
                 qPrintable("bad MCC '" + cfg.mcc + "' for " + c));
        QVERIFY2(mncRe.match(cfg.mnc).hasMatch(),
                 qPrintable("bad MNC '" + cfg.mnc + "' for " + c));
        QVERIFY2(!cfg.networkType.isEmpty(), qPrintable("empty networkType for " + c));
    }
}

void Test_CarrierSelection::deterministicPerSeed() {
    LocaleTimezoneManager& ltm = LocaleTimezoneManager::instance();
    CarrierConfig a = ltm.getCarrierForLocation("US", "", "instance-007");
    CarrierConfig b = ltm.getCarrierForLocation("US", "", "instance-007");
    QCOMPARE(a.name, b.name);
    QCOMPARE(a.mcc,  b.mcc);
    QCOMPARE(a.mnc,  b.mnc);
}

void Test_CarrierSelection::multiCarrierVariety() {
    LocaleTimezoneManager& ltm = LocaleTimezoneManager::instance();
    QSet<QString> distinct;
    for (int i = 0; i < 200; ++i)
        distinct.insert(ltm.getCarrierForLocation("US", "", QString("seed-%1").arg(i)).name);
    // US has 4 carriers; a deterministic spread over 200 seeds must hit >1.
    QVERIFY2(distinct.size() > 1,
             qPrintable(QString("US produced only %1 distinct carrier(s) — no variety").arg(distinct.size())));
    qInfo("US distinct carriers over 200 seeds: %d", int(distinct.size()));
}

void Test_CarrierSelection::documentedCarriers() {
    LocaleTimezoneManager& ltm = LocaleTimezoneManager::instance();
    const QStringList us = {"T-Mobile","AT&T","Verizon","US Cellular",
                            "Metro by T-Mobile","AT&T FirstNet"};
    const QStringList gb = {"EE","O2","Vodafone UK","Three UK"};
    const QStringList in = {"Jio","Airtel","Vi","BSNL"};
    const QStringList bd = {"Grameenphone","Robi","Banglalink","Teletalk"};

    for (int i = 0; i < 50; ++i) {
        QString s = QString("doc-%1").arg(i);
        QVERIFY2(us.contains(ltm.getCarrierForLocation("US","",s).name), "US carrier not documented");
        QVERIFY2(gb.contains(ltm.getCarrierForLocation("GB","",s).name), "GB carrier not documented");
        QVERIFY2(in.contains(ltm.getCarrierForLocation("IN","",s).name), "IN carrier not documented");
        QVERIFY2(bd.contains(ltm.getCarrierForLocation("BD","",s).name), "BD carrier not documented");
    }
    // MCC must match the country (US=310/311, GB=234, IN=404/405, BD=470).
    QVERIFY(QStringList({"310","311","312","313"}).contains(ltm.getCarrierForLocation("US","","x").mcc));
    QCOMPARE(ltm.getCarrierForLocation("GB","","x").mcc, QString("234"));
    QVERIFY(QStringList({"404","405"}).contains(ltm.getCarrierForLocation("IN","","x").mcc));
    QCOMPARE(ltm.getCarrierForLocation("BD","","x").mcc, QString("470"));
}

void Test_CarrierSelection::unknownCountryFallback() {
    LocaleTimezoneManager& ltm = LocaleTimezoneManager::instance();
    CarrierConfig cfg = ltm.getCarrierForLocation("XX", "", "any-seed");
    QVERIFY(!cfg.name.isEmpty());
    QCOMPARE(cfg.mcc, QString("310"));
    QCOMPARE(cfg.mnc, QString("260"));
}

QTEST_MAIN(Test_CarrierSelection)
#include "Test_CarrierSelection.moc"
