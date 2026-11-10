param(
  [string]$Configuration = "Release",
  [int]$Width = 1920,
  [int]$Height = 48,
  [int]$Fps = 20,
  [double]$DurationSec = 6,
  [switch]$SkipBuild,
  [switch]$PostersOnly
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$outThemes = Join-Path $root "site\public\themes"
$framesRoot = Join-Path $root "out\theme-frames"
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
  -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe |
  Select-Object -First 1
if (-not $msbuild) { throw "MSBuild not found." }

$ffmpeg = Get-Command ffmpeg -ErrorAction SilentlyContinue
if (-not $PostersOnly -and -not $ffmpeg) {
  throw "ffmpeg not found on PATH (needed for WebM encode)."
}

$proj = Join-Path $root "src\ThemePreviewExport\ThemePreviewExport.vcxproj"
if (-not $SkipBuild) {
  & $msbuild $proj /p:Configuration=$Configuration /p:Platform=x64 /m /v:minimal
  if ($LASTEXITCODE -ne 0) { throw "ThemePreviewExport build failed" }
}

$exeCandidates = @(
  (Join-Path $root "out\$Configuration\ThemePreviewExport.exe"),
  (Join-Path $root "src\out\$Configuration\ThemePreviewExport.exe")
)
$exe = $exeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $exe) { throw "ThemePreviewExport.exe not found under out\" }

New-Item -ItemType Directory -Force -Path $outThemes | Out-Null

# Still PNG posters (intensity 100%, ThemeLab start times).
& $exe $outThemes $Width $Height
if ($LASTEXITCODE -ne 0) { throw "ThemePreviewExport posters failed" }

if ($PostersOnly) {
  Write-Host "Posters only — done in $outThemes"
  exit 0
}

# Animated Native-48px sequences → WebM.
if (Test-Path -LiteralPath $framesRoot) {
  Remove-Item -LiteralPath $framesRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $framesRoot | Out-Null

& $exe --sequence $framesRoot $Width $Height $Fps $DurationSec
if ($LASTEXITCODE -ne 0) { throw "ThemePreviewExport --sequence failed" }

$stems = @("rainbow", "sea", "weather", "space", "aurora")
$mid = [int]([math]::Floor(($Fps * $DurationSec) / 2))
foreach ($stem in $stems) {
  $dir = Join-Path $framesRoot $stem
  $pattern = Join-Path $dir "frame_%04d.png"
  $webm = Join-Path $outThemes "$stem.webm"
  Write-Host "Encoding $stem.webm ..."
  & ffmpeg -y -hide_banner -loglevel error `
    -framerate $Fps -i $pattern `
    -c:v libvpx-vp9 -b:v 0 -crf 32 -row-mt 1 -pix_fmt yuv420p `
    -an $webm
  if ($LASTEXITCODE -ne 0) { throw "ffmpeg failed for $stem" }

  # Mid-loop poster (more representative than t0).
  $midFrame = Join-Path $dir ("frame_{0:D4}.png" -f $mid)
  if (-not (Test-Path -LiteralPath $midFrame)) {
    $midFrame = Join-Path $dir "frame_0000.png"
  }
  Copy-Item -Force $midFrame (Join-Path $outThemes "$stem.png")
  Write-Host "Poster $stem.png ← $(Split-Path $midFrame -Leaf)"
}

# Remove legacy moodboard JPEGs so the site cannot accidentally use them.
Get-ChildItem -LiteralPath $outThemes -Filter "*.jpg" -ErrorAction SilentlyContinue |
  ForEach-Object {
    Write-Host "Removing moodboard $($_.Name)"
    Remove-Item $_.FullName -Force
  }

Write-Host "Theme WebMs + posters ready in $outThemes"
