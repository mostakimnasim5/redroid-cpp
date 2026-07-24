/**
 * @file SystemAppSimulator.cpp
 * @brief System App & Carrier Bloatware Simulator Implementation
 */

#include "VirtualPhonePro/SystemAppSimulator.h"
#include "VirtualPhonePro/ReDroidController.h"

#include <QDebug>
#include <QRandomGenerator>
#include <QCoreApplication>

namespace VirtualPhonePro {

SystemAppSimulator* SystemAppSimulator::s_instance = nullptr;

SystemAppSimulator& SystemAppSimulator::instance() {
    if (!s_instance) {
        s_instance = new SystemAppSimulator();
    }
    return *s_instance;
}

SystemAppSimulator::SystemAppSimulator() {
    initializeCarrierConfigs();
}

// ============================================================================
// Configuration
// ============================================================================

bool SystemAppSimulator::configureForCarrier(const QString& instanceId, CarrierProvider carrier) {
    SystemAppState& state = m_states[instanceId];
    state.instanceId = instanceId;
    state.activeCarrier = carrier;
    
    CarrierBloatwareConfig config = getDefaultConfigForCarrier(carrier);
    state.carrierConfigs[carrier] = config;
    
    // Add pre-installed apps
    QStringList packages = getCarrierBloatwarePackages(carrier);
    for (const QString& pkg : packages) {
        PreinstalledApp app;
        app.packageName = pkg;
        app.appName = pkg.section('.', -1);
        app.version = generateAppVersion("carrier");
        app.versionCode = QString::number(QRandomGenerator::global()->bounded(1000, 9999));
        app.category = SystemAppCategory::CARRIER_BLOATWARE;
        app.isEnabled = true;
        app.isSystemApp = true;
        app.canBeDisabled = false;
        app.isUpdated = QRandomGenerator::global()->bounded(100) > 30;
        app.priority = QRandomGenerator::global()->bounded(1, 100);
        app.carrierBundle = carrierProviderToString(carrier);
        
        state.installedApps[pkg] = app;
    }
    
    state.totalBloatwareCount = packages.size();
    
    qDebug() << "Configured carrier bloatware for instance:" << instanceId 
             << "- Carrier:" << carrierProviderToString(carrier)
             << "- Apps:" << packages.size();
    
    return applyToInstance(instanceId);
}

bool SystemAppSimulator::configureForRegion(const QString& instanceId, CarrierRegion region, const QString& countryCode) {
    SystemAppState& state = m_states[instanceId];
    state.instanceId = instanceId;
    state.allAppsRealistic = true;
    
    // Add region-specific apps
    QStringList regionApps;
    
    switch (region) {
        case CarrierRegion::US:
            regionApps = {
                "com.att.digital.att",
                "com.att.myatt",
                "com.att.tv",
                "com.verizon.cloud",
                "com.verizon.messaging",
                "com.verizon.vzwot",
                "com.tmobile.tmom",
                "com.tmobile.prival",
                "com.sprint.cm",
                "com.sprint.ui"
            };
            break;
            
        case CarrierRegion::UK:
            regionApps = {
                "com.ee.myee",
                "com.o2.uk",
                "com.vodafone.myv",
                "com.three.uk.mythree"
            };
            break;
            
        case CarrierRegion::EUROPE:
            regionApps = {
                "com.dt.🇩🇪",
                "com.orange.orange",
                "com.bouygues.monbouygues"
            };
            break;
            
        case CarrierRegion::ASIA:
            regionApps = {
                "com.jio.jioapps",
                "com.jio.jiocharge",
                "com.airtel.airtel",
                "com.softbank.softbank",
                "com.ntt.docomo",
                "com.ntt.docomo.safety",
                "com.sktelecom.tworld",
                "com.kt.ktapp"
            };
            break;
            
        default:
            break;
    }
    
    for (const QString& pkg : regionApps) {
        PreinstalledApp app;
        app.packageName = pkg;
        app.appName = pkg.section('.', -1);
        app.version = generateAppVersion("region");
        app.versionCode = QString::number(QRandomGenerator::global()->bounded(1000, 9999));
        app.category = SystemAppCategory::CARRIER_BLOATWARE;
        app.isEnabled = true;
        app.isSystemApp = true;
        app.canBeDisabled = false;
        app.isUpdated = true;
        app.priority = QRandomGenerator::global()->bounded(1, 100);
        app.carrierBundle = carrierRegionToString(region);
        
        state.installedApps[pkg] = app;
    }
    
    state.totalBloatwareCount = regionApps.size();
    
    qDebug() << "Configured region bloatware for instance:" << instanceId 
             << "- Region:" << carrierRegionToString(region)
             << "- Apps:" << regionApps.size();
    
    return applyToInstance(instanceId);
}

bool SystemAppSimulator::addCustomCarrier(const QString& instanceId, const CarrierBloatwareConfig& config) {
    SystemAppState& state = m_states[instanceId];
    state.carrierConfigs[config.provider] = config;
    
    for (const QString& pkg : config.preinstalledApps) {
        PreinstalledApp app;
        app.packageName = pkg;
        app.appName = pkg.section('.', -1);
        app.version = generateAppVersion("custom");
        app.versionCode = QString::number(QRandomGenerator::global()->bounded(1000, 9999));
        app.category = SystemAppCategory::CARRIER_BLOATWARE;
        app.isEnabled = true;
        app.isSystemApp = true;
        app.canBeDisabled = false;
        app.isUpdated = true;
        app.priority = QRandomGenerator::global()->bounded(1, 100);
        app.carrierBundle = carrierProviderToString(config.provider);
        
        state.installedApps[pkg] = app;
    }
    
    return true;
}

bool SystemAppSimulator::applyToInstance(const QString& instanceId) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    ReDroidController& ctrl = ReDroidController::instance();
    SystemAppState& state = m_states[instanceId];
    
