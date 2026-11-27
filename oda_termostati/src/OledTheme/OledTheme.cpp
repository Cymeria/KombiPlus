/*
 * ============================================================================
 *  OledTheme - OLED tema modulu (uygulama)
 * ============================================================================
 */

#include "OledTheme.h"

#include <Adafruit_SSD1306.h>
#include <LittleFS.h>

// Gomulu varsayilan temalar. Kolonlar:
// id, ad, header, tempSize, tempCenter, setpointCol, separator, statusStyle,
// showHum, showBoost, showWater, showPage, labels, invert, dim, icons, font
// Tum temalar ayni ana ekran yerlesimini kullanir; yalnizca simge stili ve
// buyuk sicaklik fontu degisir.
static const OledTheme THEME_TABLE[OLED_THEME_COUNT] = {
  //    id          ad           hdr ts tc sp sep st nem bst su syf lbl inv dim ikn fnt
  {"klasik",  "Klasik",      1, 3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
  {"ince",    "Ince",        1, 3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0},
  {"dolu",    "Dolu",        1, 3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 2, 0},
  {"kutu",    "Kutu",        1, 3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 3, 0},
  {"nokta",   "Nokta",       1, 3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 4, 0},
  {"dijital", "Dijital",     1, 3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
  {"neon",    "Neon",        1, 3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 2, 1},
  {"panel",   "Panel",       1, 3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 3, 1},
  {"minimal", "Minimal",     1, 3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1},
  {"retro",   "Retro",       1, 3, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 4, 1},
};

// ============================ YARDIMCILAR ==================================

void oledThemeDefaults(OledTheme &t) {
  t = THEME_TABLE[0];
}

static uint8_t clampByte(const String &v, uint8_t lo, uint8_t hi, uint8_t def) {
  if (v.length() == 0) return def;
  long n = v.toInt();
  if (n < (long)lo) n = lo;
  if (n > (long)hi) n = hi;
  return (uint8_t)n;
}

static void setText(char *dst, size_t size, const String &v) {
  if (v.length() == 0) return;
  strncpy(dst, v.c_str(), size - 1);
  dst[size - 1] = '\0';
}

bool oledThemeParse(OledTheme &t, const String &rawKey, const String &rawVal) {
  String key = rawKey;
  String val = rawVal;
  key.trim();
  key.toLowerCase();
  val.trim();
  if (key.length() == 0) return false;

  if (key == "id")                    setText(t.id, sizeof(t.id), val);
  else if (key == "adi" || key == "name")  setText(t.name, sizeof(t.name), val);
  else if (key == "header")           t.header = clampByte(val, 0, 4, t.header);
  else if (key == "tempsize")         t.tempSize = clampByte(val, 2, 3, t.tempSize);
  else if (key == "tempcenter")       t.tempCenter = clampByte(val, 0, 1, t.tempCenter);
  else if (key == "setpointcol")      t.setpointCol = clampByte(val, 0, 1, t.setpointCol);
  else if (key == "separator")        t.separator = clampByte(val, 0, 3, t.separator);
  else if (key == "status")           t.statusStyle = clampByte(val, 0, 3, t.statusStyle);
  else if (key == "nem" || key == "hum")    t.showHum = clampByte(val, 0, 1, t.showHum);
  else if (key == "boost")            t.showBoost = clampByte(val, 0, 1, t.showBoost);
  else if (key == "su" || key == "water")   t.showWater = clampByte(val, 0, 1, t.showWater);
  else if (key == "sayfa" || key == "page") t.showPage = clampByte(val, 0, 1, t.showPage);
  else if (key == "etiket" || key == "labels") t.labels = clampByte(val, 0, 1, t.labels);
  else if (key == "ters" || key == "invert")   t.invert = clampByte(val, 0, 1, t.invert);
  else if (key == "dim")              t.dim = clampByte(val, 0, 1, t.dim);
  else if (key == "ikon" || key == "icons")  t.icons = clampByte(val, 0, 4, t.icons);
  else if (key == "font")             t.font = clampByte(val, 0, 1, t.font);
  else return false;

  return true;
}

String oledThemeToFile(const OledTheme &t) {
  String s;
  s.reserve(420);
  s  = "# KombiPlus OLED temasi\n";
  s += "# Degerleri degistirip cihazi yeniden baslatin (veya /api/theme?reload=1).\n";
  s += "# header: 0 sade, 1 tek cizgi, 2 cift cizgi, 3 kutu, 4 dolu serit\n";
  s += "# status: 0 metin, 1 talep bar, 2 cerceve, 3 ters serit\n";
  s += "# separator: 0 yok, 1 tek, 2 cift, 3 noktali\n";
  s += "# ikon: 0 cizgi, 1 ince, 2 dolu, 3 kutu, 4 nokta\n";
  s += "# font: 0 gfx, 1 yedi parcali (dijital)\n";
  s += "id=" + String(t.id) + "\n";
  s += "adi=" + String(t.name) + "\n";
  s += "header=" + String(t.header) + "\n";
  s += "tempsize=" + String(t.tempSize) + "\n";
  s += "tempcenter=" + String(t.tempCenter) + "\n";
  s += "setpointcol=" + String(t.setpointCol) + "\n";
  s += "separator=" + String(t.separator) + "\n";
  s += "status=" + String(t.statusStyle) + "\n";
  s += "nem=" + String(t.showHum) + "\n";
  s += "boost=" + String(t.showBoost) + "\n";
  s += "su=" + String(t.showWater) + "\n";
  s += "sayfa=" + String(t.showPage) + "\n";
  s += "etiket=" + String(t.labels) + "\n";
  s += "ters=" + String(t.invert) + "\n";
  s += "dim=" + String(t.dim) + "\n";
  s += "ikon=" + String(t.icons) + "\n";
  s += "font=" + String(t.font) + "\n";
  return s;
}

// ============================ GOMULU / FS ==================================

static String themePath(const char *id) {
  return String(OLED_THEME_DIR) + "/" + id + ".txt";
}

static bool writeTextFile(const String &path, const String &content) {
  File f = LittleFS.open(path.c_str(), "w");
  if (!f) return false;
  f.print(content);
  f.close();
  return true;
}

bool OledThemeStore::begin(bool ready) {
  for (uint8_t i = 0; i < OLED_THEME_COUNT; i++) defs[i] = THEME_TABLE[i];
  cur = defs[0];
  idx = 0;
  fsReady = ready;
  filesReady = false;

  if (!fsReady) {
    Serial.println(F("Tema: LittleFS yok, gomulu temalar kullaniliyor."));
    return false;
  }

  ensureFiles();
  filesReady = true;

  // Son secilen tema active.txt'de tutulur; ayar dosyasi daha onceliklidir.
  String saved = "";
  if (LittleFS.exists(OLED_THEME_ACTIVE)) {
    File f = LittleFS.open(OLED_THEME_ACTIVE, "r");
    if (f) {
      while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;
        int eq = line.indexOf('=');
        if (eq <= 0) continue;
        String k = line.substring(0, eq);
        k.trim();
        k.toLowerCase();
        if (k == "id") {
          saved = line.substring(eq + 1);
          saved.trim();
          break;
        }
      }
      f.close();
    }
  }
  if (saved.length() > 0) setActive(saved);
  return true;
}

bool OledThemeStore::ensureFiles() {
  if (!LittleFS.exists(OLED_THEME_DIR)) {
    if (!LittleFS.mkdir(OLED_THEME_DIR)) {
      Serial.println(F("Tema: /themes klasoru olusturulamadi."));
      return false;
    }
  }

  uint8_t created = 0;
  for (uint8_t i = 0; i < OLED_THEME_COUNT; i++) {
    String p = themePath(defs[i].id);
    if (LittleFS.exists(p)) continue;
    if (writeTextFile(p, oledThemeToFile(defs[i]))) created++;
    else Serial.printf("Tema yazilamadi: %s\n", p.c_str());
  }

  if (!LittleFS.exists(OLED_THEME_ACTIVE)) {
    String s = F("# Aktif OLED temasi\n");
    s += "id=";
    s += defs[0].id;
    s += "\n";
    writeTextFile(OLED_THEME_ACTIVE, s);
  }

  Serial.printf("Tema: %u dosya olusturuldu (%s)\n", created, OLED_THEME_DIR);
  return true;
}

bool OledThemeStore::loadThemeFile(const String &id, OledTheme &out) {
  String p = themePath(id.c_str());
  if (!LittleFS.exists(p)) return false;
  File f = LittleFS.open(p.c_str(), "r");
  if (!f) return false;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || line.startsWith("#")) continue;
    int eq = line.indexOf('=');
    if (eq <= 0) continue;
    oledThemeParse(out, line.substring(0, eq), line.substring(eq + 1));
  }
  f.close();

  // Dosyadan gelen kimlik bos/bozuksa varsayilani koru
  if (out.id[0] == '\0') setText(out.id, sizeof(out.id), id);
  return true;
}

