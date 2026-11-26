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
// showHum, showBoost, showWater, showPage, labels, invert, dim
static const OledTheme THEME_TABLE[OLED_THEME_COUNT] = {
  {"klasik",  "Klasik",     1, 2, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
  {"minimal", "Minimal",    0, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {"ters",    "Ters",       4, 2, 0, 1, 1, 3, 1, 1, 0, 1, 1, 1, 0},
  {"kutu",    "Kutu",       3, 2, 0, 1, 2, 2, 1, 1, 1, 1, 1, 0, 0},
  {"genis",   "Genis",      1, 3, 1, 0, 2, 1, 1, 1, 0, 1, 0, 0, 0},
  {"gece",    "Gece",       2, 3, 1, 0, 3, 0, 0, 1, 0, 0, 0, 0, 1},
  {"retro",   "Retro",      2, 2, 0, 1, 3, 2, 1, 1, 1, 1, 1, 0, 0},
  {"durum",   "Durum",      1, 2, 0, 0, 0, 3, 1, 1, 1, 1, 0, 0, 0},
  {"kompakt", "Kompakt",    4, 2, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0},
  {"dev",     "Dev Rakam",  0, 3, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
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

void oledThemeHeader(Adafruit_GFX &d, const OledTheme &t,
                     const __FlashStringHelper *title, int page, int pageCount) {
  d.setTextSize(1);
  d.setTextColor(SSD1306_WHITE);

  char pg[8];
  pg[0] = '\0';
  int pgW = 0;
  if (t.showPage && pageCount > 1) {
    snprintf(pg, sizeof(pg), "%d/%d", page + 1, pageCount);
    pgW = (int)strlen(pg) * 6;
  }

  // Sari bant (0..15): baslik her temada burada, gövde 16'nin altinda.
  if (t.header == 4) {
    d.fillRect(0, 0, 128, OLED_BAND_H, SSD1306_WHITE);
    d.setTextColor(SSD1306_BLACK);
  }

  d.setCursor(2, 3);
  d.print(title);
  if (pgW > 0) {
    d.setCursor(126 - pgW, 3);
    d.print(pg);
  }
  d.setTextColor(SSD1306_WHITE);

  switch (t.header) {
    case 1: d.drawFastHLine(0, OLED_BAND_H - 1, 128, SSD1306_WHITE); break;
    case 2:
      d.drawFastHLine(0, OLED_BAND_H - 2, 128, SSD1306_WHITE);
      d.drawFastHLine(0, OLED_BAND_H - 1, 128, SSD1306_WHITE);
      break;
    case 3: d.drawRect(0, 2, 128, OLED_BAND_H - 3, SSD1306_WHITE); break;
    default: break;   // 0: sade baslik, 4: bant zaten dolu
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
