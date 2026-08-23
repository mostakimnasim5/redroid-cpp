/**
 * @file Test_ProfileGenerator.cpp
 * @brief Unit Tests for DeviceProfileGenerator
 * @version 4.0.0
 * 
 * Tests:
 * 1. Deterministic generation (same inputs = same outputs)
 * 2. IMEI Luhn validation
 * 3. MAC address format and locally administered check
 * 4. All 23 parameters generated correctly
 * 5. Profile regeneration produces identical results
 */

#include <QtTest>
#include <QCoreApplication>
#include <QDebug>
#include <set>
#include "VirtualPhonePro/DeviceProfileGenerator.hpp"
#include "VirtualPhonePro/RealisticDeviceProfile.hpp"

using namespace VirtualPhonePro;

// DeviceIdentityProfile fields are std::string; bridge to QString for Qt Test macros
static QString qs(const std::string& v) { return QString::fromStdString(v); }

class Test_ProfileGenerator : public QObject {
    Q_OBJECT

private slots:

    // ════════════════════════════════════════════════════════════════════════
    // TEST 1: DETERMINISTIC GENERATION
    // ════════════════════════════════════════════════════════════════════════
    
    void test_01_DeterministicGeneration() {
        // Setup
        DeviceProfileGenerator gen("HWID-TEST-123", "LICENSE-KEY-456");
        
        // Generate same profile twice
        auto profile1 = gen.generateProfile(1, "dm3q");
        auto profile2 = gen.generateProfile(1, "dm3q");
        
        // Verify all fields match
        QVERIFY2(profile1.imei1 == profile2.imei1, "IMEI1 should be deterministic");
        QVERIFY2(profile1.imei2 == profile2.imei2, "IMEI2 should be deterministic");
        QVERIFY2(profile1.wifi_mac == profile2.wifi_mac, "WiFi MAC should be deterministic");
        QVERIFY2(profile1.serial_number == profile2.serial_number, "Serial should be deterministic");
        QVERIFY2(profile1.android_id == profile2.android_id, "Android ID should be deterministic");
        QVERIFY2(profile1.master_seed_hex == profile2.master_seed_hex, "Master seed should be deterministic");
        
        qDebug() << "✅ Deterministic Generation: PASS";
    }
    
    // ════════════════════════════════════════════════════════════════════════
    // TEST 2: IMEI LUHN VALIDATION
    // ════════════════════════════════════════════════════════════════════════
    
    void test_02_IMEILuhnValidation() {
        DeviceProfileGenerator gen("HWID-TEST-123", "LICENSE-KEY-456");
        auto profile = gen.generateProfile(1, "dm3q");
        
        // Check IMEI1
        QVERIFY2(profile.imei1.length() == 15, "IMEI1 should be 15 digits");
        QVERIFY2(DeviceProfileGenerator::validateIMEI(profile.imei1), 
                 qPrintable(QStringLiteral("IMEI1 should pass Luhn: ") + qs(profile.imei1)));
        
        // Check IMEI2
        QVERIFY2(profile.imei2.length() == 15, "IMEI2 should be 15 digits");
        QVERIFY2(DeviceProfileGenerator::validateIMEI(profile.imei2),
                 qPrintable(QStringLiteral("IMEI2 should pass Luhn: ") + qs(profile.imei2)));
        
        // Check IMEIs are different
        QVERIFY2(profile.imei1 != profile.imei2, "IMEI1 and IMEI2 should be different");
        
        // Known test cases
        QVERIFY2(DeviceProfileGenerator::validateIMEI("358751071234567"), "Valid IMEI test");
        QVERIFY2(!DeviceProfileGenerator::validateIMEI("358751071234568"), "Invalid IMEI test");
        
        qDebug() << "✅ IMEI Luhn Validation: PASS";
        qDebug() << "   IMEI1:" << qs(profile.imei1);
        qDebug() << "   IMEI2:" << qs(profile.imei2);
    }
    
    // ════════════════════════════════════════════════════════════════════════
    // TEST 3: LOCALLY ADMINISTERED MAC ADDRESSES
    // ════════════════════════════════════════════════════════════════════════
    
