<#
.SYNOPSIS
  Creates Cloudflare DNS records for phostide.doticca.com (Vercel).

.DESCRIPTION
  Requires Cloudflare CLI auth for the account that owns the doticca.com zone.
#>
param(
  [string]$ZoneName = "doticca.com",
  [string]$CnameTarget = "cname.vercel-dns.com",
  [string]$VerifyTxt = "vc-domain-verify=phostide.doticca.com,246b5dee5a05d95fee9f"
)

$ErrorActionPreference = "Stop"

Write-Host "Looking up zone $ZoneName ..."
$zones = cf zones list --json | ConvertFrom-Json
$zone = @($zones) | Where-Object { $_.name -eq $ZoneName } | Select-Object -First 1
if (-not $zone) {
  Write-Host "Available zones:"
  @($zones) | ForEach-Object { Write-Host (" - {0}" -f $_.name) }
  throw "Zone $ZoneName not visible. Authenticate the Cloudflare account that owns doticca.com."
}

function New-DnsRecord([hashtable]$Body) {
  $json = $Body | ConvertTo-Json -Compress
  Write-Host ("Creating {0} {1} ..." -f $Body.type, $Body.name)
  cf dns records create --zone-id $zone.id --json --body $json
}

New-DnsRecord @{
  type = "TXT"
  name = "_vercel"
  content = $VerifyTxt
  ttl = 60
  proxied = $false
  comment = "Vercel domain verification for phostide.doticca.com"
}

New-DnsRecord @{
  type = "CNAME"
  name = "phostide"
  content = $CnameTarget
  ttl = 1
  proxied = $false
  comment = "Phostide Vercel custom domain (DNS only)"
}

Write-Host "Done. Next: verify domain in Vercel (phostide project)."
