/**
 * @file admin_control_panel.h
 * @brief Admin Control Panel Header
 * @version 3.0.0
 * 
 * User-friendly admin panel for managing device profiles and users.
 */

#ifndef ADMIN_CONTROL_PANEL_H
#define ADMIN_CONTROL_PANEL_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QJsonObject>

/**
 * @brief Admin Control Panel Dialog
 * 
 * This dialog provides a user-friendly interface for:
 * - Entering user information (name, phone, email)
 * - Configuring number of profiles
 * - Setting duration
 * - Managing auto-renewal settings
 */
class AdminControlPanel : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Construct Admin Control Panel
     * @param parent Parent widget
     */
    explicit AdminControlPanel(QWidget* parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~AdminControlPanel();
    
    /**
     * @brief Get current configuration as JSON object
     * @return Configuration object
     */
    QJsonObject getConfiguration() const;

signals:
    /**
     * @brief Emitted when configuration is submitted successfully
     * @param config Submitted configuration
     */
    void configurationSubmitted(const QJsonObject& config);

private slots:
    /**
     * @brief Handle form validation
     */
    void validateForm();
    
    /**
     * @brief Handle clear button click
     */
    void onClearForm();
    
    /**
     * @brief Handle submit button click
     */
    void onSubmit();

private:
    /**
     * @brief Setup UI components
     */
    void setupUI();
    
    /**
     * @brief Load previous configuration if exists
     */
    void loadPreviousData();

    // User Information
    QLineEdit* m_userNameEdit;
    QLineEdit* m_phoneEdit;
    QLineEdit* m_emailEdit;
    
    // Profile Configuration
    QSpinBox* m_profileCountSpin;
    QComboBox* m_profileTypeCombo;
    QSpinBox* m_durationSpin;
    QComboBox* m_autoRenewCheck;
    
    // Notes
    QLineEdit* m_notesEdit;
    
    // Buttons
    QPushButton* m_submitButton;
    
    // Status
    QLabel* m_statusLabel;
    
    // State
    bool m_isSubmitting;
};

#endif // ADMIN_CONTROL_PANEL_H