bool OledThemeStore::saveActiveId() {
  if (!filesReady) return false;
  String s = F("# Aktif OLED temasi\n");
  s += "id=";
  s += cur.id;
  s += "\n";
  return writeTextFile(OLED_THEME_ACTIVE, s);
}

bool OledThemeStore::setActive(const String &id) {
  String want = id;
  want.trim();

  int found = -1;
  for (uint8_t i = 0; i < OLED_THEME_COUNT; i++) {
    if (want.equalsIgnoreCase(defs[i].id)) { found = i; break; }
  }
  if (found < 0) return false;

  idx = (uint8_t)found;
  cur = defs[idx];

  if (filesReady) {
    OledTheme t = defs[idx];
    if (loadThemeFile(defs[idx].id, t)) {
      setText(t.id, sizeof(t.id), String(defs[idx].id));   // kimlik degismesin
      cur = t;
    }
  }
  saveActiveId();
  return true;
}

bool OledThemeStore::step(int dir) {
  int n = (int)idx + (dir < 0 ? -1 : 1);
  if (n < 0) n = OLED_THEME_COUNT - 1;
  if (n >= OLED_THEME_COUNT) n = 0;
  return setActive(String(defs[(uint8_t)n].id));
}

bool OledThemeStore::reload() {
  if (!filesReady) return false;
  OledTheme t = defs[idx];
  if (!loadThemeFile(defs[idx].id, t)) return false;
  setText(t.id, sizeof(t.id), String(defs[idx].id));
  cur = t;
  return true;
}

