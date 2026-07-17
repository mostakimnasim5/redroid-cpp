/**
 * @file Test_UniqueDeviceGenerator.cpp
 * @brief Unit Tests for UniqueDeviceGenerator
 * @version 3.0.0
 */

#include <QtTest>
#include <QCoreApplication>
#include <QDebug>
#include <QSet>
#include <QThread>
#include <QFuture>
#include <QtConcurrent>

#include "VirtualPhonePro/UniqueDeviceGenerator.h"
#include "Data/TACDatabase.h"

class Test_UniqueDeviceGenerator : public QObject {
    Q_OBJECT

private slots:
    // Initialization tests
    void testSingleton();
    void testInitialization();
    
    // IMEI generation tests
    void testIMEIGeneration();
    void testIMEIUniqueness();
    void testIMEILuhnValidation();
    void testIMEIForManufacturer();
    void testIMEIRandomDistribution();
    
    // Serial number tests
    void testSerialGeneration();
    void testSerialUniqueness();
    void testSamsungSerialFormat();
    void testGoogleSerialFormat();
    
    // MAC address tests
    void testMACGeneration();
    void testMACFormat();
    void testMACUniqueness();
    void testMACWithOUI();
    
    // Android ID tests
    void testAndroidIdGeneration();
    void testAndroidIdFormat();
    void testAndroidIdUniqueness();
    
    // GSF ID tests
    void testGSFIdGeneration();
    void testGSFIdFormat();
    
    // ICCID tests
    void testICCIDGeneration();
    void testICCIDFormat();
    
    // IMSI tests
    void testIMSIGeneration();
    void testIMSIMCCMNC();
    
    // Complete identity tests
    void testCompleteIdentityGeneration();
    void testIdentityUniqueness();
    
    // Thread safety tests
    void testThreadSafety();
    void testConcurrentGeneration();
    
    // Hash tests
    void testHashGeneration();
    
    // Edge cases
    void testEmptyManufacturer();
    void testInvalidInputs();
};

// ============================================================================
// Test Implementations
// ============================================================================

void Test_UniqueDeviceGenerator::testSingleton() {
    UniqueDeviceGenerator& instance1 = UniqueDeviceGenerator::instance();
    UniqueDeviceGenerator& instance2 = UniqueDeviceGenerator::instance();
    
    // Singleton pattern - should return same instance
    QVERIFY2(&instance1 == &instance2, "Singleton pattern broken!");
}

void Test_UniqueDeviceGenerator::testInitialization() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    QVERIFY(gen.isInitialized());
    
    // TAC Database should be initialized
    size_t tacCount = RedroidCPP::TACDatabase::getInstance().size();
    QVERIFY2(tacCount >= 100, "TAC Database should have at least 100 entries");
    qDebug() << "TAC Database has" << tacCount << "entries";
}

void Test_UniqueDeviceGenerator::testIMEIGeneration() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QString imei = gen.generateUniqueIMEI("samsung");
    
    // IMEI should be 15 digits
    QVERIFY2(imei.length() == 15, qPrintable(QString("IMEI length should be 15, got %1").arg(imei.length())));
    QVERIFY2(imei.contains(QRegExp("^[0-9]{15}$")), "IMEI should contain only digits");
}

void Test_UniqueDeviceGenerator::testIMEIUniqueness() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QSet<QString> imeis;
    const int testCount = 100;
    
    for (int i = 0; i < testCount; ++i) {
        QString imei = gen.generateUniqueIMEI();
        QVERIFY2(!imeis.contains(imei), qPrintable(QString("Duplicate IMEI generated: %1").arg(imei)));
        imeis.insert(imei);
    }
    
    // All should be unique
    QVERIFY2(imeis.size() == testCount, "Not all IMEIs are unique");
}

