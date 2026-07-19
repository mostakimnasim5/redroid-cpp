/**
 * @file RealisticDeviceProfile.cpp
 * @brief 100% Realistic Device Profile Generator Implementation
 * @version 4.0.0
 */

#include "VirtualPhonePro/RealisticDeviceProfile.hpp"
#include "VirtualPhonePro/UniqueDeviceGenerator.h"

#include <QUuid>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>
#include <QDebug>

namespace VirtualPhonePro {

RealisticDeviceProfile* RealisticDeviceProfile::s_instance = nullptr;

RealisticDeviceProfile& RealisticDeviceProfile::instance() {
    if (!s_instance) {
        s_instance = new RealisticDeviceProfile();
    }
    return *s_instance;
}

RealisticDeviceProfile::RealisticDeviceProfile() {
    // QRandomGenerator auto-seeds in Qt6 (qsrand removed)
}

// ========================================================================
// JSON Conversion Methods
// ========================================================================

QJsonObject CompleteDeviceIdentity::toJson() const {
    QJsonObject obj;
    obj["imei"] = imei;
    obj["imei2"] = imei2;
    obj["meid"] = meid;
    obj["serialNumber"] = serialNumber;
    obj["androidId"] = androidId;
    obj["gsfId"] = gsfId;
    obj["advertisingId"] = advertisingId;
    obj["fireAdvertisingId"] = fireAdvertisingId;
    obj["deviceKey"] = deviceKey;
    obj["deviceCredential"] = deviceCredential;
    obj["androidVnci"] = androidVnci;
    obj["wlanMac"] = wlanMac;
    obj["bluetoothMac"] = bluetoothMac;
    obj["ethernetMac"] = ethernetMac;
    obj["nfcMac"] = nfcMac;
    obj["iccid1"] = iccid1;
    obj["iccid2"] = iccid2;
    obj["imsi1"] = imsi1;
    obj["imsi2"] = imsi2;
    obj["knoxId"] = KnoxId;
    obj["knoxVersion"] = KnoxVersion;
    obj["spBleDeviceId"] = spBleDeviceId;
    obj["samsungAccountId"] = samsungAccountId;
    obj["oemSpecificId"] = oemSpecificId;
    obj["secureId"] = secureId;
    obj["vendorId"] = vendorId;
    return obj;
}

QJsonObject CompleteCarrierConfig::toJson() const {
    QJsonObject obj;
    obj["carrierName"] = carrierName;
    obj["carrierCountry"] = carrierCountry;
    obj["carrierId"] = carrierId;
    obj["carrierType"] = carrierType;
    obj["mcc"] = mcc;
    obj["mnc"] = mnc;
    obj["mncLength"] = mncLength;
    obj["simOperatorName"] = simOperatorName;
    obj["simCountry"] = simCountry;
    obj["simSerialNumber"] = simSerialNumber;
    obj["networkType"] = networkType;
    obj["phoneType"] = phoneType;
    obj["dataNetworkType"] = dataNetworkType;
    obj["volteEnabled"] = volteEnabled;
    obj["vowifiEnabled"] = vowifiEnabled;
    obj["wifiCallingEnabled"] = wifiCallingEnabled;
    obj["carrierSpecificKey"] = carrierSpecificKey;
    obj["carrierSpecificValue"] = carrierSpecificValue;
    obj["imsPrivateUri"] = imsPrivateUri;
    obj["imsPublicUri"] = imsPublicUri;
    obj["mmtelImsi"] = mmtelImsi;
    obj["apnName"] = apnName;
    obj["apn"] = apn;
    obj["mmsc"] = mmsc;
    obj["mmsProxy"] = mmsProxy;
    obj["mmsPort"] = mmsPort;
    obj["mccApn"] = mccApn;
    obj["mncApn"] = mncApn;
    return obj;
}

QJsonObject CompleteHardwareSpec::toJson() const {
    QJsonObject obj;
    obj["processor"] = processor;
    obj["cpuAbi"] = cpuAbi;
    obj["cpuAbi2"] = cpuAbi2;
    obj["hardware"] = hardware;
    obj["board"] = board;
    obj["systemOnChip"] = systemOnChip;
    obj["coreCount"] = coreCount;
    obj["bigCoreCount"] = bigCoreCount;
    obj["midCoreCount"] = midCoreCount;
    obj["littleCoreCount"] = littleCoreCount;
    obj["coreFreqMax"] = coreFreqMax;
    obj["coreFreqMin"] = coreFreqMin;
    obj["cpuGovernor"] = cpuGovernor;
    obj["bogoMips"] = bogoMips;
    obj["maxCpuFreq"] = maxCpuFreq;
    obj["minCpuFreq"] = minCpuFreq;
    obj["gpuVendor"] = gpuVendor;
    obj["gpuRenderer"] = gpuRenderer;
    obj["gpuVersion"] = gpuVersion;
    obj["gpuExtensions"] = gpuExtensions;
    obj["vulkanVersion"] = vulkanVersion;
    obj["vulkanConformance"] = vulkanConformance;
    obj["glEsVersion"] = glEsVersion;
    obj["glEsRenderer"] = glEsRenderer;
    obj["glVendor"] = glVendor;
    obj["totalRam"] = QString::number(totalRam);
    obj["totalRamMB"] = QString::number(totalRamMB);
    obj["totalRamGB"] = QString::number(totalRamGB);
    obj["heapSize"] = QString::number(heapSize);
    obj["largeHeapSize"] = QString::number(largeHeapSize);
    obj["lowMemory"] = QString::number(lowMemory);
    obj["memoryPageSize"] = memoryPageSize;
    obj["internalStorage"] = QString::number(internalStorage);
    obj["externalStorage"] = QString::number(externalStorage);
    obj["sdCardId"] = sdCardId;
    obj["batteryCapacity"] = batteryCapacity;
    obj["batteryLevel"] = batteryLevel;
    obj["batteryTemp"] = batteryTemp;
    obj["batteryVoltage"] = batteryVoltage;
    obj["batteryCurrent"] = batteryCurrent;
    obj["batteryStatus"] = batteryStatus;
    obj["batteryHealth"] = batteryHealth;
    obj["batteryTechnology"] = batteryTechnology;
    obj["plugged"] = plugged;
    obj["displayWidth"] = displayWidth;
    obj["displayHeight"] = displayHeight;
    obj["densityDpi"] = densityDpi;
    obj["density"] = QString::number(density);
    obj["refreshRate"] = refreshRate;
    obj["hdrRefreshRate"] = hdrRefreshRate;
    obj["hdrCapabilities"] = hdrCapabilities;
    obj["wideColorGamut"] = wideColorGamut;
    obj["colorMode"] = colorMode;
    obj["audioCodec"] = audioCodec;
    obj["audioOutput"] = audioOutput;
    obj["audioEffect"] = audioEffect;
    obj["speakerCount"] = speakerCount;
    obj["frontCamera"] = frontCamera;
    obj["backCamera"] = backCamera;
    obj["cameraCount"] = cameraCount;
    obj["cameraFeatures"] = cameraFeatures;
    return obj;
}

QJsonObject CompleteBuildInfo::toJson() const {
    QJsonObject obj;
    obj["brand"] = brand;
    obj["manufacturer"] = manufacturer;
    obj["model"] = model;
    obj["device"] = device;
    obj["product"] = product;
    obj["board"] = board;
    obj["fingerprint"] = fingerprint;
    obj["bootimageFingerprint"] = bootimageFingerprint;
    obj["androidVersion"] = androidVersion;
    obj["releaseVersion"] = releaseVersion;
    obj["sdkVersion"] = sdkVersion;
    obj["securityPatch"] = securityPatch;
    obj["buildDescription"] = buildDescription;
    obj["buildId"] = buildId;
    obj["buildDisplay"] = buildDisplay;
    obj["buildType"] = buildType;
    obj["buildTags"] = buildTags;
    obj["bootloader"] = bootloader;
    obj["radioVersion"] = radioVersion;
    obj["baseband"] = baseband;
    obj["buildDate"] = buildDate;
    obj["buildDateTime"] = buildDateTime;
    obj["buildTimeSeconds"] = buildTimeSeconds;
    obj["hardwareRevision"] = hardwareRevision;
    obj["deviceRevision"] = deviceRevision;
    obj["firstApiLevel"] = firstApiLevel;
    obj["targetApiLevel"] = targetApiLevel;
    obj["otaUpdates"] = otaUpdates;
    obj["otaStatus"] = otaStatus;
    return obj;
}

QJsonObject CompleteSecurityConfig::toJson() const {
    QJsonObject obj;
    obj["selinuxStatus"] = selinuxStatus;
    obj["selinuxEnforce"] = selinuxEnforce;
    obj["selinuxMode"] = selinuxMode;
    obj["verifiedBootState"] = verifiedBootState;
    obj["verifiedBootLocked"] = verifiedBootLocked;
    obj["verificationBoot"] = verificationBoot;
    obj["roBootloader"] = roBootloader;
    obj["vbmetaDigest"] = vbmetaDigest;
    obj["vbmetaVersion"] = vbmetaVersion;
    obj["vbmetaAlgorithm"] = vbmetaAlgorithm;
    obj["keymasterVersion"] = keymasterVersion;
    obj["keymasterSecurityLevel"] = keymasterSecurityLevel;
    obj["gatekeeperVersion"] = gatekeeperVersion;
    obj["gatekeeperSecurityLevel"] = gatekeeperSecurityLevel;
    obj["hasStrongbox"] = hasStrongbox;
    obj["strongboxVersion"] = strongboxVersion;
    obj["strongboxSecurityLevel"] = strongboxSecurityLevel;
    obj["trustyVersion"] = trustyVersion;
    obj["hasTrusty"] = hasTrusty;
    obj["attestationEnabled"] = attestationEnabled;
    obj["attestationStatus"] = attestationStatus;
    obj["hasHardwareAttestation"] = hasHardwareAttestation;
    obj["widevineLevel"] = widevineLevel;
    obj["drmSecurityLevel"] = drmSecurityLevel;
    obj["hdcpLevel"] = hdcpLevel;
    obj["fileBasedEncryption"] = fileBasedEncryption;
    obj["fullDiskEncryption"] = fullDiskEncryption;
    obj["isEncrypted"] = isEncrypted;
    obj["knoxVersion"] = knoxVersion;
    obj["knoxId"] = knoxId;
    obj["isKnoxDevice"] = isKnoxDevice;
    obj["odeMode"] = odeMode;
    obj["secureStorageStatus"] = secureStorageStatus;
    obj["huaweiDevice"] = huaweiDevice;
    obj["huaweiSecPlatform"] = huaweiSecPlatform;
    obj["isMiuiDevice"] = isMiuiDevice;
    obj["miuiVersion"] = miuiVersion;
    return obj;
}

QJsonObject CompleteSensorCalibration::toJson() const {
    QJsonObject obj;
    
    // Accelerometer
    QJsonObject accel;
    accel["name"] = accelerometer.name;
    accel["vendor"] = accelerometer.vendor;
    accel["version"] = accelerometer.version;
    accel["resolution"] = QString::number(accelerometer.resolution);
    accel["maxRange"] = QString::number(accelerometer.maxRange);
    accel["power"] = accelerometer.power;
    accel["minDelay"] = QString::number(accelerometer.minDelay);
    accel["type"] = accelerometer.type;
    accel["typeString"] = accelerometer.typeString;
    obj["accelerometer"] = accel;
    
    // Gyroscope
    QJsonObject gyro;
    gyro["name"] = gyroscope.name;
    gyro["vendor"] = gyroscope.vendor;
    gyro["version"] = gyroscope.version;
    obj["gyroscope"] = gyro;
    
    // Magnetometer
    QJsonObject mag;
    mag["name"] = magnetometer.name;
    mag["vendor"] = magnetometer.vendor;
    obj["magnetometer"] = mag;
    
    // Barometer
    QJsonObject baro;
    baro["name"] = barometer.name;
    baro["vendor"] = barometer.vendor;
    obj["barometer"] = baro;
    
    // Light
    QJsonObject lightJson;
    lightJson["name"] = light.name;
    lightJson["vendor"] = light.vendor;
    obj["light"] = lightJson;
    
    // Proximity
    QJsonObject prox;
    prox["name"] = proximity.name;
    prox["vendor"] = proximity.vendor;
    obj["proximity"] = prox;
    
    // Fingerprint
    QJsonObject fp;
    fp["name"] = fingerprint.name;
    fp["vendor"] = fingerprint.vendor;
    fp["version"] = fingerprint.version;
    fp["type"] = fingerprint.type;
    fp["strength"] = fingerprint.strength;
    fp["sensorId"] = fingerprint.sensorId;
    obj["fingerprint"] = fp;
    
    // Face Recognition
    QJsonObject face;
    face["name"] = faceRecognition.name;
    face["vendor"] = faceRecognition.vendor;
    face["enrolled"] = faceRecognition.enrolled;
    face["faceCount"] = faceRecognition.faceCount;
    face["supportsIris"] = faceRecognition.supportsIris;
    obj["faceRecognition"] = face;
    
    return obj;
}

QJsonObject CompleteNetworkConfig::toJson() const {
    QJsonObject obj;
    obj["hostname"] = hostname;
    obj["dhcpHostname"] = dhcpHostname;
    obj["ipAddress"] = ipAddress;
    obj["wifiIpAddress"] = wifiIpAddress;
    obj["mobileIpAddress"] = mobileIpAddress;
    obj["subnetMask"] = subnetMask;
    obj["gateway"] = gateway;
    obj["dns1"] = dns1;
    obj["dns2"] = dns2;
    obj["wifiSSID"] = wifiSSID;
    obj["wifiBSSID"] = wifiBSSID;
    obj["wifiSignal"] = wifiSignal;
    obj["wifiLinkSpeed"] = wifiLinkSpeed;
    obj["wifiFrequency"] = wifiFrequency;
    obj["wifiDriver"] = wifiDriver;
    obj["wifiChipset"] = wifiChipset;
    obj["proxyHost"] = proxyHost;
    obj["proxyPort"] = proxyPort;
    obj["proxyExclusionList"] = proxyExclusionList;
    obj["vpnInterface"] = vpnInterface;
    obj["vpnAddress"] = vpnAddress;
    obj["vpnGateway"] = vpnGateway;
    obj["networkScore"] = networkScore;
    obj["connectivityScore"] = connectivityScore;
    return obj;
}

QJsonObject CompleteLocationConfig::toJson() const {
    QJsonObject obj;
    obj["gpsEnabled"] = gpsEnabled;
    obj["gpsStatus"] = gpsStatus;
    obj["latitude"] = QString::number(latitude);
    obj["longitude"] = QString::number(longitude);
    obj["altitude"] = QString::number(altitude);
    obj["accuracy"] = QString::number(accuracy);
    obj["speed"] = QString::number(speed);
    obj["bearing"] = QString::number(bearing);
    obj["gpsTime"] = QString::number(gpsTime);
    obj["gpsWeek"] = gpsWeek;
    obj["gpsTimeOfWeek"] = gpsTimeOfWeek;
    obj["nmeaSentence"] = nmeaSentence;
    obj["nmeaDate"] = nmeaDate;
    obj["gpsConstellation"] = gpsConstellation;
    obj["glonassConstellation"] = glonassConstellation;
    obj["beidouConstellation"] = beidouConstellation;
    obj["galileoConstellation"] = galileoConstellation;
    obj["qzssConstellation"] = qzssConstellation;
    obj["agpsServer"] = agpsServer;
    obj["suplServer"] = suplServer;
    obj["suplPort"] = suplPort;
    obj["geofencingEnabled"] = geofencingEnabled;
    obj["geofenceCount"] = geofenceCount;
    return obj;
}

QJsonObject CompleteTimingConfig::toJson() const {
    QJsonObject obj;
    obj["systemTime"] = QString::number(systemTime);
    obj["systemTimeNanos"] = QString::number(systemTimeNanos);
    obj["elapsedRealtime"] = QString::number(elapsedRealtime);
    obj["elapsedRealtimeNanos"] = QString::number(elapsedRealtimeNanos);
    obj["uptimeMillis"] = QString::number(uptimeMillis);
    obj["bootCount"] = QString::number(bootCount);
    obj["bootTime"] = QString::number(bootTime);
    obj["bootCompleted"] = bootCompleted;
    obj["bootAnimation"] = bootAnimation;
    obj["wallClockTime"] = QString::number(wallClockTime);
    obj["timeZoneOffset"] = timeZoneOffset;
    obj["timeZoneId"] = timeZoneId;
    obj["autoTime"] = autoTime;
    obj["autoTimeZone"] = autoTimeZone;
    obj["nitzTime"] = nitzTime;
    obj["nitzTimeZone"] = nitzTimeZone;
    obj["timeDrift"] = timeDrift;
    obj["clockDrift"] = clockDrift;
    return obj;
}

// ========================================================================
// Helper Methods
// ========================================================================

QString RealisticDeviceProfile::generateHex(int length) {
    QString hex;
    const QString chars = "0123456789ABCDEF";
    for (int i = 0; i < length; i++) {
        hex += chars[static_cast<int>(QRandomGenerator::global()->bounded(16))];
    }
    return hex;
}

QString RealisticDeviceProfile::generateNumeric(int length) {
    QString num;
    for (int i = 0; i < length; i++) {
        num += QString::number(static_cast<int>(QRandomGenerator::global()->bounded(10)));
    }
    return num;
}

double RealisticDeviceProfile::generateCoordinate(bool isLatitude) {
    if (isLatitude) {
        // Latitude: -90 to 90
        return (static_cast<int>(QRandomGenerator::global()->bounded(18000))) / 100.0 - 90.0;
    } else {
        // Longitude: -180 to 180
        return (static_cast<int>(QRandomGenerator::global()->bounded(36000))) / 100.0 - 180.0;
    }
}

int RealisticDeviceProfile::calculateLuhnDigit(const QString& base) {
    int sum = 0;
    bool alternate = true;
    for (int i = base.length() - 1; i >= 0; i--) {
        int n = base[i].digitValue();
        if (alternate) {
            n *= 2;
            if (n > 9) n -= 9;
        }
        sum += n;
        alternate = !alternate;
    }
    return (10 - (sum % 10)) % 10;
}

// ========================================================================
// Internal Generators
// ========================================================================

CompleteDeviceIdentity RealisticDeviceProfile::generateIdentity(const QString& manufacturer) {
    CompleteDeviceIdentity identity;
    
    // TAC Database
    QStringList samsungTACs = {"35875107", "35875108", "35746608", "35673009", "35469609"};
    QStringList googleTACs = {"35746608", "35746610", "35924909", "35863209"};
    QStringList xiaomiTACs = {"86917102", "86917103", "86831103"};
    QStringList oneplusTACs = {"86161306", "86161307"};
    
    QStringList allTACs;
    if (manufacturer.toLower() == "samsung") allTACs = samsungTACs;
    else if (manufacturer.toLower() == "google") allTACs = googleTACs;
    else if (manufacturer.toLower() == "xiaomi") allTACs = xiaomiTACs;
    else if (manufacturer.toLower() == "oneplus") allTACs = oneplusTACs;
    else allTACs = samsungTACs;
    
    // Generate IMEI
    QString tac = allTACs[static_cast<int>(QRandomGenerator::global()->bounded(allTACs.size()))];
    QString sn = generateNumeric(6);
    identity.imei = tac + sn + QString::number(calculateLuhnDigit(tac + sn));
    
    // Second IMEI for dual SIM
    QString sn2 = generateNumeric(6);
    identity.imei2 = tac + sn2 + QString::number(calculateLuhnDigit(tac + sn2));
    
    // MEID (14 hex digits)
    identity.meid = generateHex(14);
    
    // Serial Number
    if (manufacturer.toLower() == "samsung") {
        identity.serialNumber = "R" + generateNumeric(6) + "X" + generateNumeric(2);
    } else if (manufacturer.toLower() == "google") {
        identity.serialNumber = "AG" + generateNumeric(8);
    } else if (manufacturer.toLower() == "xiaomi") {
        identity.serialNumber = generateNumeric(10) + "AA";
    } else {
        identity.serialNumber = generateHex(12).toUpper();
    }
    
    // Android ID
    identity.androidId = generateHex(16).toUpper();
    
    // GSF ID
    identity.gsfId = generateNumeric(10);
    
    // Advertising ID
    identity.advertisingId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Fire Advertising ID
    identity.fireAdvertisingId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Device Key
    identity.deviceKey = generateHex(64).toUpper();
    
    // Device Credential
    identity.deviceCredential = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // Android VNC ID
    identity.androidVnci = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // MAC Addresses
    QStringList wifiOUIs;
    if (manufacturer.toLower() == "samsung") wifiOUIs = {"8C:71:F8", "D0:22:BE", "54:88:0E"};
    else if (manufacturer.toLower() == "google") wifiOUIs = {"3C:5A:B4", "54:60:09"};
    else if (manufacturer.toLower() == "xiaomi") wifiOUIs = {"34:80:B3", "F4:F5:D8"};
    else wifiOUIs = {"00:1A:11"};
    
    QString oui = wifiOUIs[static_cast<int>(QRandomGenerator::global()->bounded(wifiOUIs.size()))];
    identity.wlanMac = oui + ":" + generateHex(2) + ":" + generateHex(2) + ":" + generateHex(2);
    identity.bluetoothMac = "00:1A:7D:" + generateHex(2) + ":" + generateHex(2) + ":" + generateHex(2);
    identity.ethernetMac = "00:1B:44:" + generateHex(2) + ":" + generateHex(2) + ":" + generateHex(2);
    identity.nfcMac = "00:1C:2D:" + generateHex(2) + ":" + generateHex(2) + ":" + generateHex(2);
    
    // ICCID
    identity.iccid1 = "8961" + generateNumeric(16);
    identity.iccid2 = "8961" + generateNumeric(16);
    
    // IMSI
    QStringList mccList = {"310", "310", "310", "310", "404", "510", "424"};
    QStringList mncList = {"260", "410", "120", "030", "45", "03", "02"};
    int idx = static_cast<int>(QRandomGenerator::global()->bounded(mccList.size()));
    QString mcc = mccList[idx];
    QString mnc = mncList[idx];
    identity.imsi1 = mcc + mnc + generateNumeric(9);
    identity.imsi2 = mcc + mnc + generateNumeric(9);
    
    // Samsung-specific
    if (manufacturer.toLower() == "samsung") {
        identity.KnoxId = generateHex(32).toUpper();
        identity.KnoxVersion = "3.9";
        identity.spBleDeviceId = "BLE_" + generateHex(12).toUpper();
        identity.samsungAccountId = "samsung_" + generateHex(16).toLower();
    }
    
    // OEM Specific
    identity.oemSpecificId = generateHex(32).toUpper();
    identity.secureId = generateHex(48).toUpper();
    identity.vendorId = generateHex(16).toUpper();
    
    return identity;
}

CompleteCarrierConfig RealisticDeviceProfile::generateCarrier() {
    CompleteCarrierConfig carrier;
    
    QStringList carriers = {"T-Mobile", "AT&T", "Verizon", "Sprint", "US Cellular", "MetroPCS"};
    QStringList countries = {"US", "US", "US", "US", "US", "US"};
    QStringList mccs = {"310", "310", "311", "310", "312", "310"};
    QStringList mncs = {"260", "410", "480", "120", "030", "260"};
    
    int idx = static_cast<int>(QRandomGenerator::global()->bounded(carriers.size()));
    carrier.carrierName = carriers[idx];
    carrier.carrierCountry = countries[idx];
    carrier.mcc = mccs[idx];
    carrier.mnc = mncs[idx];
    carrier.carrierId = QString::number(static_cast<int>(QRandomGenerator::global()->bounded(1000)));
    carrier.carrierType = "postpaid";
    carrier.simOperatorName = carriers[idx];
    carrier.simCountry = countries[idx];
    carrier.simSerialNumber = "CN" + generateNumeric(12);
    carrier.networkType = "LTE";
    carrier.phoneType = "GSM";
    carrier.dataNetworkType = "LTE_CA";
    carrier.volteEnabled = true;
    carrier.vowifiEnabled = true;
    carrier.wifiCallingEnabled = true;
    
    // IMS
    carrier.imsPrivateUri = "sip:ims." + carrier.carrierName.toLower() + ".com";
    carrier.imsPublicUri = "sip:public.ims." + carrier.carrierName.toLower() + ".com";
    carrier.mmtelImsi = carrier.mcc + carrier.mnc + generateNumeric(9);
    
    // APN
    carrier.apnName = "carrier-data";
    carrier.apn = "internet";
    carrier.mmsc = "http://mms.vtext.com/servlets/mms";
    carrier.mmsProxy = "";
    carrier.mmsPort = 80;
    carrier.mccApn = carrier.mcc;
    carrier.mncApn = carrier.mnc;
    
    return carrier;
}

CompleteHardwareSpec RealisticDeviceProfile::generateHardware(const QString& manufacturer) {
    CompleteHardwareSpec hw;
    
    // CPU
    hw.processor = "ARMv8 Processor";
    hw.cpuAbi = "arm64-v8a";
    hw.cpuAbi2 = "armeabi-v7a";
    
    if (manufacturer.toLower() == "samsung") {
        hw.hardware = "qcom";
        hw.board = "dm3q";
        hw.systemOnChip = "SM8650";
        hw.coreCount = 8;
        hw.bigCoreCount = 1;
        hw.midCoreCount = 3;
        hw.littleCoreCount = 4;
        hw.gpuVendor = "Qualcomm";
        hw.gpuRenderer = "Adreno (TM) 750";
        hw.totalRam = 12288000000ULL;
    } else if (manufacturer.toLower() == "google") {
        hw.hardware = "google";
        hw.board = "husky";
        hw.systemOnChip = "Tensor G3";
        hw.coreCount = 8;
        hw.bigCoreCount = 1;
        hw.midCoreCount = 4;
        hw.littleCoreCount = 3;
        hw.gpuVendor = "ARM";
        hw.gpuRenderer = "Mali-G715";
        hw.totalRam = 12288000000ULL;
    } else if (manufacturer.toLower() == "xiaomi") {
        hw.hardware = "qcom";
        hw.board = "diting";
        hw.systemOnChip = "SM8650";
        hw.coreCount = 8;
        hw.bigCoreCount = 1;
        hw.midCoreCount = 5;
        hw.littleCoreCount = 2;
        hw.gpuVendor = "Qualcomm";
        hw.gpuRenderer = "Adreno (TM) 750";
        hw.totalRam = 16106127360ULL;
    } else {
        hw.hardware = "qcom";
        hw.board = "custom";
        hw.systemOnChip = "Snapdragon 8";
        hw.coreCount = 8;
        hw.totalRam = 8589934592ULL;
        hw.gpuVendor = "Qualcomm";
        hw.gpuRenderer = "Adreno (TM) 730";
    }
    
    hw.coreFreqMax = 3360;
    hw.coreFreqMin = 300;
    hw.cpuGovernor = 4; // SCHEDUTIL
    hw.bogoMips = hw.coreCount * 48.0;
    hw.maxCpuFreq = hw.coreFreqMax;
    hw.minCpuFreq = hw.coreFreqMin;
    
    // GPU Details
    hw.gpuVersion = "OpenGL ES 3.2 V@0533.0 (GIT@I09a65b6e02)";
    hw.gpuExtensions = "GL_ARM_shader_framebuffer_fetch GL_ARM_shader_framebuffer_fetch_depth_stencil GL_OES_depth24 GL_OES_depth_texture GL_OES_depth_texture_cube_map GL_OES_EGL_image GL_OES_EGL_image_external GL_OES_EGL_image_external_essl3 GL_OES_EGL_sync GL_OES_fbo_render_mipmap";
    hw.vulkanVersion = "1.1.279";
    hw.vulkanConformance = "1.3.2";
    hw.glEsVersion = "3.2";
    hw.glEsRenderer = hw.gpuRenderer;
    hw.glVendor = hw.gpuVendor;
    
    // Memory
    hw.totalRamMB = hw.totalRam / (1024 * 1024);
    hw.totalRamGB = hw.totalRam / (1024 * 1024 * 1024);
    hw.heapSize = 512 * 1024 * 1024;
    hw.largeHeapSize = hw.totalRam / 3;
    hw.lowMemory = 128 * 1024 * 1024;
    hw.memoryPageSize = 4096;
    
    // Storage
    hw.internalStorage = 256ULL * 1024 * 1024 * 1024;
    hw.externalStorage = 0;
    hw.sdCardId = "";
    
    // Battery
    hw.batteryCapacity = 5000;
    hw.batteryLevel = 75 + (static_cast<int>(QRandomGenerator::global()->bounded(20)));
    hw.batteryTemp = 320 + (static_cast<int>(QRandomGenerator::global()->bounded(30)));
    hw.batteryVoltage = 4000 + (static_cast<int>(QRandomGenerator::global()->bounded(500)));
    hw.batteryCurrent = -500 + (static_cast<int>(QRandomGenerator::global()->bounded(200)));
    hw.batteryStatus = "discharging";
    hw.batteryHealth = "good";
    hw.batteryTechnology = "Li-ion";
    hw.plugged = "none";
    
    // Display
    hw.displayWidth = 1440;
    hw.displayHeight = 3120;
    hw.densityDpi = 480;
    hw.density = 3.0;
    hw.refreshRate = 120;
    hw.hdrRefreshRate = 144;
    hw.hdrCapabilities = "HDR10, HDR10+, DolbyVision, HLG";
    hw.wideColorGamut = "Display P3, sRGB";
    hw.colorMode = "natural";
    
    // Audio
    hw.audioCodec = "qcom,msm-slimbus,WM5110,SndEvent";
    hw.audioOutput = "deep-buffer,primary";
    hw.audioEffect = "offload_effects_on";
    hw.speakerCount = 2;
    
    // Camera
    hw.cameraCount = 4;
    hw.frontCamera = "48MP, f/1.9, 26mm, PDAF";
    hw.backCamera = "200MP, f/1.7, 24mm, OIS, PDAF, Laser AF";
    hw.cameraFeatures = "HDR, panorama, 8K@30fps, 4K@60fps, OIS, EIS";
    
    return hw;
}

CompleteBuildInfo RealisticDeviceProfile::generateBuild(const QString& manufacturer, const QString& model, const QString& androidVersion) {
    CompleteBuildInfo build;
    
    build.brand = manufacturer.toLower();
    build.manufacturer = manufacturer;
    build.model = model.isEmpty() ? "SM-S928B" : model;
    build.buildType = "user";
    build.buildTags = "release-keys";
    
    if (manufacturer.toLower() == "samsung") {
        build.device = "dm3q";
        build.product = "dm3q";
        build.board = "dm3q";
        build.bootloader = "S928BXXU1AXXX";
        build.radioVersion = "SM8650_C1.0";
        build.baseband = "SM8650_C1.0";
        build.hardwareRevision = "REV0.2";
        build.deviceRevision = "2";
    } else if (manufacturer.toLower() == "google") {
        build.device = "husky";
        build.product = "husky";
        build.board = "husky";
        build.bootloader = "husky-4.6827970";
        build.radioVersion = "g5123b-123456.123456789";
        build.baseband = "g5123b-123456.123456789";
        build.hardwareRevision = "rev10";
        build.deviceRevision = "10";
    } else {
        build.device = manufacturer.toLower();
        build.product = build.device;
        build.board = build.device;
        build.bootloader = generateHex(8).toUpper();
        build.radioVersion = "1.0";
        build.baseband = "1.0";
        build.hardwareRevision = "REV1.0";
        build.deviceRevision = "1";
    }
    
    build.fingerprint = generateFingerprint(manufacturer, build.model, androidVersion);
    build.bootimageFingerprint = build.fingerprint;
    
    build.androidVersion = androidVersion;
    build.releaseVersion = "14"; // UpsideDownCake for 14, Revelation for 15
    build.sdkVersion = androidVersion == "14" ? "34" : "35";
    build.securityPatch = "2024-01-01";
    build.buildDescription = build.model + " " + androidVersion + " " + build.buildId + " release-keys";
    build.buildId = "UP1A.231005.007";
    build.buildDisplay = "UP1A.231005.007";
    build.buildDate = "Wed Dec 20 00:00:00 UTC 2023";
    build.buildDateTime = 1703030400;
    build.buildTimeSeconds = static_cast<int>(QDateTime::currentSecsSinceEpoch() % 86400);
    
    build.firstApiLevel = androidVersion == "14" ? 29 : 30;
    build.targetApiLevel = build.sdkVersion.toInt();
    build.otaUpdates = "";
    build.otaStatus = "";
    
    return build;
}

QString RealisticDeviceProfile::generateFingerprint(const QString& manufacturer, const QString& model, const QString& androidVersion) {
    QString brand = manufacturer.toLower();
    QString device = model.isEmpty() ? "dm3q" : model;
    QString buildId = "UP1A.231005.007";
    QString serial = "R" + generateNumeric(6) + "X" + generateNumeric(2);
    
    return brand + "/" + device + "/" + device + ":" + 
           androidVersion + "/" + buildId + "/" + serial + ":user/release-keys";
}

CompleteSecurityConfig RealisticDeviceProfile::generateSecurity(const QString& manufacturer) {
    CompleteSecurityConfig sec;
    
    // SELinux
    sec.selinuxStatus = "Enforcing";
    sec.selinuxEnforce = "Enforcing";
    sec.selinuxMode = "Enforcing";
    
    // Verified Boot
    sec.verifiedBootState = "green";
    sec.verifiedBootLocked = true;
    sec.verificationBoot = "true";
    sec.roBootloader = "locked";
    
    // VBMeta
    sec.vbmetaDigest = generateHex(64).toUpper();
    sec.vbmetaVersion = "1.0";
    sec.vbmetaAlgorithm = "SHA-256";
    
    // Keymaster
    sec.keymasterVersion = 4;
    sec.keymasterSecurityLevel = "SOFTWARE";
    
    // Gatekeeper
    sec.gatekeeperVersion = QString::number(4);
    sec.gatekeeperSecurityLevel = "SOFTWARE";
    
    // StrongBox
    sec.hasStrongbox = true;
    sec.strongboxVersion = 4;
    sec.strongboxSecurityLevel = "STRONGBOX";
    
    // Trusty
    sec.trustyVersion = "4.0";
    sec.hasTrusty = true;
    
    // Attestation
    sec.attestationEnabled = "true";
    sec.attestationStatus = "true";
    sec.hasHardwareAttestation = true;
    
    // DRM
    sec.widevineLevel = "L1";
    sec.drmSecurityLevel = "HW_SECURE_CODECS";
    sec.hdcpLevel = "HDCP2.3";
    
    // Encryption
    sec.fileBasedEncryption = "file_based_encryption_enabled";
    sec.fullDiskEncryption = "default_encryption";
    sec.isEncrypted = true;
    
    // Samsung
    if (manufacturer.toLower() == "samsung") {
        sec.isKnoxDevice = true;
        sec.knoxVersion = "3.9";
        sec.knoxId = generateHex(32).toUpper();
        sec.odeMode = "false";
        sec.secureStorageStatus = "enabled";
    }
    
    // Huawei
    if (manufacturer.toLower() == "huawei") {
        sec.huaweiDevice = "true";
        sec.huaweiSecPlatform = "HuaweiSecureBoot";
    }
    
    // Xiaomi
    if (manufacturer.toLower() == "xiaomi") {
        sec.isMiuiDevice = true;
        sec.miuiVersion = "14";
    }
    
    return sec;
}

CompleteSensorCalibration RealisticDeviceProfile::generateSensors() {
    CompleteSensorCalibration sensors;
    
    // Accelerometer
    sensors.accelerometer.name = "LSM6DSO Accelerometer";
    sensors.accelerometer.vendor = "STMicroelectronics";
    sensors.accelerometer.version = 1;
    sensors.accelerometer.resolution = 0.001196;
    sensors.accelerometer.maxRange = 78.4532;
    sensors.accelerometer.power = 0.26;
    sensors.accelerometer.minDelay = 10000;
    sensors.accelerometer.type = "1";
    sensors.accelerometer.typeString = "SENSOR_TYPE_ACCELEROMETER";
    
    // Gyroscope
    sensors.gyroscope.name = "LSM6DSO Gyroscope";
    sensors.gyroscope.vendor = "STMicroelectronics";
    sensors.gyroscope.version = 1;
    sensors.gyroscope.resolution = 0.001221;
    sensors.gyroscope.maxRange = 35.0;
    sensors.gyroscope.power = 0.5;
    
    // Magnetometer
    sensors.magnetometer.name = "AK09918 Magnetometer";
    sensors.magnetometer.vendor = "AKM";
    sensors.magnetometer.version = 1;
    sensors.magnetometer.resolution = 0.0015;
    sensors.magnetometer.maxRange = 4912.0;
    
    // Barometer
    sensors.barometer.name = "ICP-10111 Barometer";
    sensors.barometer.vendor = "InvenSense";
    sensors.barometer.version = 1;
    sensors.barometer.resolution = 0.00390625;
    sensors.barometer.maxRange = 1100.0;
    
    // Light
    sensors.light.name = "STK3338 Light Sensor";
    sensors.light.vendor = "Sensortek";
    sensors.light.version = 1;
    sensors.light.resolution = 0.01;
    sensors.light.maxRange = 43000.0;
    
    // Proximity
    sensors.proximity.name = "VCAP7538 Proximity Sensor";
    sensors.proximity.vendor = "VC";
    sensors.proximity.version = 1;
    
    // Heart Rate
    sensors.heartRate.name = "Samsung Heart Rate Sensor";
    sensors.heartRate.vendor = "Samsung";
    sensors.heartRate.version = 1;
    
    // Fingerprint
    sensors.fingerprint.name = "Qualcomm 3D Sonic Sensor";
    sensors.fingerprint.vendor = "Qualcomm";
    sensors.fingerprint.version = 1;
    sensors.fingerprint.type = "UNDER_DISPLAY_ULTRASONIC";
    sensors.fingerprint.strength = "STRONG";
    sensors.fingerprint.sensorId = static_cast<int>(QRandomGenerator::global()->bounded(256));
    
    // Face Recognition
    sensors.faceRecognition.name = "Samsung Face Recognition";
    sensors.faceRecognition.vendor = "Samsung";
    sensors.faceRecognition.enrolled = true;
    sensors.faceRecognition.faceCount = 1;
    sensors.faceRecognition.supportsIris = true;
    
    return sensors;
}

CompleteNetworkConfig RealisticDeviceProfile::generateNetwork() {
    CompleteNetworkConfig net;
    
    // Hostname
    net.hostname = "android-" + generateHex(6).toLower();
    net.dhcpHostname = net.hostname;
    
    // IP
    net.ipAddress = "192.168.1." + QString::number(100 + static_cast<int>(QRandomGenerator::global()->bounded(50)));
    net.wifiIpAddress = net.ipAddress;
    net.mobileIpAddress = "10.0.0." + QString::number(2 + static_cast<int>(QRandomGenerator::global()->bounded(10)));
    net.subnetMask = "255.255.255.0";
    net.gateway = "192.168.1.1";
    net.dns1 = "8.8.8.8";
    net.dns2 = "8.8.4.4";
    
    // WiFi
    net.wifiSSID = "Home_WiFi_" + generateNumeric(4);
    net.wifiBSSID = "AA:BB:CC:" + generateHex(2) + ":" + generateHex(2) + ":" + generateHex(2);
    net.wifiSignal = QString::number(-40 - static_cast<int>(QRandomGenerator::global()->bounded(30))) + " dBm";
    net.wifiLinkSpeed = 866;
    net.wifiFrequency = 5180 + (static_cast<int>(QRandomGenerator::global()->bounded(2))) * 20;
    net.wifiDriver = "android-driver";
    net.wifiChipset = "QC802BE";
    
    // Proxy
    net.proxyHost = "";
    net.proxyPort = 0;
    net.proxyExclusionList = "";
    
    // VPN
    net.vpnInterface = "";
    net.vpnAddress = "";
    net.vpnGateway = "";
    
    // Scores
    net.networkScore = 87;
    net.connectivityScore = 100;
    
    return net;
}

CompleteLocationConfig RealisticDeviceProfile::generateLocation() {
    CompleteLocationConfig loc;
    
    // GPS Enabled
    loc.gpsEnabled = true;
    loc.gpsStatus = "OK";
    
    // Coordinates (New York area as example)
    loc.latitude = 40.7128 + (static_cast<int>(QRandomGenerator::global()->bounded(1000))) / 10000.0;
    loc.longitude = -74.0060 + (static_cast<int>(QRandomGenerator::global()->bounded(1000))) / 10000.0;
    loc.altitude = 10.0 + (static_cast<int>(QRandomGenerator::global()->bounded(100))) / 10.0;
    loc.accuracy = 3.5 + (static_cast<int>(QRandomGenerator::global()->bounded(100))) / 100.0;
    loc.speed = static_cast<int>(QRandomGenerator::global()->bounded(5)) / 3.6;
    loc.bearing = static_cast<int>(QRandomGenerator::global()->bounded(360));
    
    // Time
    loc.gpsTime = QDateTime::currentMSecsSinceEpoch();
    loc.gpsWeek = 2340 + static_cast<int>(QRandomGenerator::global()->bounded(10));
    loc.gpsTimeOfWeek = (static_cast<int>(QRandomGenerator::global()->bounded(604800000)));
    
    // NMEA
    loc.nmeaSentence = "$GPGGA," + QString::number(loc.latitude, 'f', 4) + ",N," + 
                       QString::number(loc.longitude, 'f', 4) + ",W,1,08,0.9,545.4,M,46.9,M,,*47";
    loc.nmeaDate = QDate::currentDate().toString("ddMMyy");
    
    // Constellations
    loc.gpsConstellation = true;
    loc.glonassConstellation = true;
    loc.beidouConstellation = true;
    loc.galileoConstellation = true;
    loc.qzssConstellation = true;
    
    // AGPS
    loc.agpsServer = "nl.supl.google.com";
    loc.suplServer = "http://supl.google.com:7276";
    loc.suplPort = 7276;
    
    // Geofencing
    loc.geofencingEnabled = true;
    loc.geofenceCount = 0;
    
    return loc;
}

CompleteTimingConfig RealisticDeviceProfile::generateTiming() {
    CompleteTimingConfig timing;
    
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    
    // System Time
    timing.systemTime = now;
    timing.systemTimeNanos = now * 1000000;
    timing.elapsedRealtime = 86400000 + static_cast<int>(QRandomGenerator::global()->bounded(86400000)); // Random uptime
    timing.elapsedRealtimeNanos = timing.elapsedRealtime * 1000000;
    timing.uptimeMillis = timing.elapsedRealtime;
    
    // Boot Time
    timing.bootCount = 1 + static_cast<int>(QRandomGenerator::global()->bounded(5));
    timing.bootTime = now - timing.elapsedRealtime;
    timing.bootCompleted = true;
    timing.bootAnimation = false;
    
    // Wall Clock
    timing.wallClockTime = now;
    timing.timeZoneOffset = -18000000; // EST: -5 hours in ms
    timing.timeZoneId = "America/New_York";
    timing.autoTime = true;
    timing.autoTimeZone = true;
    
    // NITZ
    timing.nitzTime = "+00 000000 00";
    timing.nitzTimeZone = "GMT+00:00";
    
    // Drift
    timing.timeDrift = static_cast<int>(QRandomGenerator::global()->bounded(100)) - 50;
    timing.clockDrift = static_cast<int>(QRandomGenerator::global()->bounded(500)) - 250;
    
    return timing;
}

// ========================================================================
// Main Generator
// ========================================================================

QJsonObject RealisticDeviceProfile::generateCompleteProfile(
    const QString& manufacturer,
    const QString& model,
    const QString& androidVersion
) {
    QJsonObject profile;
    
    // Generate all components
    profile["identity"] = generateIdentity(manufacturer).toJson();
    profile["carrier"] = generateCarrier().toJson();
    profile["hardware"] = generateHardware(manufacturer).toJson();
    profile["build"] = generateBuild(manufacturer, model, androidVersion).toJson();
    profile["security"] = generateSecurity(manufacturer).toJson();
    profile["sensors"] = generateSensors().toJson();
    profile["network"] = generateNetwork().toJson();
    profile["location"] = generateLocation().toJson();
    profile["timing"] = generateTiming().toJson();
    
    // Metadata
    profile["profileVersion"] = "4.0.0";
    profile["generatedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    profile["generator"] = "RealisticDeviceProfile v4.0.0";
    
    return profile;
}

QJsonObject RealisticDeviceProfile::generateSamsungS24Ultra() {
    return generateCompleteProfile("Samsung", "SM-S928B", "14");
}

QJsonObject RealisticDeviceProfile::generateGooglePixel8Pro() {
    return generateCompleteProfile("Google", "Pixel 8 Pro", "14");
}

QJsonObject RealisticDeviceProfile::generateXiaomi14Ultra() {
    return generateCompleteProfile("Xiaomi", "Mi 14", "14");
}

QJsonObject RealisticDeviceProfile::generateOnePlus12() {
    return generateCompleteProfile("OnePlus", "CPH2573", "14");
}

QJsonObject RealisticDeviceProfile::generateHuaweiMate60() {
    return generateCompleteProfile("Huawei", "Mate 60 Pro", "14");
}

QJsonObject RealisticDeviceProfile::generateSamsungA54() {
    return generateCompleteProfile("Samsung", "SM-A546E", "14");
}

QJsonObject RealisticDeviceProfile::generateRandomRealisticDevice() {
    QStringList manufacturers = {"Samsung", "Google", "Xiaomi", "OnePlus", "Huawei", "OPPO", "Vivo"};
    QString manufacturer = manufacturers[static_cast<int>(QRandomGenerator::global()->bounded(manufacturers.size()))];
    return generateCompleteProfile(manufacturer, "", "14");
}

QJsonObject RealisticDeviceProfile::getCompleteDeviceJson(const QString& manufacturer) {
    if (manufacturer.toLower() == "samsung") {
        return generateSamsungS24Ultra();
    } else if (manufacturer.toLower() == "google") {
        return generateGooglePixel8Pro();
    } else if (manufacturer.toLower() == "xiaomi") {
        return generateXiaomi14Ultra();
    } else if (manufacturer.toLower() == "oneplus") {
        return generateOnePlus12();
    } else if (manufacturer.toLower() == "huawei") {
        return generateHuaweiMate60();
    }
    return generateRandomRealisticDevice();
}

bool RealisticDeviceProfile::isProfileComplete(const QJsonObject& profile) {
    QStringList required = {"identity", "carrier", "hardware", "build", 
                           "security", "sensors", "network", "location", "timing"};
    for (const QString& key : required) {
        if (!profile.contains(key)) return false;
    }
    return true;
}

QStringList RealisticDeviceProfile::getMissingFields(const QJsonObject& profile) {
    QStringList missing;
    QStringList required = {"identity", "carrier", "hardware", "build", 
                           "security", "sensors", "network", "location", "timing"};
    for (const QString& key : required) {
        if (!profile.contains(key)) missing.append(key);
    }
    return missing;
}

} // namespace VirtualPhonePro
