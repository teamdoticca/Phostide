<#
.SYNOPSIS
  Point packaging\scoop\bucket\phostide.json at a published GitHub release.

.DESCRIPTION
  Downloads Phostide-<ver>-win-x64.zip from the release (the bytes users will get),
  hashes it and rewrites version / url / hash. Run after the release is promoted,
  then copy the manifest into the teamdoticca/scoop-bucket repo (bucket\phostide.json).

.EXAMPLE
  .\scripts\update-scoop-manifest.ps1 -Version 0.1.40
#>
param(
  [Parameter(Mandatory = $true)][string]$Version
)
$ErrorActionPreference = 'Stop'

if ($Version -notmatch '^\d{1,2}\.\d{1,2}\.\d{1,2}$') { throw "Version must be x.y.z (got '$Version')." }

$repoRoot = Split-Path $PSScriptRoot -Parent
$manifestPath = Join-Path $repoRoot 'packaging\scoop\bucket\phostide.json'
$asset = "Phostide-$Version-win-x64.zip"
$url = "https://github.com/teamdoticca/Phostide/releases/download/v$Version/$asset"

$tmp = Join-Path ([IO.Path]::GetTempPath()) "phostide-scoop-$Version"
New-Item -ItemType Directory -Force $tmp | Out-Null
gh release download "v$Version" --repo teamdoticca/Phostide -p $asset -D $tmp --clobber
if ($LASTEXITCODE -ne 0) { throw "Could not download $asset from release v$Version." }
$hash = (Get-FileHash (Join-Path $tmp $asset) -Algorithm SHA256).Hash.ToLowerInvariant()

$json = Get-Content $manifestPath -Raw | ConvertFrom-Json
$json.version = $Version
$json.architecture.'64bit'.url = $url
$json.architecture.'64bit'.hash = $hash
# Scoop's own style: 4-space indent, LF, trailing newline.
$text = ($json | ConvertTo-Json -Depth 10) -replace "`r`n", "`n"
$text = [regex]::Replace($text, '(?m)^( +)', { param($m) ' ' * ($m.Groups[1].Value.Length * 2) })
[IO.File]::WriteAllText($manifestPath, $text + "`n", [Text.UTF8Encoding]::new($false))

Write-Host "phostide.json -> $Version"
Write-Host "  url  $url"
Write-Host "  hash $hash"