void Test_UniqueDeviceGenerator::testIMEILuhnValidation() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    for (int i = 0; i < 50; ++i) {
        QString imei = gen.generateUniqueIMEI();
        
        // Validate Luhn algorithm
        int sum = 0;
        bool alternate = false;
        for (int j = 14; j >= 0; --j) {
            int digit = imei[j].digitValue();
            if (alternate) {
                digit *= 2;
                if (digit > 9) digit -= 9;
            }
            sum += digit;
            alternate = !alternate;
        }
        
        QVERIFY2(sum % 10 == 0, qPrintable(QString("IMEI %1 failed Luhn validation").arg(imei)));
    }
}

void Test_UniqueDeviceGenerator::testIMEIForManufacturer() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QStringList manufacturers = {"samsung", "google", "xiaomi", "huawei", "oneplus", "oppo", "vivo"};
    
    for (const QString& mfr : manufacturers) {
        QString imei = gen.generateUniqueIMEI(mfr);
        QVERIFY2(imei.length() == 15, qPrintable(QString("IMEI for %1 should be 15 digits").arg(mfr)));
    }
}

void Test_UniqueDeviceGenerator::testIMEIRandomDistribution() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    // Generate many IMEIs and check TAC distribution
    QMap<QString, int> tacDistribution;
    const int testCount = 500;
    
    for (int i = 0; i < testCount; ++i) {
        QString imei = gen.generateUniqueIMEI("samsung");
        QString tac = imei.left(8);
        tacDistribution[tac]++;
    }
    
    // Samsung should have multiple different TACs
    QVERIFY2(tacDistribution.size() > 1, "TAC distribution should show variety");
    qDebug() << "TAC distribution for Samsung:" << tacDistribution.size() << "unique TACs";
}

void Test_UniqueDeviceGenerator::testSerialGeneration() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QString serial = gen.generateUniqueSerial("samsung");
    QVERIFY(!serial.isEmpty());
    
    // Samsung format: R + 6 digits + X + 2 digits
    QVERIFY2(serial.startsWith("R"), "Samsung serial should start with R");
}

void Test_UniqueDeviceGenerator::testSerialUniqueness() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QSet<QString> serials;
    const int testCount = 100;
    
    for (int i = 0; i < testCount; ++i) {
        QString serial = gen.generateUniqueSerial("samsung");
        QVERIFY2(!serials.contains(serial), "Duplicate serial generated");
        serials.insert(serial);
    }
}

void Test_UniqueDeviceGenerator::testSamsungSerialFormat() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    for (int i = 0; i < 10; ++i) {
        QString serial = gen.generateUniqueSerial("samsung");
        // Format: R + 6 digits + X + 2 digits
        QRegExp rx("^R\\d{6}X\\d{2}$");
        QVERIFY2(rx.exactMatch(serial), qPrintable(QString("Invalid Samsung serial format: %1").arg(serial)));
    }
}

void Test_UniqueDeviceGenerator::testGoogleSerialFormat() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    for (int i = 0; i < 10; ++i) {
        QString serial = gen.generateUniqueSerial("google");
        // Format: AG + 8 digits
        QRegExp rx("^AG\\d{8}$");
        QVERIFY2(rx.exactMatch(serial), qPrintable(QString("Invalid Google serial format: %1").arg(serial)));
    }
}

void Test_UniqueDeviceGenerator::testMACGeneration() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QString mac = gen.generateUniqueMAC();
    QVERIFY(!mac.isEmpty());
}

void Test_UniqueDeviceGenerator::testMACFormat() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    for (int i = 0; i < 10; ++i) {
        QString mac = gen.generateUniqueMAC();
        QRegExp rx("^[0-9A-Fa-f]{2}(:[0-9A-Fa-f]{2}){5}$");
        QVERIFY2(rx.exactMatch(mac), qPrintable(QString("Invalid MAC format: %1").arg(mac)));
    }
}

void Test_UniqueDeviceGenerator::testMACUniqueness() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QSet<QString> macs;
    const int testCount = 100;
    
    for (int i = 0; i < testCount; ++i) {
        QString mac = gen.generateUniqueMAC();
        QVERIFY2(!macs.contains(mac), "Duplicate MAC generated");
        macs.insert(mac);
    }
}

