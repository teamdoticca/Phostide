# Phostide ops checklist

## Done

- Local repo: `Z:\Doticca\Phostide`
- GitHub: https://github.com/teamdoticca/Phostide (**public**)
- Release: https://github.com/teamdoticca/Phostide/releases/tag/v0.1.0
- Vercel project: `phostide` (Doticca team) — production live at https://phostide.vercel.app
- Custom domain live: **https://phostide.doticca.com** → HTTP 200
- Cloudflare DNS on `doticca.com`:
  - `CNAME` `phostide` → `cname.vercel-dns.com` (DNS-only / grey cloud)
  - Vercel domain verification completed

## Notes

Records for reference remain in `scripts/dns-records.json` and `scripts/setup-dns.ps1`.
Re-run setup only if the zone is rebuilt or the domain is re-added on Vercel.