    QStringList commands;
    
    // Set carrier-specific properties
    if (state.carrierConfigs.contains(state.activeCarrier)) {
        const CarrierBloatwareConfig& config = state.carrierConfigs[state.activeCarrier];
        
        commands += {
            QString("setprop ro.carrier %1").arg(config.networkOperator),
            QString("setprop ro.carrier_name %1").arg(config.networkOperatorName),
            QString("setprop ro.setupwizard.carrier %1").arg(carrierProviderToString(config.provider).toLower()),
            
            // Wi-Fi Calling
            QString("setprop persist.wfc.enable %1").arg(config.isWiFiCallingEnabled ? "true" : "false"),
            QString("setprop ro.config.wifi_callingsvc %1").arg(config.isWiFiCallingEnabled ? "true" : "false"),
            
            // VoLTE
            QString("setprop persist.volte.enable %1").arg(config.isVoLTEEnabled ? "true" : "false"),
            QString("ro.config.hw_volte_activated %1").arg(config.isVoLTEEnabled ? "true" : "false"),
            
            // VoWiFi
            QString("setprop persist.wfc.wifi.enable %1").arg(config.isVoWiFiEnabled ? "true" : "false"),
        };
        
        // Add carrier service packages
        for (const QString& service : config.carrierServices) {
            commands += QString("pm install -i %1 %2").arg(carrierProviderToString(config.provider).toLower(), service);
        }
    }
    
    // Add system properties for pre-installed apps
    commands += {
        "setprop ro.setupwizard.mode OPTIONAL",
        "setprop ro.setupwizard.disable_uiautomator_mode false",
        "setprop ro.oem.carrier.reseller enable",
        "setprop ro.config.per_app_memcg false",
    };
    
    // Execute commands
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    qDebug() << "System app configuration applied to instance:" << instanceId;
    return true;
}

// ============================================================================
// Pre-installed Apps
// ============================================================================

QList<PreinstalledApp> SystemAppSimulator::getPreinstalledApps(const QString& instanceId) const {
    QList<PreinstalledApp> apps;
    
    if (m_states.contains(instanceId)) {
        const SystemAppState& state = m_states[instanceId];
        for (const auto& app : state.installedApps) {
            apps.append(app);
        }
    }
    
    return apps;
}

