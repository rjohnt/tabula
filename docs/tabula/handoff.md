# Tabula — CrossPoint Fork Handoff

**For:** Ryan Timmons, plus the AI coding agent he hands this to
**Written:** Mon Oct 26, 2026, 9:45 AM CT
**Upstream inspected:** `crosspoint-reader/crosspoint-reader`, branch `develop`, commit `0727f9f` (Oct 26, 2026); submodule `freeink-sdk` at `44427f8`
**Status legend:** ✅ seen in the repo or official docs · ⚠️ not verified / inferred

---

## 1. Project goal

Build **Tabula**, a rebranded fork of the CrossPoint Reader firmware for the **Xteink X3** (X4 and X4 Pro are possible alternates). Tabula should:

- Show a **Tabula boot/splash screen** and Tabula sleep screen.
- Boot straight into a **Tabula home** that lists preloaded Catholic content: the traditional Latin Mass missal, daily readings, literature, and letters.
- Have **no AI features** and no unneeded network features (OPDS, KOReader sync, plugins, OTA from upstream).
- Offer a locked-down **"Parish Edition"** mode for 25–50 units sold at cost through the parish store, with proceeds going to the church.

---

## 2. Upstream repo and license

| Item | Value |
|---|---|
| Repo | https://github.com/crosspoint-reader/crosspoint-reader ✅ |
| Default branch | `develop` ✅ (crosspoint-tools CI builds from `master` ⚠️ naming differs) |
| Version in `platformio.ini` | `[crosspoint] version = 2.0.0` ✅ |
| Activity | Very active. Multiple commits per day (5 on Oct 25–26, 2026). Main contributors in the last month: Sung-jin Brian Hong, Uri Tauber, Eszter Schuffert, Phạm Bình An, Justin Mitchell ✅. Moderation contacts in `GOVERNANCE.md`: @itsthisjustin, @Uri-Tauber. Original author of early releases: @daveallie ✅ |
| Size | About 8.5k stars and 2.1k forks (GitHub API via search) ⚠️ |
| Website / web flasher | https://crosspointreader.com (`#flash-tools`, `#unlock-tool`) ✅ (README) |

### License obligations (MIT) ✅
- `LICENSE`: "MIT License, Copyright (c) 2025 CrossPoint Reader organization".
- **Submodule `freeink-sdk/`** is a separate MIT license: "Copyright (c) 2026 FreeInk" (`freeink-sdk/LICENSE`, plus `freeink-sdk/NOTICE`). Keep both.
- Third-party libs pulled by PlatformIO have their own licenses (wolfSSL, SdFat, expat, miniz, NimBLE, ArduinoJson, JPEGDEC/PNGdec/AnimatedGIF, lucide icons…). ⚠️ **wolfSSL is GPLv2 / commercial**, not MIT. If Tabula still links it, the binaries you sell may carry GPL obligations (offer source). The simplest fix is to strip the networking and content-protection code that needs wolfSSL (see §7.4). **Have someone check this before selling.**
- **What to do:** keep `LICENSE` and `freeink-sdk/LICENSE`/`NOTICE` in the fork; add a `NOTICE`/About screen line: "Tabula is based on CrossPoint Reader (MIT) © CrossPoint Reader organization and FreeInk SDK (MIT) © FreeInk." Include the same text on a printed insert or in a `/LICENSES.txt` on the SD card. You don't have to publish your fork's source under MIT, but doing so is easy and friendly.
- README: "CrossPoint Reader is **not affiliated with Xteink**". Don't imply Tabula is endorsed by CrossPoint or Xteink.

### Community forks (README "Community forks") ✅
CrossInk (uxjulia/CrossInk), papyrix-reader (bigbag), inx (obijuankenobiii), Witch(hunt) Reader (jpirnay). Fork CrossPoint itself, not one of these.

---

## 3. Hardware and targets ✅

