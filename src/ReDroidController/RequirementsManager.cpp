#include "VirtualPhonePro/RequirementsManager.hpp"
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QCoreApplication>

namespace VirtualPhonePro {

// ---------------------------------------------------------------------------
// Embedded PowerShell provisioning scripts
//
// Written to %TEMP% at runtime and executed with
// `powershell -NoProfile -ExecutionPolicy Bypass -File <path>` so the app
// binary stays fully self-contained (no external .ps1 files to ship).
// ---------------------------------------------------------------------------

// Step A — download the binder-enabled kernel bzImage published by the
// mostakimnasim5/WSL2-Linux-Kernel-Rolling fork, then MERGE (not overwrite)
// kernel=/nestedVirtualization= into the [wsl2] section of .wslconfig.
static const char kKernelSetupScript[] = R"PS(
$ErrorActionPreference = 'Stop'
$kernelDir  = Join-Path $env:USERPROFILE 'wsl-kernel'
$kernelPath = Join-Path $kernelDir 'bzImage'
$kernelUrl  = 'https://github.com/mostakimnasim5/WSL2-Linux-Kernel-Rolling/releases/latest/download/bzImage'

New-Item -ItemType Directory -Force -Path $kernelDir | Out-Null

if (Test-Path $kernelPath) {
    Write-Host "Kernel already present at $kernelPath - skipping download."
} else {
    Write-Host "Downloading binder-enabled WSL2 kernel:"
    Write-Host "  $kernelUrl"
    try {
        Invoke-WebRequest -Uri $kernelUrl -OutFile $kernelPath -UseBasicParsing
    } catch {
        Write-Host "[ERROR] Kernel download failed: $($_.Exception.Message)"
        Write-Host "[ERROR] The mostakimnasim5/WSL2-Linux-Kernel-Rolling fork must publish a"
        Write-Host "[ERROR] GitHub release with a 'bzImage' asset for one-click install to work."
        Write-Host "[ERROR] Until then, build the kernel per docs/WSL2_KERNEL_SETUP.md and"
        Write-Host "[ERROR] place it at: $kernelPath  - then click Install again."
        exit 1
    }
}

# --- Merge .wslconfig: set kernel + nestedVirtualization inside [wsl2],
#     preserving every other section and key the user already has. ---
$wslConfigPath = Join-Path $env:USERPROFILE '.wslconfig'
$kernelValue = ($kernelPath -replace '\\', '/')
$lines = @()
if (Test-Path $wslConfigPath) { $lines = Get-Content $wslConfigPath }

$out = New-Object System.Collections.Generic.List[string]
$inWsl2 = $false
$seenWsl2 = $false
$wroteKernel = $false
$wroteNested = $false
foreach ($line in $lines) {
    $trim = $line.Trim()
    if ($trim -match '^\[') {
        if ($inWsl2) {
            if (-not $wroteKernel) { $out.Add("kernel=$kernelValue") | Out-Null; $wroteKernel = $true }
            if (-not $wroteNested) { $out.Add('nestedVirtualization=true') | Out-Null; $wroteNested = $true }
        }
        $inWsl2 = ($trim -eq '[wsl2]')
        if ($inWsl2) { $seenWsl2 = $true }
        $out.Add($line) | Out-Null
        continue
    }
    if ($inWsl2 -and $trim -match '^kernel\s*=') {
        if (-not $wroteKernel) { $out.Add("kernel=$kernelValue") | Out-Null; $wroteKernel = $true }
        continue
    }
    if ($inWsl2 -and $trim -match '^nestedVirtualization\s*=') {
        if (-not $wroteNested) { $out.Add('nestedVirtualization=true') | Out-Null; $wroteNested = $true }
        continue
    }
    $out.Add($line) | Out-Null
}
if ($inWsl2) {
    if (-not $wroteKernel) { $out.Add("kernel=$kernelValue") | Out-Null }
    if (-not $wroteNested) { $out.Add('nestedVirtualization=true') | Out-Null }
}
if (-not $seenWsl2) {
    $out.Add('[wsl2]') | Out-Null
    $out.Add("kernel=$kernelValue") | Out-Null
    $out.Add('nestedVirtualization=true') | Out-Null
}
$out | Set-Content -Path $wslConfigPath -Encoding ASCII
Write-Host "Updated $wslConfigPath (kernel + nestedVirtualization)."
)PS";

// Step B — import the dedicated 'redroid-engine' distro from a minimal
// Ubuntu rootfs and install docker-ce (Docker Engine + containerd + CLI)
// inside it, headlessly, via the official get.docker.com script. systemd is
// enabled in /etc/wsl.conf so dockerd starts automatically with the distro.
static const char kEngineSetupScript[] = R"PS(
$ErrorActionPreference = 'Stop'
$distro     = 'redroid-engine'
$installDir = Join-Path $env:LOCALAPPDATA 'redroid-engine'
$rootfsUrl  = 'https://cloud-images.ubuntu.com/wsl/jammy/current/ubuntu-jammy-wsl-amd64-ubuntu22.04lts.rootfs.tar.gz'
$rootfsFile = Join-Path $env:TEMP 'redroid-engine-rootfs.tar.gz'

