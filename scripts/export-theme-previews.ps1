param(
  [string]$Configuration = "Release",
  [int]$Width = 1920,
  [int]$Height = 48
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$outThemes = Join-Path $root "site\public\themes"
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
  -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe |
  Select-Object -First 1
if (-not $msbuild) { throw "MSBuild not found." }

$proj = Join-Path $root "src\ThemePreviewExport\ThemePreviewExport.vcxproj"
& $msbuild $proj /p:Configuration=$Configuration /p:Platform=x64 /m
if ($LASTEXITCODE -ne 0) { throw "ThemePreviewExport build failed" }

$exeCandidates = @(
  (Join-Path $root "out\$Configuration\ThemePreviewExport.exe"),
  (Join-Path $root "src\out\$Configuration\ThemePreviewExport.exe")
)
$exe = $exeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $exe) { throw "ThemePreviewExport.exe not found under out\" }
& $exe $outThemes $Width $Height
if ($LASTEXITCODE -ne 0) { throw "ThemePreviewExport failed" }

# Remove legacy moodboard JPEGs so the site cannot accidentally use them.
Get-ChildItem -LiteralPath $outThemes -Filter "*.jpg" -ErrorAction SilentlyContinue |
  ForEach-Object {
    Write-Host "Removing moodboard $($_.Name)"
    Remove-Item $_.FullName -Force
  }

Write-Host "Theme strips ready in $outThemes"