// ============================ CIZIM ========================================

// Baslik her temada ust (sari) bantta durur; gövde OLED_BAND_H'den sonra baslar.
int oledThemeHeaderHeight(const OledTheme &t) {
  (void)t;
  return OLED_BAND_H;
}

// ---- Bildirim simgeleri (12 px yuva, ~11 px cizim) -----------------------
// Tum temalar ayni govde geometrisini kullanir; stil yalnizca kalinligi ve
// dolguyu degistirir: 0 cizgi (2 px), 1 ince (1 px), 2 dolu (3 px),
// 3 kutu (1 px + cerceve), 4 noktali (seyrek pikseller).

static int iconT(uint8_t s) { return (s == 2) ? 3 : (s == 1 ? 1 : 2); }

static void iconH(Adafruit_GFX &d, uint8_t s, int x, int y, int w, uint16_t c) {
  if (w <= 0) return;
  if (s == 4) { for (int i = 0; i < w; i += 2) d.drawPixel(x + i, y, c); return; }
  d.fillRect(x, y, w, iconT(s), c);
}

static void iconV(Adafruit_GFX &d, uint8_t s, int x, int y, int h, uint16_t c) {
  if (h <= 0) return;
  if (s == 4) { for (int i = 0; i < h; i += 2) d.drawPixel(x, y + i, c); return; }
  d.fillRect(x, y, iconT(s), h, c);
}

