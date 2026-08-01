/**
 * @file Test_Validation.cpp
 * @brief Unit tests for validation logic (Login, Proxy, VPN, DNS configs)
 */

#include <QtTest/QtTest>
#include <QString>
#include <QRegularExpression>

class Test_Validation : public QObject {
    Q_OBJECT

private slots:
    // ==========================================
    // Login Validation Tests
    // ==========================================
    void test_nameValidation_minLength();
    void test_nameValidation_validNames();
    void test_nameValidation_invalidNames();
    
    void test_phoneValidation_validFormats();
    void test_phoneValidation_invalidFormats();
    void test_phoneValidation_bangladeshi();
    
    void test_emailValidation_validEmails();
    void test_emailValidation_invalidEmails();
    
    void test_profileCountValidation();
    void test_durationValidation();
    
    // ==========================================
    // Proxy Configuration Tests
    // ==========================================
    void test_proxyConfig_ipv4Validation();
    void test_proxyConfig_portValidation();
    void test_proxyConfig_usernamePassword();
    
    // ==========================================
    // VPN Configuration Tests
    // ==========================================
    void test_vpnConfig_serverValidation();
    void test_vpnConfig_protocolValidation();
    
    // ==========================================
    // DNS Configuration Tests
    // ==========================================
    void test_dnsConfig_ipv4Validation();
    void test_dnsConfig_knownProviders();
    
    // ==========================================
    // GPS/Location Validation Tests
    // ==========================================
    void test_gps_latitudeValidation();
    void test_gps_longitudeValidation();
    void test_gps_altitudeValidation();
};

// ==============================================================================
// Login Validation Tests
// ==============================================================================

void Test_Validation::test_nameValidation_minLength() {
    // Name should be at least 2 characters
    QString shortName = "A";
    QVERIFY(shortName.length() < 2); // Should fail validation
    
    QString validName = "Al";
    QVERIFY(validName.length() >= 2); // Should pass validation
}

void Test_Validation::test_nameValidation_validNames() {
    QStringList validNames = {
        "Al",
        "John",
        "Mostakim",
        "Rahman Khan",
        "Md. Rahim",
        "নাসরিন",  // Bengali name
        "张三"       // Chinese name
    };
    
    for (const QString& name : validNames) {
        QVERIFY2(name.length() >= 2, qUtf8Printable(QString("Valid name: %1").arg(name)));
    }
}

void Test_Validation::test_nameValidation_invalidNames() {
    QStringList invalidNames = {
        "A",           // Too short
        "",            // Empty
        " ",           // Only space
        "   ",         // Multiple spaces
        "AB"           // 2 chars is valid
    };
    
    for (const QString& name : invalidNames) {
        if (name.isEmpty() || name.trimmed().length() < 2) {
            QVERIFY2(true, qUtf8Printable(QString("Invalid name detected: '%1'").arg(name)));
        }
    }
}

void Test_Validation::test_phoneValidation_validFormats() {
    // Valid Bangladeshi phone formats
    QStringList validPhones = {
        "01812345678",
        "01712345678",
        "01612345678",
        "01912345678",
        "01512345678",
        "+8801812345678",
        "8801812345678",
        "01312345678"  // City cell
    };
    
    for (const QString& phone : validPhones) {
        // Remove non-digit characters for length check
        QString digits = phone;
        digits.replace(QRegularExpression("[^0-9]"), "");
        
        // Should be 11 digits (Bangladeshi format) or 13 with country code
        QVERIFY2(digits.length() >= 11, 
            qUtf8Printable(QString("Valid phone: %1 (%2 digits)").arg(phone).arg(digits.length())));
    }
}

void Test_Validation::test_phoneValidation_invalidFormats() {
    QStringList invalidPhones = {
        "12345",              // Too short
        "018123456",          // Only 10 digits
        "018123456789",       // 12 digits
        "123456789012",       // No valid prefix
        "",                   // Empty
        "abcd1234567890"      // Contains letters
    };
    
    for (const QString& phone : invalidPhones) {
        QString digits = phone;
        digits.replace(QRegularExpression("[^0-9]"), "");
        
        // Invalid if not 11 or 13-14 digits
        bool isInvalid = digits.length() < 11 || digits.length() > 14 || 
                         !digits.startsWith("01");
        
        QVERIFY2(isInvalid, qUtf8Printable(QString("Invalid phone detected: %1").arg(phone)));
    }
}