void Test_UniqueDeviceGenerator::testMACWithOUI() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    // Samsung OUI
    QString samsungMac = gen.generateUniqueMAC("8C:71:F8");
    QVERIFY2(samsungMac.startsWith("8C:71:F8:"), qPrintable(QString("Samsung MAC should start with 8C:71:F8, got: %1").arg(samsungMac)));
    
    // Google OUI
    QString googleMac = gen.generateUniqueMAC("3C:5A:B4");
    QVERIFY2(googleMac.startsWith("3C:5A:B4:"), qPrintable(QString("Google MAC should start with 3C:5A:B4, got: %1").arg(googleMac)));
}

void Test_UniqueDeviceGenerator::testAndroidIdGeneration() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QString androidId = gen.generateUniqueAndroidId();
    QVERIFY(!androidId.isEmpty());
}

void Test_UniqueDeviceGenerator::testAndroidIdFormat() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    for (int i = 0; i < 10; ++i) {
        QString androidId = gen.generateUniqueAndroidId();
        // Android ID is 16 hex characters
        QRegExp rx("^[0-9A-Fa-f]{16}$");
        QVERIFY2(rx.exactMatch(androidId), qPrintable(QString("Invalid Android ID format: %1").arg(androidId)));
    }
}

void Test_UniqueDeviceGenerator::testAndroidIdUniqueness() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QSet<QString> ids;
    const int testCount = 100;
    
    for (int i = 0; i < testCount; ++i) {
        QString id = gen.generateUniqueAndroidId();
        QVERIFY2(!ids.contains(id), "Duplicate Android ID generated");
        ids.insert(id);
    }
}

void Test_UniqueDeviceGenerator::testGSFIdGeneration() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QString gsfId = gen.generateUniqueGSFId();
    QVERIFY(!gsfId.isEmpty());
}

void Test_UniqueDeviceGenerator::testGSFIdFormat() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    for (int i = 0; i < 10; ++i) {
        QString gsfId = gen.generateUniqueGSFId();
        // GSF ID is 10 digits
        QRegExp rx("^\\d{10}$");
        QVERIFY2(rx.exactMatch(gsfId), qPrintable(QString("Invalid GSF ID format: %1").arg(gsfId)));
    }
}

void Test_UniqueDeviceGenerator::testICCIDGeneration() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QString iccid = gen.generateUniqueICCID();
    QVERIFY(!iccid.isEmpty());
}

void Test_UniqueDeviceGenerator::testICCIDFormat() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    for (int i = 0; i < 10; ++i) {
        QString iccid = gen.generateUniqueICCID();
        // ICCID starts with 8961 + 13+ digits
        QRegExp rx("^8961\\d+$");
        QVERIFY2(rx.exactMatch(iccid), qPrintable(QString("Invalid ICCID format: %1").arg(iccid)));
    }
}

void Test_UniqueDeviceGenerator::testIMSIGeneration() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QString imsi = gen.generateUniqueIMSI("310", "260");
    QVERIFY(!imsi.isEmpty());
}

void Test_UniqueDeviceGenerator::testIMSIMCCMNC() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    // T-Mobile (MCC=310, MNC=260)
    QString imsi = gen.generateUniqueIMSI("310", "260");
    QVERIFY2(imsi.startsWith("310260"), qPrintable(QString("IMSI should start with 310260, got: %1").arg(imsi)));
    
    // AT&T (MCC=310, MNC=410)
    imsi = gen.generateUniqueIMSI("310", "410");
    QVERIFY2(imsi.startsWith("310410"), qPrintable(QString("IMSI should start with 310410, got: %1").arg(imsi)));
}