# wsl.exe writes UTF-16LE; without this PowerShell 5.1 mangles the output.
[Console]::OutputEncoding = [System.Text.Encoding]::Unicode

$existing = (wsl.exe -l -q) -join "`n"
if ($existing -match [regex]::Escape($distro)) {
    Write-Host "Distro '$distro' already imported - skipping import."
} else {
    if (-not (Test-Path $rootfsFile)) {
        Write-Host "Downloading minimal Ubuntu rootfs (~250 MB)..."
        Invoke-WebRequest -Uri $rootfsUrl -OutFile $rootfsFile -UseBasicParsing
    }
    New-Item -ItemType Directory -Force -Path $installDir | Out-Null
    Write-Host "Importing '$distro' distro..."
    wsl.exe --import $distro $installDir $rootfsFile
    if ($LASTEXITCODE -ne 0) { throw "wsl --import failed (exit $LASTEXITCODE)" }
}

Write-Host "Enabling systemd inside '$distro'..."
wsl.exe -d $distro -u root -- bash -c "printf '[boot]\nsystemd=true\n' > /etc/wsl.conf"
if ($LASTEXITCODE -ne 0) { throw "failed to write /etc/wsl.conf (exit $LASTEXITCODE)" }

Write-Host "Installing Docker Engine (docker-ce) inside '$distro' - this can take several minutes..."
$dockerInstall = @'
export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y curl ca-certificates
curl -fsSL https://get.docker.com | sh
'@
wsl.exe -d $distro -u root -- bash -c $dockerInstall
if ($LASTEXITCODE -ne 0) { throw "docker-ce installation failed (exit $LASTEXITCODE)" }

Write-Host "Restarting WSL so systemd boots, then enabling dockerd..."
wsl.exe --shutdown
Start-Sleep -Seconds 5
wsl.exe -d $distro -u root -- systemctl enable --now docker
if ($LASTEXITCODE -ne 0) { throw "failed to enable the docker service (exit $LASTEXITCODE)" }

Write-Host "'$distro' is ready: in-WSL Docker Engine installed and running."
)PS";

// Uninstall cleanup — remove the kernel file and the two .wslconfig keys we
// added, leaving every other user setting untouched.
static const char kUninstallCleanupScript[] = R"PS(
$ErrorActionPreference = 'Continue'
$kernelDir = Join-Path $env:USERPROFILE 'wsl-kernel'
if (Test-Path $kernelDir) {
    Remove-Item -Recurse -Force $kernelDir
    Write-Host "Removed $kernelDir"
}
$wslConfigPath = Join-Path $env:USERPROFILE '.wslconfig'
if (Test-Path $wslConfigPath) {
    $lines = Get-Content $wslConfigPath
    $out = $lines | Where-Object { $_.Trim() -notmatch '^(kernel|nestedVirtualization)\s*=' }
    $out | Set-Content -Path $wslConfigPath -Encoding ASCII
    Write-Host "Removed kernel/nestedVirtualization keys from $wslConfigPath"
}
)PS";

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
#ifdef Q_OS_WIN
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
    if (!wslOk) return false;

    // 2. The dedicated 'redroid-engine' distro must exist.
    //    wsl.exe emits UTF-16LE; decode accordingly.
    {
        QProcess p;
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.start("wsl.exe", {"-l", "-q"});
        if (!p.waitForFinished(8000) || p.exitCode() != 0) return false;
        QByteArray raw = p.readAll();
        raw.truncate(raw.size() & ~1);  // fromUtf16 needs an even byte count
        const QString distros =
            QString::fromUtf16(reinterpret_cast<const char16_t*>(raw.constData()),
                               raw.size() / 2);
        if (!distros.contains(engineDistroName())) return false;
    }

    // 3. Its in-WSL Docker Engine must answer. (No Docker Desktop lookup.)
    {
        QProcess p;
        p.setProcessChannelMode(QProcess::MergedChannels);
        p.start("wsl.exe", {"-d", engineDistroName(), "--", "docker", "info",
                            "--format", "{{.ServerVersion}}"});
        return p.waitForFinished(10000) && p.exitCode() == 0;
    }
#else
    // Linux host: docker runs natively, no distro bridge.
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start("docker", {"info", "--format", "{{.ServerVersion}}"});
    return p.waitForFinished(10000) && p.exitCode() == 0;
#endif
}

// ---------------------------------------------------------------------------
// Install / Uninstall orchestration
// ---------------------------------------------------------------------------