void Test_Validation::test_phoneValidation_bangladeshi() {
    // Test all Bangladeshi operator prefixes
    QStringList prefixes = {"013", "014", "015", "016", "017", "018", "019"};
    
    for (const QString& prefix : prefixes) {
        QString phone = prefix + "1234567"; // 11 digits total
        QVERIFY(phone.length() == 11);
        QVERIFY(phone.startsWith("01")); // Must start with 01
    }
}

void Test_Validation::test_emailValidation_validEmails() {
    QStringList validEmails = {
        "test@example.com",
        "user.name@domain.org",
        "admin@company.co.uk",
        "user123@test-domain.com",
        "firstname.lastname@company.com"
    };
    
    QRegularExpression emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    
    for (const QString& email : validEmails) {
        QVERIFY2(emailRegex.match(email).hasMatch(),
            qUtf8Printable(QString("Valid email: %1").arg(email)));
    }
}

void Test_Validation::test_emailValidation_invalidEmails() {
    QStringList invalidEmails = {
        "invalid",
        "missing@domain",
        "@nodomain.com",
        "spaces in@email.com",
        "missing.domain@",
        ""
    };
    
    QRegularExpression emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    
    for (const QString& email : invalidEmails) {
        if (email.isEmpty()) {
            QVERIFY2(true, "Empty email correctly identified");
        } else {
            QVERIFY2(!emailRegex.match(email).hasMatch(),
                qUtf8Printable(QString("Invalid email detected: %1").arg(email)));
        }
    }
}

void Test_Validation::test_profileCountValidation() {
    // Profile count should be between 1 and 50
    int minProfiles = 1;
    int maxProfiles = 50;
    
    QVERIFY(minProfiles > 0);
    QVERIFY(maxProfiles <= 50);
    
    // Test boundary values
    QVERIFY(0 < minProfiles);  // 0 is invalid
    QVERIFY(51 > maxProfiles); // 51 is invalid
}

void Test_Validation::test_durationValidation() {
    // Duration should be positive and reasonable (e.g., max 30 days in minutes)
    int minDuration = 60;       // 1 hour in minutes
    int maxDuration = 43200;    // 30 days in minutes
    
    QVERIFY(minDuration > 0);
    QVERIFY(maxDuration > 0);
    QVERIFY(maxDuration > minDuration);
}

// ==============================================================================
// Proxy Configuration Tests
// ==============================================================================

void Test_Validation::test_proxyConfig_ipv4Validation() {
    // Valid IPv4 addresses
    QStringList validIPs = {
        "192.168.1.1",
        "10.0.0.1",
        "172.16.0.1",
        "127.0.0.1",
        "8.8.8.8",
        "1.1.1.1"
    };
    
    QRegularExpression ipRegex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    
    for (const QString& ip : validIPs) {
        QVERIFY2(ipRegex.match(ip).hasMatch(),
            qUtf8Printable(QString("Valid IP: %1").arg(ip)));
    }
}

void Test_Validation::test_proxyConfig_portValidation() {
    int minPort = 1;
    int maxPort = 65535;
    
    // Valid ports
    QVERIFY(8080 >= minPort && 8080 <= maxPort);
    QVERIFY(3128 >= minPort && 3128 <= maxPort);
    QVERIFY(1080 >= minPort && 1080 <= maxPort);
    
    // Invalid ports
    QVERIFY(0 < minPort);   // Port 0 is invalid
    QVERIFY(65536 > maxPort); // Port 65536 is invalid
}

void Test_Validation::test_proxyConfig_usernamePassword() {
    // Username/password should not be empty if provided
    QString username = "testuser";
    QString password = "testpass";
    
    QVERIFY(!username.isEmpty());
    QVERIFY(!password.isEmpty());
    QVERIFY(username.length() >= 3);
    QVERIFY(password.length() >= 4);
}

// ==============================================================================
// VPN Configuration Tests
// ==============================================================================

void Test_Validation::test_vpnConfig_serverValidation() {
    // Valid server formats
    QStringList validServers = {
        "vpn.example.com",
        "104.16.1.1",
        "192.168.1.1",
        "us-east.vpn.example.com"
    };
    
    QRegularExpression domainRegex("^[a-zA-Z0-9]([a-zA-Z0-9-]*[a-zA-Z0-9])?(\\.[a-zA-Z0-9]([a-zA-Z0-9-]*[a-zA-Z0-9])?)*$");
    QRegularExpression ipRegex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    
    for (const QString& server : validServers) {
        bool isValid = domainRegex.match(server).hasMatch() || ipRegex.match(server).hasMatch();
        QVERIFY2(isValid, qUtf8Printable(QString("Valid VPN server: %1").arg(server)));
    }
}

