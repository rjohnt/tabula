# Tabula Parish Edition: system design

**Decisions (Oct 26, 2026):** updates are hosted at `tabula.acutisignatius.com` (Cloudflare Pages). The repo is the public fork `rjohnt/tabula`, which satisfies GPL for wolfSSL. Bluetooth is off for v1. "Hours" means the Liturgy of the Hours, delivered as daily content updates (§4–5).

Base: CrossPoint Reader `develop` @ `0727f9f`. See `handoff.md` for setup, hardware, and content notes.

There are two build targets:

| Env | Who | What |
|---|---|---|
| `tabula_dev` | Ryan | Tabula branding, all CrossPoint features. Used for development and service work. |
| `tabula_parish` | Parishioners | `-DTABULA_PARISH=1`. Locked down as described below. This is the only build that ships. |

## 1. Goals for parishioners

1. **They can't get stuck.** Nothing they can reach leaves the device unreadable or unrecoverable.
2. **They never see CrossPoint.** Home and Settings are Tabula screens with Tabula words.
3. **It stays current without a computer.** Content, time, and (rarely) firmware arrive over Wi-Fi.

## 2. What a parishioner sees

**Home**
```
  [ Continue: Missale Romanum — XXII Sunday after Pentecost ]
  Today                → today's Mass propers / readings (see §5)
  Missal               → today's Mass (propers or full, per setting) + /Tabula/Missal
  Order of Mass        → sequence view (§5a)
  Readings             → /Tabula/Readings
  Prayers & Hours      → /Tabula/Hours
  Library              → /Tabula/Literature, /Tabula/Letters
  Settings             → Tabula Settings (below)
```

**Tabula Settings** (one flat list, no CrossPoint sections)
```
  Text size            S / M / L / XL
  Font                 Serif / Sans
  Line spacing         Normal / Wide
  Mass text            Propers only / Full Mass
  Page-turn buttons    Standard / Swapped
  Sleep after          5 / 10 / 30 min
  Wi-Fi                Join a network… / Set up from phone…
  Date & time          Time zone · Sync now · Set manually
  Check for updates    content + firmware, Tabula server only
  Restore defaults
  About Tabula         version, serial, attribution, parish contact
```

## 3. Lock-out analysis: what exists upstream and what we do with it

| Upstream path that can strand a user | Where | Parish build |
|---|---|---|
| Language set to one they can't read | `SettingsActivity.cpp:110` | Removed. Built with `en` only (`custom_i18n_builtin_langs`) |
| Front-button remap | `SettingsActivity.cpp:117` | Replaced by a two-preset "Standard / Swapped" toggle that can't produce an unusable map |
| Delete / rename / move in the file browser | `FileBrowserActivity.cpp` long-press | Disabled. Browser is read-only and rooted at `/Tabula` |
| **Check for updates → stock CrossPoint** | `OtaUpdater.cpp:22`, `SettingsActivity.cpp:146` | Repointed to the Tabula server. Accepts **signed Tabula images only** |
| **Recovery mode** (hold Up + Power at boot → SD .bin picker) | `main.cpp:618`, `main.cpp:780` | Kept as the service lifeline. Accepts **signed Tabula images only** |
| SD firmware update | `SettingsActivity.cpp:147` | Service menu only. Signed images only |
| Web file manager / WebDAV / Calibre / USB drive | `src/activities/network/*`, `CrossPointWebServer.cpp:201-266` | Removed, except a Wi-Fi-setup-only page (see §6) |
| Plugins, OPDS, KOReader sync, font download | `SettingsActivity.cpp:113,137-139`, `HomeActivity.cpp:527` | Removed from UI. Phase 2 removes the code |
| Settings combinations that look broken | `CrossPointSettings.h` | Parish defaults set in code. "Restore defaults" in Settings, plus a boot combo (below) |

**Recovery paths for parishioners (no computer):**
- **Restore defaults** in Settings.
- **Boot reset:** hold Back + Power at boot. Deletes `/.crosspoint/settings.json` and returns to parish defaults. Content stays.
- **Service menu (Ryan):** on Home, hold Back + Confirm for 5 s, then press Up, Up, Down. This opens the full CrossPoint settings and the SD firmware update for that session only, and isn't persisted.

