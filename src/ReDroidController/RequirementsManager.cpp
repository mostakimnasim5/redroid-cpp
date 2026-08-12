#include "VirtualPhonePro/RequirementsManager.hpp"
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QCoreApplication>

namespace VirtualPhonePro {

RequirementsManager::RequirementsManager(QObject* parent)
    : QObject(parent)
{
}

RequirementsManager::~RequirementsManager()
{
    if (m_process) {
        m_process->kill();
        m_process->waitForFinished(2000);
    }
}

// ---------------------------------------------------------------------------
// Detection
// ---------------------------------------------------------------------------

bool RequirementsManager::areRequirementsInstalled()
{
    // 1. WSL2 — check the feature is enabled via the registry key that
    //    "wsl --status" / "Get-WindowsOptionalFeature" consults.
    bool wslOk = false;
    {
        QSettings reg("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Lxss",
                       QSettings::NativeFormat);
        // Presence of any distro or the DefaultVersion==2 key is a strong signal.
        // DefaultVersion=2 means WSL2 is the default.
        QVariant v = reg.value("DefaultVersion");
        wslOk = (v.isValid() && v.toInt() == 2);
    }
    if (!wslOk) {
        // Fallback: look for wsl.exe itself (installed with the feature).
        wslOk = QFileInfo::exists("C:/Windows/System32/wsl.exe");
    }

    // 2. Docker Desktop — the installer registers an Uninstall entry.
    bool dockerOk = false;
    {
        QSettings reg64("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
                        QSettings::NativeFormat);
        QStringList keys = reg64.childGroups();
        for (const QString& k : keys) {
            reg64.beginGroup(k);
            QString name = reg64.value("DisplayName").toString();
            reg64.endGroup();
            if (name.contains("Docker Desktop", Qt::CaseInsensitive)) {
                dockerOk = true;
                break;
            }
        }
    }
    if (!dockerOk) {
        // Also accept the per-user install (HKCU) and the exe being present.
        QSettings regCU("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
                        QSettings::NativeFormat);
        QStringList keys = regCU.childGroups();
        for (const QString& k : keys) {
            regCU.beginGroup(k);
            QString name = regCU.value("DisplayName").toString();
            regCU.endGroup();
            if (name.contains("Docker Desktop", Qt::CaseInsensitive)) {
                dockerOk = true;
                break;
            }
        }
    }
    if (!dockerOk) {
        dockerOk = QFileInfo::exists("C:/Program Files/Docker/Docker/Docker Desktop.exe");
    }

    return wslOk && dockerOk;
}

// ---------------------------------------------------------------------------
// Install / Uninstall orchestration
// ---------------------------------------------------------------------------

void RequirementsManager::install()
{
    if (m_busy) return;

    // Build the step queue. We prefer winget (ships with Win10 1809+/Win11)
    // because it supports silent install and streams progress to stdout.
    // `wsl --install --no-distribution` enables WSL2 + the VM platform.
    m_steps.clear();
    m_steps.append({"wsl.exe", {"--install", "--no-distribution"}, 10,
                    "Enabling WSL2 + Virtual Machine Platform..."});
    m_steps.append({"winget", {"install", "-e", "--id", "Docker.DockerDesktop",
                               "--silent", "--accept-package-agreements",
                               "--accept-source-agreements"}, 60,
                    "Installing Docker Desktop..."});

    m_busy = true;
    m_lastExitOk = true;
    m_stepIndex = 0;
    m_percentPerStep = 90 / m_steps.size();  // reserve 10% for final "Done"
    emit progress(0);
    emit logMessage("Starting requirements installation...");

    runStep(m_steps[0].program, m_steps[0].args, m_steps[0].percent, m_steps[0].label);
}

void RequirementsManager::uninstall()
{
    if (m_busy) return;

    m_steps.clear();
    m_steps.append({"winget", {"uninstall", "-e", "--id", "Docker.DockerDesktop",
                               "--silent", "--accept-source-agreements"}, 40,
                    "Uninstalling Docker Desktop..."});
    m_steps.append({"wsl.exe", {"--uninstall"}, 70,
                    "Removing WSL2 components..."});

    m_busy = true;
    m_lastExitOk = true;
    m_stepIndex = 0;
    m_percentPerStep = 90 / m_steps.size();
    emit progress(0);
    emit logMessage("Starting requirements uninstall...");

    runStep(m_steps[0].program, m_steps[0].args, m_steps[0].percent, m_steps[0].label);
}

// ---------------------------------------------------------------------------
// Step runner
// ---------------------------------------------------------------------------

void RequirementsManager::runStep(const QString& program, const QStringList& args, int stepPercent, const QString& label)
{
    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &RequirementsManager::onReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &RequirementsManager::onProcessFinished);

    emit progress(stepPercent);
    emit logMessage(label);

    m_process->start(program, args);
    // Non-blocking: if start itself fails (e.g. winget missing), finished()
    // is still emitted by Qt with a non-zero/error code path; we also guard
    // below in case waitForStarted is needed for a fast failure.
    if (!m_process->waitForStarted(5000)) {
        emit logMessage("[ERROR] Failed to launch: " + program + " " + args.join(" "));
        onProcessFinished(-1, QProcess::CrashExit);
    }
}

void RequirementsManager::onReadyRead()
{
    if (!m_process) return;
    QByteArray out = m_process->readAllStandardOutput();
    for (const QByteArray& line : out.split('\n')) {
        QString s = QString::fromUtf8(line).trimmed();
        if (!s.isEmpty()) emit logMessage(s);
    }
}

void RequirementsManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    // WSL may exit non-zero if already installed — treat that as ok.
    const bool ok = (exitCode == 0) ||
                    (exitCode == 1 && m_stepIndex == 0 &&
                     m_steps.first().program == "wsl.exe");
    if (!ok) {
        m_lastExitOk = false;
        emit logMessage(QString("[ERROR] Step %1 failed (exit %2).")
                            .arg(m_stepIndex + 1).arg(exitCode));
    }

    ++m_stepIndex;
    if (m_stepIndex < m_steps.size() && m_lastExitOk) {
        const Step& next = m_steps[m_stepIndex];
        runStep(next.program, next.args, next.percent, next.label);
    } else {
        finishSequence(m_lastExitOk,
                       m_lastExitOk ? "Requirements installed successfully."
                                    : "Installation failed — see log above.");
    }
}

void RequirementsManager::finishSequence(bool success, const QString& summary)
{
    m_busy = false;
    emit progress(success ? 100 : (m_stepIndex > 0 ? (m_stepIndex * m_percentPerStep) : 0));
    emit logMessage(summary);
    emit finished(success, summary);
}

} // namespace VirtualPhonePro
