# VEOR Browser — CI/CD Setup Guide

## Overview

This project includes two CI/CD workflows:

| Workflow | File | Use Case | Reliability |
|----------|------|----------|-------------|
| **Self-Hosted** | `.github/workflows/build-self-hosted.yml` | Your own server/VM | High — no time/disk limits |
| **GitHub-Hosted** | `.github/workflows/build-github-hosted.yml` | GitHub's free runners | Experimental — may timeout |

**Strong recommendation**: Use the self-hosted runner. GitHub's free runners have a 6-hour job limit and ~50GB disk, which is tight for Chromium builds.

---

## Option 1: Self-Hosted Runner (Recommended)

### Prerequisites

- Ubuntu 22.04 or 24.04 LTS
- 16GB+ RAM
- 200GB+ free disk space (SSD strongly recommended)
- 8+ CPU cores
- Internet access to GitHub

### Step 1: Prepare Your Machine

Use the same Azure VM or any dedicated server. Run:

```bash
sudo apt update && sudo apt upgrade -y

sudo apt install -y \
  git curl python3 python3-pip python3-venv \
  build-essential clang lld ninja-build cmake pkg-config \
  libnss3-dev libdbus-1-dev libatk1.0-dev \
  libgtk-3-dev libgdk-pixbuf2.0-dev libpango1.0-dev \
  libcairo2-dev libx11-dev libxcomposite-dev \
  libxdamage-dev libxext-dev libxfixes-dev \
  libxrandr-dev libxtst-dev libxss-dev \
  libpciaccess-dev libxt-dev libgconf-2-4 \
  libasound2-dev libpulse-dev libdrm-dev \
  libgbm-dev libcurl4-openssl-dev libelf-dev \
  libgl1-mesa-dev libgles2-mesa-dev libvulkan-dev \
  libva-dev libvdpau-dev libwebp-dev libjpeg-dev \
  libpng-dev libtiff-dev libxml2-dev libxslt1-dev \
  libsqlite3-dev libffi-dev liblzma-dev libbz2-dev \
  zlib1g-dev libncurses5-dev libreadline-dev \
  libedit-dev libexpat1-dev uuid-dev \
  libjsoncpp-dev libre2-dev libminizip-dev \
  libhunspell-dev libopenjp2-7-dev libopus-dev \
  libsnappy-dev libusb-1.0-0-dev

# Optional: swap for machines with < 16GB RAM
sudo fallocate -l 8G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

### Step 2: Register Runner with GitHub

1. Go to your repository: `https://github.com/Laicadax/veor-browser`
2. Navigate to **Settings → Actions → Runners**
3. Click **New self-hosted runner**
4. Select **Linux** and **x64**
5. Copy the commands from the page (they will look like below)

Run on your VM:

```bash
# Create a folder for the runner
mkdir -p ~/actions-runner && cd ~/actions-runner

# Download the latest runner package (replace X.XXX.X with version from GitHub page)
curl -o actions-runner-linux-x64-2.317.0.tar.gz -L https://github.com/actions/runner/releases/download/v2.317.0/actions-runner-linux-x64-2.317.0.tar.gz

# Extract
tar xzf ./actions-runner-linux-x64-2.317.0.tar.gz

# Configure (use the token from GitHub page)
./config.sh --url https://github.com/Laicadax/veor-browser --token YOUR_TOKEN_HERE

# Install as a service (runs automatically on boot)
sudo ./svc.sh install
sudo ./svc.sh start
```

### Step 3: Verify Runner is Online

Go back to **Settings → Actions → Runners** in your GitHub repo. Your runner should show as **Idle** (green dot).

### Step 4: Trigger Build

1. Go to **Actions** tab in your repo
2. Select **Build VEOR (Self-Hosted)**
3. Click **Run workflow**
4. Select `release` and click **Run workflow**

The build will start on your VM. You can monitor progress in the GitHub Actions UI.

### Step 5: Download Artifact

After successful build:
1. Go to the completed workflow run
2. Scroll to **Artifacts** section
3. Download `veor-browser-linux-amd64`

---

## Option 2: GitHub-Hosted Runner (Experimental)

### Limitations

| Limit | Value |
|-------|-------|
| Job timeout | 6 hours |
| Disk space | ~50GB (after cleanup) |
| RAM | 7GB |
| CPU | 2 cores |
| Monthly minutes | 2000 (free plan) |

**One full Chromium build consumes ~400-600 minutes.** You get ~3 builds per month on the free plan.

### How to Use

1. Go to **Actions** tab
2. Select **Build VEOR (GitHub-Hosted)**
3. Click **Run workflow**

**Warning**: This workflow may fail due to:
- Timeout (6h limit)
- Disk space exhaustion
- Memory pressure

If it fails, use the self-hosted runner instead.

---

## Troubleshooting

### Runner shows "Offline"

```bash
# Check if service is running
sudo systemctl status actions.runner.Laicadax-veor-browser.*

# Restart if needed
sudo ./svc.sh stop
sudo ./svc.sh start
```

### Build fails with "No space left on device"

Self-hosted runner: Free up disk space or attach larger volume.
GitHub-hosted runner: The workflow includes a cleanup step, but Chromium may still exceed limits.

### Build fails with "Killed" (OOM)

Increase swap:
```bash
sudo swapoff /swapfile
sudo rm /swapfile
sudo fallocate -l 16G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

### Cache not working

The first build will always be slow (fetching Chromium). Subsequent builds use cache if the runner is persistent. If you destroy the VM between builds, cache is lost.

---

## Security Notes

Self-hosted runners execute code from your repository. Only use them with:
- Private repositories, or
- Trusted contributors only

Do **not** use self-hosted runners on public repos with untrusted PRs.

---

## Next Steps

1. Set up self-hosted runner on your Azure VM
2. Push these workflow files to your repo
3. Trigger first build
4. Wait 4-8 hours
5. Download binary

If you need help — open an issue in the repo.