bool SystemAppSimulator::addPreinstalledApp(const QString& instanceId, const PreinstalledApp& app) {
    if (!m_states.contains(instanceId)) {
        m_states[instanceId] = SystemAppState();
        m_states[instanceId].instanceId = instanceId;
    }
    
    m_states[instanceId].installedApps[app.packageName] = app;
    m_states[instanceId].totalBloatwareCount++;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("pm install -i %1 %2").arg(app.carrierBundle, app.packageName));
    
    return true;
}

bool SystemAppSimulator::removePreinstalledApp(const QString& instanceId, const QString& packageName) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    if (m_states[instanceId].installedApps.contains(packageName)) {
        m_states[instanceId].installedApps.remove(packageName);
        m_states[instanceId].totalBloatwareCount--;
        
        ReDroidController& ctrl = ReDroidController::instance();
        ctrl.executeShell(instanceId, QString("pm uninstall -k --user 0 %1").arg(packageName));
        
        return true;
    }
    
    return false;
}

bool SystemAppSimulator::setAppEnabled(const QString& instanceId, const QString& packageName, bool enabled) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    if (m_states[instanceId].installedApps.contains(packageName)) {
        m_states[instanceId].installedApps[packageName].isEnabled = enabled;
        
        ReDroidController& ctrl = ReDroidController::instance();
        QString cmd = enabled 
            ? QString("pm enable %1").arg(packageName)
            : QString("pm disable-user --user 0 %1").arg(packageName);
        ctrl.executeShell(instanceId, cmd);
        
        return true;
    }
    
    return false;
}

bool SystemAppSimulator::simulateAppUpdate(const QString& instanceId, const QString& packageName, const QString& newVersion) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    if (m_states[instanceId].installedApps.contains(packageName)) {
        PreinstalledApp& app = m_states[instanceId].installedApps[packageName];
        app.version = newVersion;
        app.versionCode = QString::number(QRandomGenerator::global()->bounded(10000, 99999));
        app.isUpdated = true;
        app.installDate = QDateTime::currentDateTime();
        
        qDebug() << "Simulated update for" << packageName << "to version" << newVersion;
        return true;
    }
    
    return false;
}

// ============================================================================
// Carrier Bloatware
// ============================================================================

QStringList SystemAppSimulator::getCarrierBloatware(const QString& instanceId) const {
    QStringList packages;
    
    if (m_states.contains(instanceId)) {
        const SystemAppState& state = m_states[instanceId];
        for (const auto& app : state.installedApps) {
            if (app.category == SystemAppCategory::CARRIER_BLOATWARE) {
                packages.append(app.packageName);
            }
        }
    }
    
    return packages;
}

bool SystemAppSimulator::configureWiFiCalling(const QString& instanceId, bool enabled) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].carrierConfigs[m_states[instanceId].activeCarrier].isWiFiCallingEnabled = enabled;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop persist.wfc.enable %1").arg(enabled ? "true" : "false"));
    ctrl.executeShell(instanceId, QString("settings put global wifi_call_on %1").arg(enabled ? "1" : "0"));
    
    return true;
}

bool SystemAppSimulator::configureVoLTE(const QString& instanceId, bool enabled) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].carrierConfigs[m_states[instanceId].activeCarrier].isVoLTEEnabled = enabled;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop persist.volte.enable %1").arg(enabled ? "true" : "false"));
    ctrl.executeShell(instanceId, QString("settings put global volteo_toggle %1").arg(enabled ? "1" : "0"));
    
    return true;
}

bool SystemAppSimulator::configureVoWiFi(const QString& instanceId, bool enabled) {
    if (!m_states.contains(instanceId)) {
        return false;
    }
    
    m_states[instanceId].carrierConfigs[m_states[instanceId].activeCarrier].isVoWiFiEnabled = enabled;
    
    ReDroidController& ctrl = ReDroidController::instance();
    ctrl.executeShell(instanceId, QString("setprop persist.wfc.wifi.enable %1").arg(enabled ? "true" : "false"));
    
    return true;
}

// ============================================================================
// Background Processes
// ============================================================================

BackgroundProcessConfig SystemAppSimulator::getBackgroundProcessConfig(const QString& instanceId) const {
    BackgroundProcessConfig config;
    config.simulateCarrierSync = true;
    config.simulateSystemServices = true;
    config.simulateManufacturerServices = true;
    config.simulateOperatorServices = true;
    config.syncIntervalMinutes = 15;
    config.activeBackgroundProcesses = {
        "com.android.carrier:CarrierService",
        "com.android.carrier:ConfigService",
        "com.android.systemui:StatusBar",
        "com.android.phone:TelecomService"
    };
    
    return config;
}

