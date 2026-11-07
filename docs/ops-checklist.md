# Phostide ops checklist

## Done

- Local repo: `Z:\Doticca\Phostide`
- GitHub: https://github.com/teamdoticca/Phostide
- Release: https://github.com/teamdoticca/Phostide/releases/tag/v0.1.0
- Vercel project: `phostide` (Doticca team) — production live at https://phostide.vercel.app
- Custom domain **added** on Vercel: `phostide.doticca.com` (pending DNS)

## Remaining: Cloudflare DNS for doticca.com

The authenticated Cloudflare session / `cf` CLI only sees **intale.net**, not **doticca.com** (still on Cloudflare NS `rick`/`harleigh`).

1. Authenticate the Cloudflare account that owns `doticca.com` (`cf` login / MCP re-auth).
2. Run:

```powershell
.\scripts\setup-dns.ps1
```

This creates:

| Type | Name | Content | Proxied |
|------|------|---------|---------|
| TXT | `_vercel` | `vc-domain-verify=phostide.doticca.com,246b5dee5a05d95fee9f` | no |
| CNAME | `phostide` | `cname.vercel-dns.com` | **no** (grey cloud) |

3. Then verify:

```text
Vercel → project phostide → Domains → verify phostide.doticca.com
```

Records are also listed in `scripts/dns-records.json`.