**Signed firmware.** Recovery and OTA verify an ECDSA-P256 signature appended to the image. ESP-IDF's mbedTLS does the check, and the public key is compiled in. The private key never leaves your Mac or the CI secret. As a result, a stray `.bin` on the SD card, stock CrossPoint, or another fork can't be installed through any on-device path. USB/esptool flashing still works. We deliberately do **not** burn Secure Boot eFuses: that's irreversible and would block our own recovery of a bad unit. Parishioners won't be running esptool.

## 4. How a device stays updated without reflashing

Everything that changes often is **data on the SD card**, not firmware.

```
Tabula server (static files: Cloudflare Pages / GitHub Releases)
  manifest.json          signed; lists files + sha256 + min firmware
  content/Missal/…       EPUBs
  content/Today/2026-11/ per-day propers & readings
  content/Hours/…        Liturgy of the Hours / prayers
  content/parish.json    Mass & confession times, announcements, contact
  firmware/tabula-1.0.3.bin  signed; only if manifest says so
```

On-device **Tabula Update** (new, small `TabulaUpdater` reusing `HttpDownloader` + `FirmwareFlasher`):
1. Join a saved Wi-Fi network (`WifiCredentialStore`, already on SD).
2. `trustedtime::syncNow()`: the clock gets set on every update.
3. Fetch the manifest and verify its signature. Download only the files whose sha256 differs. Write to a temp file, then rename, so a power loss leaves the old file intact.
4. If the manifest names newer firmware, download it into the spare OTA slot and verify the signature before switching. The A/B partitions (`partitions.csv`) let a bad image roll back.
5. Show "Updated: 12 items" and turn Wi-Fi off.

Triggers: **Check for updates** in Settings (manual). Optionally, at most once a day when the device goes to sleep with >40% battery. This reuses the existing sleep-time Wi-Fi join in `main.cpp:395-420`.

The manifest also carries **remote config** (e.g. parish name, default time zone, which Home tiles show). Small behavior changes then don't need a firmware release.

## 5. Time and "Today"

- **NTP** syncs whenever Wi-Fi is up (`trustedtime::startSync()` already runs on every station connect: `WifiSelectionActivity.cpp:534`, `main.cpp:418`).
- **X3:** a battery-backed RTC keeps time through power-off (`main.cpp:675` adopts it at boot). Correct time without Wi-Fi.
- **X4:** no RTC. The clock falls back to the last-seen time persisted in NVS (`lib/TrustedTime`) and is corrected at the next sync. If `trustedNow()` returns 0, "Today" asks "Which day?" instead of guessing.
- **Time zone** defaults to the parish's zone (POSIX rule handles DST: `timezones::applyToClock()`), and users can change it in Date & time.
- **Today** opens `/Tabula/Today/YYYY-MM-DD.*` when present. Otherwise it opens the year EPUB at that date's chapter. Liturgical calendar logic stays server-side, so the firmware never has to compute Easter or 1962 rubrics, and calendar fixes ship as data.

## 5a. Missal content model

Each day's Mass comes in two forms. A setting picks which one **Today** and **Missal** open.

| Setting: *Mass text* | Opens | Content |
|---|---|---|
| **Propers only** (default) | `/Tabula/Today/YYYY-MM-DD.propers.epub` | Introit, Collect, Epistle, Gradual/Alleluia/Tract, Gospel, Offertory, Secret, Communion, Postcommunion, plus any proper Preface/Communicantes. Short cue lines mark where the Ordinary goes, e.g. *"Kyrie · Gloria → Order of Mass"* |
| **Full Mass** | `/Tabula/Today/YYYY-MM-DD.full.epub` | The Ordinary with that day's propers interleaved, read straight through |

The server builds both files from Divinum Officium's Missa output. The firmware doesn't assemble Mass texts. A new rubric or a correction ships as data.

**Order of Mass (a separate Home entry):** a native screen that shows the Mass as a vertical sequence, so a parishioner can see where they are at a glance:

```
  ORDER OF MASS                        Today: XXII Sun. after Pentecost
  ○  Prayers at the Foot of the Altar     kneel
  ●  Introit                     proper   stand
  ○  Kyrie
  ○  Gloria
  ●  Collect                     proper
  ●  Epistle                     proper   sit
  ●  Gradual · Alleluia          proper
  ●  Gospel                      proper   stand
  ○  Credo
  ── Mass of the Faithful ──
  ●  Offertory                   proper
  …
  ○  Canon                                kneel
  ○  Last Gospel                          stand
  ○ = Ordinary (same every day)   ● = Proper (today's text)
```

