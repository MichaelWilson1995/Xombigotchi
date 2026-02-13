# 🧟 Zombigotchi

> *A Pwnagotchi-style WiFi handshake harvester for the ESP32 CYD. No Python. No Pi. Just a $15 board and a button.*

Zombigotchi runs on the **ESP32 CYD (Cheap Yellow Display)** — a sub-$15 ESP32 board with a built-in 2.8" ILI9341 screen and SD card slot. It sniffs WiFi traffic, sends deauth frames to force handshakes, captures EAPOL packets, and saves everything to SD card as `.pcap` files ready for cracking. All wrapped in a zombie RPG with 55 levels, 27 achievements, and unlockable cosmetics.

---

## Hardware

| Part | Details |
|------|---------|
| **ESP32-2432S028R** (CYD) | ESP32 + 2.8" ILI9341 TFT, XPT2046 touch, MicroSD slot — all wired up |
| MicroSD card | Any size, FAT32 formatted |
| USB power bank | For field use |

No soldering. No shields. Just the board and a card.

---

## What It Does

### WiFi Capture
- **Promiscuous sniffing** on all 13 channels with 1-second channel hop
- **Deauth attack** — sends bidirectional deauth frames every 2 seconds (AP→broadcast to kick clients + fake client→AP to trigger re-authentication)
- **EAPOL handshake detection** — catches all 4-way handshake frames including QoS data frames, LLC/SNAP wrapped and bare EAPOL formats
- **PCAP capture** — every handshake written to SD card immediately as a valid `.pcap`

### SD Card — Everything Gets Saved
```
/zbg/
├── brainz/
│   ├── s0001.pcap     ← WPA handshakes. Crack with hashcat or aircrack-ng.
│   └── s0002.pcap     ← New file each session.
├── nets/
│   ├── nets_0001.csv  ← Every AP found: SSID, BSSID, channel. Appended live.
│   └── nets_0002.csv
├── stats/
│   ├── session_0001.txt  ← Full session dump: level, XP, brainz, network list.
│   └── session_0002.txt  ← Updated on every capture event.
└── passwords.txt      ← Put cracked passwords here. They scroll on the main screen.
```

All folders are created automatically on first boot. Just insert a blank FAT32 card.

---

## RPG System

### 55 Levels

XP formula: `500 + level² × 150` — fast early, brutal late.

| Level | Title | XP to next |
|-------|-------|-----------|
| 1 | Freshly Dead | 650 |
| 5 | Walker | 4,250 |
| 10 | Brain Eater | 15,500 |
| 20 | Death Knight | 61,500 |
| 25 | Void Stalker | 94,250 |
| 30 | Doom Bringer | 135,500 |
| 40 | Eternal One | 241,500 |
| 50 | SINGULARITY | ~376k |
| 55 | **Z O M B I E** | — |

Full title progression:
`Freshly Dead → Shambler → Groaner → Crawler → Walker → Runner → Infected → Feral → Ravager → Brain Eater → Necromancer → Undead Lord → Lich → Revenant → Ghoul → Wraith → Specter → Phantom → Banshee → Death Knight → Plague Lord → Bone Tyrant → Soul Reaper → Crypt Walker → Void Stalker → Hex Warden → Dark Shaman → Blood Witch → Grave Titan → Doom Bringer → Ash Walker → Rot Champion → Infernal One → Chaos Fiend → Abyss Spawn → Hellion → Dreadlord → Warlord Lich → Undying King → Eternal One → Apocalypse → Extinction → Oblivion → Annihilator → Worldbreaker → Voidwalker → Armageddon → Ragnarok → CATACLYSM → SINGULARITY → TRANSCENDENCE → OMNICIDE → THE END → BEYOND DEATH → Z O M B I E`

### XP Streak Multiplier
Capture multiple handshakes within 60 seconds to build a streak:

| Streak | Multiplier |
|--------|-----------|
| 3x | ×1.5 |
| 5x | ×2.0 |
| 10x | ×3.0 |

