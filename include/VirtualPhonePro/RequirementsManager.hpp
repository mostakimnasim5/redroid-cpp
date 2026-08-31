#ifndef VIRTUALPHONEPRO_REQUIREMENTS_MANAGER_HPP
#define VIRTUALPHONEPRO_REQUIREMENTS_MANAGER_HPP

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QProcess>

namespace VirtualPhonePro {

/**
 * @brief RequirementsManager — self-contained one-click environment setup.
 *
 * The single "Install" button provisions EVERYTHING without Docker Desktop:
 *   1. wsl --install --no-distribution   (WSL2 + VM platform)
 *   2. Download the binder-enabled kernel bzImage published by
 *      github.com/mostakimnasim5/WSL2-Linux-Kernel-Rolling and merge
 *      kernel=/nestedVirtualization= into %USERPROFILE%\.wslconfig
 *   3. wsl --shutdown                    (reload the custom kernel)
 *   4. Import the 'redroid-engine' distro (Ubuntu rootfs) and install
 *      docker-ce (Docker Engine + containerd + CLI) inside it via the
 *      official get.docker.com script; systemd is enabled so dockerd
 *      starts automatically.
 *   5. Verify binder + `docker info` inside the distro.
 *
 * Windows-side work is done with embedded PowerShell scripts written to
 * %TEMP% at runtime, so the app binary stays fully self-contained.
 * stdout/stderr stream line-by-line via signals so the UI shows a live log.
 * All work happens in the GUI thread via QProcess signals — no blocking.
 */
class RequirementsManager : public QObject {
    Q_OBJECT
public:
    explicit RequirementsManager(QObject* parent = nullptr);
    ~RequirementsManager();

    // True if WSL2 is present, the 'redroid-engine' distro exists, and its
    // in-WSL Docker Engine answers `docker info`. No Docker Desktop check.
    static bool areRequirementsInstalled();

    // Name of the dedicated WSL distro that hosts the Docker Engine.
    static QString engineDistroName() { return QStringLiteral("redroid-engine"); }

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

    // Writes an embedded PowerShell script to %TEMP% so it can be run with
    // `powershell -File`. Returns the path, or empty string on failure.
    static QString writeTempScript(const QString& name, const QString& content);

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