bool SystemAppSimulator::configureBackgroundProcesses(const QString& instanceId, const BackgroundProcessConfig& config) {
    ReDroidController& ctrl = ReDroidController::instance();
    
    QStringList commands = {
        QString("setprop persist.sys.bgsync.interval %1").arg(config.syncIntervalMinutes),
    };
    
    if (config.simulateCarrierSync) {
        commands += "setprop persist.carrier.sync.enabled true";
    }
    
    if (config.simulateSystemServices) {
        commands += "setprop persist.sys.system.services enabled";
    }
    
    for (const QString& cmd : commands) {
        ctrl.executeShell(instanceId, cmd);
    }
    
    return true;
}

QStringList SystemAppSimulator::getActiveBackgroundProcesses(const QString& instanceId) const {
    BackgroundProcessConfig config = getBackgroundProcessConfig(instanceId);
    return config.activeBackgroundProcesses;
}

// ============================================================================
// App Updates
// ============================================================================

AppUpdateStatus SystemAppSimulator::getAppUpdateStatus(const QString& instanceId, const QString& packageName) const {
    AppUpdateStatus status;
    status.packageName = packageName;
    status.isAutoUpdateEnabled = true;
    status.isUpdateAvailable = QRandomGenerator::global()->bounded(100) < 20;
    
    if (m_states.contains(instanceId) && m_states[instanceId].installedApps.contains(packageName)) {
        const PreinstalledApp& app = m_states[instanceId].installedApps[packageName];
        status.currentVersion = app.version;
        status.latestVersion = app.isUpdated ? app.version : QString("1.%1.0").arg(app.versionCode);
        status.lastUpdateTime = app.installDate;
        status.updateSize = QRandomGenerator::global()->bounded(5, 50) * 1024 * 1024; // 5-50 MB
    }
    
    return status;
}

QList<AppUpdateStatus> SystemAppSimulator::checkForUpdates(const QString& instanceId) {
    QList<AppUpdateStatus> updates;
    
    if (!m_states.contains(instanceId)) {
        return updates;
    }
    
    const SystemAppState& state = m_states[instanceId];
    for (const auto& app : state.installedApps) {
        if (!app.isUpdated) {
            AppUpdateStatus status;
            status.packageName = app.packageName;
            status.currentVersion = app.version;
            status.latestVersion = generateAppVersion("update");
            status.isUpdateAvailable = true;
            status.isAutoUpdateEnabled = true;
            status.updateSize = QRandomGenerator::global()->bounded(5, 100) * 1024 * 1024;
            updates.append(status);
        }
    }
    
    m_states[instanceId].lastUpdateCheck = QDateTime::currentDateTime();
    
    return updates;
}

bool SystemAppSimulator::setAutoUpdate(const QString& instanceId, const QString& packageName, bool enabled) {
    ReDroidController& ctrl = ReDroidController::instance();
    QString cmd = QString("cmd package set-carrier-package-name %1 %2").arg(
        enabled ? "enabled" : "disabled", packageName);
    ctrl.executeShell(instanceId, cmd);
    
    return true;
}

// ============================================================================
// Utility
// ============================================================================

SystemAppState SystemAppSimulator::getSystemAppState(const QString& instanceId) const {
    if (m_states.contains(instanceId)) {
        return m_states[instanceId];
    }
    
    SystemAppState defaultState;
    defaultState.instanceId = instanceId;
    defaultState.totalBloatwareCount = 0;
    defaultState.disabledAppsCount = 0;
    defaultState.allAppsRealistic = true;
    
    return defaultState;
}

QStringList SystemAppSimulator::getOEMPackages(const QString& instanceId) const {
    QStringList packages;
    
    if (m_states.contains(instanceId)) {
        const SystemAppState& state = m_states[instanceId];
        for (const auto& app : state.installedApps) {
            if (app.category == SystemAppCategory::OEM_APP || 
                app.category == SystemAppCategory::MANUFACTURER_APP) {
                packages.append(app.packageName);
            }
        }
    }
    
    return packages;
}