| Device | Chip | Panel | PlatformIO env | Notes |
|---|---|---|---|---|
| **Xteink X3** | ESP32-C3, 16 MB flash, **no PSRAM**, ~380 KB usable RAM | 792×528, UC8253 **or** UC8279d (newer units, auto-detected) | `default` / `gh_release` / `slim` | Pogo-pin charging (no USB-C on device). Has gyroscope (tilt page turn) and RTC |
| Xteink X4 | ESP32-C3 (same binary as X3) | 800×480 SSD1677 / UC8179 | same `default` | USB-C |
| Xteink X4 Pro | **ESP32-S3**, 16 MB flash, 8 MB PSRAM | 800×480 | `x4pro` / `x4pro-gh_release` | **Separate binary**, touch, frontlight, USB mass-storage |
| X4 Classic | ESP32-S3 | 800×480 | `x4c` | buttons only |

- **One C3 binary covers both X3 and X4**: `env:default` sets `-DFREEINK_DEVICE_X4=1 -DFREEINK_DEVICE_X3=1`, and the board is detected at boot (`freeink-sdk/libs/hardware/XteinkDetect`, `BoardConfig.h` ~L985–1110).
- Ryan's own $99 unit is probably the X4 Pro (S3). **If you test on it, build `x4pro`, not `default`.** For the parish run on X3s, build `default`/`gh_release`.
- Sleep/cover images: X3 = **528×792**, X4 = 480×800 (USER_GUIDE §3.7).
- Partition table `partitions.csv`: nvs, otadata, app0 @0x10000 (6.25 MB), app1 @0x650000 (6.25 MB), spiffs 3.4 MB, coredump. Two OTA slots.

