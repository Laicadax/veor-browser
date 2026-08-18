# Copyright (c) 2026 VEOR Browser Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

[CmdletBinding()]
param(
  [string]$ChromiumRoot = (Join-Path $HOME "chromium"),
  [string]$DepotToolsRoot = (Join-Path $HOME "depot_tools"),
  [switch]$SkipDepotTools,
  [switch]$SkipFetch,
  [switch]$SkipSync,
  [switch]$SkipHooks,
  [switch]$SkipBuild,
  [switch]$SkipPackage
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest
$env:DEPOT_TOOLS_WIN_TOOLCHAIN = "0"

$RepoRoot = [System.IO.Path]::GetFullPath($PSScriptRoot)
$ChromiumRoot = [System.IO.Path]::GetFullPath($ChromiumRoot)
$ChromiumSrc = Join-Path $ChromiumRoot "src"
$OutDir = Join-Path $ChromiumSrc "out\veor"
$DistDir = Join-Path $ChromiumSrc "veor-dist"
$VeorExe = Join-Path $OutDir "veor_browser.exe"
$PackagePath = Join-Path $ChromiumSrc "veor-browser-windows-x64.zip"
$MinimumFreeDiskGb = 250
$MinimumRamGb = 16
$MinimumCpuCount = 8

function Write-Log {
  param([string]$Message)
  Write-Host "[VEOR BUILD] $Message" -ForegroundColor Green
}

function Write-WarningMessage {
  param([string]$Message)
  Write-Host "[VEOR BUILD] $Message" -ForegroundColor Yellow
}

function Fail {
  param([string]$Message)
  Write-Host "[VEOR BUILD] $Message" -ForegroundColor Red
  exit 1
}

function Invoke-External {
  param(
    [string]$Command,
    [string[]]$Arguments,
    [string]$WorkingDirectory
  )

  Push-Location $WorkingDirectory
  try {
    Write-Log ("Running: {0} {1}" -f $Command, ($Arguments -join " "))
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) {
      throw "'$Command' exited with code $LASTEXITCODE."
    }
  } finally {
    Pop-Location
  }
}

function Find-VisualStudio {
  $candidates = @(
    (Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"),
    (Join-Path $env:ProgramFiles "Microsoft Visual Studio\Installer\vswhere.exe")
  )
  $vswhere = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
  if (-not $vswhere) {
    throw "Visual Studio 2022 was not found. Install it with the Chromium C++ workload."
  }

  $installationPath = (& $vswhere -latest -products * `
      -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
      -property installationPath 2>$null | Select-Object -First 1)
  if (-not $installationPath) {
    throw "Visual Studio C++ build tools were not found. Install Desktop development with C++ and MSVC v143."
  }
  return [System.IO.Path]::GetFullPath($installationPath.Trim())
}

function Test-Preflight {
  Write-Log "Checking Windows build prerequisites..."

  $driveName = (Split-Path -Qualifier $RepoRoot).TrimEnd(":")
  $drive = Get-PSDrive -Name $driveName
  $freeDiskGb = [math]::Floor($drive.Free / 1GB)
  if ($freeDiskGb -lt $MinimumFreeDiskGb) {
    throw ("At least {0} GB free disk space is required; found {1} GB on {2}:." -f `
        $MinimumFreeDiskGb, $freeDiskGb, $driveName)
  }

  $computer = Get-CimInstance Win32_ComputerSystem
  $ramGb = [math]::Floor($computer.TotalPhysicalMemory / 1GB)
  if ($ramGb -lt $MinimumRamGb) {
    throw ("At least {0} GB RAM is required; found {1} GB." -f `
        $MinimumRamGb, $ramGb)
  }
  if ([Environment]::ProcessorCount -lt $MinimumCpuCount) {
    throw ("At least {0} logical CPUs are required; found {1}." -f `
        $MinimumCpuCount, [Environment]::ProcessorCount)
  }

  foreach ($tool in @("git.exe", "python.exe")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
      throw "$tool was not found on PATH."
    }
  }

  $longPaths = Get-ItemPropertyValue `
      -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" `
      -Name LongPathsEnabled -ErrorAction SilentlyContinue
  if ($longPaths -ne 1) {
    throw "Windows long paths are disabled. Enable LongPathsEnabled=1 and reboot."
  }
  $gitLongPaths = (& git config --global --get core.longpaths 2>$null) -join ""
  if ($gitLongPaths.Trim().ToLowerInvariant() -ne "true") {
    throw "Git long paths are disabled. Run: git config --global core.longpaths true"
  }

  $vsPath = Find-VisualStudio
  $msvcRoot = Join-Path $vsPath "VC\Tools\MSVC"
  $clPath = Get-ChildItem $msvcRoot -Filter cl.exe -Recurse `
      -ErrorAction SilentlyContinue | Select-Object -First 1
  if (-not $clPath) {
    throw "cl.exe was not found under Visual Studio. Install MSVC v143 x64/x86 build tools."
  }
  $sdkRoot = Get-ItemPropertyValue `
      -Path "HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots" `
      -Name KitsRoot10 -ErrorAction SilentlyContinue
  if (-not $sdkRoot -or -not (Test-Path $sdkRoot)) {
    throw "Windows 10/11 SDK was not found. Install Windows 11 SDK 10.0.22621 or newer."
  }

  Write-Log ("RAM: {0} GB | CPUs: {1} | Free disk: {2} GB" -f `
      $ramGb, [Environment]::ProcessorCount, $freeDiskGb)
  Write-Log ("Visual Studio: {0}" -f $vsPath)
}