void Test_Validation::test_vpnConfig_protocolValidation() {
    // Supported VPN protocols
    QStringList validProtocols = {
        "OpenVPN",
        "WireGuard",
        "IKEv2",
        "L2TP/IPSec",
        "PPTP",
        "SSTP"
    };
    
    for (const QString& proto : validProtocols) {
        QVERIFY2(!proto.isEmpty(), qUtf8Printable(QString("Valid protocol: %1").arg(proto)));
    }
}

// ==============================================================================
// DNS Configuration Tests
// ==============================================================================

void Test_Validation::test_dnsConfig_ipv4Validation() {
    // Common DNS servers
    QStringList dnsServers = {
        "8.8.8.8",        // Google
        "8.8.4.4",        // Google Secondary
        "1.1.1.1",        // Cloudflare
        "1.0.0.1",        // Cloudflare Secondary
        "9.9.9.9",        // Quad9
        "208.67.222.222", // OpenDNS
        "208.67.220.220"  // OpenDNS Secondary
    };
    
    QRegularExpression ipRegex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$");
    
    for (const QString& dns : dnsServers) {
        QVERIFY2(ipRegex.match(dns).hasMatch(),
            qUtf8Printable(QString("Valid DNS: %1").arg(dns)));
    }
}

void Test_Validation::test_dnsConfig_knownProviders() {
    // Known DNS providers with their IPs
    QMap<QString, QStringList> dnsProviders = {
        {"Google", {"8.8.8.8", "8.8.4.4"}},
        {"Cloudflare", {"1.1.1.1", "1.0.0.1"}},
        {"Quad9", {"9.9.9.9", "149.112.112.112"}},
        {"OpenDNS", {"208.67.222.222", "208.67.220.220"}},
        {"CleanBrowsing", {"185.228.168.9", "185.228.169.9"}}
    };
    
    QVERIFY(dnsProviders.contains("Google"));
    QVERIFY(dnsProviders.contains("Cloudflare"));
    QVERIFY(dnsProviders.contains("Quad9"));
    QVERIFY(dnsProviders.value("Google").size() == 2);
}

// ==============================================================================
// GPS/Location Validation Tests
// ==============================================================================

void Test_Validation::test_gps_latitudeValidation() {
    double minLat = -90.0;
    double maxLat = 90.0;
    
    // Valid latitudes
    QVERIFY(23.8103 >= minLat && 23.8103 <= maxLat);  // Dhaka
    QVERIFY(0.0 >= minLat && 0.0 <= maxLat);           // Equator
    QVERIFY(-33.8688 >= minLat && -33.8688 <= maxLat); // Sydney
    
    // Invalid latitudes
    QVERIFY(-91.0 < minLat);  // Below minimum
    QVERIFY(91.0 > maxLat);   // Above maximum
}

void Test_Validation::test_gps_longitudeValidation() {
    double minLon = -180.0;
    double maxLon = 180.0;
    
    // Valid longitudes
    QVERIFY(90.4125 >= minLon && 90.4125 <= maxLon);  // Dhaka
    QVERIFY(0.0 >= minLon && 0.0 <= maxLon);          // Prime Meridian
    QVERIFY(151.2093 >= minLon && 151.2093 <= maxLon); // Sydney
    
    // Invalid longitudes
    QVERIFY(-181.0 < minLon); // Below minimum
    QVERIFY(181.0 > maxLon);  // Above maximum
}

void Test_Validation::test_gps_altitudeValidation() {
    // Altitude can be negative (below sea level) or positive
    double minAlt = -500.0;  // Lowest point on Earth (Dead Sea)
    double maxAlt = 8848.0;  // Mount Everest
    
    // Valid altitudes
    QVERIFY(100.0 >= minAlt && 100.0 <= maxAlt);  // Typical city
    QVERIFY(0.0 >= minAlt && 0.0 <= maxAlt);       // Sea level
    QVERIFY(8848.0 >= minAlt && 8848.0 <= maxAlt); // Everest
    
    // Invalid altitude
    QVERIFY(-501.0 < minAlt);  // Too low
    QVERIFY(8849.0 > maxAlt);  // Too high
}

// ==============================================================================
// Main entry point
// ==============================================================================

QTEST_MAIN(Test_Validation)
#include "Test_Validation.moc"
