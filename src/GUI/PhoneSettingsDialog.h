/**
 * @file PhoneSettingsDialog.h
 * @brief Phone Settings Dialog with GPS, DNS, VPN, Network features
 * @version 2.0.0
 */

#ifndef VIRTUALPHONEPRO_PHONE_SETTINGS_DIALOG_H
#define VIRTUALPHONEPRO_PHONE_SETTINGS_DIALOG_H

#include <QDialog>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QMessageBox>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>

#include "VirtualPhonePro/ReDroidController.h"

namespace VirtualPhonePro {

/**
 * @brief Phone Settings Dialog - Advanced Configuration Panel
 * 
 * Contains:
 * - GPS/Location Spoofing
 * - DNS Configuration
 * - VPN Setup
 * - Network Isolation
 * - IPv6 Blocking
 * - Network Leak Test
 * - Device Uniqueness Verify
 * - Profile Save/Load
 * - Leak Prevention
 */
class PhoneSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit PhoneSettingsDialog(const QString& instanceId, QWidget* parent = nullptr);
    ~PhoneSettingsDialog();

private slots:
    // GPS Tab
    void onGPSLocationChanged();
    void onEnableMockLocation();
    void onDisableMockLocation();
    void onRandomizeGPS();
    void onPresetsGPS(const QString& preset);

    // DNS Tab
    void onApplyDNS();
    void onResetDNS();
    void onPresetDNS(const QString& preset);

    // VPN Tab
    void onSetupVPN();
    void onDisconnectVPN();
    void onImportVPNConfig();

    // Network Tab
    void onCreateNetwork();
    void onDeleteNetwork();
    void onTestLeaks();
    void onApplyLeakPrevention();
    void onToggleIPv6(bool enabled);

    // Device Info Tab
    void onVerifyUniqueness();
    void onRefreshDeviceInfo();

    // Profile Tab
    void onSaveProfile();
    void onLoadProfile();
    void onExportProfile();

	private:
    void setupUI();
    QWidget* createGPSPage();
    QWidget* createDNSPage();
    QWidget* createVPNPage();
    QWidget* createNetworkPage();
    QWidget* createDeviceInfoPage();
    QWidget* createProfilePage();
    
    void refreshDeviceInfo();
    QString executeAdbSync(const QStringList& args, int timeoutMs = 10000);
    void showMessage(const QString& title, const QString& message, bool isError = false);

private:
    QString m_instanceId;
    QTabWidget* m_tabWidget;
    
    // GPS Page
    QDoubleSpinBox* m_latitudeSpin;
    QDoubleSpinBox* m_longitudeSpin;
    QDoubleSpinBox* m_altitudeSpin;
    QDoubleSpinBox* m_accuracySpin;
    QPushButton* m_applyGPSBtn;
    QPushButton* m_enableMockBtn;
    QPushButton* m_disableMockBtn;
    QPushButton* m_randomizeGPSBtn;
    QComboBox* m_gpsPresetCombo;
    QLabel* m_gpsStatusLabel;
    
    // DNS Page
    QLineEdit* m_dns1Edit;
    QLineEdit* m_dns2Edit;
    QPushButton* m_applyDNSBtn;
    QPushButton* m_resetDNSBtn;
    QComboBox* m_dnsPresetCombo;
    QLabel* m_dnsStatusLabel;
    
    // VPN Page
    QLineEdit* m_vpnNameEdit;
    QLineEdit* m_vpnServerEdit;
    QSpinBox* m_vpnPortSpin;
    QLineEdit* m_vpnUsernameEdit;
    QLineEdit* m_vpnPasswordEdit;
    QComboBox* m_vpnTypeCombo;
    QPushButton* m_connectVPNBtn;
    QPushButton* m_disconnectVPNBtn;
    QPushButton* m_importVPNBtn;
    QLabel* m_vpnStatusLabel;
    
    // Network Page
    QPushButton* m_createNetworkBtn;
    QPushButton* m_deleteNetworkBtn;
    QPushButton* m_testLeaksBtn;
    QPushButton* m_applyLeakPreventionBtn;
    QCheckBox* m_ipv6BlockCheck;
    QLabel* m_networkStatusLabel;
    QTextEdit* m_leakTestResult;
    QProgressBar* m_leakTestProgress;
    
    // Device Info Page
    QLabel* m_deviceModelLabel;
    QLabel* m_androidVersionLabel;
    QLabel* m_imeiLabel;
    QLabel* m_androidIdLabel;
    QLabel* m_ipAddressLabel;
    QLabel* m_macAddressLabel;
    QLabel* m_uniquenessStatusLabel;
    QPushButton* m_verifyUniquenessBtn;
    QPushButton* m_refreshInfoBtn;
    QTextEdit* m_deviceDetailsText;
    
    // Profile Page
    QLineEdit* m_profileNameEdit;
    QPushButton* m_saveProfileBtn;
    QPushButton* m_loadProfileBtn;
    QPushButton* m_exportProfileBtn;
    QLabel* m_profileStatusLabel;
    QListWidget* m_savedProfilesList;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_PHONE_SETTINGS_DIALOG_H
