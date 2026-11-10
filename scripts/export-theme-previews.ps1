param(
  [string]$Configuration = "Release",
  [int]$Width = 1920,
  [int]$Height = 48,
  [int]$Fps = 20,
  [double]$DurationSec = 12,
  # Space full film loop: tCalm=t*0.20, kPeriod=7 → real = 35s until nebula returns.
  [double]$SpaceDurationSec = 35,
  [switch]$SkipBuild,
  [switch]$PostersOnly,
  [string]$OnlyStem = ""
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

# Still PNG posters via temp (avoids locks on site/public/themes/*.png).
$posterTmp = Join-Path $root "out\theme-posters-tmp"
if (Test-Path -LiteralPath $posterTmp) { Remove-Item -LiteralPath $posterTmp -Recurse -Force }
New-Item -ItemType Directory -Force -Path $posterTmp | Out-Null
& $exe $posterTmp $Width $Height
if ($LASTEXITCODE -ne 0) { throw "ThemePreviewExport posters failed" }
Copy-Item -Force (Join-Path $posterTmp "*.png") $outThemes

if ($PostersOnly) {
  Write-Host "Posters only — done in $outThemes"
  exit 0
}

$durations = @{
  rainbow = $DurationSec
  sea     = $DurationSec
  weather = $DurationSec
  space   = $SpaceDurationSec
  aurora  = $DurationSec
}
$stems = @("rainbow", "sea", "weather", "space", "aurora")
if (-not [string]::IsNullOrWhiteSpace($OnlyStem)) {
  if (-not $durations.ContainsKey($OnlyStem)) { throw "Unknown theme stem: $OnlyStem" }
  $stems = @($OnlyStem)
}

New-Item -ItemType Directory -Force -Path $framesRoot | Out-Null

foreach ($stem in $stems) {
  $dur = [double]$durations[$stem]
  $dir = Join-Path $framesRoot $stem
  if (Test-Path -LiteralPath $dir) { Remove-Item -LiteralPath $dir -Recurse -Force }
  Write-Host "Capturing $stem ($dur s @ ${Fps}fps) ..."
  & $exe --sequence $framesRoot $Width $Height $Fps $dur $stem
  if ($LASTEXITCODE -ne 0) { throw "ThemePreviewExport --sequence failed for $stem" }

  $pattern = Join-Path $dir "frame_%04d.png"
  $webm = Join-Path $outThemes "$stem.webm"
  Write-Host "Encoding $stem.webm ..."
  & ffmpeg -y -hide_banner -loglevel error `
    -framerate $Fps -i $pattern `
    -c:v libvpx-vp9 -b:v 0 -crf 32 -row-mt 1 -pix_fmt yuv420p `
    -an $webm
  if ($LASTEXITCODE -ne 0) { throw "ffmpeg failed for $stem" }

  $mid = [int]([math]::Floor(($Fps * $dur) / 2))
  $midFrame = Join-Path $dir ("frame_{0:D4}.png" -f $mid)
  if (-not (Test-Path -LiteralPath $midFrame)) {
    $midFrame = Join-Path $dir "frame_0000.png"
  }
  Copy-Item -Force $midFrame (Join-Path $outThemes "$stem.png")
  Write-Host "Poster $stem.png ← $(Split-Path $midFrame -Leaf)"
}

Get-ChildItem -LiteralPath $outThemes -Filter "*.jpg" -ErrorAction SilentlyContinue |
  ForEach-Object {
    Write-Host "Removing moodboard $($_.Name)"
    Remove-Item $_.FullName -Force
  }

Write-Host "Theme WebMs + posters ready in $outThemes"
