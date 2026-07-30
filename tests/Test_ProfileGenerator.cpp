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
#include "VirtualPhonePro/DeviceProfileGenerator.hpp"

using namespace VirtualPhonePro;

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
        auto profile1 = gen.generateProfile(1, "samsung_s24_ultra");
        auto profile2 = gen.generateProfile(1, "samsung_s24_ultra");
        
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
        auto profile = gen.generateProfile(1, "samsung_s24_ultra");
        
        // Check IMEI1
        QVERIFY2(profile.imei1.length() == 15, "IMEI1 should be 15 digits");
        QVERIFY2(DeviceProfileGenerator::validateIMEI(profile.imei1), 
                 qPrintable("IMEI1 should pass Luhn: " + profile.imei1));
        
        // Check IMEI2
        QVERIFY2(profile.imei2.length() == 15, "IMEI2 should be 15 digits");
        QVERIFY2(DeviceProfileGenerator::validateIMEI(profile.imei2),
                 qPrintable("IMEI2 should pass Luhn: " + profile.imei2));
        
        // Check IMEIs are different
        QVERIFY2(profile.imei1 != profile.imei2, "IMEI1 and IMEI2 should be different");
        
        // Known test cases
        QVERIFY2(DeviceProfileGenerator::validateIMEI("358751071234567"), "Valid IMEI test");
        QVERIFY2(!DeviceProfileGenerator::validateIMEI("358751071234568"), "Invalid IMEI test");
        
        qDebug() << "✅ IMEI Luhn Validation: PASS";
        qDebug() << "   IMEI1:" << profile.imei1;
        qDebug() << "   IMEI2:" << profile.imei2;
    }
    
    // ════════════════════════════════════════════════════════════════════════
    // TEST 3: LOCALLY ADMINISTERED MAC ADDRESSES
    // ════════════════════════════════════════════════════════════════════════
    
    void test_03_LocallyAdministeredMAC() {
        DeviceProfileGenerator gen("HWID-TEST-123", "LICENSE-KEY-456");
        auto profile = gen.generateProfile(1, "samsung_s24_ultra");
        
        // Check MAC format
        QVERIFY2(DeviceProfileGenerator::validateMAC(profile.wifi_mac), 
                 qPrintable("WiFi MAC format invalid: " + profile.wifi_mac));
        QVERIFY2(DeviceProfileGenerator::validateMAC(profile.bluetooth_mac),
                 qPrintable("Bluetooth MAC format invalid: " + profile.bluetooth_mac));
        QVERIFY2(DeviceProfileGenerator::validateMAC(profile.bssid),
                 qPrintable("BSSID format invalid: " + profile.bssid));
        
        // Check Locally Administered bit (zero collision with real devices)
        QVERIFY2(DeviceProfileGenerator::isLocallyAdministeredMAC(profile.wifi_mac),
                 qPrintable("WiFi MAC should be locally administered: " + profile.wifi_mac));
        QVERIFY2(DeviceProfileGenerator::isLocallyAdministeredMAC(profile.bluetooth_mac),
                 qPrintable("Bluetooth MAC should be locally administered: " + profile.bluetooth_mac));
        QVERIFY2(DeviceProfileGenerator::isLocallyAdministeredMAC(profile.bssid),
                 qPrintable("BSSID should be locally administered: " + profile.bssid));
        
        // Check different MACs
        QVERIFY2(profile.wifi_mac != profile.bluetooth_mac, "WiFi and Bluetooth MAC should differ");
        
        qDebug() << "✅ Locally Administered MAC: PASS";
        qDebug() << "   WiFi MAC:" << profile.wifi_mac;
        qDebug() << "   Bluetooth MAC:" << profile.bluetooth_mac;
        qDebug() << "   BSSID:" << profile.bssid;
    }
    
    // ════════════════════════════════════════════════════════════════════════
    // TEST 4: ALL 23 PARAMETERS GENERATED
    // ════════════════════════════════════════════════════════════════════════
    
    void test_04_All23Parameters() {
        DeviceProfileGenerator gen("HWID-TEST-123", "LICENSE-KEY-456");
        auto profile = gen.generateProfile(1, "samsung_s24_ultra");
        
        // Deterministic Parameters (20)
        QVERIFY2(!profile.imei1.isEmpty(), "IMEI1 missing");
        QVERIFY2(!profile.imei2.isEmpty(), "IMEI2 missing");
        QVERIFY2(!profile.wifi_mac.isEmpty(), "WiFi MAC missing");
        QVERIFY2(!profile.bluetooth_mac.isEmpty(), "Bluetooth MAC missing");
        QVERIFY2(!profile.bssid.isEmpty(), "BSSID missing");
        QVERIFY2(!profile.android_id.isEmpty(), "Android ID missing");
        QVERIFY2(!profile.gsf_id.isEmpty(), "GSF ID missing");
        QVERIFY2(!profile.advertising_id.isEmpty(), "AAID missing");
        QVERIFY2(!profile.imsi1.isEmpty(), "IMSI1 missing");
        QVERIFY2(!profile.imsi2.isEmpty(), "IMSI2 missing");
        QVERIFY2(!profile.iccid1.isEmpty(), "ICCID1 missing");
        QVERIFY2(!profile.iccid2.isEmpty(), "ICCID2 missing");
        QVERIFY2(!profile.phone_number1.isEmpty(), "Phone1 missing");
        QVERIFY2(!profile.phone_number2.isEmpty(), "Phone2 missing");
        QVERIFY2(!profile.serial_number.isEmpty(), "Serial Number missing");
        QVERIFY2(!profile.local_ip.isEmpty(), "Local IP missing");
        QVERIFY2(!profile.bootloader_version.isEmpty(), "Bootloader missing");
        QVERIFY2(!profile.radio_version.isEmpty(), "Radio Version missing");
        QVERIFY2(!profile.device_key.isEmpty(), "Device Key missing");
        QVERIFY2(!profile.auth_token.isEmpty(), "Auth Token missing");
        
        // Static Template Parameters (3)
        QVERIFY2(!profile.build_fingerprint.isEmpty(), "Build Fingerprint missing");
        QVERIFY2(!profile.model_name.isEmpty(), "Model Name missing");
        QVERIFY2(!profile.hardware_board.isEmpty(), "Hardware Board missing");
        
        // Metadata
        QVERIFY2(profile.android_version > 0, "Android version missing");
        QVERIFY2(profile.sdk_version > 0, "SDK version missing");
        QVERIFY2(!profile.master_seed_hex.isEmpty(), "Master seed missing");
        
        qDebug() << "✅ All 23 Parameters Generated: PASS";
        qDebug() << "   Model:" << profile.model_name;
        qDebug() << "   Build:" << profile.build_fingerprint;
        qDebug() << "   Board:" << profile.hardware_board;
    }
    
    // ════════════════════════════════════════════════════════════════════════
    // TEST 5: PROFILE REGENERATION (DELETE/RECREATE)
    // ════════════════════════════════════════════════════════════════════════
    
    void test_05_ProfileRegeneration() {
        DeviceProfileGenerator gen("HWID-TEST-123", "LICENSE-KEY-456");
        
        // First creation
        auto profile1 = gen.generateProfile(100);
        
        // Simulate deletion
        QString saved_imei1 = profile1.imei1;
        
        // Regeneration (after "deletion")
        auto profile2 = gen.regenerateProfile(100);
        
        // Should be identical
        QVERIFY2(profile1.imei1 == profile2.imei1, 
                 qPrintable("Regenerated IMEI should match: " + saved_imei1 + " vs " + profile2.imei1));
        QVERIFY2(profile1.master_seed_hex == profile2.master_seed_hex, 
                 "Master seed should match after regeneration");
        
        qDebug() << "✅ Profile Regeneration: PASS";
        qDebug() << "   Same IMEI after regeneration:" << profile1.imei1;
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
        
        std::set<QString> imeis;
        std::set<QString> serials;
        
        // Generate 100 profiles
        for (uint32_t i = 1; i <= 100; i++) {
            auto profile = gen.generateProfile(i);
            
            // Check uniqueness
            QVERIFY2(imeis.find(profile.imei1) == imeis.end(),
                     qPrintable("Duplicate IMEI found: " + profile.imei1));
            QVERIFY2(serials.find(profile.serial_number) == serials.end(),
                     qPrintable("Duplicate Serial found: " + profile.serial_number));
            
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
        auto samsung = gen.generateProfile(1, "samsung_s24_ultra");
        QVERIFY2(samsung.manufacturer == "Samsung", "Samsung manufacturer");
        QVERIFY2(samsung.model_name == "SM-S928B", "Samsung model");
        QVERIFY2(samsung.hardware_board == "kalama", "Samsung board");
        QVERIFY2(samsung.build_fingerprint.contains("samsung/dm3q/dm3q"),
                 "Samsung fingerprint");
        
        // Google template
        auto google = gen.generateProfile(2, "pixel_8_pro");
        QVERIFY2(google.manufacturer == "Google", "Google manufacturer");
        QVERIFY2(google.model_name == "Pixel 8 Pro", "Google model");
        QVERIFY2(google.hardware_board == "husky", "Google board");
        QVERIFY2(google.build_fingerprint.contains("google/husky/husky"),
                 "Google fingerprint");
        
        qDebug() << "✅ Static Template Properties: PASS";
        qDebug() << "   Samsung:" << samsung.model_name << "/" << samsung.hardware_board;
        qDebug() << "   Google:" << google.model_name << "/" << google.hardware_board;
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
};

QTEST_MAIN(Test_ProfileGenerator)
#include "Test_ProfileGenerator.moc"
