# 🧟 Zombigotchi

A WiFi-hunting zombie for the **ESP32 CYD** (Cheap Yellow Display). Inspired by Pwnagotchi — passive WiFi scanning, WPA handshake capture, RPG leveling, and a face that reacts to what it finds. Green terminal aesthetic. No external libraries needed beyond the ESP32 core.

---

## What it does

Zombigotchi sits on your desk (or in your bag) and passively sniffs WiFi traffic. It hops through channels 1–13 looking for networks and WPA handshakes. Every handshake captured ("brainz") earns XP and levels up the zombie. Captured handshakes are saved as `.pcap` files to an SD card — pop the card into your computer and open them in Wireshark or feed them to `hashcat`.

---

## Hardware

| Part | Details |
|------|---------|
| Board | ESP32-2432S028R ("Cheap Yellow Display" / CYD) |
| Display | ILI9341 320×240 TFT — built in, driven via raw SPI |
| Touch | XPT2046 — raw SPI bit-bang, no library needed |
| Storage | MicroSD card (FAT32, any size) — optional but recommended |

The CYD is available for ~$10–15. Search **"ESP32-2432S028R"** or **"ESP32 Cheap Yellow Display"** on AliExpress or Amazon.

---

## Pin Map

These are hardwired on the CYD board — no wiring needed.

| Function | GPIO |
|----------|------|
| TFT MOSI | 13 |
| TFT SCLK | 14 |
| TFT MISO | 12 |
| TFT CS   | 15 |
| TFT DC   | 2  |
| TFT BL   | 21 |
| Touch CS | 33 |
| SD CS    | 5  |
| BOOT btn | 0  |

---

## Libraries Required

**None beyond the standard ESP32 Arduino core.** All of these ship with it:

- `SPI.h`
- `WiFi.h`
- `esp_wifi.h`
- `SD.h`
- `Preferences.h`

---

## Installation

1. Install [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. Add ESP32 board support — go to **File → Preferences** and add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to **Tools → Board → Board Manager**, search `esp32`, install **esp32 by Espressif Systems**
4. Select **Tools → Board → ESP32 Arduino → ESP32 Dev Module**
5. Set **Tools → Partition Scheme → Huge APP (3MB No OTA)**
6. Open `zombigotchi.ino` and upload

---

## Screen Layout

```
┌─ ch:1  aps:3 ─────────── SD  00:01:23 ─┐
│                    │  lv5               │
│                    │  Brain Eater       │
│      (O_O)         │  400/500 xp        │
│                    │  ▓▓▓▓▓░░░          │
│      Hunting       │  brainz:           │
│                    │  3                 │
│                    │  pkts: 1247        │
│                    │  hold=menu         │
├─────────────────────────────────────────┤
│ > CoffeeShop_2.4GHz                     │
└─────────────────────────────────────────┘
```

- **Top bar** — current channel, AP count, SD indicator, uptime
- **Left panel** — animated face + current mood
- **Right panel** — level, title, XP bar, brainz count, packet count
- **Bottom bar** — latest network discovered

---

## Controls

| Action | Result |
|--------|--------|
| Hold BOOT button (1.5s) | Open menu |
| Tap menu item (touch) | Activate item |
| Short button press (in menu) | Move to next item |
| Long button press / 0.8s (in menu) | Activate selected item |
| Long press (in stats) | Return to menu |

---

## Menu

```
┌─ zombigotchi menu ──────────────────────┐
│                                         │
│  DEAUTH   OFF - enable deauth attack    │
│                                         │
│  STATS    xp  brainz  networks  sd card │
│                                         │
│  EXIT     back to hunting               │
│                                         │
│  SD: session_0003.pcap                  │
│      /zombigotchi/brainz/               │
└─────────────────────────────────────────┘
```

---

## Moods

The face changes based on what the zombie is experiencing. Each mood has 6 rotating faces that cycle every 3 seconds.

| Mood | Trigger | Example faces |
|------|---------|---------------|
| Lurking | Default / low activity | `(x_x)` `(@_@)` `[x_x]` |
| Hunting | 10+ APs found | `(O_O)` `(>_<)` `{O_O}` |
| Feasting | Handshake captured | `(^_^)` `(*_*)` `(X_X)` |
| Shambling | Very low packet count | `(-_-)` `(z_z)` `(+_+)` |

---

## Levels

XP is earned by capturing WPA handshakes (100 XP each). Level and XP persist across reboots via the ESP32's built-in flash storage.

| Level | Title |
|-------|-------|
| 1 | Freshly Dead |
| 2 | Shambler |
| 3 | Groaner |
| 4 | Crawler |
| 5 | Walker |
| 6 | Runner |
| 7 | Infected |
| 8 | Feral |
| 9 | Ravager |
| 10 | Brain Eater |
| 11 | Necromancer |
| 12 | Undead Lord |
| 13 | Zombie God |
| 14 | APOCALYPSE |
| 15 | EXTINCTION |

---

## SD Card & PCAP Files

Insert a FAT32 formatted microSD card before powering on. Zombigotchi will create this folder structure automatically:

```
/zombigotchi/
  brainz/
    session_0001.pcap
    session_0002.pcap
    ...
```

Each boot creates a new session file. Files are valid `.pcap` format and can be opened directly in **Wireshark** or processed with **hcxtools** / **hashcat**.

If no SD card is inserted, handshakes are still counted and displayed but not saved to disk.

---

## Deauth

The deauth feature sends 802.11 deauthentication frames to all discovered APs, cycling through them every 600ms. This can force devices to reconnect, making handshake capture more likely.

Enable it from the menu. When active, the right panel shows `deauth: N sent`.

> ⚠️ **Legal Warning** — Sending deauth frames is illegal on networks you do not own or have explicit written permission to test. This feature is provided for authorized penetration testing and educational use only. The author is not responsible for misuse.

---

## License

MIT — do what you want, don't blame me.