QStringList SystemAppSimulator::getGooglePackages(const QString& instanceId) const {
    QStringList packages;
    
    if (m_states.contains(instanceId)) {
        const SystemAppState& state = m_states[instanceId];
        for (const auto& app : state.installedApps) {
            if (app.category == SystemAppCategory::GOOGLE_APP) {
                packages.append(app.packageName);
            }
        }
    }
    
    return packages;
}

bool SystemAppSimulator::generateRealisticAppList(const QString& instanceId, const QString& oemType) {
    SystemAppState& state = m_states[instanceId];
    state.instanceId = instanceId;
    state.allAppsRealistic = true;
    
    // OEM-specific apps
    QStringList oemApps;
    if (oemType.toLower() == "samsung") {
        oemApps = {
            "com.sec.android.app.launcher",
            "com.sec.android.app.myfiles",
            "com.sec.android.app.sbrowser",
            "com.sec.android.app.samsungapps",
            "com.sec.android.app.voiceeditor",
            "com.samsung.android.app.camera",
            "com.samsung.android.app.galaxy",
            "com.samsung.android.samsungpass",
            "com.samsung.android.service.peoplestrip",
            "com.samsung.android.themecenter",
            "com.samsung.android.app.notes",
            "com.sec.android.app.kidshome",
            "com.samsung.knox.securefolder",
            "com.samsung.android.knox.containeragent",
            "com.samsung.android.bixby.service"
        };
    } else if (oemType.toLower() == "google") {
        oemApps = {
            "com.google.android.apps.nexuslauncher",
            "com.google.android.apps.photos",
            "com.google.android.apps.wellbeing",
            "com.google.android.apps.nbu.files",
            "com.google.android.gms.location.history",
            "com.google.android.apps.subscriptions.redbull"
        };
    } else if (oemType.toLower() == "xiaomi") {
        oemApps = {
            "com.miui.home",
            "com.miui.securitycenter",
            "com.miui.cleanmaster",
            "com.xiaomi.midrop",
            "com.miui.weather2",
            "com.miui.notes",
            "com.miui.misettings",
            "com.xiaomi.glgm",
            "com.miui.yellowpage",
            "com.xiaomi.payment"
        };
    } else if (oemType.toLower() == "huawei") {
        oemApps = {
            "com.huawei.android.launcher",
            "com.huawei.systemmanager",
            "com.huawei.powermanager",
            "com.huawei.HwMultiScreenShot",
            "com.huawei.appmarket",
            "com.huawei.contacts",
            "com.huawei.parenting",
            "com.huawei.battery"
        };
    }
    
    // Add OEM apps
    for (const QString& pkg : oemApps) {
        PreinstalledApp app;
        app.packageName = pkg;
        app.appName = pkg.section('.', -1);
        app.version = generateAppVersion("oem");
        app.versionCode = QString::number(QRandomGenerator::global()->bounded(1000, 9999));
        app.category = SystemAppCategory::OEM_APP;
        app.isEnabled = true;
        app.isSystemApp = true;
        app.canBeDisabled = false;
        app.isUpdated = true;
        app.priority = QRandomGenerator::global()->bounded(1, 100);
        app.carrierBundle = oemType.toLower();
        
        state.installedApps[pkg] = app;
    }
    
    qDebug() << "Generated realistic app list for" << oemType << "-" << oemApps.size() << "apps";
    
    return applyToInstance(instanceId);
}

bool SystemAppSimulator::reset(const QString& instanceId) {
    if (m_states.contains(instanceId)) {
        m_states.remove(instanceId);
        return true;
    }
    return false;
}

// ============================================================================
// Private Helpers
// ============================================================================

