#ifndef VIRTUALPHONEPRO_REQUIREMENTS_MANAGER_HPP
#define VIRTUALPHONEPRO_REQUIREMENTS_MANAGER_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QProcess>

namespace VirtualPhonePro {

/**
 * @brief RequirementsManager — installs/uninstalls WSL2 + Docker Desktop.
 *
 * Drives winget / the Docker Desktop silent installer as child processes
 * and streams their stdout/stderr line-by-line via signals so the UI can
 * show a live log. Emits progress in coarse steps (0–100) and a final
 * finished(success) signal. All work happens in the same GUI thread via
 * QProcess signals — no blocking, no extra worker thread needed.
 */
class RequirementsManager : public QObject {
    Q_OBJECT
public:
    explicit RequirementsManager(QObject* parent = nullptr);
    ~RequirementsManager();

    // True if WSL2 + Docker appear to be installed on this machine.
    static bool areRequirementsInstalled();

    void install();
    void uninstall();

    bool isBusy() const { return m_busy; }

signals:
    void logMessage(const QString& line);
    void progress(int percent);     // 0..100
    void finished(bool success, const QString& summary);

private slots:
    void onReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void runStep(const QString& program, const QStringList& args, int stepPercent, const QString& label);
    void finishSequence(bool success, const QString& summary);

    QProcess* m_process = nullptr;
    bool m_busy = false;
    bool m_lastExitOk = true;

    // Step queue: each entry is (program, args, percent to emit when it starts).
    struct Step {
        QString program;
        QStringList args;
        int percent;
        QString label;
    };
    QList<Step> m_steps;
    int m_stepIndex = 0;
    int m_percentPerStep = 0;  // derived from step count
};

} // namespace VirtualPhonePro

#endif // VIRTUALPHONEPRO_REQUIREMENTS_MANAGER_HPP