function Install-DepotTools {
  if (Test-Path (Join-Path $DepotToolsRoot "gclient.bat")) {
    Write-Log "depot_tools already exists. Skipping install."
  } elseif ($SkipDepotTools) {
    throw "depot_tools is missing and -SkipDepotTools was specified."
  } else {
    Write-Log "Installing depot_tools..."
    $parent = Split-Path -Parent $DepotToolsRoot
    New-Item -ItemType Directory -Force -Path $parent | Out-Null
    Invoke-External "git" @(
      "clone",
      "https://chromium.googlesource.com/chromium/tools/depot_tools.git",
      $DepotToolsRoot
    ) $parent
  }
  $env:PATH = "$DepotToolsRoot;$env:PATH"
}

function Prepare-Chromium {
  New-Item -ItemType Directory -Force -Path $ChromiumRoot | Out-Null
  if (-not (Test-Path $ChromiumSrc)) {
    if ($SkipFetch) {
      throw "Chromium source is missing and -SkipFetch was specified."
    }
    Write-Log "Fetching Chromium source (this is a large download)..."
    Invoke-External "fetch" @("--nohooks", "chromium") $ChromiumRoot
  } else {
    Write-Log "Chromium source already exists. Skipping fetch."
  }

  if (-not (Test-Path (Join-Path $ChromiumRoot ".gclient"))) {
    Write-Log "Creating a gclient configuration..."
    Invoke-External "gclient" @(
      "config",
      "https://chromium.googlesource.com/chromium/src.git"
    ) $ChromiumRoot
  }

  if (-not $SkipSync) {
    Write-Log "Synchronizing Chromium dependencies..."
    Invoke-External "gclient" @("sync", "-D") $ChromiumRoot
  } else {
    Write-WarningMessage "Skipping gclient sync."
  }

  if (-not $SkipHooks) {
    Write-Log "Running Chromium hooks..."
    Invoke-External "gclient" @("runhooks") $ChromiumRoot
  } else {
    Write-WarningMessage "Skipping gclient runhooks."
  }
}

function Install-Veor {
  $veorDestination = Join-Path $ChromiumSrc "veor"
  if (-not (Test-Path $veorDestination)) {
    Write-Log "Linking VEOR into Chromium as src\veor..."
    New-Item -ItemType Junction -Path $veorDestination -Target $RepoRoot | Out-Null
  } else {
    $destination = Get-Item $veorDestination
    if ($destination.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
      Write-Log "VEOR link already exists. Reusing it."
    } else {
      Write-Log "Copying VEOR into the existing src\veor directory..."
      & robocopy $RepoRoot $veorDestination /E /XD .git .github
      if ($LASTEXITCODE -gt 7) {
        throw "robocopy failed with exit code $LASTEXITCODE."
      }
    }
  }

  $rootBuild = Join-Path $ChromiumSrc "BUILD.gn"
  $importLine = 'import("//veor/BUILD.gn")'
  $rootBuildText = Get-Content -Raw $rootBuild
  if ($rootBuildText -notmatch [regex]::Escape($importLine)) {
    Add-Content -Path $rootBuild -Value $importLine -Encoding utf8
    Write-Log "Added the VEOR BUILD.gn import to Chromium's root BUILD.gn."
  } else {
    Write-Log "VEOR BUILD.gn import already exists."
  }
}

function Configure-AndBuild {
  $gnArgs = @"
target_os = "win"
target_cpu = "x64"
is_debug = false
is_component_build = false
symbol_level = 1
enable_nacl = false
treat_warnings_as_errors = false
chrome_pgo_phase = 0
"@
  New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
  $argsPath = Join-Path $OutDir "args.gn"
  Set-Content -Path $argsPath -Value $gnArgs -Encoding utf8
  Write-Log "Generating GN files..."
  Invoke-External "gn" @("gen", "out\veor") $ChromiumSrc

  if (-not $SkipBuild) {
    Write-Log "Building veor_browser.exe..."
    Invoke-External "autoninja" @("-C", "out\veor", "veor_browser") $ChromiumSrc
  } else {
    Write-WarningMessage "Skipping the build."
  }
}

function Package-Build {
  if ($SkipPackage) {
    Write-WarningMessage "Skipping packaging."
    return
  }
  if (-not (Test-Path $VeorExe)) {
    throw "Expected executable was not found: $VeorExe"
  }

  if (Test-Path $DistDir) {
    Remove-Item -Recurse -Force $DistDir
  }
  New-Item -ItemType Directory -Force -Path $DistDir | Out-Null
  Copy-Item $VeorExe $DistDir

  foreach ($pattern in @("*.dll", "*.pak", "*.dat")) {
    Get-ChildItem $OutDir -Filter $pattern -File -ErrorAction SilentlyContinue |
        Copy-Item -Destination $DistDir
  }
  $locales = Join-Path $OutDir "locales"
  if (Test-Path $locales) {
    Copy-Item $locales $DistDir -Recurse
  }

  if (Test-Path $PackagePath) {
    Remove-Item -Force $PackagePath
  }
  Compress-Archive -Path (Join-Path $DistDir "*") -DestinationPath $PackagePath
  Write-Log "Package created: $PackagePath"
}

try {
  Test-Preflight
  Install-DepotTools
  Prepare-Chromium
  Install-Veor
  Configure-AndBuild
  Package-Build
  Write-Log "Windows build completed."
} catch {
  Fail $_.Exception.Message
}
