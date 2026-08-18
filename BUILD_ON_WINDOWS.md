# Building VEOR on Windows

VEOR is a Chromium Content API application. A complete Windows build requires
building Chromium and must run on a dedicated machine; GitHub-hosted runners
do not have enough disk space.

## Hardware and operating system

- Windows 10 or Windows Server 2022 or newer.
- At least 250 GB of free disk space on the checkout volume; 300 GB or more
  leaves room for updates and build output.
- At least 16 GB RAM and 8 logical CPUs; 32 GB RAM and more CPUs are
  recommended.
- An NTFS volume with Windows long paths enabled.
- Exclude the Chromium and VEOR checkout directories from Microsoft Defender
  real-time scanning. This can substantially reduce build time on a dedicated
  build machine.

## Visual Studio and SDK

Install Visual Studio 2022 with the **Desktop development with C++** workload
and these components:

- MSVC v143 - VS 2022 C++ x64/x86 build tools.
- Windows 11 SDK 10.0.22621 or newer.
- **Debugging Tools for Windows** from the SDK optional features. Chromium
  requires these tools even when no debugger is used directly.

Open a Developer PowerShell for VS 2022, or otherwise make sure the selected
MSVC tools and SDK are available to the build environment. VEOR's script uses
the external Visual Studio installation and sets:

```powershell
$env:DEPOT_TOOLS_WIN_TOOLCHAIN = "0"
```

This means depot_tools does not download or select a toolchain; the builder's
own Visual Studio installation is used.

`gclient runhooks` runs Chromium's toolchain setup, including
`vs_toolchain.py`. With `DEPOT_TOOLS_WIN_TOOLCHAIN=0`, that setup locates the
installed Visual Studio and Windows SDK instead of downloading a depot_tools
toolchain. Rerun `gclient runhooks` after changing the VS installation.

## Long paths, Git, and depot_tools

Enable the Windows long-path policy as Administrator, then reboot:

```powershell
New-ItemProperty `
  -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" `
  -Name LongPathsEnabled -PropertyType DWord -Value 1 -Force
git config --global core.longpaths true
```

Install Git and Python, then install depot_tools in a persistent location:

```powershell
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git `
  "$HOME\depot_tools"
$env:PATH = "$HOME\depot_tools;$env:PATH"
$env:DEPOT_TOOLS_WIN_TOOLCHAIN = "0"
```

## Fetch Chromium and place VEOR

Pin Chromium to a specific tag or known-good revision for reproducible
results. This repository has not yet been compiled against a real Chromium
checkout, so current Chromium API drift may require small follow-up fixes.

```powershell
New-Item -ItemType Directory -Force "$HOME\chromium" | Out-Null
Set-Location "$HOME\chromium"
fetch --nohooks chromium
gclient sync -D
gclient runhooks
```

Place this repository at `src\veor` (a directory junction is convenient for
development), and add this line once to Chromium's `src\BUILD.gn`:

```gn
import("//veor/BUILD.gn")
```

The automated script performs these steps idempotently and can be run from
the VEOR checkout:

```powershell
.\build_windows.ps1
```

Heavy operations can be skipped when reusing an existing checkout:

```powershell
.\build_windows.ps1 -SkipFetch -SkipSync -SkipHooks
```

The script checks disk, memory, CPU, Visual Studio, SDK, Python, Git, and long
path support before downloading anything.

## Configure, build, run, and package

The script writes `src\out\veor\args.gn` with these non-component release
arguments:

```gn
target_os = "win"
target_cpu = "x64"
is_debug = false
is_component_build = false
symbol_level = 1
enable_nacl = false
treat_warnings_as_errors = false
chrome_pgo_phase = 0
```

To perform the individual steps manually:

```powershell
Set-Location "$HOME\chromium\src"
New-Item -ItemType Directory -Force out\veor | Out-Null
notepad out\veor\args.gn
gn gen out\veor
autoninja -C out\veor veor_browser
.\out\veor\veor_browser.exe
```

The automated script packages `veor_browser.exe`, top-level DLL/Pak/Data
files, and the `locales` directory when present:

```powershell
Compress-Archive `
  -Path "$HOME\chromium\src\veor-dist\*" `
  -DestinationPath "$HOME\chromium\src\veor-browser-windows-x64.zip"
```

## Troubleshooting

| Symptom | Action |
| --- | --- |
| Disk preflight fails | Use a volume with at least 250 GB free; Chromium source, dependencies, and output are all large. |
| `cl.exe` or SDK not found | Install the listed VS 2022 workload/components and run from a VS Developer PowerShell. |
| `DEPOT_TOOLS_WIN_TOOLCHAIN` errors | Set it to `0`; use the machine's VS 2022 installation rather than depot_tools' toolchain. |
| `gclient sync` fails | Check Git/Python on PATH, proxy access, disk space, and retry without deleting the checkout. |
| Path-too-long errors | Enable Windows long paths, reboot, and set `git config --global core.longpaths true`. |
| `gn gen` reports unknown arguments | Use the pinned Chromium revision's supported args and remove stale output before regenerating. |
| Linker runs out of memory | Increase RAM/pagefile size, reduce parallelism, or use a larger build machine. |
| Executable starts and exits due to resources | See the known resource-packing gap below; verify runtime DLLs, Pak files, and locales are beside the exe. |
| `vs_toolchain.py` selects the wrong toolchain | Keep `DEPOT_TOOLS_WIN_TOOLCHAIN=0`, open a VS Developer PowerShell, and rerun `gclient runhooks`. |

## Known gaps

1. This codebase has never been compiled against a real Chromium checkout.
   Expect API-drift compile errors and pin a specific Chromium tag before
   diagnosing build failures.
2. VEOR's GN files do not currently define a `.pak` or resource-packing
   target. `ResourceBundle` therefore has no VEOR resources, and the
   executable may fail at runtime until a repack target is added.
