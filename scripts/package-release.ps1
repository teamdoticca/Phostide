param(
  [string]$Configuration = "Release",
  [string]$Version = "0.1.0"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $root "out\$Configuration"
$stage = Join-Path $root "out\stage\Phostide-$Version-win-x64"
$zip = Join-Path $root "out\Phostide-$Version-win-x64.zip"

if (-not (Test-Path (Join-Path $outDir "Phostide.exe"))) {
  Write-Host "Building native solution..."
  $msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe |
    Select-Object -First 1
  if (-not $msbuild) { throw "MSBuild not found. Install VS 2022 C++ workload." }
  & $msbuild (Join-Path $root "src\Phostide.sln") /p:Configuration=$Configuration /p:Platform=x64 /m
}

New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item (Join-Path $outDir "Phostide.exe") $stage -Force
Copy-Item (Join-Path $outDir "ExplorerTAP.dll") $stage -Force
Copy-Item (Join-Path $root "README.md") $stage -Force
Copy-Item (Join-Path $root "LICENSE") $stage -Force

# Brand icons for future installer (Inno/WiX SetupIcon) and store listings.
$brand = Join-Path $root "assets\brand"
if (Test-Path (Join-Path $brand "setup-phostide.ico")) {
  $brandStage = Join-Path $stage "brand"
  New-Item -ItemType Directory -Force -Path $brandStage | Out-Null
  Copy-Item (Join-Path $brand "setup-phostide.ico") $brandStage -Force
  Copy-Item (Join-Path $brand "phostide.ico") $brandStage -Force
  Copy-Item (Join-Path $brand "phostide-1024.png") $brandStage -Force
}

if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
Write-Host "Packed $zip"
Write-Host "Publish with: gh release create v$Version `"$zip`" --title `"Phostide $Version`" --generate-notes"