    void test_03_LocallyAdministeredMAC() {
        DeviceProfileGenerator gen("HWID-TEST-123", "LICENSE-KEY-456");
        auto profile = gen.generateProfile(1, "dm3q");
        
        // Check MAC format
        QVERIFY2(DeviceProfileGenerator::validateMAC(profile.wifi_mac), 
                 qPrintable(QStringLiteral("WiFi MAC format invalid: ") + qs(profile.wifi_mac)));
        QVERIFY2(DeviceProfileGenerator::validateMAC(profile.bluetooth_mac),
                 qPrintable(QStringLiteral("Bluetooth MAC format invalid: ") + qs(profile.bluetooth_mac)));
        QVERIFY2(DeviceProfileGenerator::validateMAC(profile.bssid),
                 qPrintable(QStringLiteral("BSSID format invalid: ") + qs(profile.bssid)));
        
        // Check Locally Administered bit (zero collision with real devices)
        QVERIFY2(DeviceProfileGenerator::isLocallyAdministeredMAC(profile.wifi_mac),
                 qPrintable(QStringLiteral("WiFi MAC should be locally administered: ") + qs(profile.wifi_mac)));
        QVERIFY2(DeviceProfileGenerator::isLocallyAdministeredMAC(profile.bluetooth_mac),
                 qPrintable(QStringLiteral("Bluetooth MAC should be locally administered: ") + qs(profile.bluetooth_mac)));
        QVERIFY2(DeviceProfileGenerator::isLocallyAdministeredMAC(profile.bssid),
                 qPrintable(QStringLiteral("BSSID should be locally administered: ") + qs(profile.bssid)));
        
        // Check different MACs
        QVERIFY2(profile.wifi_mac != profile.bluetooth_mac, "WiFi and Bluetooth MAC should differ");
        
        qDebug() << "✅ Locally Administered MAC: PASS";
        qDebug() << "   WiFi MAC:" << qs(profile.wifi_mac);
        qDebug() << "   Bluetooth MAC:" << qs(profile.bluetooth_mac);
        qDebug() << "   BSSID:" << qs(profile.bssid);
    }
    
    // ════════════════════════════════════════════════════════════════════════
    // TEST 4: ALL 23 PARAMETERS GENERATED
    // ════════════════════════════════════════════════════════════════════════
    
    void test_04_All23Parameters() {
        DeviceProfileGenerator gen("HWID-TEST-123", "LICENSE-KEY-456");
        auto profile = gen.generateProfile(1, "dm3q");
        
        // Deterministic Parameters (20)
        QVERIFY2(!profile.imei1.empty(), "IMEI1 missing");
        QVERIFY2(!profile.imei2.empty(), "IMEI2 missing");
        QVERIFY2(!profile.wifi_mac.empty(), "WiFi MAC missing");
        QVERIFY2(!profile.bluetooth_mac.empty(), "Bluetooth MAC missing");
        QVERIFY2(!profile.bssid.empty(), "BSSID missing");
        QVERIFY2(!profile.android_id.empty(), "Android ID missing");
        QVERIFY2(!profile.gsf_id.empty(), "GSF ID missing");
        QVERIFY2(!profile.advertising_id.empty(), "AAID missing");
        QVERIFY2(!profile.imsi1.empty(), "IMSI1 missing");
        QVERIFY2(!profile.imsi2.empty(), "IMSI2 missing");
        QVERIFY2(!profile.iccid1.empty(), "ICCID1 missing");
        QVERIFY2(!profile.iccid2.empty(), "ICCID2 missing");
        QVERIFY2(!profile.phone_number1.empty(), "Phone1 missing");
        QVERIFY2(!profile.phone_number2.empty(), "Phone2 missing");
        QVERIFY2(!profile.serial_number.empty(), "Serial Number missing");
        QVERIFY2(!profile.local_ip.empty(), "Local IP missing");
        QVERIFY2(!profile.bootloader_version.empty(), "Bootloader missing");
        QVERIFY2(!profile.radio_version.empty(), "Radio Version missing");
        QVERIFY2(!profile.device_key.empty(), "Device Key missing");
        QVERIFY2(!profile.auth_token.empty(), "Auth Token missing");
        
        // Static Template Parameters (3)
        QVERIFY2(!profile.build_fingerprint.empty(), "Build Fingerprint missing");
        QVERIFY2(!profile.model_name.empty(), "Model Name missing");
        QVERIFY2(!profile.hardware_board.empty(), "Hardware Board missing");
        
        // Metadata
        QVERIFY2(profile.android_version > 0, "Android version missing");
        QVERIFY2(profile.sdk_version > 0, "SDK version missing");
        QVERIFY2(!profile.master_seed_hex.empty(), "Master seed missing");
        
        qDebug() << "✅ All 23 Parameters Generated: PASS";
        qDebug() << "   Model:" << qs(profile.model_name);
        qDebug() << "   Build:" << qs(profile.build_fingerprint);
        qDebug() << "   Board:" << qs(profile.hardware_board);
    }
    
