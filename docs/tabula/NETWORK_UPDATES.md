# Tabula network updates (spec, not built)

Goal: a parish X3 keeps its texts and firmware current over Wi-Fi, with no
computer, no USB, and nothing for the parishioner to understand beyond one
"Update" row.

## What gets updated

| Thing | Size | How often | Today |
|---|---|---|---|
| Daily books (Mass, Hours, 3 editions) | ~350 KB/day, ~128 MB/year | Corrections rarely; a new liturgical year once a year | Serial loader / SD card |
| Day artwork (Today header, sleep) | ~5 KB/day | With the books | Same |
| Ordo Missae, fonts, LICENSES | ~1 MB | Rarely | Same |
| Firmware | ~5.4 MB | A few times a year | USB flash |

## Device constraints

- **ESP32-C3: 2.4 GHz Wi-Fi only.** A 5 GHz-only network can't be used.
  Fallbacks: a phone hotspot (iPhone: Personal Hotspot → Maximize
  Compatibility), the parish office network, or the serial/SD path.
- No PSRAM and about 250 KB of free heap. TLS takes about 40 KB of that, so
  everything streams to the SD card in chunks and never sits in RAM.
- Upstream already has the pieces:
  - `HttpDownloader::downloadToFile` / `fetchUrl` (verified HTTPS, redirects)
  - `OtaUpdater` (GitHub-release OTA)
  - dual OTA partitions (`app0`/`app1`, 6.25 MB each) for rollback

## 1. Content sync (manifest diff)

**Publisher side (the Mac / CI):**
- `build_sd_content.py` and `render_tabula_art.py` already produce
  `build/tabula-sd`. A new step writes `Tabula/manifest.json`:
  ```json
  {"version": 42, "generated": "2026-10-28T12:00Z", "channel": "roman1962",
   "files": [{"p": "Tabula/Today/2026-10-28.full.epub", "s": 351204, "c": "9f1c22ab"}, ...],
   "delete": ["Tabula/Today/2025-10-27.full.epub"]}
  ```
  `c` is CRC32, the same check the serial loader uses. A detached
  `manifest.sig` (Ed25519) signs it, and the public key is compiled into
  the firmware.
- Upload to static hosting such as Cloudflare R2 or GitHub Releases:
  `https://<host>/tabula/<channel>/manifest.json` plus each file at its path.
  No server code.

**Device side (new `TabulaSyncActivity`):**
1. Connect with the saved network (existing `WifiSelectionActivity`).
2. GET `manifest.json` and `manifest.sig`, then verify the signature. If it
   fails, stop and keep everything as it is.
3. Stream-parse the manifest without holding it whole: about 4,000 entries,
   roughly 400 KB. Compare each entry with a local index
   `/Tabula/.index` (path, size, crc) rather than re-hashing the card.
4. Download each changed file to `<path>.part`, check its CRC, rename over
   the old file (the serial loader's pattern), and append to `.index`.
   It can resume: a restart skips files already in `.index`.
5. Apply `delete`, then clear the reader's section cache for any book that
   changed, since cached pages would show the old text.
6. Write `manifest.json` last. Turn Wi-Fi off and show
   "Updated · N texts".

**Order:** today and the next 14 days first, so a sync cut short still
leaves the coming fortnight correct. The rest of the year follows.

**Cost:** a correction is a few files and takes seconds. A new liturgical
year is about 128 MB, roughly 15–30 minutes on a 2.4 GHz link. Do it on the
charger and show progress.

## 2. Firmware OTA

- Point `OtaUpdater` at a Tabula feed under `#if TABULA`. Right now it points
  at upstream crosspoint-reader releases, so an upstream image could replace
  Tabula. The feed is
  `https://<host>/tabula/firmware/latest.json` →
  `{version, url, size, sha256, board: "x3", minContent: 42}`.
- Sign the image. The bootloader rolls back if the new app doesn't mark
  itself valid after a good boot (ESP-IDF
  `esp_ota_mark_app_valid_cancel_rollback`).
- `minContent` lets firmware wait for content it depends on. Content syncs
  first, then firmware.
- Until this lands, hide upstream's OTA row in the parish build. It's
  reachable today only through All Settings in the dev build.

## 3. When it runs

- **Phase 1:** manual. Plura → Settings → "Update texts" (also checks
  firmware). Shows last-updated date.
- **Phase 2:** automatic. When charging, on a known network, at most once a
  day. Sleep wakes, syncs quietly, and sleeps again. The sleep screen could
  show a small "updated" mark.
- The radio stays off except during a sync. No background connection.

## 4. Channels (later)

`channel` lets one firmware serve different content: `roman1962` now, and
later `roman1962-<parish>` for local feasts, a parish notice page, or a
different translation. The device stores its channel in settings, and the
parish build sets it at provisioning.

## Open questions

1. Hosting: R2 (cheap and fast, needs a Cloudflare account) or GitHub
   Releases (free, but 2 GB per file and rate limits on anonymous API calls)?
2. Who publishes: a CI job on push to the content tools, or a manual
   `publish.py` from the Mac?
3. Should a parish card hold one year, or a rolling window (for example 90
   days) so updates stay small and the card can be smaller?
4. A parishioner without any 2.4 GHz network: parish-office sync day, phone
   hotspot instructions, or swap the SD card?

## Build order (estimate)

1. `publish_manifest.py` + signing + hosting: about a day.
2. `TabulaSyncActivity` (manifest stream parse, diff, download, cache clear):
   2–3 days, plus tests on the C3's heap.
3. Repoint OTA + signed images + rollback check: about a day.
4. Auto-sync on charge: about a day, after 1–3 have run on hardware.
