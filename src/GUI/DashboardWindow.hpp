
#ifndef VIRTUALPHONEPRO_DASHBOARD_WINDOW_H
#define VIRTUALPHONEPRO_DASHBOARD_WINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QTimer>
#include <QMenuBar>
#include <QMenu>

#include <QProgressBar>

#include "VirtualPhonePro/ReDroidController.hpp"
#include "VirtualPhonePro/DeviceProfile.hpp"
#include "VirtualPhonePro/DeviceProfileGenerator.hpp"
#include "VirtualPhonePro/MultiInstanceManager.hpp"
#include "VirtualPhonePro/BankingAppSpoofer.hpp"
#include "VirtualPhonePro/SafetyNetSpoofer.hpp"
#include "VirtualPhonePro/RequirementsManager.hpp"
#include "PhoneWindow.hpp"

namespace VirtualPhonePro {

/**
 * @brief Dialog for creating a new phone instance
 */
class NewPhoneDialog : public QDialog {
    Q_OBJECT

public:
    explicit NewPhoneDialog(QWidget* parent = nullptr);
    ~NewPhoneDialog();
    
    QString getInstanceId() const { return m_instanceId; }
    bool isProtected() const { return m_isProtected; }
    DeviceProfile getProfile() const { return m_profile; }
    uint32_t getProfileIndex() const { return m_profileIndex; }
    QString getManufacturer() const { return m_manufacturer; }
    QString getAndroidVersion() const { return m_androidVersion; }
    
    // Proxy configuration
    bool useProxy() const { return m_useProxy; }
    int getProxyMode() const { return m_proxyModeCombo->itemData(m_proxyModeCombo->currentIndex()).toInt(); }
    QString getProxyHost() const { return m_proxyHost; }
    int getProxyPort() const { return m_proxyPort; }
    QString getProxyUsername() const { return m_proxyUsername; }
    QString getProxyPassword() const { return m_proxyPassword; }

private slots:
    void onManufacturerChanged(const QString& manufacturer);
    void onOk();
    void onRandomizeProfile();
    void onProxyModeChanged(int index);

private:
    void setupUI();
    void loadManufacturerModels();
    bool generateDeterministicIdentity();

    QString m_instanceId;
    bool m_isProtected = false;
    DeviceProfile m_profile;
    QString m_manufacturer;
    QString m_androidVersion;

    // Hardware-anchored deterministic identity (see fix/hwid-foundation)
    DeviceIdentityProfile m_identity;
    uint32_t m_profileIndex = 0;
    
    QComboBox* m_manufacturerCombo;
    QComboBox* m_modelCombo;
    QComboBox* m_androidVersionCombo;
    QLineEdit* m_instanceNameEdit;
    QSpinBox* m_memorySpin;
    
    QLineEdit* m_imeiEdit;
    QLineEdit* m_serialEdit;
    QLineEdit* m_androidIdEdit;
    
    // Proxy configuration
    QComboBox* m_proxyModeCombo;
    QLineEdit* m_proxyHostEdit;
    QSpinBox* m_proxyPortSpin;
    QLineEdit* m_proxyUsernameEdit;
    QLineEdit* m_proxyPasswordEdit;
    QWidget* m_proxyDetailsWidget;
    
    bool m_useProxy;
    QString m_proxyHost;
    int m_proxyPort;
    QString m_proxyUsername;
    QString m_proxyPassword;
};

/**
 * @brief Phone Instance Card Widget
 * Displays a single phone instance with controls
 */
class PhoneCard : public QFrame {
    Q_OBJECT

public:
    explicit PhoneCard(const QString& instanceId, QWidget* parent = nullptr);
    ~PhoneCard();
    
    void setupUI();
    void setInstanceInfo(const InstanceInfo& info);
    void setProfile(const DeviceProfile& profile);
    void setScreenshot(const QPixmap& pixmap);
    void updateStatus();
    void setProtectionStatus(bool isProtected);
    
    QString getInstanceId() const { return m_instanceId; }
    bool isProtected() const { return m_isProtected; }

signals:
    void openRequested(const QString& instanceId);
    void startRequested(const QString& instanceId);
    void stopRequested(const QString& instanceId);
    void deleteRequested(const QString& instanceId);
    void cloneRequested(const QString& instanceId);

private slots:
    void onOpenClicked();
    void onStartClicked();
    void onStopClicked();
    void onDeleteClicked();
    void onCloneClicked();

private:
    void updateUI();
    void updateProtectionIcon();

    QString m_instanceId;
    InstanceInfo m_info;
    bool m_isProtected;
    DeviceProfile m_profile;

    QLabel* m_nameLabel;
    QLabel* m_statusLabel;
    QLabel* m_portLabel;
    QLabel* m_screenshotLabel;
    QLabel* m_modelLabel;
    QLabel* m_shieldLabel;
    QPushButton* m_openButton;
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QPushButton* m_deleteButton;
    QPushButton* m_cloneButton;
};

/**
 * @brief Dashboard Window - Main multi-instance management
 */
class DashboardWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit DashboardWindow(QWidget* parent = nullptr);
    ~DashboardWindow();
    
    void refreshInstances();

private slots:
    void onNewPhoneClicked();
    void onBatchLaunchClicked();
    void onPhoneCardOpen(const QString& instanceId);
    void onPhoneCardStart(const QString& instanceId);
    void onPhoneCardStop(const QString& instanceId);
    void onPhoneCardDelete(const QString& instanceId);
    void onPhoneCardClone(const QString& instanceId);
    void onRefreshClicked();
    void onRefreshScreenshots();
    void updateInstanceCard(const QString& instanceId, InstanceState state);
    void onAdbConnectionChanged(const QString& instanceId, bool connected);
    void setupConnections();

    // Requirements (WSL2 + Docker Desktop) install/uninstall
    void onInstallRequirementsClicked();
    void onUninstallRequirementsClicked();
    void onRequirementsLog(const QString& line);
    void onRequirementsProgress(int percent);
    void onRequirementsFinished(bool success, const QString& summary);
    void refreshRequirementsState();

private:
    void setupUI();
    void setupMenuBar();
    void createPhoneCard(const QString& instanceId);
    void removePhoneCard(const QString& instanceId);
    void openPhoneWindow(const QString& instanceId);
    
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_scrollContent;
    QGridLayout* m_cardGrid;
    
    QPushButton* m_newPhoneButton;
    QPushButton* m_refreshButton;
    QPushButton* m_installRequirementsButton;
    QPushButton* m_batchLaunchButton;
    QPushButton* m_uninstallRequirementsButton;
    QProgressBar* m_requirementsProgressBar;
    RequirementsManager* m_requirementsManager;
    QLabel* m_statusLabel;
    
    QMap<QString, PhoneCard*> m_phoneCards;
    QMap<QString, PhoneWindow*> m_phoneWindows;
    
    QTimer* m_screenshotTimer;
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_DASHBOARD_WINDOW_H
