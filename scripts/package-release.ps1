param(
  [string]$Configuration = "Release",
  # Keep in sync with src/Common/PhostideCommon.h kAppVersion and Host VERSIONINFO.
  [string]$Version = "0.1.5",
  [switch]$SkipBuild,
  [switch]$SkipCodeSign,
  [switch]$SkipInno,
  [string]$TrustedSigningMetadata = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $root "out\$Configuration"
$stage = Join-Path $root "out\stage\Phostide-$Version-win-x64"
$zip = Join-Path $root "out\Phostide-$Version-win-x64.zip"
$setup = Join-Path $root "out\Phostide-Setup-$Version-win-x64.exe"
$iss = Join-Path $root "installer\Phostide.iss"
$defaultTsConfig = Join-Path $root "installer\trusted-signing.json"

function Resolve-SignToolPath {
  $sdkBin = Join-Path ${env:ProgramFiles(x86)} "Windows Kits\10\bin"
  if (Test-Path -LiteralPath $sdkBin) {
    $hit = Get-ChildItem -LiteralPath $sdkBin -Directory |
      Sort-Object Name -Descending |
      ForEach-Object {
        $candidate = Join-Path $_.FullName "x64\signtool.exe"
        if (Test-Path -LiteralPath $candidate) { $candidate }
      } |
      Select-Object -First 1
    if ($hit) { return $hit }
  }
  $cmd = Get-Command signtool.exe -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Source }
  throw "signtool.exe not found — install Windows SDK 10 (10.0.22621+) Build Tools"
}

function Resolve-AzureCodeSigningDlibPath {
  $nuget = Join-Path $env:USERPROFILE ".nuget\packages\microsoft.trusted.signing.client"
  if (Test-Path -LiteralPath $nuget) {
    $hit = Get-ChildItem -LiteralPath $nuget -Recurse -Filter "Azure.CodeSigning.Dlib.dll" -ErrorAction SilentlyContinue |
      Where-Object { $_.FullName -match '[\\/]bin[\\/]x64[\\/]' } |
      Sort-Object FullName -Descending |
      Select-Object -First 1
    if ($hit) { return $hit.FullName }
  }
  throw "Azure.CodeSigning.Dlib.dll not found — nuget install Microsoft.Trusted.Signing.Client"
}

function Resolve-TrustedSigningMetadataPath {
  param([string]$Override)
  if (-not [string]::IsNullOrWhiteSpace($Override)) {
    $p = $Override
    if (-not [System.IO.Path]::IsPathRooted($p)) { $p = Join-Path $root $p }
    return (Resolve-Path -LiteralPath $p).Path
  }
  $envPath = $env:PHOSTIDE_TRUSTED_SIGNING_METADATA
  if (-not [string]::IsNullOrWhiteSpace($envPath)) {
    return (Resolve-Path -LiteralPath $envPath).Path
  }
  $localOverride = Join-Path $root "installer\trusted-signing.local.json"
  $source = if (Test-Path -LiteralPath $localOverride) {
    $localOverride
  } elseif (Test-Path -LiteralPath $defaultTsConfig) {
    $defaultTsConfig
  } else {
    throw "Trusted Signing metadata not found. Expected installer/trusted-signing.json (or trusted-signing.local.json)."
  }
  $cfg = Get-Content -LiteralPath $source -Raw | ConvertFrom-Json
  foreach ($required in @("Endpoint", "CodeSigningAccountName", "CertificateProfileName")) {
    $hasField = $cfg.PSObject.Properties.Name -contains $required
    if (-not $hasField -or [string]::IsNullOrWhiteSpace([string]$cfg.$required)) {
      throw "Trusted Signing metadata missing required field: $required ($source)"
    }
  }
  $meta = @{
    Endpoint = $cfg.Endpoint
    CodeSigningAccountName = $cfg.CodeSigningAccountName
    CertificateProfileName = $cfg.CertificateProfileName
  }
  $tmp = Join-Path $env:TEMP "phostide-trusted-signing.metadata.json"
  $json = $meta | ConvertTo-Json -Compress
  [System.IO.File]::WriteAllText($tmp, $json, [System.Text.UTF8Encoding]::new($false))
  return $tmp
}