static void iconFrame(Adafruit_GFX &d, uint8_t s, int x, int y, int w, int h, uint16_t c) {
  if (s == 4) {
    for (int i = 0; i < w; i += 2) { d.drawPixel(x + i, y, c); d.drawPixel(x + i, y + h - 1, c); }
    for (int i = 0; i < h; i += 2) { d.drawPixel(x, y + i, c); d.drawPixel(x + w - 1, y + i, c); }
    return;
  }
  int t = (s == 2) ? 2 : 1;
  for (int i = 0; i < t; i++) d.drawRect(x + i, y + i, w - 2 * i, h - 2 * i, c);
}

// Oda sensoru: termometre (saglikli).
static void iconThermo(Adafruit_GFX &d, uint8_t s, int x, int y, uint16_t c) {
  int cx = x + 4;
  int t  = iconT(s);
  iconV(d, s, cx - t / 2, y + 1, 6, c);
  if (s == 2)      d.fillCircle(cx, y + 9, 3, c);
  else if (s == 1) d.drawPixel(cx, y + 9, c);
  else             d.fillCircle(cx, y + 9, 2, c);
}

// Oda sensoru hatasi: termometre + "!"
static void iconSensorErr(Adafruit_GFX &d, uint8_t s, int x, int y, uint16_t c) {
  iconThermo(d, s, x, y, c);
  iconV(d, s, x + 9, y + 1, 6, c);
  if (s == 2) d.fillRect(x + 8, y + 9, 3, 2, c);
  else        d.drawPixel(x + 9, y + 9, c);
}

// Kombi bagi var: ic ice iki halka.
static void iconLinkOn(Adafruit_GFX &d, uint8_t s, int x, int y, uint16_t c) {
  iconFrame(d, s, x, y + 3, 7, 6, c);
  iconFrame(d, s, x + 4, y + 3, 7, 6, c);
}

// Kombi bagi yok: kopuk halka.
static void iconLinkOff(Adafruit_GFX &d, uint8_t s, int x, int y, uint16_t c) {
  iconFrame(d, s, x, y + 3, 6, 6, c);
  iconFrame(d, s, x + 5, y + 3, 6, 6, c);
  d.drawLine(x + 3, y + 1, x + 7, y + 10, c);
  if (s == 2) d.drawLine(x + 4, y + 1, x + 8, y + 10, c);
}

// Wifi bagi var: yaylar.
static void iconWifiOn(Adafruit_GFX &d, uint8_t s, int x, int y, uint16_t c) {
  int cx = x + 5;
  if (s == 2)      d.fillRect(cx - 1, y + 7, 3, 3, c);
  else if (s == 1) d.drawPixel(cx, y + 8, c);
  else             d.fillRect(cx, y + 7, 2, 2, c);
  iconH(d, s, cx - 1, y + 5, 3, c);
  iconH(d, s, cx - 3, y + 3, 7, c);
  iconH(d, s, cx - 4, y + 1, 9, c);
}

// Wifi bagi yok (AP modu): yaylar + capraz.
static void iconWifiOff(Adafruit_GFX &d, uint8_t s, int x, int y, uint16_t c) {
  iconWifiOn(d, s, x, y, c);
  d.drawLine(x + 0, y + 1, x + 10, y + 10, c);
  if (s == 2) d.drawLine(x + 1, y + 1, x + 11, y + 10, c);
}

// Mod kapali: guc simgesi.
static void iconPower(Adafruit_GFX &d, uint8_t s, int x, int y, uint16_t c) {
  int cx = x + 5;
  d.drawCircle(cx, y + 6, 4, c);
  if (s == 2) d.drawCircle(cx, y + 6, 3, c);
  iconV(d, s, cx - iconT(s) / 2, y, 5, c);
}