### Region-locked units ✅
README "USB-locked devices": units from AliExpress/Taobao (the Chinese version) may have USB flashing locked. **Units from xteink.com are not locked.** The Xteink Unlocker (crosspointreader.com/#unlock-tool) officially supports only CrossPoint and CrossInk. Flashing anything else onto a locked unit "may permanently brick the device." **Buy only global units from xteink.com or Xteink's official Amazon store ("Developer Edition").** Current price is $69 on xteink.com (sale from $79) and $79 on Amazon Developer Edition ✅ (search, Oct 2026).

---

## 4. Build system ✅

- **PlatformIO with the _pioarduino_ fork**, **Arduino framework** on top of ESP-IDF (pioarduino `platform-espressif32` 55.03.311, hybrid "custom_sdkconfig" core rebuild). Board `esp32-c3-devkitm-1`.
- Pre/post build Python scripts in `scripts/` (patch_wolfssl, build_html, gen_i18n, git_branch, patch_sdfat, …). Strings are generated from YAML (`lib/I18n/translations/*.yaml`) by `scripts/gen_i18n.py`.
- `platformio.local.ini` (git-ignored) is for personal overrides (ports, flags). It is loaded by `extra_configs`.
- Toolchain pins (from `docs/contributing/getting-started.md`): pioarduino Core **v6.1.19**, Python 3.13, clang-format 21.
- Repo ships `AGENTS.md` (also `CLAUDE.md`) and `.agents/rules/` and `.agents/skills/`. **Your AI agent should read `AGENTS.md` first.** Note that it forbids pushing or committing without human approval. That's fine for a fork, too.
- ⚠️ I started a test build on Linux but stopped it when the handoff was requested. **A full build of `default` has not been verified.** The first build rebuilds the Arduino core and is slow (tens of minutes, likely). `platformio.ini` L190–191 says the custom-core rebuild "needs the CMake pin in platformio.local.ini on macOS". **I could not find what that pin is.** Watch for CMake errors on the Mac (see §5 troubleshooting).

---

## 5. Mac (M5, Apple Silicon) dev setup

```bash
# 1. Tools
xcode-select --install                     # git, compilers
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"   # if no Homebrew
brew install python@3.13 clang-format cmake  # cmake: ⚠️ see CMake note below

# 2. Fork on GitHub first (github.com/crosspoint-reader/crosspoint-reader → Fork → e.g. ryantimmons/tabula), then:
git clone --recursive https://github.com/<you>/tabula.git
cd tabula
git remote add upstream https://github.com/crosspoint-reader/crosspoint-reader.git
git config core.hooksPath .githooks && chmod +x .githooks/pre-commit

# 3. Pinned pioarduino core in a local venv (from docs/contributing/getting-started.md)
python3.13 -m venv .venv
. .venv/bin/activate
python -m pip install --upgrade 'https://github.com/pioarduino/platformio-core/archive/refs/tags/v6.1.19.zip'
pio --version                               # expect 6.1.19

# 4. Pin the nested core used by custom_sdkconfig profiles (default, gh_release, slim)
python -m pip install uv
pio pkg install -e default
uv pip install --python ~/.platformio/penv/bin/python 'pioarduino==6.1.19'

# 5. Build the stock firmware (C3: X3 + X4)
pio run -e default
#   X4 Pro instead:  pio run -e x4pro
# output: .pio/build/default/firmware.bin
```

- **USB drivers:** none needed. The X3/X4 use the ESP32-C3's native USB Serial/JTAG (`-DARDUINO_USB_MODE=1 -DARDUINO_USB_CDC_ON_BOOT=1`). It appears as `/dev/cu.usbmodemXXXX`. Find it with `ls /dev/cu.usbmodem*`.
- X3 has pogo pins: you need the **Xteink magnetic pogo cable that carries data** (xteink.com, $4.99). ⚠️ Confirm the $9 Amazon two-pack carries data, not just power.
- Troubleshooting from `platformio.ini`: if an interrupted build fails with "multiple definition of 'app_main'", run `rm -rf .dummy CMakeLists.txt sdkconfig.default sdkconfig.defaults .pio/build/default`. **Do not run `git clean -fdX`** (it deletes `platformio.local.ini`).

---

## 6. Flash a stock build first (do this before changing anything)

1. **Back up the factory firmware** (one time, on one unit) so you can always restore:
   ```bash
   pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32c3 -p /dev/cu.usbmodemXXXX -b 921600 \
     read_flash 0x000000 0x1000000 x3_factory_backup.bin
   ```
   (Command adapted from `docs/fix-bricked-xteink.md`. Xteink stock firmware can also be restored from crosspointreader.com flasher.)
2. **Flash** (wake/unlock the device, connect USB):
   - From source: `pio run -e default -t upload` (add `--upload-port /dev/cu.usbmodemXXXX` if needed), or
   - esptool: `esptool.py --chip esp32c3 --port /dev/cu.usbmodemXXXX --baud 921600 write_flash 0x10000 .pio/build/default/firmware.bin` (README "Command line"), or
   - Web flasher: Chrome/Edge → https://crosspointreader.com/#flash-tools → select X3 → **"Custom .bin"** → upload your `firmware.bin`. (WebSerial: Chrome/Edge only, not Safari. The web flasher writes to the spare OTA slot and flips the boot selector.)
3. **Serial logs:** `python3 -m pip install pyserial colorama matplotlib && python3 scripts/debugging_monitor.py /dev/cu.usbmodemXXXX`
4. Confirm the stock build boots, opens an EPUB, and sleeps and wakes. **Only then start Tabula changes.**

---

## 7. Tabula change list (files actually seen)

Make each step a separate commit and build and flash after each.

### 7.1 Rename and branding
- `lib/I18n/translations/english.yaml` L6: `STR_CROSSPOINT: "CrossPoint"` → `"Tabula"`. It is used on the boot and sleep screens and as the KOReader server-type label. Add a **new** `STR_TABULA` key instead if you want to keep the KOReader label. Other languages override it too (e.g. french, romanian, bulgarian, persian `.yaml`). Either set it in all of them or limit languages with `custom_i18n_builtin_langs = en` in `platformio.ini` (also saves flash).
- `platformio.ini`: add `[tabula] version = 1.0.0`, and in your release env set `-DCROSSPOINT_VERSION=\"tabula-1.0.0\"`. For S3 envs, `USB_PRODUCT`/`USB_MANUFACTURER` are already set (e.g. `CrossPoint_X4_Pro`); change those to Tabula. The C3 env doesn't set them (native USB-JTAG).
- `src/images/Logo120.h` (120×120 bitmap; source `src/images/Logo120.png`, `logo.svg`): replace with a Tabula logo (e.g. chi-rho / IHS). Regenerate with `python scripts/convert_icon.py tabula.png Logo120 120 120` (needs Pillow, plus cairosvg for SVG). ⚠️ Check the output format matches the existing header before swapping.
- `src/activities/settings/AboutActivity.cpp`: add the "Based on CrossPoint Reader (MIT)…" attribution.
- Web UI titles (`src/network/html/*.html`, e.g. `FilesPage.html` "Files - CrossPoint Reader"). These only matter if you keep Wi-Fi file transfer.
- **Don't rename internal identifiers** (`CrossPointSettings`, `/.crosspoint/` cache dir, `CROSSPOINT_*` macros). It's a huge diff for no user-visible gain, and it makes merging upstream fixes painful.

### 7.2 Splash / boot screen
- `src/activities/boot_sleep/BootActivity.cpp` `BootActivity::onEnter()` (21 lines): draws `Logo120` centered, `tr(STR_CROSSPOINT)`, `tr(STR_BOOTING)`, `CROSSPOINT_VERSION`. Replace it with a Tabula splash: full-screen bitmap or logo + "Tabula" + short line (e.g. "Ad maiorem Dei gloriam"). Keep it monochrome, sized for 528×792 (X3) and 480×800 (X4). Use `renderer.getScreenWidth()/Height()` as the code already does.
- `src/activities/boot_sleep/SleepActivity.cpp` ~L710–712: default "Dark/Light" sleep screen draws the same logo + `STR_CROSSPOINT` + `STR_SLEEPING`. Rebrand it here. **No code is needed for custom sleep images.** Put `sleep.bmp` (or a `.sleep/` folder of BMPs, 24-bit, 528×792 for X3) on the SD card and set sleep mode to `CUSTOM` (`SLEEP_SCREEN_MODE` enum in `src/CrossPointSettings.h` L20–31; default `sleepScreen = DARK` L236). This is a nice place for sacred art (public-domain images only).

### 7.3 Boot straight into a Tabula home
- Boot routing is in `src/main.cpp` `setup()` (~L780–815). After the splash it goes to the crash report, silent-resume targets, `goHome()`, or **auto-resumes the last book** (`goToReader(APP_STATE.openEpubPath)` when last sleep was from the reader). Keep the auto-resume, since it's nice for the missal.
- Home menu is in `src/activities/home/HomeActivity.cpp` `render` (~L527–539: `menuItems = {Browse Files, Library, [Plugins/OPDS], File Transfer, Settings}`), with activation in `loop()` ~L305–330. Index mapping is in `src/activities/home/HomeActivity.h` L40–65. The enum is in `src/activities/ActivityManager.h` L21: `enum class HomeMenuItem { NONE, FILE_BROWSER, LIBRARY, OPDS_BROWSER, FILE_TRANSFER, SETTINGS_MENU }`.
- **Suggested approach (smallest diff):** replace the menu with Tabula entries mapped to folders on the SD card:
  - "Missal" → `activityManager.goToFileBrowser("/Tabula/Missal")` (`goToFileBrowser(std::string path = {})` exists, `ActivityManager.h` L92)
  - "Daily Readings" → `/Tabula/Readings`
  - "Literature" → `/Tabula/Literature`
  - "Letters" → `/Tabula/Letters`
  - "Settings" (hidden in Parish Edition, see 7.5)
  - Keep the "Continue reading" cover tile (recent books) at the top.
  Alternatively, point a single "Library" entry at `goToLibrary()` (`src/activities/library/LibraryListActivity.cpp`). It indexes up to 4,096 books by title and author with Recent/Added/Title/Author tabs.
- File browser is `src/activities/home/FileBrowserActivity.cpp`. It shows hidden dot-folders and allows **delete/rename/move** via long-press Confirm (USER_GUIDE §3.3). For Parish Edition, disable delete/rename/move and **restrict navigation to `/Tabula`** (no going above the root path).
- "Daily readings" for a given date: CrossPoint has no calendar-aware content. Options: (a) one EPUB per liturgical year with a TOC by date (the user picks the date from chapter selection, `EpubReaderChapterSelectionActivity`); (b) **later**: a small "Today" entry that uses the X3 RTC (`lib/TrustedTime`, `ClockSettingsActivity`) to open the right chapter. ⚠️ The RTC needs the clock set once, and the X4 has no RTC per the device comparison. Start with (a).

### 7.4 Remove / hide unneeded features
Settings actions are registered in `src/activities/settings/SettingsActivity.cpp` L110–149 (`addAction(...)`). Settings rows/categories are in `src/SettingsList.cpp`.

| Feature | Where | Tabula action |
|---|---|---|
| **OTA "Check for updates"** | `SettingsActivity.cpp` L146, `src/network/OtaUpdater.cpp` L22 (`api.github.com/repos/crosspoint-reader/crosspoint-reader/releases/latest`, asset `crosspoint-<ver>-x3-x4.bin`) | **Must remove or repoint.** Otherwise one tap reinstalls stock CrossPoint over Tabula. Point it at your own GitHub releases or remove it |
| SD firmware update | `SettingsActivity.cpp` L147, `src/activities/settings/SdFirmwareUpdateActivity.cpp` (picks a `.bin` from SD); recovery mode wired in `main.cpp` (`recoveryFirmwareMode`) | Keep but hide behind a service combo. It's a good way to update Tabula without a computer |
| OPDS browser | `src/activities/browser/OpdsBookBrowserActivity.cpp`, `SettingsActivity.cpp` L138 | remove from menu |
| KOReader sync | `src/activities/reader/KOReaderSyncActivity.cpp`, `settings/KOReader*`, `lib/KOReaderSync`, L137 | remove |
| Plugins | `src/activities/plugins/`, `settings/PluginHubInstallActivity.cpp`, L139; `docs/sd-plugins.md` | remove / disable (`SETTINGS.pluginsEnabled`) |
| Wi-Fi file transfer / Calibre / WebDAV / hotspot | `src/activities/network/*`, `src/network/CrossPointWebServer.cpp`, `WebDAVHandler.cpp` | hide in Parish Edition (keep for dev) |
| BLE page turner | `[ble_page_turner]` in `platformio.ini`, `lib/BlePageTurner`, L135 | can drop (saves RAM/flash) |
| Content protection (LCP/DRM) | `-DFREEINK_CONTENT_WOLFSSL`, `src/network/LcpDevice.cpp`, `src/LcpPassphraseStore.*` | drop (not needed for public-domain texts; may let you drop wolfSSL ⚠️) |
| Font download | `settings/FontDownloadActivity.cpp`, L113 | hide |
| Dictionary, clippings, screenshots | `src/util/Dictionary*`, `src/clippings/` | optional; harmless |
| AI features | **None found in CrossPoint.** Nothing to remove ✅ |

**Approach:** hide features by first gating them with a build flag, e.g. `-DTABULA_PARISH=1`. Wrap the `addAction` calls and home items in `#if !TABULA_PARISH`. Actually deleting code and libraries is phase 2, after it works. That keeps upstream merges manageable. Removing libs (wolfSSL, NimBLE, WebSockets) will need edits to `platformio.ini` `lib_deps`/`build_flags`/`extra_scripts` (`patch_wolfssl.py`). ⚠️ Expect compile fallout and ask the agent to do it incrementally.

### 7.5 "Parish Edition" locked-down mode
New env in `platformio.ini`, cloned from `gh_release`:
```ini
[env:tabula_parish]
extends = base, firmware_tuned_c3
build_flags =
  ${firmware_tuned_c3.build_flags}
  -DFREEINK_DEVICE_X4=1
  -DFREEINK_DEVICE_X3=1
  -DCROSSPOINT_VERSION=\"tabula-1.0.0\"
  -DTABULA_PARISH=1
  -DLOG_LEVEL=1
custom_i18n_builtin_langs = en
```
(Add `es` or another language if the parish needs it. There is **no Latin UI translation**, but Latin *book text* renders fine with the built-in Latin fonts.)

Under `TABULA_PARISH`:
- Home shows only Tabula content folders and Continue Reading. Settings shrinks to reader comfort only (font size, line spacing, orientation, sleep timeout, button layout, About).
- No Wi-Fi/BLE UI, no OTA, no delete/rename in the file browser.
- Hidden service menu (e.g. hold Back + Confirm on Home for 5 s) → full settings + SD firmware update, for you.
- Defaults for Parish units: preload `/.crosspoint/settings.json` (path from `CrossPointSettings::getFilePath()`, `src/CrossPointSettings.h` L470) from a "golden" unit, or change the defaults in `CrossPointSettings.h`. Changing the defaults in code is more robust. ⚠️ settings.json schema not inspected in detail.

### 7.6 Preloading content
- **SD card (recommended).** Formats read: **EPUB** (main path, `lib/Epub`), **TXT** (`lib/Txt`), **Markdown `.md`**, **XTC** (Xteink's pre-rendered format, `lib/Xtc`, `XtcReaderActivity`), and BMP images (`FileBrowserActivity.cpp` ~L1070). Put content on the bundled 16 GB microSD:
  ```
  /Tabula/Missal/    Missale-Romanum-1962-Latin-English.epub, Lasance-1945.epub …
  /Tabula/Readings/  Readings-1962-Calendar.epub
  /Tabula/Literature/ …  /Tabula/Letters/ …
  /sleep.bmp  (or /.sleep/*.bmp)
  /LICENSES.txt
  ```
  The firmware creates `/.crosspoint/` (caches, `settings.json`, `state.json`, `recent.json`). **Pre-warm it on a golden card** by opening each book once, so customers don't wait on first open. Then image that card for all units. Caches are keyed by content hash (README "Data caching"), so they should be portable between identical cards. ⚠️ Not tested. Also check X3 vs X4 caches (different screen sizes, so different layout caches).
  - SD card filesystem: ⚠️ FAT32 vs exFAT not confirmed in docs. The bundled card's factory format presumably works, so format the golden card with the same scheme.
- **Into flash:** possible (the 3.4 MB `spiffs` partition exists, but `CONFIG_ARDUINO_SELECTIVE_SPIFFS=n`/`LittleFS=n` in `[firmware_tuned]`, so a filesystem driver would have to be re-enabled). **Not recommended:** the C3 is RAM-tight, the reader is built around SdFat, and the SD card is already included. Use SD.

---

## 8. Content prep

- **Tooling:** write in Markdown/HTML and convert with **Pandoc** (`brew install pandoc`; `pandoc missal.md -o missal.epub --toc --toc-depth=2 --metadata title="Missale Romanum"`), or use **Calibre** (`brew install --cask calibre`, `ebook-convert`). There's also a Calibre plugin for CrossPoint (`crosspoint-reader/calibre-plugins`) ⚠️ not inspected.
- **EPUB tips for this device:** a good nested TOC (Proper of Time → Sunday → parts) is essential, since that's how users navigate (chapter selection). Split into many small chapters. Big chapters are slow on the C3 (~380 KB RAM). Keep cover images small (USER_GUIDE §6: large covers take ~10 s to convert). Avoid complex CSS. Use simple two-column Latin/English, since it renders as alternating paragraphs (tables aren't reliable ⚠️). Rubrics in red will render as black/gray, so use italics.
- **Fonts:** built-in reader fonts cover Latin Extended (accents like á, æ, œ ✅). Greek in the Kyrie is fine in Latin transliteration. Real Greek/Hebrew text needs SD-card fonts (`docs/sd-card-fonts.md`). Signs like ℣ ℟ ✠ are ⚠️ probably missing from built-in fonts. Test them, substitute "V." "R." "+", or add an SD font.
- **Public-domain caveats (carried over from the earlier research, still open):**
  - **1945 Lasance "The New Roman Missal"**: US PD status depends on whether the copyright was renewed (works from 1929–1977 need a renewal check). ⚠️ **Open, verify before shipping** (search the Copyright Office renewal records / Stanford renewal DB). The 1937/1942 editions may be safer. Pre-1931 editions are PD in the US.
  - Latin texts of the Missale Romanum are PD. **Modern English translations** (e.g. Angelus Press 1962 missal, Baronius) **are copyrighted**, so don't use them without permission.
  - Daily readings: Douay-Rheims (Challoner) is PD. **NABRE/RSV-CE are copyrighted** (USCCB/NCC), so don't use them for the Novus Ordo lectionary without a license.
  - Literature/letters: pick pre-1931 works (e.g. Imitation of Christ, Confessions (old translations), St. Francis de Sales). Project Gutenberg EPUBs are a good source. Strip the Gutenberg license/trademark header if you redistribute modified versions, per their terms.
  - Get the **pastor's OK** on content and, ideally, on the Tabula branding before the parish store sells it.

---

## 9. Per-unit flashing / provisioning checklist (25–50 units)

**One time:** build `tabula_parish` → `firmware.bin`; record its SHA-256. Make a golden SD image: `diskutil list` → `sudo dd if=/dev/rdiskN of=tabula-golden.img bs=4m` (macOS). ⚠️ Confirm the disk number every time, since `dd` to the wrong disk wipes it.

For each unit (target about 3–5 min):
1. ☐ Unbox; confirm it's a **global / Developer Edition** unit (bought from xteink.com or the official Amazon store). Log the serial/lot.
2. ☐ Charge enough to flash (>30%).
3. ☐ Connect the data-capable pogo cable; wake the device. Check that `ls /dev/cu.usbmodem*` shows a port. **If no port appears, the unit is likely locked: set it aside. Do NOT use the unlocker with Tabula** (README warning: non-CrossPoint/CrossInk firmware on a locked unit can perma-brick).
4. ☐ Flash: `esptool.py --chip esp32c3 --port /dev/cu.usbmodemXXXX --baud 921600 write_flash 0x10000 firmware.bin` (or `pio run -e tabula_parish -t upload`). ⚠️ For a fresh unit the web flasher's OTA-slot method may be the safer path. Test both on unit #1 and use whichever works.
5. ☐ Write the SD card from the golden image (`sudo dd if=tabula-golden.img of=/dev/rdiskN bs=4m`), or copy the folder tree. Insert it.
6. ☐ Reboot: Tabula splash → Tabula home. Open the Missal and turn 3 pages; open Readings; sleep → Tabula sleep image; wake → resumes.
7. ☐ Confirm About shows `tabula-1.0.0` + attribution.
8. ☐ Package: device, cable, printed quick-start card (buttons, how to find today's readings, attribution/license, contact for help).
9. ☐ Log: serial, firmware SHA, date, tester initials.

A small script can automate steps 4–5 and the log (e.g. `provision.sh` looping over newly attached `/dev/cu.usbmodem*`). Ask the agent to write it once the manual flow works on 2–3 units.

---

## 10. Risks and open questions

1. ⚠️ **Build not verified yet** (stopped mid-setup). The macOS "CMake pin in platformio.local.ini" mentioned in `platformio.ini` L190 is undocumented in what I read. Check upstream issues/Discussions if the Mac build fails.
2. ⚠️ **wolfSSL GPL** licensing if you keep networking/content-protection code. Removing it is preferable, but check.
3. **Upstream OTA would overwrite Tabula.** Remove or repoint it (must-do).
4. Locked units: only buy from Xteink directly. Ask Xteink, in the wholesale quote request, to confirm a **global, unlocked** firmware for all units.
5. Upstream moves fast (multiple commits/day, v2.0.0). Pin your fork to a **tagged release**, not `develop`, and merge upstream deliberately (`git fetch upstream && git merge <tag>`).
6. X3 hardware variants: newer X3s use a UC8279d panel (auto-detected per `freeink-sdk/docs/xteink-x3-uc8279-support.md`). Test on a unit from the actual batch.
7. ⚠️ Can pre-warmed `/.crosspoint` caches be copied between cards? Untested.
8. ⚠️ Liturgical glyphs (℣ ℟ ✠) in the built-in fonts are untested.
9. ⚠️ PD status of the 1945 Lasance missal is still open. So is how sales tax works for the church store (from the cost tracker).
10. ⚠️ The X3 **Developer Edition** link via crosspointreader.com shares revenue with CrossPoint devs. That's a nice gesture, but compare it against direct xteink.com pricing ($69 sale) and the wholesale quote. A third-party reseller (HONGTE) lists $62/pc "indicative", but their firmware/lock status is **unconfirmed**, so avoid them unless they confirm global units.
11. The X3 has no frontlight, so it can't be read in a dark church. Set expectations on the product card.
12. Support: who fixes a unit that stops booting? Plan for a hidden SD-firmware recovery path (exists: `SdFirmwareUpdateActivity`, recovery mode in `main.cpp`). ⚠️ How to trigger recovery mode with buttons was not checked.

---

## 11. Copy-paste prompt for the AI coding agent

```
You are working in my fork of CrossPoint Reader (ESP32-C3 e-reader firmware, PlatformIO/pioarduino,
Arduino framework). Read AGENTS.md and tabula-handoff.md (in the repo root) before doing anything.
Goal: rebrand as "Tabula", a Catholic reading device for the Xteink X3 (C3 env `default`; my
own test unit may be an X4 Pro → env `x4pro`). Do NOT push, open PRs, or commit without asking me.

Work in small steps; after each step tell me exactly how to build and verify on the device:
1. Confirm a clean stock build: `pio run -e default`. Fix only environment issues (macOS, Apple Silicon).
2. Add `[env:tabula_parish]` (clone of gh_release) with -DTABULA_PARISH=1, version "tabula-1.0.0",
   custom_i18n_builtin_langs = en.
3. Branding: STR_CROSSPOINT -> "Tabula" (lib/I18n/translations/english.yaml), new Logo120 from
   tabula.png via scripts/convert_icon.py, new BootActivity splash
   (src/activities/boot_sleep/BootActivity.cpp) and default sleep screen
   (SleepActivity.cpp ~L710). Add MIT attribution to AboutActivity. Keep LICENSE + freeink-sdk/LICENSE.
4. Home (src/activities/home/HomeActivity.{h,cpp}, ActivityManager.h HomeMenuItem): replace menu with
   Missal, Daily Readings, Literature, Letters -> goToFileBrowser("/Tabula/<Folder>"); keep
   Continue Reading; Settings only when !TABULA_PARISH or via a hidden 5-second Back+Confirm hold.
5. Under TABULA_PARISH: remove OTA (OtaUpdater + SettingsActivity.cpp L146), OPDS, KOReader sync,
   plugins, Wi-Fi transfer, BLE, font download from menus (SettingsActivity.cpp L110-149);
   disable delete/rename/move and navigation above /Tabula in FileBrowserActivity.
6. Only after all of that works on hardware: propose removing now-unused libs (wolfSSL, NimBLE,
   WebSockets, content protection) from platformio.ini, with a flash/RAM size comparison.
Cite file paths and line numbers for each change. Never use `git clean -fdX`.
```