void RequirementsManager::install()
{
    if (m_busy) return;

    // Self-contained chain: WSL2 + custom binder kernel + a dedicated
    // 'redroid-engine' WSL distro running docker-ce. No Docker Desktop.
    m_steps.clear();
#ifdef Q_OS_WIN
    const QString kernelScript = writeTempScript("redroid-kernel-setup.ps1", kKernelSetupScript);
    const QString engineScript = writeTempScript("redroid-engine-setup.ps1", kEngineSetupScript);
    if (kernelScript.isEmpty() || engineScript.isEmpty()) {
        emit logMessage("[ERROR] Could not write provisioning scripts to %TEMP%.");
        finishSequence(false, "Installation failed — temp directory not writable.");
        return;
    }

    m_steps.append({"wsl.exe", {"--install", "--no-distribution"}, 5,
                    "Enabling WSL2 + Virtual Machine Platform..."});
    m_steps.append({"powershell.exe",
                    {"-NoProfile", "-ExecutionPolicy", "Bypass", "-File", kernelScript}, 15,
                    "Downloading custom binder kernel + configuring .wslconfig..."});
    m_steps.append({"wsl.exe", {"--shutdown"}, 25,
                    "Restarting WSL2 so the custom kernel loads..."});
    m_steps.append({"powershell.exe",
                    {"-NoProfile", "-ExecutionPolicy", "Bypass", "-File", engineScript}, 80,
                    "Provisioning 'redroid-engine' distro with in-WSL Docker Engine..."});
    // binderfs registers in /proc/filesystems at boot without any mount;
    // /dev/binderfs and /dev/binder only appear after a binderfs mount,
    // so checking those would falsely fail on a fresh distro.
    m_steps.append({"wsl.exe", {"-d", engineDistroName(), "--", "bash", "-c",
                                "grep -qw binder /proc/filesystems || ls /dev/binderfs >/dev/null 2>&1 || ls /dev/binder >/dev/null 2>&1"}, 92,
                    "Verifying binder kernel support inside 'redroid-engine'..."});
    m_steps.append({"wsl.exe", {"-d", engineDistroName(), "--", "docker", "info",
                                "--format", "{{.ServerVersion}}"}, 96,
                    "Verifying in-WSL Docker Engine..."});
#else
    m_steps.append({"bash", {"-c",
                             "curl -fsSL https://get.docker.com | sh && "
                             "(systemctl enable --now docker || service docker start)"}, 80,
                    "Installing Docker Engine via get.docker.com..."});
#endif

    m_busy = true;
    m_lastExitOk = true;
    m_stepIndex = 0;
    m_percentPerStep = 90 / m_steps.size();  // reserve 10% for final "Done"
    emit progress(0);
    emit logMessage("Starting self-contained environment setup (no Docker Desktop required)...");

    runStep(m_steps[0].program, m_steps[0].args, m_steps[0].percent, m_steps[0].label);
}

void RequirementsManager::uninstall()
{
    if (m_busy) return;

    m_steps.clear();
#ifdef Q_OS_WIN
    const QString cleanupScript = writeTempScript("redroid-engine-cleanup.ps1", kUninstallCleanupScript);
    if (cleanupScript.isEmpty()) {
        emit logMessage("[ERROR] Could not write cleanup script to %TEMP%.");
        finishSequence(false, "Uninstall failed — temp directory not writable.");
        return;
    }

    // Remove only what we installed: the 'redroid-engine' distro, the custom
    // kernel file, and the .wslconfig keys. WSL2 itself and any other
    // distros the user has are left untouched.
    m_steps.append({"wsl.exe", {"--unregister", engineDistroName()}, 40,
                    "Removing 'redroid-engine' distro (in-WSL Docker Engine)..."});
    m_steps.append({"powershell.exe",
                    {"-NoProfile", "-ExecutionPolicy", "Bypass", "-File", cleanupScript}, 70,
                    "Removing custom kernel + .wslconfig keys..."});
    m_steps.append({"wsl.exe", {"--shutdown"}, 85,
                    "Shutting down WSL2..."});
#else
    m_steps.append({"bash", {"-c", "systemctl disable --now docker || true"}, 50,
                    "Stopping Docker Engine..."});
#endif

    m_busy = true;
    m_lastExitOk = true;
    m_stepIndex = 0;
    m_percentPerStep = 90 / m_steps.size();
    emit progress(0);
    emit logMessage("Starting uninstall of the self-contained environment...");

    runStep(m_steps[0].program, m_steps[0].args, m_steps[0].percent, m_steps[0].label);
}

// ---------------------------------------------------------------------------
// Step runner
// ---------------------------------------------------------------------------

QString RequirementsManager::writeTempScript(const QString& name, const QString& content)
{
    const QString path = QDir::temp().filePath(name);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return QString();
    f.write(content.toUtf8());
    f.close();
    return path;
}

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
    // Non-blocking: if start itself fails, finished() is still emitted by Qt
    // with a non-zero/error code path; we also guard below in case
    // waitForStarted is needed for a fast failure.
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
    // Tolerate non-zero exit on the first wsl.exe step in both directions:
    // `wsl --install` exits 1 when WSL is already present, and
    // `wsl --unregister` fails when the distro was never imported.
    const bool ok = (exitCode == 0) ||
                    (m_stepIndex == 0 && m_steps.first().program == "wsl.exe");
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
                       m_lastExitOk ? "Environment installed successfully — no Docker Desktop needed."
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