// Uyku modu: hilal
static void iconMoon(Adafruit_GFX &d, uint8_t s, int x, int y, uint16_t c) {
  uint16_t cut = c ? SSD1306_BLACK : SSD1306_WHITE;
  if (s == 1) {
    d.fillCircle(x + 4, y + 5, 3, c);
    d.fillCircle(x + 7, y + 3, 3, cut);
    return;
  }
  d.fillCircle(x + 5, y + 5, 4, c);
  d.fillCircle(x + 8, y + 4, 4, cut);
}

// Boost: simsek
static void iconBolt(Adafruit_GFX &d, uint8_t s, int x, int y, uint16_t c) {
  d.drawLine(x + 6, y + 1, x + 2, y + 6, c);
  d.drawLine(x + 2, y + 6, x + 5, y + 6, c);
  d.drawLine(x + 5, y + 6, x + 3, y + 11, c);
  d.drawLine(x + 3, y + 11, x + 8, y + 5, c);
  d.drawLine(x + 8, y + 5, x + 5, y + 5, c);
  d.drawLine(x + 5, y + 5, x + 6, y + 1, c);
  if (s == 2) {
    d.drawLine(x + 7, y + 1, x + 3, y + 6, c);
    d.drawLine(x + 4, y + 11, x + 9, y + 5, c);
  }
}

// Kutu stilinde her simgenin arkasina 11x12 cerceve.
static void iconBox(Adafruit_GFX &d, uint8_t s, int x, int y, uint16_t c) {
  if (s == 3) d.drawRect(x, y, 11, 12, c);
}

// Ust (sari) bant = durum alani: solda sensor/kombi/wifi simgeleri,
// ortada sayfa noktalari, en sagda guncel saat. Baslik metni cizilmez.
void oledThemeStatusBar(Adafruit_GFX &d, const OledTheme &t, uint8_t notes,
                        int page, int pageCount, const char *clock) {
  const int band = OLED_BAND_H;
  uint16_t fg = SSD1306_WHITE;

  if (t.header == 4) {
    d.fillRect(0, 0, 128, band, SSD1306_WHITE);
    fg = SSD1306_BLACK;
  }

  // Uc baglanti durumu her zaman gorunur; hata bayragi simgeyi degistirir.
  const uint8_t st = t.icons;
  int x = 2;
  if (notes & OLED_NOTE_SENSOR) iconSensorErr(d, st, x, 2, fg);
  else                          iconThermo(d, st, x, 2, fg);
  iconBox(d, st, x, 2, fg);
  x += 12;
  if (notes & OLED_NOTE_NOLINK) iconLinkOff(d, st, x, 2, fg);
  else                          iconLinkOn(d, st, x, 2, fg);
  iconBox(d, st, x, 2, fg);
  x += 12;
  if (notes & OLED_NOTE_AP)     iconWifiOff(d, st, x, 2, fg);
  else                          iconWifiOn(d, st, x, 2, fg);
  iconBox(d, st, x, 2, fg);
  x += 12;
  if (notes & OLED_NOTE_OFF)   { iconPower(d, st, x, 2, fg); iconBox(d, st, x, 2, fg); x += 12; }
  if (notes & OLED_NOTE_SLEEP) { iconMoon(d, st, x, 2, fg);  iconBox(d, st, x, 2, fg); x += 12; }
  if (notes & OLED_NOTE_BOOST) { iconBolt(d, st, x, 2, fg);  iconBox(d, st, x, 2, fg); x += 12; }

  int clockX = 0;
  if (clock && clock[0]) {
    int w = (int)strlen(clock) * 6;
    clockX = 128 - w;
    if (clockX < x) clockX = x;
    d.setTextSize(1);
    d.setTextColor(fg);
    d.setCursor(clockX, 4);
    d.print(clock);
  }

  if (pageCount > 1 && t.showPage) {
    const int dot = 2, gap = 2;
    int total = pageCount * dot + (pageCount - 1) * gap;
    int dx = (clockX > 0 ? clockX - 3 : 126) - total;
    if (dx < x) dx = x;
    int cy = band / 2 - 1;
    for (int i = 0; i < pageCount; i++) {
      int px = dx + i * (dot + gap);
      if (i == page) d.fillRect(px, cy, dot, dot, fg);
      else          d.drawRect(px, cy, dot, dot, fg);
    }
  }

  d.setTextColor(SSD1306_WHITE);

  switch (t.header) {
    case 1: d.drawFastHLine(0, band - 1, 128, SSD1306_WHITE); break;
    case 2:
      d.drawFastHLine(0, band - 2, 128, SSD1306_WHITE);
      d.drawFastHLine(0, band - 1, 128, SSD1306_WHITE);
      break;
    case 3: d.drawRect(0, 1, 128, band - 2, SSD1306_WHITE); break;
    default: break;   // 0: sade, 4: bant zaten dolu
  }
}

