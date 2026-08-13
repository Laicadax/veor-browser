#!/bin/bash
# VEOR Browser — Automated Build Script for Ubuntu VM
# Usage: ./build_vm.sh [github_username]

set -e

GITHUB_USER="${1:-}"
VEOR_REPO="${GITHUB_USER:+https://github.com/$GITHUB_USER/veor-browser.git}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log() {
    echo -e "${GREEN}[VEOR BUILD]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[VEOR BUILD]${NC} $1"
}

error() {
    echo -e "${RED}[VEOR BUILD]${NC} $1"
}

# ─── Check prerequisites ─────────────────────────────────────────────────────
log "Checking prerequisites..."

if [ "$(id -u)" -eq 0 ]; then
    error "Do not run as root. Use a regular user with sudo access."
    exit 1
fi

TOTAL_RAM=$(free -m | awk '/^Mem:/{print $2}')
FREE_DISK=$(df -BG . | awk 'NR==2{print $4}' | tr -d 'G')

log "RAM: ${TOTAL_RAM}MB | Free disk: ${FREE_DISK}GB"

if [ "$TOTAL_RAM" -lt 4096 ]; then
    error "Minimum 4GB RAM required. 8GB+ recommended."
    exit 1
fi

if [ "$FREE_DISK" -lt 50 ]; then
    error "Minimum 50GB free disk space required. 100GB+ recommended."
    exit 1
fi

# ─── Install dependencies ────────────────────────────────────────────────────
log "Installing system dependencies..."

sudo apt update
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

# ─── Configure swap if needed ────────────────────────────────────────────────
if [ "$TOTAL_RAM" -lt 16384 ] && [ ! -f /swapfile ]; then
    warn "RAM < 16GB. Creating 8GB swap file..."
    sudo fallocate -l 8G /swapfile
    sudo chmod 600 /swapfile
    sudo mkswap /swapfile
    sudo swapon /swapfile
    echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
    log "Swap configured."
fi

# ─── Install depot_tools ─────────────────────────────────────────────────────
if [ ! -d "$HOME/depot_tools" ]; then
    log "Installing depot_tools..."
    cd ~
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
fi

export PATH="$HOME/depot_tools:$PATH"

# ─── Fetch Chromium ──────────────────────────────────────────────────────────
if [ ! -d "$HOME/chromium/src" ]; then
    log "Fetching Chromium source (~20GB, this will take a while)..."
    mkdir -p ~/chromium
    cd ~/chromium
    fetch --nohooks chromium
else
    log "Chromium source already exists. Skipping fetch."
fi

# ─── Sync Chromium ───────────────────────────────────────────────────────────
log "Syncing Chromium..."
cd ~/chromium/src
gclient sync -D

# ─── Install Chromium build deps ─────────────────────────────────────────────
log "Installing Chromium build dependencies..."
cd ~/chromium/src
./build/install-build-deps.sh --no-prompt || true
./build/linux/sysroot_scripts/install-sysroot.py --arch=amd64
gclient runhooks

# ─── Clone VEOR ──────────────────────────────────────────────────────────────
if [ -n "$VEOR_REPO" ]; then
    log "Cloning VEOR from GitHub: $VEOR_REPO"
    if [ -d "veor" ]; then
        rm -rf veor
    fi
    git clone "$VEOR_REPO" veor
elif [ -f "/tmp/veor-browser-source.tar.gz" ]; then
    log "Extracting VEOR from local archive..."
    rm -rf veor
    tar xzf /tmp/veor-browser-source.tar.gz
    mv veor-browser veor 2>/dev/null || true
else
    warn "No GitHub repo or local archive found."
    warn "Please either:"
    warn "  1. Pass GitHub username: ./build_vm.sh your_username"
    warn "  2. Copy veor-browser-source.tar.gz to /tmp/"
    exit 1
fi

# ─── Configure GN ────────────────────────────────────────────────────────────
log "Configuring GN build args..."
mkdir -p ~/chromium/src/out/veor

cat > ~/chromium/src/out/veor/args.gn << 'GNEOF'
is_debug = false
is_component_build = false
symbol_level = 1
enable_nacl = false
treat_warnings_as_errors = false
use_sysroot = true
use_custom_libcxx = false
GNEOF

cd ~/chromium/src
gn gen out/veor

# ─── Build ───────────────────────────────────────────────────────────────────
log "Starting build. This will take 2-6 hours..."
autoninja -C out/veor veor_browser

# ─── Success ─────────────────────────────────────────────────────────────────
if [ -f ~/chromium/src/out/veor/veor_browser ]; then
    log "Build successful!"
    log "Binary: ~/chromium/src/out/veor/veor_browser"
    log ""
    log "To run:"
    log "  cd ~/chromium/src && ./out/veor/veor_browser"
else
    error "Build failed. Check logs above."
    exit 1
fi