    // ════════════════════════════════════════════════════════════════════════
    // TEST 5: PROFILE REGENERATION (DELETE/RECREATE)
    // ════════════════════════════════════════════════════════════════════════
    
    void test_05_ProfileRegeneration() {
        DeviceProfileGenerator gen("HWID-TEST-123", "LICENSE-KEY-456");
        
        // First creation
        auto profile1 = gen.generateProfile(100);
        
        // Simulate deletion
        QString saved_imei1 = qs(profile1.imei1);
        
        // Regeneration (after "deletion")
        auto profile2 = gen.regenerateProfile(100);
        
        // Should be identical
        QVERIFY2(profile1.imei1 == profile2.imei1, 
                 qPrintable(QStringLiteral("Regenerated IMEI should match: ") + saved_imei1 + " vs " + qs(profile2.imei1)));
        QVERIFY2(profile1.master_seed_hex == profile2.master_seed_hex, 
                 "Master seed should match after regeneration");
        
        qDebug() << "✅ Profile Regeneration: PASS";
        qDebug() << "   Same IMEI after regeneration:" << qs(profile1.imei1);
    }
    
    // ════════════════════════════════════════════════════════════════════════
    // TEST 6: DIFFERENT HWID = DIFFERENT PROFILES
    // ════════════════════════════════════════════════════════════════════════
    
    void test_06_DifferentHWID() {
        DeviceProfileGenerator gen1("HWID-111", "LICENSE-KEY");
        DeviceProfileGenerator gen2("HWID-222", "LICENSE-KEY");
        
        auto profile1 = gen1.generateProfile(1);
        auto profile2 = gen2.generateProfile(1);
        
        // Different HWID should produce different profiles
        QVERIFY2(profile1.imei1 != profile2.imei1, 
                 "Different HWID should produce different IMEI");
        QVERIFY2(profile1.wifi_mac != profile2.wifi_mac,
                 "Different HWID should produce different WiFi MAC");
        
        qDebug() << "✅ Different HWID: PASS";
    }
    
    // ════════════════════════════════════════════════════════════════════════
    // TEST 7: UNIQUE ACROSS MULTIPLE PROFILES
    // ════════════════════════════════════════════════════════════════════════
    
    void test_07_UniqueAcrossProfiles() {
        DeviceProfileGenerator gen("HWID-UNIQUE-TEST", "LICENSE-KEY");
        
        std::set<std::string> imeis;
        std::set<std::string> serials;
        
        // Generate 100 profiles
        for (uint32_t i = 1; i <= 100; i++) {
            auto profile = gen.generateProfile(i);
            
            // Check uniqueness
            QVERIFY2(imeis.find(profile.imei1) == imeis.end(),
                     qPrintable(QStringLiteral("Duplicate IMEI found: ") + qs(profile.imei1)));
            QVERIFY2(serials.find(profile.serial_number) == serials.end(),
                     qPrintable(QStringLiteral("Duplicate Serial found: ") + qs(profile.serial_number)));
            
            imeis.insert(profile.imei1);
            serials.insert(profile.serial_number);
        }
        
        QVERIFY2(imeis.size() == 100, "Should have 100 unique IMEIs");
        QVERIFY2(serials.size() == 100, "Should have 100 unique Serials");
        
        qDebug() << "✅ Unique Across 100 Profiles: PASS";
        qDebug() << "   Unique IMEIs:" << imeis.size();
        qDebug() << "   Unique Serials:" << serials.size();
    }
    
    // ════════════════════════════════════════════════════════════════════════
    // TEST 8: STATIC TEMPLATE PROPERTIES
    // ════════════════════════════════════════════════════════════════════════
    