### Moods
Your zombie's face changes based on what's happening:
- **Lurking** — idle
- **Hunting** — actively scanning
- **Feasting!** — just captured brainz
- **Shambling** — random idle

24 face variants per mood (96 total).

---

## 27 Achievements

| ID | Name | Requirement | Reward |
|----|------|-------------|--------|
| b1 | First Blood | Capture 1 brainz | +500 XP |
| b5 | Feeding Frenzy | Capture 5 brainz | Streak x2 |
| b10 | Brain Collector | Capture 10 brainz | +1000 XP |
| b25 | Horde Mind | Capture 25 brainz | Mood boost |
| b50 | Nom Nom Nom | Capture 50 brainz | +5000 XP |
| b100 | The Hunger | 100 lifetime brainz | Legendary |
| l5 | Rising Dead | Reach level 5 | New faces |
| l10 | Apex Predator | Reach level 10 | Brain Eater |
| l25 | Half Damned | Reach level 25 | Void Stalker |
| l40 | Eternal Curse | Reach level 40 | Undying King |
| l55 | Z O M B I E | Reach max level 55 | You transcend |
| n10 | Neighbourhood Watch | Find 10 networks | AP radar |
| n50 | City Scanner | Find 50 networks | +2x AP XP |
| n100 | The Surveyor | 100 networks total | Ghost of WiFi |
| s3 | On A Roll | 3x streak | ×1.5 XP |
| s5 | Unstoppable | 5x streak | ×2.0 XP |
| s10 | RAMPAGE | 10x streak | ×3.0 XP |
| ss1 | Night Stalker | Active 30 minutes | Uptime badge |
| ss2 | Sleepless Dead | Active 2 hours | Insomniac |
| ss3 | Marathon Zombie | Active 8 hours | Legendary |
| p1k | Eavesdropper | Sniff 1,000 packets | Passive master |
| p10k | Deep Listener | Sniff 10,000 packets | +1000 XP |
| sec1 | Insomniac | Run 24 hours straight | Get help |
| sec2 | Glutton | 3 brainz in one session | Greedy zombie |
| sec3 | Speed Demon | 2 brainz within 10 seconds | ×2 XP 5min |
| sec4 | Explorer | Scan all 13 channels | Full spectrum |
| sec5 | Dedicated | Complete 10 sessions | Veteran zombie |

---

## Cosmetics (unlock via achievements)

All cosmetics are saved to NVS flash and restored on boot.

### Borders (drawn around screen edge)
| Name | Unlock |
|------|--------|
| None | default |
| Classic | First Blood (b1) |
| Double | Rising Dead (l5) |
| Skull | Horde Mind (b25) |
| Glitch | Unstoppable (s5) |
| Crown | Z O M B I E (l55) |