- Selecting **○** opens `/Tabula/Missal/Ordo-Missae.epub` at that part.
- Selecting **●** opens today's propers file at that part.
- The steps, their labels (Latin + English), postures, and section markers come from `/Tabula/Missal/ordo.json`. Server updates can fix or translate the sequence without firmware.
- Dots, rules, and text only: 1-bit drawing, paged with the buttons. No images are needed.
- Opening a part uses `ActivityManager::goToReader(path, false, "<id>.xhtml")`. That one-shot start href overrides saved progress (`EpubReaderActivity::loadBook`).

**Content convention (the server's EPUB builder must follow it):** every Mass part is its own chapter file named after its `ordo.json` step id: `introit.xhtml`, `collect.xhtml`, `kyrie.xhtml`, and so on. That applies both to `Ordo-Missae.epub` (the Ordinary) and to each `YYYY-MM-DD.propers.epub`. Href matching is by file name (`Epub::resolveHrefToSpineIndex`), so folder layout inside the EPUB doesn't matter.

Live `ordo.json`: `docs/tabula/sd/Tabula/Missal/ordo.json`. Previews: `docs/tabula/tools/render_ordo_previews.py` (1-bit, 528×792, device fonts).

## 6. Wi-Fi and Bluetooth

**Wi-Fi setup, three ways (easiest first):**
1. **Preloaded:** parish Wi-Fi goes onto the golden SD image (`WifiCredentialStore` JSON). Units update at church with no setup.
2. **Set up from phone:** the device opens a hotspot "Tabula-XXXX" and shows a QR code. The phone opens a one-page form (SSID + password). This reuses the hotspot mode and `/api/wifi` from `CrossPointWebServer.cpp:264-266`, with every other route compiled out.
3. **Join a network:** the existing scan + on-screen keyboard (`WifiSelectionActivity`). Works, but it's tedious on buttons.

Wi-Fi is off except during setup and updates, to save battery.

**Bluetooth:** upstream BLE is only the remote page turner (`lib/BlePageTurner`). It can't do setup or updates without a phone app. Default: off in the parish build (it saves RAM and flash). Could return later as a "Page-turn remote" toggle.

## 7. Licensing consequence

wolfSSL is the firmware's only TLS stack (`HttpDownloader.cpp:41`) and is GPLv2. HTTPS updates keep it in the binary, so the shipped firmware's source must be available. The easiest way is a **public fork**. The alternative is plain-HTTP downloads protected by the signature check (integrity without TLS), which lets wolfSSL be removed. That's a phase 2 option if a private repo matters.

## 8. Build order

1. Stock `pio run -e default` builds on the Mac; flash a test unit.
2. `tabula_dev` / `tabula_parish` envs + branding (name, logo, splash, sleep, About attribution).
3. Parish Home + Tabula Settings + read-only browser rooted at `/Tabula`.
4. Gate removed features behind `TABULA_PARISH`; boot reset; service menu.
5. Firmware signing (keygen script, post-build sign step, verify in `FirmwareFlasher`).
6. `TabulaUpdater` + manifest format + server layout; repoint "Check for updates".
7. Phone Wi-Fi setup page; Today; parish.json screen (Mass times).
8. Hardware pass on 2–3 X3 units, then provisioning script.

## 9. Credits and attribution

These appear in three places: **About Tabula** on the device, `/LICENSES.txt` on the SD card, and the printed quick-start card.

| Credit | License | Why |
|---|---|---|
| CrossPoint Reader © CrossPoint Reader organization | MIT | Firmware base |
| FreeInk SDK © FreeInk | MIT | Board drivers (`freeink-sdk/LICENSE`, `NOTICE`) |
| **Divinum Officium** © Divinum Officium (divinumofficium.com, github.com/DivinumOfficium/divinum-officium) | MIT | Source of the Liturgy of the Hours and 1962 Missal propers that the Tabula server builds into daily content |
| wolfSSL | GPLv2 | TLS for updates. Source is offered via github.com/rjohnt/tabula |

On-device About text (short):
> Tabula is based on CrossPoint Reader and the FreeInk SDK (MIT). Liturgical texts from the Divinum Officium Project (divinumofficium.com). Source: github.com/rjohnt/tabula

⚠️ Divinum Officium's MIT license covers the project. The Latin texts are public domain, but some English translations in the project may have their own provenance. Check the source of each translation we ship (`web/www/horas/English/…`) before the first parish sale.
