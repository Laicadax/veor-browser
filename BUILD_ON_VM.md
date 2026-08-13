# VEOR Browser — Build on Virtual Machine

## Prerequisites

- Ubuntu 24.04 LTS (recommended) or 22.04 LTS
- Minimum 8GB RAM (16GB recommended)
- 100GB free disk space
- 4+ CPU cores

## Quick Start (One Command)

```bash
# Download the build script
wget https://raw.githubusercontent.com/YOUR_USERNAME/veor-browser/main/build_vm.sh
chmod +x build_vm.sh
./build_vm.sh
```

## Manual Build Steps

### Step 1: System Dependencies

```bash
sudo apt update && sudo apt upgrade -y

sudo apt install -y \
  git curl python3 python3-pip python3-venv \
  build-essential clang lld \
  ninja-build cmake pkg-config \
  libnss3-dev libdbus-1-dev libatk1.0-dev \
  libgtk-3-dev libgdk-pixbuf2.0-dev \
  libpango1.0-dev libcairo2-dev libx11-dev \
  libxcomposite-dev libxdamage-dev libxext-dev \
  libxfixes-dev libxrandr-dev libxtst-dev \
  libxss-dev libpciaccess-dev libxt-dev \
  libgconf-2-4 libasound2-dev libpulse-dev \
  libdrm-dev libgbm-dev libcurl4-openssl-dev \
  libelf-dev libgl1-mesa-dev libgles2-mesa-dev \
  libvulkan-dev libva-dev libvdpau-dev \
  libwebp-dev libjpeg-dev libpng-dev \
  libtiff-dev libxml2-dev libxslt1-dev \
  libsqlite3-dev libffi-dev liblzma-dev \
  libbz2-dev zlib1g-dev libncurses5-dev \
  libreadline-dev libedit-dev libexpat1-dev \
  uuid-dev libjsoncpp-dev libre2-dev \
  libminizip-dev libhunspell-dev \
  libopenjp2-7-dev libopus-dev \
  libsnappy-dev libusb-1.0-0-dev
```

### Step 2: Configure Swap (if RAM < 16GB)

```bash
sudo fallocate -l 8G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

### Step 3: Install Depot Tools

```bash
cd ~
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH="$HOME/depot_tools:$PATH"
echo 'export PATH="$HOME/depot_tools:$PATH"' >> ~/.bashrc
```

### Step 4: Fetch Chromium Source

```bash
mkdir -p ~/chromium && cd ~/chromium
fetch --nohooks chromium
```

This downloads ~20GB. Go get coffee.

### Step 5: Sync to Compatible Version

```bash
cd ~/chromium/src
git checkout 134.0.6998.0  # Or latest stable
gclient sync -D
```

### Step 6: Install Build Dependencies

```bash
cd ~/chromium/src
./build/install-build-deps.sh
./build/linux/sysroot_scripts/install-sysroot.py --arch=amd64
gclient runhooks
```

### Step 7: Clone VEOR into Chromium

```bash
cd ~/chromium/src
git clone https://github.com/YOUR_USERNAME/veor-browser.git veor
```

### Step 8: Configure GN Args

```bash
cd ~/chromium/src
mkdir -p out/veor

cat > out/veor/args.gn << 'GNEOF'
is_debug = false
is_component_build = false
symbol_level = 1
enable_nacl = false
treat_warnings_as_errors = false
use_sysroot = true
use_custom_libcxx = false
GNEOF

gn gen out/veor
```

### Step 9: Build

```bash
cd ~/chromium/src
autoninja -C out/veor veor_browser
```

Build time: 2-6 hours depending on hardware.

### Step 10: Run

```bash
cd ~/chromium/src
./out/veor/veor_browser
```

## Troubleshooting

| Error | Solution |
|-------|----------|
| `No space left on device` | Free up disk space or mount larger volume |
| `fatal: unable to access` | Check internet connection, use VPN if needed |
| `ninja: error: unknown target` | Verify `veor` directory is in `~/chromium/src/veor` |
| `missing sysroot` | Run `./build/linux/sysroot_scripts/install-sysroot.py` |
| `LLVM out of memory` | Increase swap or reduce `jumbo_file_merge_limit` |

## After Build

The binary is at `~/chromium/src/out/veor/veor_browser`.

To create a distributable package:

```bash
cd ~/chromium/src
cp out/veor/veor_browser ~/veor-browser
cp -r veor/src/ui/assets ~/veor-browser/assets  # if any
tar czf veor-browser-linux-amd64.tar.gz ~/veor-browser
```

---
