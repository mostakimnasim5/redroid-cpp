# WSL2 Custom Kernel Setup for ReDroidCPP

ReDroid requires Android binder IPC support in the WSL2 Linux kernel.
The default Microsoft kernel (`5.15.x`) does **not** include this.

> **You usually do not need this guide.** The app's **"⬇ Install"** button
> does all of it automatically: enables WSL2, downloads the binder-enabled
> kernel from our fork
> ([mostakimnasim5/WSL2-Linux-Kernel-Rolling](https://github.com/mostakimnasim5/WSL2-Linux-Kernel-Rolling)),
> merges it into `%USERPROFILE%\.wslconfig`, imports the `redroid-engine`
> distro and installs Docker Engine (docker-ce) inside it.
> **Docker Desktop is not required.**
>
> This document remains for manual setup / troubleshooting.

---

---

## Prerequisites

- Windows 10 (Build 19041+) or Windows 11
- WSL2 installed (`wsl --install --no-distribution`)
- ~30 minutes for kernel compilation (Method 2 only)

---

## Method 1 — Pre-built Kernel from our Fork (Fastest, Recommended)

The Install button downloads the kernel from
[mostakimnasim5/WSL2-Linux-Kernel-Rolling](https://github.com/mostakimnasim5/WSL2-Linux-Kernel-Rolling),
a WSL2 kernel fork with Android binder support. Manually:

```powershell
# 1. Download the kernel binary
$kernelUrl = "https://github.com/mostakimnasim5/WSL2-Linux-Kernel-Rolling/releases/latest/download/bzImage"
$kernelPath = "$env:USERPROFILE\wsl-kernel\bzImage"
New-Item -ItemType Directory -Force "$env:USERPROFILE\wsl-kernel"
Invoke-WebRequest -Uri $kernelUrl -OutFile $kernelPath

# 2. Point WSL2 to the custom kernel
$wslConfig = "$env:USERPROFILE\.wslconfig"
@"
[wsl2]
kernel=$($kernelPath -replace '\\', '\\')
nestedVirtualization=true
"@ | Set-Content $wslConfig

# 3. Restart WSL2
wsl --shutdown
Start-Sleep -Seconds 3
wsl -- uname -r   # Should show a custom kernel version
```

---

## Method 2 — Build from Source

```bash
# Run inside WSL2 Ubuntu

# 1. Install build dependencies
sudo apt-get update
sudo apt-get install -y build-essential flex bison libssl-dev \
  libelf-dev bc pahole python3 cpio

# 2. Clone the WSL2 kernel source
git clone --depth=1 \
  https://github.com/microsoft/WSL2-Linux-Kernel.git \
  ~/wsl2-kernel
cd ~/wsl2-kernel

# 3. Start from the Microsoft default config
cp Microsoft/config-wsl .config

# 4. Enable Android binder (required for ReDroid)
scripts/config --enable CONFIG_ANDROID_BINDER_IPC
scripts/config --enable CONFIG_ANDROID_BINDERFS
scripts/config --enable CONFIG_ANDROID_BINDER_DEVICES

# 5. Enable KVM nested virtualisation (optional, faster boot)
scripts/config --enable CONFIG_KVM
scripts/config --enable CONFIG_KVM_INTEL   # Intel CPU
# scripts/config --enable CONFIG_KVM_AMD   # AMD CPU

# 6. Build (uses all CPU cores)
make -j$(nproc) KCONFIG_CONFIG=.config

# 7. Copy the kernel image to Windows
cp arch/x86/boot/bzImage /mnt/c/Users/$USER/wsl-kernel/bzImage
```

Then follow steps 2–3 from Method 1 to point WSL2 to the new kernel.

> **Note:** the fork's CI builds on `linux-wsl-stable-*` tags. If the
> `releases/latest/download/bzImage` link 404s, no pre-built release has been
> published yet — use Method 2 (build from source) and the Install button
> will pick up `%USERPROFILE%\wsl-kernel\bzImage` on its next run.

---

## Verify Kernel Is Correct

```bash
# Run inside WSL2 after reboot
uname -r                            # Should NOT contain "microsoft-standard"
ls /dev/binder                      # Should exist
ls /dev/binderfs                    # Should exist (if binderfs compiled in)
grep -i binder /proc/filesystems    # Should list "binder" or "binderfs"
```

ReDroidCPP also checks this automatically on startup and reports the
result in the System Requirements dialog.

---

## Enable KVM (Optional — 2× faster Android boot)

```powershell
# PowerShell as Administrator
dism /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart
dism /online /enable-feature /featurename:HypervisorPlatform /all /norestart

# Add to .wslconfig
Add-Content "$env:USERPROFILE\.wslconfig" "nestedVirtualization=true"

# Restart
wsl --shutdown
```

Inside WSL2, verify:
```bash
ls /dev/kvm    # Should exist
```

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `no binder device` in container logs | Default Microsoft kernel | Follow Method 1 or 2 above |
| Container exits immediately | `/dev/binder` not mounted | Verify kernel with `grep binder /proc/filesystems` |
| Very slow boot (~90s) | KVM not available | Enable nested virtualisation (see above) |
| `docker: error response from daemon` | in-WSL dockerd not running | `wsl -d redroid-engine -- sudo systemctl start docker` |
| `port already allocated` | Previous container not removed | Run `docker rm -f $(docker ps -aq)` |

---

## References

- [ReDroid GitHub](https://github.com/remote-android/redroid-doc)
- [Binder kernel fork](https://github.com/mostakimnasim5/WSL2-Linux-Kernel-Rolling)
- [WSL2 Custom Kernel](https://learn.microsoft.com/en-us/windows/wsl/kernel)
- [Microsoft WSL2 Kernel Source](https://github.com/microsoft/WSL2-Linux-Kernel)
