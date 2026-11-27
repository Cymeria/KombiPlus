/*
 * ============================================================================
 *  OledTheme - OLED tema modulu
 *
 *  Tema = ekran yerlesimini belirleyen kucuk bir veri yapisi. Temalar
 *  LittleFS'de /themes/<id>.txt dosyalari olarak tutulur; dosya yoksa ilk
 *  acilista gomulu varsayilanlardan olusturulur. Dosyadaki anahtar=deger
 *  satirlarini duzenleyip cihazi yeniden baslatarak tema ozellestirilebilir.
 *
 *  Tum temalar ayni ana ekran yerlesimini kullanir; yalnizca ust banttaki
 *  simge stili (ikon=) ve buyuk sicaklik yazisinin fontu (font=) degisir.
 * ============================================================================
 */

#ifndef OLED_THEME_H
#define OLED_THEME_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

// Gomulu temalar: 5 simge stili x 2 font = 10 tema.
#define OLED_THEME_COUNT   10
#define OLED_THEME_DIR     "/themes"
#define OLED_THEME_ACTIVE  "/themes/active.txt"

// Iki renkli (sari/mavi) SSD1306 modullerinde ust 16 piksel her zaman saridir.
// Ust bant bildirim alani olarak kullanilir; gövde 16. pikselin altinda baslar.
#define OLED_BAND_H        16

// Ust (sari) bantta simge olarak gosterilen bildirimler.
// Sensor, kombi ve wifi durumu her zaman bir simgeyle gosterilir; bayrak
// set ise simge "hata/kopuk" varyantina doner.
#define OLED_NOTE_SENSOR   0x01   // oda sensoru hatasi
#define OLED_NOTE_NOLINK   0x02   // kombi baglantisi yok
#define OLED_NOTE_AP       0x04   // wifi yok (AP modu)
#define OLED_NOTE_SLEEP    0x08   // uyku modu etkin
#define OLED_NOTE_BOOST    0x10   // boost etkin
#define OLED_NOTE_OFF      0x20   // mod kapali

struct OledTheme {
  char id[16];
  char name[20];
  uint8_t header;       // 0 sade | 1 tek cizgi | 2 cift cizgi | 3 kutu | 4 dolu serit
  uint8_t tempSize;     // 2 | 3
  uint8_t tempCenter;   // 0 sol | 1 orta
  uint8_t setpointCol;  // sagda hedef kolonu
  uint8_t separator;    // 0 yok | 1 tek | 2 cift | 3 noktali
  uint8_t statusStyle;  // 0 metin | 1 talep bar | 2 cerceve | 3 ters serit
  uint8_t showHum;      // nem
  uint8_t showBoost;    // boost
  uint8_t showWater;    // kombi su sicakligi
  uint8_t showPage;     // sayfa sayaci
  uint8_t labels;       // etiketler (HEDEF vb.)
  uint8_t invert;       // tum ekran negatif
  uint8_t dim;          // dusuk kontrast
  uint8_t icons;        // 0 cizgi | 1 ince | 2 dolu | 3 kutu | 4 nokta
  uint8_t font;         // 0 gfx | 1 yedi parcali (dijital)
};

void oledThemeDefaults(OledTheme &t);
bool oledThemeParse(OledTheme &t, const String &key, const String &val);
String oledThemeToFile(const OledTheme &t);

class OledThemeStore {
 public:
  bool begin(bool fsReady);
  bool setActive(const String &id);
  bool step(int dir);
  bool reload();

  uint8_t index() const { return idx; }
  const OledTheme &active() const { return cur; }
  const OledTheme &builtin(uint8_t i) const {
    return (i < OLED_THEME_COUNT) ? defs[i] : defs[0];
  }
  bool fsOk() const { return fsReady; }
  bool filesOk() const { return filesReady; }

 private:
  OledTheme cur;
  OledTheme defs[OLED_THEME_COUNT];
  uint8_t idx = 0;
  bool fsReady = false;
  bool filesReady = false;

  bool ensureFiles();
  bool loadThemeFile(const String &id, OledTheme &out);
  bool saveActiveId();
};

// Cizim yardimcilari (sayfa icerigi uygulamaya, cerceve temaya aittir)
int  oledThemeHeaderHeight(const OledTheme &t);
// Ust bandi bildirim simgeleri, sayfa noktalari ve saatle cizer.
// clock NULL/"" ise saat cizilmez; baslik metni hic yazilmaz.
void oledThemeStatusBar(Adafruit_GFX &d, const OledTheme &t, uint8_t notes,
                        int page, int pageCount, const char *clock);
void oledThemeSeparator(Adafruit_GFX &d, const OledTheme &t, int y);
void oledThemeStatus(Adafruit_GFX &d, const OledTheme &t, int y,
                     const __FlashStringHelper *label, bool highlight);
const __FlashStringHelper *oledThemeHeaderName(uint8_t header);
const __FlashStringHelper *oledThemeStatusName(uint8_t style);

#endif  // OLED_THEME_H