void SystemAppSimulator::initializeCarrierConfigs() {
    // AT&T
    CarrierBloatwareConfig attConfig;
    attConfig.provider = CarrierProvider::ATT;
    attConfig.region = CarrierRegion::US;
    attConfig.countryCode = "US";
    attConfig.networkOperator = "310410";
    attConfig.networkOperatorName = "AT&T";
    attConfig.preinstalledApps = {"com.att.digital.att", "com.att.myatt", "com.att.tv"};
    attConfig.carrierServices = {"com.att.services.persistence"};
    attConfig.isRoamingEnabled = true;
    attConfig.isWiFiCallingEnabled = true;
    attConfig.isVoLTEEnabled = true;
    attConfig.isVoWiFiEnabled = true;
    attConfig.wifiCallingPackage = "com.att.wifiCalling";
    attConfig.voltePackage = "com.att.volte";
    m_carrierDefaults[CarrierProvider::ATT] = attConfig;
    
    // Verizon
    CarrierBloatwareConfig vzwConfig;
    vzwConfig.provider = CarrierProvider::VERIZON;
    vzwConfig.region = CarrierRegion::US;
    vzwConfig.countryCode = "US";
    vzwConfig.networkOperator = "311480";
    vzwConfig.networkOperatorName = "Verizon";
    vzwConfig.preinstalledApps = {"com.verizon.cloud", "com.verizon.messaging", "com.verizon.vzwot"};
    vzwConfig.carrierServices = {"com.vzw.vzwapnlib"};
    vzwConfig.isRoamingEnabled = true;
    vzwConfig.isWiFiCallingEnabled = true;
    vzwConfig.isVoLTEEnabled = true;
    vzwConfig.isVoWiFiEnabled = false;
    vzwConfig.wifiCallingPackage = "com.vzw.wifiCalling";
    vzwConfig.voltePackage = "com.vzw.volte";
    m_carrierDefaults[CarrierProvider::VERIZON] = vzwConfig;
    
    // T-Mobile
    CarrierBloatwareConfig tmoConfig;
    tmoConfig.provider = CarrierProvider::T_MOBILE;
    tmoConfig.region = CarrierRegion::US;
    tmoConfig.countryCode = "US";
    tmoConfig.networkOperator = "310260";
    tmoConfig.networkOperatorName = "T-Mobile";
    tmoConfig.preinstalledApps = {"com.tmobile.tmom", "com.tmobile.prival", "com.tmobile.tos"};
    tmoConfig.carrierServices = {"com.tmobile.qxdm.service"};
    tmoConfig.isRoamingEnabled = true;
    tmoConfig.isWiFiCallingEnabled = true;
    tmoConfig.isVoLTEEnabled = true;
    tmoConfig.isVoWiFiEnabled = true;
    tmoConfig.wifiCallingPackage = "com.tmobile.wifiCalling";
    tmoConfig.voltePackage = "com.tmobile.volte";
    m_carrierDefaults[CarrierProvider::T_MOBILE] = tmoConfig;
    
    // Jio
    CarrierBloatwareConfig jioConfig;
    jioConfig.provider = CarrierProvider::JIO;
    jioConfig.region = CarrierRegion::ASIA;
    jioConfig.countryCode = "IN";
    jioConfig.networkOperator = "40586";
    jioConfig.networkOperatorName = "Jio";
    jioConfig.preinstalledApps = {"com.jio.jioapps", "com.jio.sa.jiojoin", "com.jio.jiofinance.jio"};
    jioConfig.carrierServices = {"com.jio.service"};
    jioConfig.isRoamingEnabled = true;
    jioConfig.isWiFiCallingEnabled = false;
    jioConfig.isVoLTEEnabled = true;
    jioConfig.isVoWiFiEnabled = false;
    m_carrierDefaults[CarrierProvider::JIO] = jioConfig;
}

CarrierBloatwareConfig SystemAppSimulator::getDefaultConfigForCarrier(CarrierProvider carrier) {
    if (m_carrierDefaults.contains(carrier)) {
        return m_carrierDefaults[carrier];
    }
    
    CarrierBloatwareConfig defaultConfig;
    defaultConfig.provider = carrier;
    defaultConfig.region = CarrierRegion::OTHER;
    defaultConfig.isRoamingEnabled = false;
    defaultConfig.isWiFiCallingEnabled = false;
    defaultConfig.isVoLTEEnabled = false;
    defaultConfig.isVoWiFiEnabled = false;
    
    return defaultConfig;
}