void Test_UniqueDeviceGenerator::testCompleteIdentityGeneration() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QJsonObject identity = gen.generateCompleteUniqueIdentity("samsung");
    
    // Check all required fields
    QVERIFY2(identity.contains("instanceId"), "Missing instanceId");
    QVERIFY2(identity.contains("imei"), "Missing IMEI");
    QVERIFY2(identity.contains("serialNumber"), "Missing serialNumber");
    QVERIFY2(identity.contains("androidId"), "Missing androidId");
    QVERIFY2(identity.contains("gsfId"), "Missing gsfId");
    QVERIFY2(identity.contains("wifiMac"), "Missing wifiMac");
    QVERIFY2(identity.contains("bluetoothMac"), "Missing bluetoothMac");
    QVERIFY2(identity.contains("iccid"), "Missing ICCID");
    QVERIFY2(identity.contains("imsi"), "Missing IMSI");
}

void Test_UniqueDeviceGenerator::testIdentityUniqueness() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QSet<QString> imeis;
    const int testCount = 50;
    
    for (int i = 0; i < testCount; ++i) {
        QJsonObject identity = gen.generateCompleteUniqueIdentity("samsung");
        QString imei = identity["imei"].toString();
        QVERIFY2(!imeis.contains(imei), "Duplicate IMEI in complete identity");
        imeis.insert(imei);
    }
}

void Test_UniqueDeviceGenerator::testThreadSafety() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    // Generate IMEIs from multiple threads
    QList<QString> imeis;
    QMutex mutex;
    
    QVector<QThread*> threads;
    const int threadCount = 5;
    const int perThread = 20;
    
    for (int t = 0; t < threadCount; ++t) {
        QThread* thread = QThread::create([&]() {
            for (int i = 0; i < perThread; ++i) {
                QString imei = gen.generateUniqueIMEI();
                QMutexLocker locker(&mutex);
                imeis.append(imei);
            }
        });
        threads.append(thread);
        thread->start();
    }
    
    for (QThread* thread : threads) {
        thread->wait();
    }
    
    // All IMEIs should be unique
    QSet<QString> uniqueSet = QSet<QString>::fromList(imeis);
    QVERIFY2(uniqueSet.size() == imeis.size(), "Thread safety issue - duplicate IMEIs generated");
}

void Test_UniqueDeviceGenerator::testConcurrentGeneration() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    // Concurrent Future test
    QFuture<QString> future = QtConcurrent::run([&]() {
        return gen.generateUniqueIMEI();
    });
    
    future.waitForFinished();
    QString imei = future.result();
    QVERIFY2(imei.length() == 15, "IMEI from concurrent call should be valid");
}

void Test_UniqueDeviceGenerator::testHashGeneration() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    QString hash = gen.generateHash("test_input");
    QVERIFY(!hash.isEmpty());
    
    // SHA-256 produces 64 hex characters
    QVERIFY2(hash.length() == 64, qPrintable(QString("Hash should be 64 chars, got %1").arg(hash.length())));
}

void Test_UniqueDeviceGenerator::testEmptyManufacturer() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    // Empty manufacturer should still generate valid IMEI
    QString imei = gen.generateUniqueIMEI("");
    QVERIFY2(imei.length() == 15, "IMEI should be 15 digits even with empty manufacturer");
}

void Test_UniqueDeviceGenerator::testInvalidInputs() {
    UniqueDeviceGenerator& gen = UniqueDeviceGenerator::instance();
    
    // Invalid characters in manufacturer should not crash
    QString imei = gen.generateUniqueIMEI("!!!INVALID!!!");
    QVERIFY2(imei.length() == 15, "IMEI should be 15 digits with invalid manufacturer");
    
    // Very long manufacturer name
    QString longName = QString("a").repeated(1000);
    imei = gen.generateUniqueIMEI(longName);
    QVERIFY2(imei.length() == 15, "IMEI should be 15 digits with long manufacturer");
}

// ============================================================================
// Main
// ============================================================================

QTEST_MAIN(Test_UniqueDeviceGenerator)
#include "Test_UniqueDeviceGenerator.moc"