    void test_08_StaticTemplateProperties() {
        DeviceProfileGenerator gen("HWID-TEST", "LICENSE-KEY");
        
        // Samsung template
        auto samsung = gen.generateProfile(1, "dm3q");
        QVERIFY2(samsung.manufacturer == "Samsung", "Samsung manufacturer");
        QVERIFY2(samsung.model_name == "SM-S928B", "Samsung model");
        QVERIFY2(samsung.hardware_board == "kalama", "Samsung board");
        QVERIFY2(qs(samsung.build_fingerprint).contains("samsung/dm3q/dm3q"),
                 "Samsung fingerprint");
        
        // Google template
        auto google = gen.generateProfile(2, "husky");
        QVERIFY2(google.manufacturer == "Google", "Google manufacturer");
        QVERIFY2(google.model_name == "Pixel 8 Pro", "Google model");
        QVERIFY2(google.hardware_board == "husky", "Google board");
        QVERIFY2(qs(google.build_fingerprint).contains("google/husky/husky"),
                 "Google fingerprint");
        
        qDebug() << "✅ Static Template Properties: PASS";
        qDebug() << "   Samsung:" << qs(samsung.model_name) << "/" << qs(samsung.hardware_board);
        qDebug() << "   Google:" << qs(google.model_name) << "/" << qs(google.hardware_board);
    }
    
    // ════════════════════════════════════════════════════════════════════════
    // TEST 9: PERFORMANCE BENCHMARK
    // ════════════════════════════════════════════════════════════════════════
    
    void test_09_PerformanceBenchmark() {
        DeviceProfileGenerator gen("HWID-PERF-TEST", "LICENSE-KEY");
        
        QBENCHMARK {
            for (uint32_t i = 0; i < 1000; i++) {
                volatile auto profile = gen.generateProfile(i);
                Q_UNUSED(profile);
            }
        }
        
        qDebug() << "✅ Performance Benchmark: PASS";
    }

    // ════════════════════════════════════════════════════════════════════════
    // TEST 10: CROSS-PROFILE UNIQUENESS (GPU / Timezone / IMEI / Serial / MAC)
    // ════════════════════════════════════════════════════════════════════════

    void test_10_CrossProfileUniqueness() {
        // Two generated profiles must never share the same identity tuple
        // (GPU renderer, timezone, IMEI, serial, MAC). GPU and timezone are
        // deterministic per manufacturer/country, so IMEI/serial/MAC carry
        // the hard uniqueness guarantee — assert those individually too.
        const QVector<QPair<QString, QString>> devices = {
            {"Samsung", "SM-S928B"}, {"Google", "Pixel 8 Pro"},
            {"Xiaomi", "Xiaomi 14 Ultra"}, {"OnePlus", "OnePlus 12"},
        };
        const QStringList countries = {"US", "DE", "JP", "BR", "IN", "GB"};

        std::set<QString> tuples, imeis, serials, macs;
        const int profileCount = 60;

        for (int i = 0; i < profileCount; i++) {
            const auto& dev = devices[i % devices.size()];
            const QString& country = countries[i % countries.size()];

            QJsonObject p = RealisticDeviceProfile::instance()
                .generateCompleteProfile(dev.first, dev.second, "14", country);

            const QString gpu  = p["hardware"].toObject()["gpuRenderer"].toString();
            const QString tz   = p["timing"].toObject()["timeZoneId"].toString();
            const QString imei = p["identity"].toObject()["imei"].toString();
            const QString ser  = p["identity"].toObject()["serialNumber"].toString();
            const QString mac  = p["identity"].toObject()["wlanMac"].toString();

            QVERIFY2(!gpu.isEmpty() && !tz.isEmpty() && !imei.isEmpty()
                     && !ser.isEmpty() && !mac.isEmpty(),
                     "Generated profile has empty identity fields");

            const QString tuple = gpu + "|" + tz + "|" + imei + "|" + ser + "|" + mac;
            QVERIFY2(tuples.find(tuple) == tuples.end(),
                     qPrintable("Duplicate GPU/timezone/IMEI/serial/MAC tuple: " + tuple));
            QVERIFY2(imeis.find(imei) == imeis.end(),
                     qPrintable("Duplicate IMEI across profiles: " + imei));
            QVERIFY2(serials.find(ser) == serials.end(),
                     qPrintable("Duplicate serial across profiles: " + ser));
            QVERIFY2(macs.find(mac) == macs.end(),
                     qPrintable("Duplicate MAC across profiles: " + mac));

            tuples.insert(tuple);
            imeis.insert(imei);
            serials.insert(ser);
            macs.insert(mac);
        }

        QCOMPARE(tuples.size(), static_cast<size_t>(profileCount));
        QCOMPARE(imeis.size(), static_cast<size_t>(profileCount));
        QCOMPARE(serials.size(), static_cast<size_t>(profileCount));
        QCOMPARE(macs.size(), static_cast<size_t>(profileCount));

        qDebug() << "✅ Cross-Profile Uniqueness (60 profiles): PASS";
    }
};

QTEST_MAIN(Test_ProfileGenerator)
#include "Test_ProfileGenerator.moc"