QStringList SystemAppSimulator::getCarrierBloatwarePackages(CarrierProvider carrier) {
    QStringList packages;
    
    switch (carrier) {
        case CarrierProvider::ATT:
            packages = {"com.att.digital.att", "com.att.myatt", "com.att.tv", "com.att.watch", "com.attFamilyMap"};
            break;
        case CarrierProvider::VERIZON:
            packages = {"com.verizon.cloud", "com.verizon.messaging", "com.verizon.vzwot", "com.verizon.usage"};
            break;
        case CarrierProvider::T_MOBILE:
            packages = {"com.tmobile.tmom", "com.tmobile.prival", "com.tmobile.tos", "com.tmobile.skulist"};
            break;
        case CarrierProvider::SPRINT:
            packages = {"com.sprint.cm", "com.sprint.ui", "com.sprint.w.installer"};
            break;
        case CarrierProvider::JIO:
            packages = {"com.jio.jioapps", "com.jio.sa.jiojoin", "com.jiofinance.jio", "com.jiotracker.app"};
            break;
        case CarrierProvider::AIRTEL:
            packages = {"com.airtel.apps", "com.airtel.xp", "com.airtel.selfcare"};
            break;
        case CarrierProvider::SOFTBANK:
            packages = {"com.softbank.mobile", "com.softbank.service"};
            break;
        case CarrierProvider::NTT_DOCOMO:
            packages = {"com.ntt.docomo", "com.ntt.docomo.safety", "com.ntt.docomo.hikari"};
            break;
        case CarrierProvider::SK_TELECOM:
            packages = {"com.sktelecom.leg移行", "com.sktelecom.tworld"};
            break;
        default:
            break;
    }
    
    return packages;
}

QStringList SystemAppSimulator::getSystemProcessNames(CarrierProvider carrier) {
    QStringList processes;
    
    processes = {
        "com.android.carrier:CarrierService",
        "com.android.phone:TelecomService",
        "com.android.systemui:StatusBar",
        "system_server"
    };
    
    return processes;
}

QString SystemAppSimulator::carrierProviderToString(CarrierProvider provider) const {
    switch (provider) {
        case CarrierProvider::ATT: return "ATT";
        case CarrierProvider::VERIZON: return "Verizon";
        case CarrierProvider::T_MOBILE: return "T-Mobile";
        case CarrierProvider::SPRINT: return "Sprint";
        case CarrierProvider::US_CELLULAR: return "US Cellular";
        case CarrierProvider::EE: return "EE";
        case CarrierProvider::O2: return "O2";
        case CarrierProvider::VODAFONE: return "Vodafone";
        case CarrierProvider::ORANGE: return "Orange";
        case CarrierProvider::DEUTSCHE_TELEKOM: return "Deutsche Telekom";
        case CarrierProvider::THREE: return "Three";
        case CarrierProvider::JIO: return "Jio";
        case CarrierProvider::AIRTEL: return "Airtel";
        case CarrierProvider::SOFTBANK: return "SoftBank";
        case CarrierProvider::NTT_DOCOMO: return "NTT DOCOMO";
        case CarrierProvider::SK_TELECOM: return "SK Telecom";
        case CarrierProvider::KT: return "KT";
        case CarrierProvider::LG_UPLUS: return "LG Uplus";
        default: return "Custom";
    }
}

QString SystemAppSimulator::carrierRegionToString(CarrierRegion region) const {
    switch (region) {
        case CarrierRegion::US: return "US";
        case CarrierRegion::UK: return "UK";
        case CarrierRegion::EUROPE: return "Europe";
        case CarrierRegion::ASIA: return "Asia";
        case CarrierRegion::MIDDLE_EAST: return "Middle East";
        case CarrierRegion::AFRICA: return "Africa";
        case CarrierRegion::LATIN_AMERICA: return "Latin America";
        default: return "Other";
    }
}

QString SystemAppSimulator::generateAppVersion(const QString& appType) {
    int major = QRandomGenerator::global()->bounded(1, 10);
    int minor = QRandomGenerator::global()->bounded(0, 20);
    int patch = QRandomGenerator::global()->bounded(0, 50);
    
    return QString("%1.%2.%3").arg(major).arg(minor).arg(patch);
}

} // namespace VirtualPhonePro
