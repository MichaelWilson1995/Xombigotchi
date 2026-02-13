# 🧟 Zombigotchi

A Pwnagotchi-inspired WiFi handshake harvester running on the **ESP32 CYD (Cheap Yellow Display)**. No Python, no Raspberry Pi — just a $15 board, a button, and brains.

---

## What It Does

Zombigotchi sniffs WiFi traffic in promiscuous mode, captures WPA/WPA2 EAPOL handshakes (the "brainz"), and saves them to SD card as `.pcap` files ready for offline cracking with `hashcat` or `aircrack-ng`. It keeps a running log of every network it sees and a full session stats file updated in real-time.

Everything is gamified — your zombie levels up, earns XP, unlocks achievements, and grows more powerful the more handshakes you capture.

---

## Hardware

| Part | Notes |
|------|-------|
| ESP32 CYD (ESP32-2432S028R) | The "Cheap Yellow Display" — ESP32 + 2.8" ILI9341 touchscreen |
| MicroSD card | FAT32 formatted, any size |
| USB power bank | For portable use |

The CYD has the display, touch, and SD card all wired up already. No soldering required.

---

## Features

### Core
- **Promiscuous WiFi sniffing** across all 13 channels with 1-second hop
- **EAPOL handshake capture** — detects all 4-way handshake frames including QoS data frames and both LLC/SNAP and bare EAPOL formats
- **Deauth attack** — sends bidirectional deauth frames (AP→broadcast + fake client→AP) every 2 seconds to force client reconnections and trigger fresh handshakes
- **PCAP export** — every captured handshake saved as a valid `.pcap` file, one per session, openable in Wireshark and crackable with hashcat

### SD Card Backups

Everything is saved to `/zbg/` on your SD card automatically:

```
/zbg/
  brainz/
    s0001.pcap        ← WPA handshake captures, crack with hashcat
    s0002.pcap        ← New file created each session
  nets/
    nets_0001.csv     ← Every AP seen (SSID, BSSID, channel) appended live
    nets_0002.csv
  stats/
    session_0001.txt  ← Full session summary updated on every save
    session_0002.txt
  passwords.txt       ← Put cracked passwords here to show on screen
```

- **Handshake PCAPs** are written the moment each frame is captured
- **Network CSV** is appended the instant a new AP is discovered
- **Session stats** are updated on every brainz capture and network find
- **Progress** (level, XP, achievements, cosmetics) is also saved to device NVS flash

### RPG Progression
- **15 levels** — Freshly Dead → Shambler → Groaner → Crawler → Walker → Runner → Infected → Feral → Ravager → Brain Eater → Necromancer → Undead Lord → Zombie God → APOCALYPSE → EXTINCTION
- **XP system** — 100 XP per handshake, multiplied by active streak
- **Streak multipliers** — 3x brainz in 60s = x1.5, 5x = x2.0, 10x = x3.0
- **25 achievements** spanning brainz, levels, networks, streaks, uptime, and packets

### Cosmetics (unlockable via achievements)
- 6 border styles drawn around the screen edge
- 8 face accessories — hats, halos, crowns, blood drips, scars, ghost, devil horns
- 12 equippable badges shown below the face (wear up to 4 at once)
- 6 title flair styles that wrap your zombie's title text
- All saved and restored across power cycles

### Themes
9 colour themes selectable in Settings: Green (default), Red, Blue, Cyan, Yellow, Orange, Purple, White, Invert

### Moods
Your zombie's face and mood shift based on what's happening:
- **Lurking** — idle, watching
- **Hunting** — actively scanning
- **Feasting!** — after a fresh brainz
- **Shambling** — random idle state

---

## Menu Navigation

**Hold the boot button (GPIO0) for 1.5 seconds** to open the menu.

| Press | Action |
|-------|--------|
| Short press | Cycle to next row |
| Long hold (0.8s+) | Open or activate the highlighted row |

### Menu Map

```
PAGE 1              PAGE 2                PAGE 3 — Settings
──────────────      ────────────────      ──────────────────
DEAUTH              ACHIEVEMENTS          THEME
STATS               WARDROBE              RELOAD PWS
MORE ──────────►    SETTINGS ────────►    ◄── BACK
EXIT (→ main)       ◄── BACK
```

EXIT on page 1 returns you to the main watch screen.

---

## SD Card Setup

Format your SD card as **FAT32**. Zombigotchi creates all subdirectories automatically on first boot.

To display cracked passwords on the main screen, create `/zbg/passwords.txt` with one password per line. They cycle every 4 seconds in the bottom bar. Use Settings → Reload PWS to reload the file without rebooting.

---

## Cracking Captured Handshakes

Copy `.pcap` files from `/zbg/brainz/` off the SD card to your computer.

**hashcat (recommended):**
```bash
# Convert to hc22000 format
hcxpcapngtool -o out.hc22000 s0001.pcap

# Crack against a wordlist
hashcat -m 22000 out.hc22000 wordlist.txt

# With rules
hashcat -m 22000 out.hc22000 wordlist.txt -r rules/best64.rule
```

**aircrack-ng:**
```bash
aircrack-ng s0001.pcap -w wordlist.txt
```

Good wordlists: `rockyou.txt`, SecLists WiFi passwords, or generate targeted lists with `crunch` or `CUPP`.

---

## Arduino IDE Setup

### Board Settings
| Setting | Value |
|---------|-------|
| Board | ESP32 Dev Module |
| Upload Speed | 921600 |
| Flash Size | 4MB (32Mb) |
| Partition Scheme | Default 4MB with spiffs |
| Core Debug Level | None |

### SD Library Fix
If you see **"Multiple libraries found for SD.h"** or compilation errors about SD:

1. Go to **Tools → Manage Libraries**
2. Search for `SD`
3. **Uninstall** any library named "SD" by Arduino or SparkFun
4. Keep only the one bundled with the ESP32 board package (it won't appear in the library manager)

### No External Libraries Needed
This sketch uses only what comes built-in to the ESP32 Arduino core:
- `SPI.h`
- `WiFi.h`
- `esp_wifi.h`
- `SD.h`
- `Preferences.h`

---

## Project Files

```
zombigotchi/
  zombigotchi.ino     ← Full sketch, single file
  README.md           ← This file
```

---

## Legal / Ethical Notice

This tool is for **educational and authorized security testing only**. Capturing network traffic or transmitting deauth frames on networks you do not own or have explicit written permission to test is illegal in most jurisdictions including the US (CFAA), UK (Computer Misuse Act), and EU member states.

**Only use this on your own networks or in a controlled lab environment.**

---

## Inspired By

[Pwnagotchi](https://pwnagotchi.ai/) — the original AI-powered handshake pet. Zombigotchi is a standalone reimplementation for ESP32 with no AI, no Python, no internet dependency, and full offline operation.