function Ensure-AzTrustedSigningAuth {
  # Prefer env override; never commit subscription GUIDs. Default is the Doticca MPN display name.
  param([string]$Subscription = $(
    if (-not [string]::IsNullOrWhiteSpace($env:PHOSTIDE_AZURE_SUBSCRIPTION)) {
      $env:PHOSTIDE_AZURE_SUBSCRIPTION
    } else {
      "Microsoft Partner Network"
    }
  ))
  $null = az account show -o none 2>$null
  if ($LASTEXITCODE -ne 0) {
    Write-Host "Azure CLI: sign in for Artifact Signing..."
    az login --only-show-errors
    if ($LASTEXITCODE -ne 0) { throw "az login failed" }
  }
  az account set --subscription $Subscription --only-show-errors
  if ($LASTEXITCODE -ne 0) {
    throw "Could not select Azure subscription '$Subscription' for code signing (set PHOSTIDE_AZURE_SUBSCRIPTION if needed)"
  }
}

function Invoke-TrustedSignFile {
  param(
    [Parameter(Mandatory)][string]$FilePath,
    [Parameter(Mandatory)][string]$MetadataPath
  )
  if (-not (Test-Path -LiteralPath $FilePath)) {
    throw "Cannot sign missing file: $FilePath"
  }
  $signtool = Resolve-SignToolPath
  $dlib = Resolve-AzureCodeSigningDlibPath
  Write-Host "Signing $(Split-Path $FilePath -Leaf) via Artifact Signing..."
  & $signtool sign /v /fd SHA256 /td SHA256 `
    /tr "http://timestamp.acs.microsoft.com" `
    /dlib $dlib `
    /dmdf $MetadataPath `
    $FilePath
  if ($LASTEXITCODE -ne 0) {
    throw "signtool sign failed ($LASTEXITCODE) for $FilePath"
  }
  & $signtool verify /pa /q $FilePath
  if ($LASTEXITCODE -ne 0) {
    throw "signtool verify failed ($LASTEXITCODE) for $FilePath"
  }
  Write-Host "Signed OK: $(Split-Path $FilePath -Leaf)"
}

function Resolve-IsccPath {
  $candidates = @(
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "${env:ProgramFiles}\Inno Setup 6\ISCC.exe",
    "${env:ProgramFiles}\Inno Setup 7\ISCC.exe"
  )
  foreach ($c in $candidates) {
    if (Test-Path -LiteralPath $c) { return $c }
  }
  $cmd = Get-Command ISCC.exe -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Source }
  throw "ISCC.exe not found — install Inno Setup 6+"
}

if (-not $SkipBuild -or -not (Test-Path (Join-Path $outDir "Phostide.exe"))) {
  Write-Host "Building native solution..."
  $msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
    -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe |
    Select-Object -First 1
  if (-not $msbuild) { throw "MSBuild not found. Install VS 2022 C++ workload." }
  & $msbuild (Join-Path $root "src\Phostide.sln") /p:Configuration=$Configuration /p:Platform=x64 /m
  if ($LASTEXITCODE -ne 0) { throw "MSBuild failed ($LASTEXITCODE)" }
}

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item (Join-Path $outDir "Phostide.exe") $stage -Force
Copy-Item (Join-Path $outDir "ExplorerTAP.dll") $stage -Force
Copy-Item (Join-Path $root "README.md") $stage -Force
Copy-Item (Join-Path $root "LICENSE") $stage -Force

$metadataPath = $null
if (-not $SkipCodeSign) {
  $metadataPath = Resolve-TrustedSigningMetadataPath -Override $TrustedSigningMetadata
  Ensure-AzTrustedSigningAuth
  Invoke-TrustedSignFile -FilePath (Join-Path $stage "Phostide.exe") -MetadataPath $metadataPath
  Invoke-TrustedSignFile -FilePath (Join-Path $stage "ExplorerTAP.dll") -MetadataPath $metadataPath
}

if (-not $SkipInno) {
  if (-not (Test-Path -LiteralPath $iss)) { throw "Missing Inno script: $iss" }
  $iscc = Resolve-IsccPath
  Write-Host "Compiling installer with $iscc ..."
  & $iscc `
    "/DMyAppVersion=$Version" `
    "/DStageDir=$stage" `
    "/DOutputDir=$(Join-Path $root 'out')" `
    "/DSetupIcon=$(Join-Path $root 'assets\brand\setup-phostide.ico')" `
    $iss
  if ($LASTEXITCODE -ne 0) { throw "ISCC failed ($LASTEXITCODE)" }
  if (-not (Test-Path -LiteralPath $setup)) {
    throw "Expected setup output missing: $setup"
  }
  if (-not $SkipCodeSign) {
    Invoke-TrustedSignFile -FilePath $setup -MetadataPath $metadataPath
  }
  Write-Host "Installer: $setup"
}

if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip
Write-Host "Packed $zip"

Write-Host @"
Publish with:
  gh release create v$Version `"$setup`" `"$zip`" --title `"Phostide $Version`" --generate-notes
"@