void oledThemeSeparator(Adafruit_GFX &d, const OledTheme &t, int y) {
  if (y < 0 || y > 63) return;
  switch (t.separator) {
    case 1:
      d.drawFastHLine(0, y, 128, SSD1306_WHITE);
      break;
    case 2:
      if (y > 0) d.drawFastHLine(0, y - 1, 128, SSD1306_WHITE);
      d.drawFastHLine(0, y, 128, SSD1306_WHITE);
      break;
    case 3:
      for (int x = 0; x < 128; x += 3) d.drawPixel(x, y, SSD1306_WHITE);
      break;
    default:
      break;
  }
}

void oledThemeStatus(Adafruit_GFX &d, const OledTheme &t, int y,
                     const __FlashStringHelper *label, bool highlight) {
  if (y < 0) y = 0;
  if (y > 52) y = 52;

  d.setTextSize(1);
  d.setTextColor(SSD1306_WHITE);

  // Durum metni (ISITIYOR, BEKLIYOR, ...) her temada yatay ortalanir.
  int tw = (int)strlen_P((PGM_P)label) * 6;
  int tx = (128 - tw) / 2;
  if (tx < 0) tx = 0;

  switch (t.statusStyle) {
    case 1:   // talep varken dolu bar
      if (highlight) {
        d.fillRect(0, y - 1, 128, 11, SSD1306_WHITE);
        d.setTextColor(SSD1306_BLACK);
        d.setCursor(tx, y);
        d.print(label);
        d.setTextColor(SSD1306_WHITE);
      } else {
        d.setCursor(tx, y);
        d.print(label);
      }
      break;
    case 2:   // cerceve
      d.drawRect(0, y - 1, 128, 11, SSD1306_WHITE);
      d.setCursor(tx, y);
      d.print(label);
      break;
    case 3:   // her zaman ters serit
      d.fillRect(0, y - 1, 128, 11, SSD1306_WHITE);
      d.setTextColor(SSD1306_BLACK);
      d.setCursor(tx, y);
      d.print(label);
      d.setTextColor(SSD1306_WHITE);
      break;
    default:  // sade metin
      d.setCursor(tx, y);
      d.print(label);
      break;
  }
}

const __FlashStringHelper *oledThemeHeaderName(uint8_t header) {
  switch (header) {
    case 0:  return F("sade");
    case 1:  return F("cizgi");
    case 2:  return F("cift");
    case 3:  return F("kutu");
    case 4:  return F("serit");
    default: return F("?");
  }
}

const __FlashStringHelper *oledThemeStatusName(uint8_t style) {
  switch (style) {
    case 0:  return F("metin");
    case 1:  return F("bar");
    case 2:  return F("cerceve");
    case 3:  return F("ters");
    default: return F("?");
  }
}