### Accessories (drawn on face)
| Name | Unlock |
|------|--------|
| None | default |
| Hat `^^^^` | Neighbourhood Watch (n10) |
| Halo `*o*` | Night Stalker (ss1) |
| Crown `>-<>-` | Apex Predator (l10) |
| Blood Drip `~` | Brain Collector (b10) |
| Scar `/` | On A Roll (s3) |
| Ghost `-~~~-` | Sleepless Dead (ss2) |
| Devil Horns `/v\` | Nom Nom Nom (b50) |

### Badges (wear up to 4 at once, shown below face)
`[x]` `vAv` `(#)` `###` `***` `~o~` `- -` `oOo` `>!<` `[-]` `^X^` `|V|`

Each unlocked by a specific achievement. Mix and match any 4.

### Title Flair (wraps your title text)
| Style | Example | Unlock |
|-------|---------|--------|
| Plain | Brain Eater | default |
| Star | \*Brain Eater\* | Feeding Frenzy (b5) |
| Skull | XBrain EaterX | Horde Mind (b25) |
| Crown | ^Brain Eater^ | Apex Predator (l10) |
| Fire | >Brain Eater< | Unstoppable (s5) |
| Legend | >>Brain Eater<< | The Hunger (b100) |

---

## Button Controls

Everything is controlled by the single **boot button (GPIO0)** on the CYD board.

### Main Screen
| Input | Action |
|-------|--------|
| Hold 1.5s | Open menu |

### Menu
| Input | Action |
|-------|--------|
| Short press (<0.6s) | Cycle to next row |
| Long hold (0.8s+) | Open / activate selected row |

### Menu Map
```
PAGE 1              PAGE 2                PAGE 3 — Settings
──────────────      ────────────────      ──────────────────
DEAUTH              ACHIEVEMENTS          THEME
STATS               WARDROBE              RELOAD PWS
MORE ──────────►    SETTINGS ────────►    ◄── BACK
EXIT                ◄── BACK
```
**EXIT** returns to the main watch screen.

### Wardrobe (inside)
| Hold time | Action |
|-----------|--------|
| Short (<0.6s) | Move cursor to next item |
| Medium (0.6–1.5s) | **Equip / toggle** selected item |
| Long (1.5–2.5s) | Switch to next tab |
| Very long (2.5s+) | Exit wardrobe |

The highlighted item (dark green) is what you'll equip on a medium hold.

### Other Screens
| Screen | Short press | Long hold |
|--------|-------------|-----------|
| Achievements | Next page | Exit |
| Theme picker | Next theme | Exit |
| Stats | Close | — |
| Level-up / Achievement popup | Dismiss | — |

---

## Arduino Setup

### Board Settings
| Setting | Value |
|---------|-------|
| Board | ESP32 Dev Module |
| Upload Speed | 921600 |
| Flash Size | 4MB (32Mb) |
| Partition Scheme | Default 4MB with spiffs |

### SD Library Fix (important)
If you see **"Multiple libraries found for SD.h"**:
1. Arduino IDE → **Tools → Manage Libraries**
2. Search for `SD`
3. **Uninstall** any "SD" library by Arduino or SparkFun
4. Keep only the one bundled with the ESP32 board package

### No external libraries needed
Only ESP32 built-in libraries are used: `SPI.h`, `WiFi.h`, `esp_wifi.h`, `SD.h`, `Preferences.h`

---

## Cracking Captured Handshakes

Copy `.pcap` files from `/zbg/brainz/` off the SD card.

**hashcat (recommended):**
```bash
# Convert pcap to hashcat format
hcxpcapngtool -o out.hc22000 s0001.pcap

# Crack with a wordlist
hashcat -m 22000 out.hc22000 wordlist.txt

# With rules for better coverage
hashcat -m 22000 out.hc22000 wordlist.txt -r rules/best64.rule
```

**aircrack-ng:**
```bash
aircrack-ng s0001.pcap -w wordlist.txt
```

Good wordlists: `rockyou.txt`, SecLists WiFi passwords, or generate targeted lists with `crunch` or `CUPP`.

---

## Cracked Passwords on Screen

1. Put cracked passwords in `/zbg/passwords.txt` on the SD card — one per line
2. They'll cycle every 4 seconds in the bottom bar of the main screen
3. Use **Settings → Reload PWS** in the menu to reload the file without rebooting

---

## Files

```
zombigotchi/
├── zombigotchi.ino      ← Full sketch, single file, no dependencies
├── README.md            ← This file
└── GITHUB_GUIDE.md      ← How to upload this project to GitHub
```

---

## Legal Notice

For **educational and authorized security testing only**. Sending deauth frames or capturing traffic on networks you don't own is illegal in most countries. Only use on your own networks or with explicit written permission.

---

## Inspired By

[Pwnagotchi](https://pwnagotchi.ai/) — the original AI-powered handshake pet on Raspberry Pi Zero. Zombigotchi is a standalone ESP32 reimplementation: no AI, no Python, no Linux, no internet dependency. Just raw WiFi packet handling on a microcontroller.
