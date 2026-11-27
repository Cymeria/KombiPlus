/*
 * ============================================================================
 *  KOMBIPLUS - ODA UNITESI
 *  NodeMCU v3 (ESP8266) + 0.96" SSD1306 OLED + Rotary Encoder
 *  Sensor: DHT11 / DHT22 / DS18B20 (asagidaki SENSOR_TYPE ile secilir)
 *
 *  Bu cihaz:
 *    - Oda sicakligini okur, hedefe gore isi talebini hesaplar
 *    - Talebi Wi-Fi (TCP 8266) uzerinden Kombi Role Unitesine gonderir
 *    - AP + STA modunda mobil uyumlu web arayuzu sunar
 *    - Captive portal: AP'ye baglanan telefon/PC arayuzu otomatik acar
 *    - Ilk kurulum (dil, Wi-Fi, ayarlar) web arayuzunden yapilabilir
 *    - OLED uzerinde durum, ayar ve ag bilgilerini gosterir
 *    - MQTT ile durum yayinlar, komut kabul eder
 *    - Telegram botu ile uzaktan kontrol edilebilir
 *    - Uyku saatlerinde farkli hedef sicaklik uygular
 *    - Ayarlari ve saatlik sicaklik gecmisini LittleFS'de tutar
 *
 *  ONEMLI: Kombi role unitesi bu cihazin AP'sine baglanir.
 *          AP SSID/sifresi iki projede de AYNI olmalidir.
 * ============================================================================
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <time.h>

#include "src/LittleFSExplorer/LittleFSExplorer.h"
#include "src/OledTheme/OledTheme.h"
#include "web_ui.h"
#include "web_test.h"

// ============================ KULLANICI AYARLARI ============================

#define FW_VERSION "1.1.0"

// ---- MQTT / Telegram / gecmis (derleme zamani ac/kapat) ----
#define MQTT_ENABLED        1
#define TELEGRAM_ENABLED    1
#define HISTORY_ENABLED     1
#define HISTORY_MAX_LINES   720          // ~30 gun (saatlik)
#define TZ_OFFSET_SEC       (3 * 3600)   // Turkiye (UTC+3)
#define NTP_SERVER1         "pool.ntp.org"
#define NTP_SERVER2         "time.google.com"

// ---- Sensor secimi: SENSOR_DHT11 / SENSOR_DHT22 / SENSOR_DS18B20 ----
#define SENSOR_DHT11    1
#define SENSOR_DHT22    2
#define SENSOR_DS18B20  3

#define SENSOR_TYPE     SENSOR_DHT22      // <<<<< BURAYI DEGISTIRIN

// ---- AP (Kombi role unitesi bu aga baglanir) ----
#define AP_SSID         "KombiPlus"
#define AP_PASS         "Kombi.1881"     // en az 8 karakter
#define OTA_PASS        "1881"
#define MDNS_NAME       "KombiPlus"

// ---- Pinler (NodeMCU v3 kart uzerindeki D etiketleri) ----
// OLED varsayilan pinleri: SDA = D5, SCL = D6.
// Farkli bir kablolama kullanacaksaniz asagidaki degerleri degistirin.
const uint8_t PIN_SDA      = D5;   // GPIO14 OLED SDA (varsayilan)
const uint8_t PIN_SCL      = D6;   // GPIO12 OLED SCL (varsayilan)
const uint8_t PIN_SENSOR   = D4;   // GPIO2  DHT / DS18B20 data
const uint8_t PIN_ENC_A    = D1;   // GPIO5  Encoder CLK
const uint8_t PIN_ENC_B    = D2;   // GPIO4  Encoder DT
const uint8_t PIN_ENC_SW   = D7;   // GPIO13 Encoder butonu

// ---- Zaman / davranis ----
#define SENSOR_PERIOD_MS      2500UL
#define SENSOR_STALE_MS       30000UL
#define MIN_ON_MS             60000UL    // role min. acik kalma
#define MIN_OFF_MS            180000UL   // role min. kapali kalma
#define BOOST_OFFSET_C        1.5f
#define SETPOINT_MIN_C        5.0f
#define SETPOINT_MAX_C        30.0f
#define SETPOINT_STEP_C       0.5f
#define SETPOINT_PEEK_MS      10000UL  // encoder sonrasi hedef gosterimi
#define OLED_REFRESH_MS       250UL
#define OLED_PAGE_COUNT       4        // 0 ana, 1 ag, 2 bilgi, 3 uyku
#define ENCODER_REVERSE       0        // yon ters geliyorsa 1 yapin

// ---- Web / TCP ----
const uint16_t TCP_PORT = 8266;
const uint16_t OTA_PORT = 3232;

// ---- LittleFS dosyalari ----
#define CFG_FILE       "/config.txt"
#define HIST_FILE      "/history.csv"
#define HIST_TMP_FILE  "/history.tmp"
#define MQTT_PUB_MS    10000UL
#define TG_POLL_MS     20000UL

// ============================ GLOBAL NESNELER ==============================

#if SENSOR_TYPE == SENSOR_DHT11
  #define DHT_MODEL DHT11
  DHT dht(PIN_SENSOR, DHT_MODEL);
#elif SENSOR_TYPE == SENSOR_DHT22
  #define DHT_MODEL DHT22
  DHT dht(PIN_SENSOR, DHT_MODEL);
#else
  OneWire oneWire(PIN_SENSOR);
  DallasTemperature ds18b20(&oneWire);
#endif

Adafruit_SSD1306 display(128, 64, &Wire, -1);
ESP8266WebServer webServer(80);
DNSServer dnsServer;                 // captive portal DNS (AP modu)
WiFiServer boilerServer(TCP_PORT);
WiFiClient boilerClient;

LittleFSExplorer fsExplorer;
OledThemeStore themeStore;

WiFiClient mqttNet;
PubSubClient mqttClient(mqttNet);

struct Settings {
  float setpoint;
  float hysteresis;
  float calibration;
  float frost;
  uint8_t mode;          // 0 = kapali (sadece don korumasi), 1 = acik
  uint8_t sleepEnabled;
  uint16_t sleepStartMin;  // gece yarisindan itibaren dakika
  uint16_t sleepEndMin;
  float sleepSetpoint;
  uint8_t relayFailSafeOn; // oda unitesi erisilemezse role acik kalsin mi
};

Settings cfg;

// EEPROM'da tutulan ESKI yapilandirma (yalnizca gecis/migrasyon icin okunur)
#define CFG_MAGIC   0x4B4D4231UL   // "KMB1"
#define EEPROM_SIZE 256

struct PersistedConfig {
  uint32_t magic;
  float setpoint;
  float hysteresis;
  float calibration;
  float frost;
  uint8_t mode;
  char ssid[33];
  char pass[65];
};

String wifiSsid = "";
String wifiPass = "";
String themeId = "klasik";   // aktif OLED temasi (/themes/<id>.txt)
String uiLang = "";          // arayuz dili: en/tr/ru/es/pt/ja/ko/zh (bos = secilmedi)
uint8_t langCursor = 0;      // ilk acilista OLED dil secimi imleci
bool langPrompt = false;     // dil secimi sorulsun mu (yalnizca yeni cihazda)
int8_t langMarker = -1;      // config'teki langset= degeri (-1: anahtar yok = eski kurulum)
bool setupDone = false;      // ilk kurulum (web karti) tamamlandi mi
bool fsReady = false;

// ---- MQTT ----
String mqttHost = "";
uint16_t mqttPort = 1883;
String mqttUser = "";
String mqttPass = "";
String mqttPrefix = "kombi";
bool mqttWasConnected = false;
unsigned long lastMqttAttemptMs = 0;
unsigned long lastMqttPubMs = 0;
bool mqttStateDirty = true;

// ---- Telegram ----
String tgToken = "";
String tgChat = "";
int32_t tgOffset = 0;
unsigned long lastTgPollMs = 0;
unsigned long tgLastOkMs = 0;
String tgLastError = "";
bool tgBusy = false;

// ---- Uyku / zaman ----
bool sleepActive = false;
bool timeSynced = false;
unsigned long lastTimeCheckMs = 0;

// ---- Sicaklik gecmisi ----
int histLines = 0;
unsigned long lastHistSampleMs = 0;
bool histSamplePending = false;
size_t histBytes = 0;

// ---- Sensor / kontrol durumu ----
float currentTemp = NAN;
float currentHum  = NAN;
bool  sensorOk = false;
uint8_t sensorFailCount = 0;
unsigned long lastValidSensorMs = 0;
unsigned long lastSensorPollMs = 0;

#define FILTER_N 5
float filterBuf[FILTER_N];
uint8_t filterIdx = 0;
uint8_t filterCount = 0;

bool demand = false;
unsigned long lastDemandOnMs = 0;
unsigned long lastDemandOffMs = 0;

bool boostActive = false;
unsigned long boostEndMs = 0;
int boostMinutesSet = 0;

// ---- Kombi role unitesinden gelen durum ----
bool  boilerRelay = false;
float boilerFlow = -127.0f;
int   boilerRssi = 0;
unsigned long boilerUptimeSec = 0;
unsigned long lastBoilerMsgMs = 0;
String boilerRxLine = "";

// ---- Genel ----
uint8_t oledPage = 0;
unsigned long setpointPeekMs = 0;   // hedefin ekranda tutuldugu an (0 = kapali)
bool displayReady = false;
unsigned long lastOledMs = 0;
unsigned long restartAtMs = 0;
bool forceSend = false;
bool settingsDirty = false;
unsigned long settingsChangedAt = 0;
bool otaStarted = false;
bool cleanShutdown = false;

// ============================ YARDIMCI FONKSIYONLAR ========================

float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

float roundToStep(float v, float step) {
  return roundf(v / step) * step;
}

String jsonEscape(const String &in) {
  String out = in;
  out.replace("\\", "\\\\");
  out.replace("\"", "\\\"");
  return out;
}

String currentIp() {
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  return WiFi.softAPIP().toString();
}

uint16_t boostRemainSec() {
  if (!boostActive) return 0;
  long remain = (long)(boostEndMs - millis());
  if (remain <= 0) return 0;
  return (uint16_t)(remain / 1000L);
}

// ============================ SETUP / LOOP =================================

// ISR prototipi (nitelikli tanim icin Arduino on-prototype uretmez)
void encoderISR();

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println();
  Serial.println(F("=== KombiPlus - Oda Unitesi ==="));
  Serial.println(F("FW " FW_VERSION));

  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);

  EEPROM.begin(EEPROM_SIZE);

  // ---- LittleFS: ayarlar ve gecmis ----
  if (!LittleFS.begin()) {
    Serial.println(F("LittleFS baslatilamadi, formatlaniyor..."));
    LittleFS.format();
    fsReady = LittleFS.begin();
  } else {
    fsReady = true;
  }
  Serial.printf("LittleFS: %s\n", fsReady ? "hazir" : "YOK");
  loadSettings();

  // ---- OLED temalari: /themes altinda yoksa ilk acilista olusturulur ----
  themeStore.begin(fsReady);
  if (!themeStore.setActive(themeId)) {
    Serial.printf("Tema '%s' bulunamadi, varsayilana donuldu.\n", themeId.c_str());
    themeStore.setActive("klasik");
  }
  themeId = themeStore.active().id;
  Serial.printf("Tema: %s (%u/%u) %s\n", themeStore.active().name,
                themeStore.index() + 1, OLED_THEME_COUNT,
                themeStore.filesOk() ? "LittleFS" : "gomulu");

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED bulunamadi (0x3C). Web arayuzu calismaya devam eder."));
  } else {
    displayReady = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 8);
    display.println(F("KombiPlus"));
    display.setCursor(0, 24);
    display.println(F("Baslatiliyor..."));
    display.display();
  }

#if SENSOR_TYPE == SENSOR_DS18B20
  ds18b20.begin();
  ds18b20.setResolution(12);
  ds18b20.setWaitForConversion(false);
  Serial.println(F("Sensor: DS18B20"));
#elif SENSOR_TYPE == SENSOR_DHT11
  dht.begin();
  Serial.println(F("Sensor: DHT11"));
#else
  dht.begin();
  Serial.println(F("Sensor: DHT22"));
#endif

  // Encoder interrupt'lari
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), encoderISR, CHANGE);

  // Kontrol zamanlayicilarini baslat
  lastDemandOffMs = millis() - MIN_OFF_MS;
  lastDemandOnMs  = millis() - MIN_ON_MS;

  setupWiFi();
  setupWeb();
  boilerServer.begin();

  configTime(TZ_OFFSET_SEC, 0, NTP_SERVER1, NTP_SERVER2);
  initHistory();

  Serial.print(F("AP IP: ")); Serial.println(WiFi.softAPIP());
  Serial.print(F("Web:   http://")); Serial.println(currentIp());

  delay(800);
}

void loop() {
  unsigned long now = millis();

  webServer.handleClient();
  dnsServer.processNextRequest();
  MDNS.update();
  if (otaStarted) ArduinoOTA.handle();

  updateWiFi();
  handleBoilerTcp();
  updateSensor();
  updateControl();
  updateBoost();
  updateClock();
  handleEncoder();
  handleEncoderButton();
  sendCommandLoop();
  mqttLoop();
  telegramLoop();
  historyLoop();
  drawOled();

  if (settingsDirty && (now - settingsChangedAt > 1500UL)) {
    saveSettings();
    settingsDirty = false;
  }

  if (restartAtMs != 0 && (long)(now - restartAtMs) >= 0) {
    Serial.println(F("Yeniden baslatiliyor..."));
    ESP.restart();
  }

  delay(2);
}

// ============================ AYARLAR / KALICI KAYIT ======================
// Ayarlar LittleFS'de /config.txt (anahtar=deger) olarak tutulur.
// Eski EEPROM yapisi yalnizca ilk acilista migrasyon icin okunur.

String fmtHHMM(uint16_t m) {
  char b[6];
  snprintf(b, sizeof(b), "%02u:%02u", (unsigned)((m / 60) % 24), (unsigned)(m % 60));
  return String(b);
}

uint16_t parseHHMM(const String &s, uint16_t def) {
  int c = s.indexOf(':');
  int h, m;
  if (c < 0) {
    h = s.toInt();
    m = 0;
  } else {
    h = s.substring(0, c).toInt();
    m = s.substring(c + 1).toInt();
  }
  if (h < 0 || h > 23 || m < 0 || m > 59) return def;
  return (uint16_t)(h * 60 + m);
}

// ---- Arayuz dilleri -------------------------------------------------------
// Web arayuzu 8 dili destekler. OLED'in gomulu fontu yalnizca ASCII bastigi
// icin ekran metinleri en/tr/es/pt olarak yerellesir; diger dillerde Ingilizce.
#define LANG_COUNT 8

static const char *const LANG_CODES[LANG_COUNT] = {
  "en", "tr", "ru", "es", "pt", "ja", "ko", "zh"
};

static const char *const LANG_NAMES[LANG_COUNT] = {
  "ENGLISH", "TURKCE", "RUSSIAN", "ESPANOL", "PORTUGUES", "JAPANESE", "KOREAN", "CHINESE"
};

uint8_t langIndex(const String &code) {
  String c = code;
  c.trim();
  c.toLowerCase();
  for (uint8_t i = 0; i < LANG_COUNT; i++) {
    if (c == LANG_CODES[i]) return i;
  }
  return 0xFF;
}

// Dil secimi yalnizca yeni cihazda (ayar dosyasi hic yokken) bir kez sorulur.
// Guncelleme alan cihazlarin ayarlari korunur; dil sorulmaz, arayuz Ingilizce
// baslar ve dil web arayuzundeki Dil kartindan degistirilebilir.
bool langChosen() {
  return !langPrompt;
}

void defaultSettings() {
  cfg.setpoint    = 21.0f;
  cfg.hysteresis  = 0.3f;
  cfg.calibration = 0.0f;
  cfg.frost       = 5.0f;
  cfg.mode        = 1;
  cfg.sleepEnabled = 0;
  cfg.sleepStartMin = 23 * 60;
  cfg.sleepEndMin   = 7 * 60;
  cfg.sleepSetpoint = 18.0f;
  cfg.relayFailSafeOn = 0;

  wifiSsid = "";
  wifiPass = "";
  setupDone = false;

  mqttHost = "";
  mqttPort = 1883;
  mqttUser = "";
  mqttPass = "";
  mqttPrefix = "kombi";

  tgToken = "";
  tgChat  = "";
}

void sanitizeSettings() {
  cfg.setpoint     = clampf(cfg.setpoint, SETPOINT_MIN_C, SETPOINT_MAX_C);
  cfg.hysteresis   = clampf(cfg.hysteresis, 0.1f, 3.0f);
  cfg.calibration  = clampf(cfg.calibration, -5.0f, 5.0f);
  cfg.frost        = clampf(cfg.frost, 2.0f, 10.0f);
  cfg.sleepSetpoint = clampf(cfg.sleepSetpoint, SETPOINT_MIN_C, SETPOINT_MAX_C);
  if (cfg.mode > 1) cfg.mode = 1;
  if (cfg.sleepEnabled > 1) cfg.sleepEnabled = 1;
  if (cfg.relayFailSafeOn > 1) cfg.relayFailSafeOn = 1;
  if (cfg.sleepStartMin > 1439) cfg.sleepStartMin = 23 * 60;
  if (cfg.sleepEndMin > 1439)   cfg.sleepEndMin = 7 * 60;
  if (mqttPort == 0) mqttPort = 1883;
  if (mqttPrefix.length() == 0) mqttPrefix = "kombi";
  mqttPrefix.replace("/", "");
  uiLang.trim();
  uiLang.toLowerCase();
  if (uiLang.length() && langIndex(uiLang) == 0xFF) uiLang = "";
}

void applyConfigKV(const String &key, const String &val) {
  if (key == "sp")              cfg.setpoint = val.toFloat();
  else if (key == "hyst")       cfg.hysteresis = val.toFloat();
  else if (key == "cal")        cfg.calibration = val.toFloat();
  else if (key == "frost")      cfg.frost = val.toFloat();
  else if (key == "mode")       cfg.mode = val.toInt() ? 1 : 0;
  else if (key == "sleep")      cfg.sleepEnabled = val.toInt() ? 1 : 0;
  else if (key == "sleepstart") cfg.sleepStartMin = parseHHMM(val, cfg.sleepStartMin);
  else if (key == "sleepend")   cfg.sleepEndMin = parseHHMM(val, cfg.sleepEndMin);
  else if (key == "sleepsp")    cfg.sleepSetpoint = val.toFloat();
  else if (key == "failsafe")   cfg.relayFailSafeOn = val.toInt() ? 1 : 0;
  else if (key == "theme")      themeId = val;
  else if (key == "lang")       uiLang = val;
  else if (key == "langset")    langMarker = val.toInt() ? 1 : 0;
  else if (key == "setupdone")  setupDone = val.toInt() ? 1 : 0;
  else if (key == "wifissid")   wifiSsid = val;
  else if (key == "wifipass")   wifiPass = val;
  else if (key == "mqtthost")   mqttHost = val;
  else if (key == "mqttport")   mqttPort = (uint16_t)constrain(val.toInt(), 1, 65535);
  else if (key == "mqttuser")   mqttUser = val;
  else if (key == "mqttpass")   mqttPass = val;
  else if (key == "mqttprefix") mqttPrefix = val;
  else if (key == "tgtoken")    tgToken = val;
  else if (key == "tgchat")     tgChat = val;
}

bool loadConfigFromFs() {
  if (!fsReady || !LittleFS.exists(CFG_FILE)) return false;
  File f = LittleFS.open(CFG_FILE, "r");
  if (!f) return false;

  uint16_t keys = 0;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || line.startsWith("#")) continue;
    int eq = line.indexOf('=');
    if (eq <= 0) continue;
    String k = line.substring(0, eq);
    String v = line.substring(eq + 1);
    k.trim();
    v.trim();
    if (k == "ver") continue;
    applyConfigKV(k, v);
    keys++;
  }
  f.close();
  return keys > 0;   // bos dosya "ayar yok" sayilir
}

bool loadLegacyEeprom() {
  PersistedConfig pc;
  EEPROM.get(0, pc);
  bool valid = (pc.magic == CFG_MAGIC) &&
               !isnan(pc.setpoint) && !isnan(pc.hysteresis) &&
               !isnan(pc.calibration) && !isnan(pc.frost);
  if (!valid) return false;

  cfg.setpoint    = pc.setpoint;
  cfg.hysteresis  = pc.hysteresis;
  cfg.calibration = pc.calibration;
  cfg.frost       = pc.frost;
  cfg.mode        = pc.mode;
  pc.ssid[sizeof(pc.ssid) - 1] = '\0';
  pc.pass[sizeof(pc.pass) - 1] = '\0';
  wifiSsid = String(pc.ssid);
  wifiPass = String(pc.pass);
  return true;
}

void loadSettings() {
  defaultSettings();

  bool loaded = loadConfigFromFs();
  bool freshDevice = !loaded;            // ayar dosyasi yok/bos => yeni cihaz
  if (!loaded) {
    if (loadLegacyEeprom()) {
      freshDevice = false;               // eski kurulum EEPROM'dan tasindi
      Serial.println(F("Ayar EEPROM'dan LittleFS'ye tasindi."));
    } else {
      Serial.println(F("Ayar dosyasi yok, varsayilanlar kullaniliyor."));
    }
    sanitizeSettings();
    saveSettings();
  }
  sanitizeSettings();

  // Yeni cihazda bir kez sorulur. Guncelleme alan cihazlarda (langset anahtari
  // yok) sorulmaz, secim yapilmis sayilir ve mevcut dil korunur.
  langPrompt = freshDevice || langMarker == 0;
  if (!langPrompt) langMarker = 1;

  Serial.printf("Ayar: sp=%.1f hyst=%.1f cal=%.1f frost=%.1f mode=%u\n",
                cfg.setpoint, cfg.hysteresis, cfg.calibration, cfg.frost, cfg.mode);
  Serial.printf("Uyku: %u  %s-%s  %.1fC  RoleFS=%u\n",
                cfg.sleepEnabled, fmtHHMM(cfg.sleepStartMin).c_str(),
                fmtHHMM(cfg.sleepEndMin).c_str(), cfg.sleepSetpoint, cfg.relayFailSafeOn);
}

void saveSettings() {
  if (!fsReady) return;

  String s;
  s.reserve(800);
  s  = "# KombiPlus ayarlari\n";
  s += "ver=2\n";
  s += "sp=" + String(cfg.setpoint, 1) + "\n";
  s += "hyst=" + String(cfg.hysteresis, 1) + "\n";
  s += "cal=" + String(cfg.calibration, 1) + "\n";
  s += "frost=" + String(cfg.frost, 1) + "\n";
  s += "mode=" + String(cfg.mode) + "\n";
  s += "sleep=" + String(cfg.sleepEnabled) + "\n";
  s += "sleepstart=" + fmtHHMM(cfg.sleepStartMin) + "\n";
  s += "sleepend=" + fmtHHMM(cfg.sleepEndMin) + "\n";
  s += "sleepsp=" + String(cfg.sleepSetpoint, 1) + "\n";
  s += "failsafe=" + String(cfg.relayFailSafeOn) + "\n";
  s += "theme=" + themeId + "\n";
  s += "lang=" + uiLang + "\n";
  s += "langset=" + String(langPrompt ? 0 : 1) + "\n";
  s += "setupdone=" + String(setupDone ? 1 : 0) + "\n";
  s += "wifissid=" + wifiSsid + "\n";
  s += "wifipass=" + wifiPass + "\n";
  s += "mqtthost=" + mqttHost + "\n";
  s += "mqttport=" + String(mqttPort) + "\n";
  s += "mqttuser=" + mqttUser + "\n";
  s += "mqttpass=" + mqttPass + "\n";
  s += "mqttprefix=" + mqttPrefix + "\n";
  s += "tgtoken=" + tgToken + "\n";
  s += "tgchat=" + tgChat + "\n";

  File f = LittleFS.open(CFG_FILE, "w");
  if (!f) {
    Serial.println(F("Ayar dosyasi yazilamadi!"));
    return;
  }
  f.print(s);
  f.close();
}

// Ilk kurulum karti: ayar dosyasinda setupdone yoksa ve cihaz eve bagli
// degilse web arayuzunde kurulum karti acilir. Ag baglaninca ya da kullanici
// "Simdi degil" derse bir daha sorulmaz.
bool setupNeeded() {
  if (setupDone) return false;
  if (WiFi.status() == WL_CONNECTED) return false;
  if (millis() < 15000UL) return false;    // acilista STA baglantisini bekle
  return true;
}

void markSettingsDirty() {
  settingsDirty = true;
  settingsChangedAt = millis();
  mqttStateDirty = true;
}

// ============================ UYKU / ZAMAN =================================

bool timeReady() {
  time_t t = time(nullptr);
  return t > 1700000000;   // 2023-11'den sonra
}

bool inSleepWindow(uint16_t nowMin) {
  if (!cfg.sleepEnabled) return false;
  uint16_t a = cfg.sleepStartMin;
  uint16_t b = cfg.sleepEndMin;
  if (a == b) return false;
  if (a < b) return nowMin >= a && nowMin < b;
  return nowMin >= a || nowMin < b;      // gece yarisini asan aralik
}

void updateClock() {
  unsigned long now = millis();
  if (now - lastTimeCheckMs < 1000UL) return;
  lastTimeCheckMs = now;

  if (!timeReady()) {
    if (timeSynced) timeSynced = false;
    return;
  }

  if (!timeSynced) {
    timeSynced = true;
    Serial.println(F("NTP zamani alindi."));
  }

  time_t t = time(nullptr);
  struct tm tmv;
  localtime_r(&t, &tmv);
  bool active = inSleepWindow((uint16_t)(tmv.tm_hour * 60 + tmv.tm_min));

  if (active != sleepActive) {
    sleepActive = active;
    Serial.printf("Uyku modu: %s\n", active ? "AKTIF" : "kapali");
    mqttStateDirty = true;
    forceSend = true;
    updateControl();
  }
}

// ============================ SICAKLIK GECMISI =============================

int countHistoryLines() {
  if (!fsReady) return 0;
  File f = LittleFS.open(HIST_FILE, "r");
  if (!f) return 0;
  int n = 0;
  while (f.available()) {
    String l = f.readStringUntil('\n');
    if (l.length() > 0) n++;
  }
  f.close();
  return n;
}

size_t historyFileSize() {
  if (!fsReady) return 0;
  File f = LittleFS.open(HIST_FILE, "r");
  if (!f) return 0;
  size_t s = f.size();
  f.close();
  return s;
}

void initHistory() {
  histLines = countHistoryLines();
  histBytes = historyFileSize();
  histSamplePending = (histLines == 0);
  Serial.printf("Gecmis: %d kayit, %u bayt\n", histLines, (unsigned)histBytes);
}

void trimHistory() {
  File src = LittleFS.open(HIST_FILE, "r");
  if (!src) return;
  int skip = histLines - HISTORY_MAX_LINES + 48;
  File tmp = LittleFS.open(HIST_TMP_FILE, "w");
  if (!tmp) { src.close(); return; }

  int idx = 0;
  while (src.available()) {
    String line = src.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    if (idx++ >= skip) {
      tmp.print(line);
      tmp.print('\n');
    }
  }
  src.close();
  tmp.close();

  LittleFS.remove(HIST_FILE);
  LittleFS.rename(HIST_TMP_FILE, HIST_FILE);
  histLines = countHistoryLines();
  histBytes = historyFileSize();
  Serial.println(F("Gecmis kirpildi."));
}

void appendHistoryPoint() {
  if (!fsReady || !timeReady() || !sensorOk) return;
  File f = LittleFS.open(HIST_FILE, "a");
  if (!f) return;

  f.printf("%lu,%.1f,%.1f,%.1f,%d,%d\n",
           (unsigned long)time(nullptr),
           currentTemp,
           isnan(currentHum) ? -1.0f : currentHum,
           effectiveSetpoint(),
           demand ? 1 : 0,
           sleepActive ? 1 : 0);
  f.close();

  histLines++;
  histBytes = historyFileSize();
  histSamplePending = false;
  mqttStateDirty = true;
  Serial.println(F("Gecmis kaydi eklendi."));

  if (histLines > HISTORY_MAX_LINES) trimHistory();
}

void historyLoop() {
#if HISTORY_ENABLED
  if (!fsReady) return;
  if (!timeReady() || !sensorOk) return;

  if (histSamplePending) {
    appendHistoryPoint();
    return;
  }

  static int lastLogHour = -1;
  time_t t = time(nullptr);
  struct tm tmv;
  localtime_r(&t, &tmv);

  if (lastLogHour < 0) {
    lastLogHour = tmv.tm_hour;
    return;
  }
  if (tmv.tm_hour != lastLogHour) {
    lastLogHour = tmv.tm_hour;
    appendHistoryPoint();
  }
#endif
}


// ============================ SENSOR =======================================

float filterAdd(float v) {
  filterBuf[filterIdx] = v;
  filterIdx = (filterIdx + 1) % FILTER_N;
  if (filterCount < FILTER_N) filterCount++;

  float sum = 0;
  for (uint8_t i = 0; i < filterCount; i++) sum += filterBuf[i];
  return sum / (float)filterCount;
}

void sensorFail() {
  if (sensorFailCount < 255) sensorFailCount++;
  if (sensorFailCount >= 2) {
    if (sensorOk) Serial.println(F("Sensor hatasi!"));
    sensorOk = false;
    filterCount = 0;
    filterIdx = 0;
    setDemand(false);
  }
}

void sensorSuccess(float t, float h) {
  sensorFailCount = 0;
  float filtered = filterAdd(t);
  currentTemp = filtered + cfg.calibration;
  currentHum = h;
  if (!sensorOk) Serial.println(F("Sensor OK."));
  sensorOk = true;
  lastValidSensorMs = millis();
}

void updateSensor() {
  unsigned long now = millis();

  // Uzun sure gecerli veri gelmediyse sensöru hatali say
  if (sensorOk && (now - lastValidSensorMs > SENSOR_STALE_MS)) {
    Serial.println(F("Sensor verisi bayat."));
    sensorOk = false;
    setDemand(false);
  }

  if (now - lastSensorPollMs < 100UL) return;
  lastSensorPollMs = now;

#if SENSOR_TYPE == SENSOR_DS18B20
  static bool dsPending = false;
  static unsigned long dsRequestMs = 0;
  static unsigned long dsLastStartMs = 0;

  if (!dsPending && (now - dsLastStartMs >= SENSOR_PERIOD_MS)) {
    ds18b20.requestTemperatures();
    dsPending = true;
    dsRequestMs = now;
    dsLastStartMs = now;
  } else if (dsPending && (now - dsRequestMs >= 800UL)) {
    dsPending = false;
    float t = ds18b20.getTempCByIndex(0);
    if (t == DEVICE_DISCONNECTED_C || t < -55.0f || t > 125.0f) {
      sensorFail();
    } else {
      sensorSuccess(t, NAN);
    }
  }
#else
  static unsigned long dhtLastReadMs = 0;
  if (now - dhtLastReadMs >= SENSOR_PERIOD_MS) {
    dhtLastReadMs = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(t) || t < -40.0f || t > 80.0f) {
      sensorFail();
    } else {
      sensorSuccess(t, isnan(h) ? NAN : h);
    }
  }
#endif
}

// ============================ KONTROL ======================================

float effectiveSetpoint() {
  float sp;
  if (!cfg.mode) {
    sp = cfg.frost;
  } else if (sleepActive) {
    sp = cfg.sleepSetpoint;
  } else {
    sp = cfg.setpoint;
  }
  if (cfg.mode && boostActive) sp += BOOST_OFFSET_C;
  return clampf(sp, SETPOINT_MIN_C, SETPOINT_MAX_C);
}

// Tek bir ayar anahtarini uygular (web, MQTT ve Telegram ortak kullanir).
bool applySetting(const String &key, String val) {
  val.trim();
  bool ch = false;

  if (key == "sp") {
    float v = roundToStep(val.toFloat(), SETPOINT_STEP_C);
    v = clampf(v, SETPOINT_MIN_C, SETPOINT_MAX_C);
    if (fabsf(v - cfg.setpoint) > 0.001f) { cfg.setpoint = v; ch = true; }
  } else if (key == "mode") {
    uint8_t m = val.toInt() ? 1 : 0;
    if (m != cfg.mode) { cfg.mode = m; ch = true; }
  } else if (key == "hyst") {
    float v = clampf(val.toFloat(), 0.1f, 3.0f);
    if (fabsf(v - cfg.hysteresis) > 0.001f) { cfg.hysteresis = v; ch = true; }
  } else if (key == "cal") {
    float v = clampf(val.toFloat(), -5.0f, 5.0f);
    if (fabsf(v - cfg.calibration) > 0.001f) { cfg.calibration = v; ch = true; }
  } else if (key == "frost") {
    float v = clampf(val.toFloat(), 2.0f, 10.0f);
    if (fabsf(v - cfg.frost) > 0.001f) { cfg.frost = v; ch = true; }
  } else if (key == "boost") {
    startBoost(val.toInt());
    ch = true;
  } else if (key == "sleep") {
    uint8_t v = val.toInt() ? 1 : 0;
    if (v != cfg.sleepEnabled) { cfg.sleepEnabled = v; ch = true; }
  } else if (key == "sleepstart") {
    uint16_t v = parseHHMM(val, cfg.sleepStartMin);
    if (v != cfg.sleepStartMin) { cfg.sleepStartMin = v; ch = true; }
  } else if (key == "sleepend") {
    uint16_t v = parseHHMM(val, cfg.sleepEndMin);
    if (v != cfg.sleepEndMin) { cfg.sleepEndMin = v; ch = true; }
  } else if (key == "sleepsp") {
    float v = clampf(val.toFloat(), SETPOINT_MIN_C, SETPOINT_MAX_C);
    if (fabsf(v - cfg.sleepSetpoint) > 0.001f) { cfg.sleepSetpoint = v; ch = true; }
  } else if (key == "failsafe") {
    uint8_t v = val.toInt() ? 1 : 0;
    if (v != cfg.relayFailSafeOn) { cfg.relayFailSafeOn = v; ch = true; }
  } else if (key == "theme") {
    if (setThemeById(val)) ch = true;
  } else if (key == "lang") {
    if (val.length() == 0) {
      if (uiLang.length()) { uiLang = ""; ch = true; }
      langPrompt = true;             // dil temizlendi: secim ekrani yeniden gelir
      langMarker = 0;
    } else if (langIndex(val) != 0xFF) {
      String v = val;
      v.trim();
      v.toLowerCase();
      if (v != uiLang) { uiLang = v; ch = true; }
      langPrompt = false;            // dil artik secili
      langMarker = 1;
    }
    Serial.printf("Dil: %s\n", uiLang.length() ? uiLang.c_str() : "secili degil");
  }

  if (ch) {
    updateControl();
    markSettingsDirty();
    forceSend = true;
  }
  return ch;
}

bool setThemeById(const String &id) {
  String want = id;
  want.trim();
  if (want.length() == 0) return false;
  if (!themeStore.setActive(want)) return false;
  themeId = themeStore.active().id;
  return true;
}

void themeStep(int dir) {
  if (!themeStore.step(dir)) return;
  themeId = themeStore.active().id;
  markSettingsDirty();
  Serial.printf("Tema: %s (%u/%u)\n", themeStore.active().name,
                themeStore.index() + 1, OLED_THEME_COUNT);
}

bool applyCommandString(const String &s) {
  bool changed = false;
  int i = 0;
  int n = (int)s.length();

  while (i < n) {
    while (i < n && (s[i] == '&' || s[i] == ' ' || s[i] == ',' || s[i] == ';')) i++;
    int j = i;
    while (j < n && s[j] != '&' && s[j] != ' ' && s[j] != ',' && s[j] != ';') j++;
    String part = s.substring(i, j);
    i = j;
    int eq = part.indexOf('=');
    if (eq <= 0) continue;
    String k = part.substring(0, eq);
    String v = part.substring(eq + 1);
    k.trim();
    v.trim();
    if (applySetting(k, v)) changed = true;
  }
  return changed;
}

void setDemand(bool d) {
  if (d == demand) return;
  demand = d;
  if (d) {
    lastDemandOnMs = millis();
    Serial.println(F("Isi talebi: ACIK"));
  } else {
    lastDemandOffMs = millis();
    Serial.println(F("Isi talebi: KAPALI"));
  }
  forceSend = true;
}

void updateControl() {
  if (!sensorOk || isnan(currentTemp)) {
    setDemand(false);
    return;
  }

  float sp = effectiveSetpoint();
  float h  = cfg.hysteresis;
  unsigned long now = millis();

  if (demand) {
    if (currentTemp >= (sp + h) && (now - lastDemandOnMs >= MIN_ON_MS)) {
      setDemand(false);
    }
  } else {
    if (currentTemp <= (sp - h) && (now - lastDemandOffMs >= MIN_OFF_MS)) {
      setDemand(true);
    }
  }
}

void updateBoost() {
  if (boostActive && (long)(millis() - boostEndMs) >= 0) {
    boostActive = false;
    boostMinutesSet = 0;
    Serial.println(F("Boost bitti."));
    forceSend = true;
  }
}

void startBoost(int minutes) {
  if (minutes <= 0) {
    boostActive = false;
    boostMinutesSet = 0;
    forceSend = true;
    return;
  }
  minutes = constrain(minutes, 1, 240);
  boostActive = true;
  boostEndMs = millis() + (unsigned long)minutes * 60000UL;
  boostMinutesSet = minutes;
  if (cfg.mode == 0) {
    cfg.mode = 1;          // boost baslatilinca modu ac
    markSettingsDirty();
  }
  updateControl();
  forceSend = true;
  Serial.printf("Boost: %d dk\n", minutes);
}

// ============================ ENCODER / BUTON ==============================

volatile int16_t encoderDelta = 0;
volatile uint8_t encoderState = 0;
unsigned long pendingButtonMs = 0;   // /test: sanal buton basisi (loop isler)
bool encThemeTurn = false;           // buton basili cevrilerek tema degistirildi

const int8_t ENC_TABLE[16] = {
   0, -1,  1,  0,
   1,  0,  0, -1,
  -1,  0,  0,  1,
   0,  1, -1,  0
};

void ICACHE_RAM_ATTR encoderISR() {
  uint8_t a = (digitalRead(PIN_ENC_A) == HIGH) ? 1 : 0;
  uint8_t b = (digitalRead(PIN_ENC_B) == HIGH) ? 1 : 0;
  encoderState = (uint8_t)(((encoderState << 2) | (a << 1) | b) & 0x0F);
  encoderDelta += ENC_TABLE[encoderState];
}

void changeSetpoint(float delta) {
  // Ilk cevirme yalnizca duzenleme moduna gecer: ekranda guncel oda sicakligi
  // varken hedef degismez, deger 10 sn boyunca yanip soner. Sonraki cevirmeler
  // hedefi degistirir. Boylece kazara cevirme hedefi bozmaz.
  if (setpointPeekMs == 0) {
    oledPage = 0;
    setpointPeekMs = millis();
    if (setpointPeekMs == 0) setpointPeekMs = 1;
    return;
  }

  float sp = roundToStep(cfg.setpoint + delta, SETPOINT_STEP_C);
  sp = clampf(sp, SETPOINT_MIN_C, SETPOINT_MAX_C);
  // Her cevirme ana ekrana doner ve hedefi 10 sn boyunca gosterir;
  // sinirda olsa bile kullanici degeri gorsun.
  oledPage = 0;
  setpointPeekMs = millis();
  if (setpointPeekMs == 0) setpointPeekMs = 1;
  if (fabsf(sp - cfg.setpoint) < 0.001f) return;
  cfg.setpoint = sp;
  markSettingsDirty();
  forceSend = true;
  Serial.printf("Hedef: %.1f C\n", cfg.setpoint);
}

void handleEncoder() {
  int16_t delta;
  noInterrupts();
  delta = encoderDelta;
  encoderDelta = 0;
  interrupts();

  if (ENCODER_REVERSE) delta = -delta;

  static int16_t accum = 0;
  accum += delta;

  // Ilk acilis: dil secimi ekrani (encoder cevirerek secilir)
  if (!langChosen()) {
    while (accum >= 4) { accum -= 4; langCursor = (uint8_t)((langCursor + 1) % LANG_COUNT); }
    while (accum <= -4) { accum += 4; langCursor = (uint8_t)((langCursor + LANG_COUNT - 1) % LANG_COUNT); }
    return;
  }

  // Buton basiliyken cevirmek tema degistirir: sag = sonraki, sol = onceki.
  // Ekran hangi sayfada olursa olsun tema aninda uygulanir.
  bool themeTurn = (digitalRead(PIN_ENC_SW) == LOW);
  while (accum >= 4) {
    accum -= 4;
    if (themeTurn) { themeStep(1); encThemeTurn = true; }
    else           changeSetpoint( SETPOINT_STEP_C);
  }
  while (accum <= -4) {
    accum += 4;
    if (themeTurn) { themeStep(-1); encThemeTurn = true; }
    else           changeSetpoint(-SETPOINT_STEP_C);
  }
}

void toggleMode() {
  cfg.mode = cfg.mode ? 0 : 1;
  Serial.printf("Mod: %s\n", cfg.mode ? "ACIK" : "KAPALI");
  updateControl();
  markSettingsDirty();
  forceSend = true;
}

// Buton birakildiginda yapilacak is: gercek buton ve /test ayni yolu kullanir.
void applyButtonPress(unsigned long dur) {
  if (!langChosen()) {
    // Dil secimi: kisa basista onayla, kalici olarak kaydet
    if (dur >= 40UL) {
      uiLang = LANG_CODES[langCursor];
      langPrompt = false;
      langMarker = 1;
      markSettingsDirty();
      oledPage = 0;
      Serial.printf("Dil secildi: %s\n", uiLang.c_str());
    }
  } else if (dur >= 1200UL) {
    toggleMode();
  } else if (dur >= 40UL) {
    oledPage = (oledPage + 1) % OLED_PAGE_COUNT;
  }
}

void handleEncoderButton() {
  static bool lastRaw = false;
  static bool stable = false;
  static unsigned long changeMs = 0;
  static unsigned long pressStartMs = 0;

  // /test sayfasindan gelen sanal basis
  if (pendingButtonMs) {
    unsigned long dur = pendingButtonMs;
    pendingButtonMs = 0;
    applyButtonPress(dur);
  }

  bool raw = (digitalRead(PIN_ENC_SW) == LOW);
  unsigned long now = millis();

  if (raw != lastRaw) {
    lastRaw = raw;
    changeMs = now;
  }

  if ((now - changeMs) > 30UL && stable != raw) {
    stable = raw;
    if (stable) {
      pressStartMs = now;
      encThemeTurn = false;
    } else if (encThemeTurn) {
      // Buton basili cevrilerek tema secildi: sayfa/mod degistirme
      encThemeTurn = false;
    } else {
      applyButtonPress(now - pressStartMs);
    }
  }
}

// ============================ WI-FI / OTA ==================================

void setupWiFi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);
  WiFi.hostname("termostat");

  if (!WiFi.softAP(AP_SSID, AP_PASS)) {
    Serial.println(F("AP baslatilamadi!"));
  } else {
    Serial.print(F("AP hazir: ")); Serial.println(AP_SSID);
  }

  // Captive portal: AP'ye baglanan telefon/PC'nin tum DNS sorgulari bu
  // cihaza yonlendirilir; boylece arayuz kendiliginden acilir.
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.print(F("Captive DNS: ")); Serial.println(WiFi.softAPIP());

  if (wifiSsid.length() > 0) {
    Serial.print(F("STA baglaniyor: ")); Serial.println(wifiSsid);
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  }

  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.println(F("mDNS: termostat.local"));
  }
}

void updateWiFi() {
  static unsigned long lastRetryMs = 0;

  if (WiFi.status() == WL_CONNECTED) {
    if (!setupDone) {
      setupDone = true;               // kurulum gercekten tamamlandi
      saveSettings();
      Serial.println(F("Ilk kurulum tamamlandi (ag bagli)."));
    }
    if (!otaStarted) {
      ArduinoOTA.setPort(OTA_PORT);
      ArduinoOTA.setHostname(MDNS_NAME);
      ArduinoOTA.setPassword(OTA_PASS);
      ArduinoOTA.begin();
      otaStarted = true;
      Serial.print(F("STA bagli. IP: ")); Serial.println(WiFi.localIP());
      Serial.println(F("OTA hazir."));
    }
    return;
  }

  if (wifiSsid.length() > 0 && (millis() - lastRetryMs > 20000UL)) {
    lastRetryMs = millis();
    Serial.println(F("STA yeniden deneniyor..."));
    WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  }
}

// ============================ MQTT =========================================

String stateLabel() {
  if (!sensorOk) return "SENSOR HATASI";
  if (!boilerLinked()) return "KOMBI BAGLI DEGIL";
  if (!cfg.mode) return "KAPALI/DON KOR.";
  if (demand) return "ISITIYOR";
  return "BEKLIYOR";
}

// ---- Telegram: dile bagli komut adlari ve yanit metinleri -----------------
// Komutlar cihazda secilen dilde calisir; Ingilizce ve eski Turkce adlar her
// zaman kabul edilir (geriye uyumluluk).
enum {
  TG_C_STATUS, TG_C_TEMP, TG_C_MODE, TG_C_BOOST,
  TG_C_SLEEP, TG_C_SLEEPTIME, TG_C_SLEEPTEMP, TG_C_HELP, TG_C_COUNT
};

// Satir: komut, sutun: LANG_CODES sirasi (en, tr, ru, es, pt, ja, ko, zh)
static const char *const TG_CMD[LANG_COUNT][TG_C_COUNT] = {
  {"status",   "temp",      "mode",  "boost", "sleep",  "sleeptime", "sleeptemp",  "help"},
  {"durum",    "sicaklik",  "mod",   "boost", "uyku",   "uykusaat",  "uykusicaklik", "yardim"},
  {"статус",   "температура", "режим", "буст", "сон",   "время",     "сонтемп",    "помощь"},
  {"estado",   "temperatura", "modo", "boost", "sueno", "horario",   "suenotemp",  "ayuda"},
  {"estado",   "temperatura", "modo", "boost", "dormir", "horario",  "dormirtemp", "ajuda"},
  {"状態",     "温度",       "モード", "ブースト", "睡眠", "睡眠時間",   "睡眠温度",   "ヘルプ"},
  {"상태",     "온도",       "모드",  "부스트", "수면",   "수면시간",   "수면온도",   "도움말"},
  {"状态",     "温度",       "模式",  "加热",   "睡眠",   "睡眠时间",   "睡眠温度",   "帮助"}
};

static const char *const TG_ON_W[LANG_COUNT]  = {"on",  "ac", "вкл", "encender", "ligar",  "オン", "켜기", "开"};
static const char *const TG_OFF_W[LANG_COUNT] = {"off", "kapat", "выкл", "apagar", "desligar", "オフ", "끄기", "关"};

enum {
  TG_T_ROOM, TG_T_HUM, TG_T_TARGET, TG_T_STATE, TG_T_RELAY, TG_T_LINK,
  TG_T_MODE, TG_T_SLEEP, TG_T_FS,
  TG_T_YES, TG_T_NO, TG_T_ON, TG_T_OFF, TG_T_ACTIVE, TG_T_WAITING,
  TG_T_HEATING, TG_T_ST_WAITING, TG_T_ST_OFF, TG_T_ST_SENSOR, TG_T_ST_NOLINK,
  TG_T_KEEP_ON, TG_T_OPENS, TG_T_MIN,
  TG_T_USAGE, TG_T_UNCHANGED, TG_T_MODE_ON, TG_T_MODE_OFF,
  TG_T_SLEEP_ON, TG_T_SLEEP_OFF, TG_T_SLEEP_TIME, TG_T_SLEEP_TARGET,
  TG_T_BOOST_ON, TG_T_BOOST_OFF, TG_T_UNKNOWN, TG_T_COMMANDS,
  TG_T_COUNT
};

static const char *const TG_TXT[LANG_COUNT][TG_T_COUNT] = {
  // en
  {"Room", "Hum", "Target", "State", "Relay", "Link", "Mode", "Sleep", "Relay FS",
   "yes", "no", "ON", "OFF", "ACTIVE", "waiting",
   "Heating", "Waiting", "Off", "Sensor error", "Boiler offline",
   "keeps ON", "opens", "min",
   "Usage", "Target unchanged.", "Mode ON.", "Mode OFF (frost protection active).",
   "Sleep mode on.", "Sleep mode off.", "Sleep time:", "Sleep target:",
   "Boost started", "Boost cancelled.", "Unknown command.", "Commands"},
  // tr
  {"Oda", "Nem", "Hedef", "Durum", "Role", "Baglanti", "Mod", "Uyku", "Role FS",
   "VAR", "YOK", "ACIK", "KAPALI", "AKTIF", "bekliyor",
   "Isitiyor", "Bekliyor", "Kapali", "Sensor hatasi", "Kombi yok",
   "ACIK kalir", "kapanir", "dk",
   "Kullanim", "Hedef degismedi.", "Mod ACIK.", "Mod KAPALI (don korumasi devrede).",
   "Uyku modu acildi.", "Uyku modu kapatildi.", "Uyku saati:", "Uyku hedefi:",
   "Boost baslatildi", "Boost iptal edildi.", "Bilinmeyen komut.", "Komutlar"},
  // ru
  {"Комната", "Влажность", "Цель", "Состояние", "Реле", "Связь", "Режим", "Сон", "Реле FS",
   "да", "нет", "ВКЛ", "ВЫКЛ", "АКТИВЕН", "ожидание",
   "Нагрев", "Ожидание", "Выключено", "Ошибка датчика", "Котёл не на связи",
   "остаётся ВКЛ", "открывается", "мин",
   "Использование", "Цель не изменилась.", "Режим ВКЛ.", "Режим ВЫКЛ (защита от замерзания).",
   "Ночной режим включён.", "Ночной режим выключен.", "Время сна:", "Цель ночью:",
   "Boost запущен", "Boost отменён.", "Неизвестная команда.", "Команды"},
  // es
  {"Ambiente", "Humedad", "Objetivo", "Estado", "Rele", "Enlace", "Modo", "Sueno", "FS rele",
   "si", "no", "ON", "OFF", "ACTIVO", "esperando",
   "Calentando", "Esperando", "Apagado", "Fallo del sensor", "Sin caldera",
   "queda ON", "cierra", "min",
   "Uso", "Objetivo sin cambios.", "Modo ON.", "Modo OFF (proteccion antihielo activa).",
   "Modo sueno activado.", "Modo sueno desactivado.", "Horario sueno:", "Objetivo sueno:",
   "Boost iniciado", "Boost cancelado.", "Comando desconocido.", "Comandos"},
  // pt
  {"Ambiente", "Umidade", "Alvo", "Estado", "Rele", "Conexao", "Modo", "Dormir", "FS rele",
   "sim", "nao", "ON", "OFF", "ATIVO", "aguardando",
   "Aquecendo", "Aguardando", "Desligado", "Erro no sensor", "Sem caldeira",
   "fica ON", "fecha", "min",
   "Uso", "Alvo sem alteracao.", "Modo ON.", "Modo OFF (protecao antigelo ativa).",
   "Modo dormir ativado.", "Modo dormir desativado.", "Horario dormir:", "Alvo dormir:",
   "Boost iniciado", "Boost cancelado.", "Comando desconhecido.", "Comandos"},
  // ja
  {"室温", "湿度", "目標", "状態", "リレー", "接続", "モード", "おやすみ", "リレーFS",
   "あり", "なし", "ON", "OFF", "実行中", "待機中",
   "加熱中", "待機中", "停止", "センサー異常", "ボイラー未接続",
   "ON のまま", "開く", "分",
   "使い方", "目標は変更されていません。", "モード ON。", "モード OFF（凍結防止のみ）。",
   "おやすみモード ON。", "おやすみモード OFF。", "おやすみ時間:", "おやすみ目標:",
   "ブースト開始", "ブースト解除。", "不明なコマンド。", "コマンド"},
  // ko
  {"실내", "습도", "목표", "상태", "릴레이", "연결", "모드", "취침", "릴레이 FS",
   "예", "아니오", "ON", "OFF", "실행 중", "대기 중",
   "가열 중", "대기 중", "정지", "센서 오류", "보일러 연결 안 됨",
   "ON 유지", "열림", "분",
   "사용법", "목표가 변경되지 않았습니다.", "모드 ON.", "모드 OFF(동파 방지만 동작).",
   "취침 모드 ON.", "취침 모드 OFF.", "취침 시간:", "취침 목표:",
   "부스트 시작", "부스트 해제.", "알 수 없는 명령.", "명령어"},
  // zh
  {"房间", "湿度", "目标", "状态", "继电器", "连接", "模式", "睡眠", "继电器 FS",
   "是", "否", "开", "关", "运行中", "等待中",
   "加热中", "等待中", "关闭", "传感器故障", "锅炉未连接",
   "保持开启", "断开", "分钟",
   "用法", "目标未改变。", "模式开启。", "模式关闭（仅防冻保护）。",
   "睡眠模式开启。", "睡眠模式关闭。", "睡眠时间:", "睡眠目标:",
   "加热已开始", "加热已取消。", "未知命令。", "命令"}
};

uint8_t tgLang() {
  uint8_t i = langIndex(uiLang);
  return (i == 0xFF) ? 0 : i;   // dil secilmemis: Ingilizce
}

const char *TT(uint8_t id) {
  return TG_TXT[tgLang()][id];
}

// Komut adi eslesmesi: secili dil + Ingilizce + eski Turkce adlar
bool tgCmdIs(const String &body, uint8_t cmd) {
  uint8_t li = tgLang();
  if (body.startsWith(TG_CMD[li][cmd])) return true;
  if (li != 0 && body.startsWith(TG_CMD[0][cmd])) return true;
  if (li != 1 && body.startsWith(TG_CMD[1][cmd])) return true;
  return false;
}

// "ac/kapat" karsiliklari; secili dil + Ingilizce + eski Turkce
int8_t tgOnOff(const String &low) {
  uint8_t li = tgLang();
  if (low.indexOf(TG_OFF_W[li]) >= 0) return 0;
  if (low.indexOf(TG_ON_W[li]) >= 0)  return 1;
  if (low.indexOf("kapat") >= 0 || low.indexOf("off") >= 0) return 0;
  if (low.indexOf("ac") >= 0 || low.indexOf("on") >= 0) return 1;
  return -1;
}

const char *tgStateLabel() {
  if (!sensorOk) return TT(TG_T_ST_SENSOR);
  if (!boilerLinked()) return TT(TG_T_ST_NOLINK);
  if (!cfg.mode) return TT(TG_T_ST_OFF);
  if (demand) return TT(TG_T_HEATING);
  return TT(TG_T_ST_WAITING);
}

String statusSummary() {
  String s;
  s.reserve(380);
  s  = "KombiPlus\n";
  s += String(TT(TG_T_ROOM)) + ": " + (sensorOk ? String(currentTemp, 1) : String("--.-")) + "C";
  if (sensorOk && !isnan(currentHum))
    s += "  " + String(TT(TG_T_HUM)) + ": %" + String((int)roundf(currentHum));
  s += "\n" + String(TT(TG_T_TARGET)) + ": " + String(effectiveSetpoint(), 1) + "C";
  s += "  (" + String(cfg.setpoint, 1) + "C)";
  s += "\n" + String(TT(TG_T_STATE)) + ": " + tgStateLabel();
  s += "\n" + String(TT(TG_T_RELAY)) + ": " + String(boilerRelay ? TT(TG_T_ON) : TT(TG_T_OFF));
  s += "  " + String(TT(TG_T_LINK)) + ": " + String(boilerLinked() ? TT(TG_T_YES) : TT(TG_T_NO));
  s += "\n" + String(TT(TG_T_MODE)) + ": " + String(cfg.mode ? TT(TG_T_ON) : TT(TG_T_OFF));
  s += "  Boost: " + (boostActive ? String((boostRemainSec() + 59) / 60) + TT(TG_T_MIN) : String(TT(TG_T_NO)));
  s += "\n" + String(TT(TG_T_SLEEP)) + ": ";
  if (!cfg.sleepEnabled) s += TT(TG_T_OFF);
  else s += String(sleepActive ? TT(TG_T_ACTIVE) : TT(TG_T_WAITING)) + " " +
            fmtHHMM(cfg.sleepStartMin) + "-" + fmtHHMM(cfg.sleepEndMin) +
            " " + String(cfg.sleepSetpoint, 1) + "C";
  s += "\n" + String(TT(TG_T_FS)) + ": " + String(cfg.relayFailSafeOn ? TT(TG_T_KEEP_ON) : TT(TG_T_OPENS));
  return s;
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  String msg;
  msg.reserve(length + 1);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  Serial.print(F("MQTT komut: "));
  Serial.println(msg);

  bool ok = applyCommandString(msg);
  if (ok) {
    mqttClient.publish((mqttPrefix + "/result").c_str(), "ok");
  } else {
    mqttClient.publish((mqttPrefix + "/result").c_str(), "hata");
  }
}

void publishMqttState() {
#if MQTT_ENABLED
  if (!mqttClient.connected()) return;

  String j;
  j.reserve(360);
  j  = "{";
  j += "\"temp\":" + (sensorOk ? String(currentTemp, 1) : String("null")) + ",";
  j += "\"hum\":" + ((sensorOk && !isnan(currentHum)) ? String((int)roundf(currentHum)) : String("null")) + ",";
  j += "\"sp\":" + String(cfg.setpoint, 1) + ",";
  j += "\"eff\":" + String(effectiveSetpoint(), 1) + ",";
  j += "\"mode\":" + String(cfg.mode) + ",";
  j += "\"demand\":" + String(demand ? 1 : 0) + ",";
  j += "\"relay\":" + String(boilerRelay ? 1 : 0) + ",";
  j += "\"link\":" + String(boilerLinked() ? 1 : 0) + ",";
  j += "\"sleep\":" + String(cfg.sleepEnabled ? (sleepActive ? 2 : 1) : 0) + ",";
  j += "\"sleepsp\":" + String(cfg.sleepSetpoint, 1) + ",";
  j += "\"boost\":" + String(boostRemainSec()) + ",";
  j += "\"failsafe\":" + String(cfg.relayFailSafeOn) + ",";
  j += "\"flow\":" + (boilerFlow > -100.0f ? String(boilerFlow, 1) : String("null")) + ",";
  j += "\"fw\":\"" FW_VERSION "\"";
  j += "}";

  mqttClient.publish((mqttPrefix + "/status").c_str(), j.c_str(), true);
  lastMqttPubMs = millis();
  mqttStateDirty = false;
#endif
}

void mqttLoop() {
#if MQTT_ENABLED
  if (mqttHost.length() == 0 || WiFi.status() != WL_CONNECTED) {
    if (mqttClient.connected()) mqttClient.disconnect();
    mqttWasConnected = false;
    return;
  }

  if (!mqttClient.connected()) {
    if (millis() - lastMqttAttemptMs < 5000UL) return;
    lastMqttAttemptMs = millis();

    String cid = "termostat-" + String(ESP.getChipId(), HEX);
    String statTopic = mqttPrefix + "/status";
    mqttClient.setServer(mqttHost.c_str(), mqttPort);
    mqttClient.setKeepAlive(20);

    bool ok;
    if (mqttUser.length() > 0) {
      ok = mqttClient.connect(cid.c_str(), mqttUser.c_str(), mqttPass.c_str(),
                              statTopic.c_str(), 0, true, "offline");
    } else {
      ok = mqttClient.connect(cid.c_str(), nullptr, nullptr,
                              statTopic.c_str(), 0, true, "offline");
    }

    if (ok) {
      mqttClient.subscribe((mqttPrefix + "/set").c_str());
      mqttClient.publish(statTopic.c_str(), "online", true);
      mqttWasConnected = true;
      mqttStateDirty = true;
      Serial.println(F("MQTT baglandi."));
    } else if (mqttWasConnected) {
      mqttWasConnected = false;
      Serial.println(F("MQTT baglantisi koptu."));
    }
    return;
  }

  mqttClient.loop();
  if (mqttStateDirty || (millis() - lastMqttPubMs >= MQTT_PUB_MS)) publishMqttState();
#endif
}

// ============================ TELEGRAM =====================================

String urlEncode(const String &s) {
  static const char *hex = "0123456789ABCDEF";
  String out;
  out.reserve(s.length() * 3);
  for (size_t i = 0; i < s.length(); i++) {
    uint8_t c = (uint8_t)s[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      out += (char)c;
    } else if (c == ' ') {
      out += "%20";
    } else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 0x0F];
    }
  }
  return out;
}

String jsonStringAt(const String &src, int start) {
  String out;
  bool esc = false;
  for (int i = start; i < (int)src.length(); i++) {
    char c = src[i];
    if (esc) {
      if (c == 'n') out += '\n';
      else if (c == 't') out += '\t';
      else out += c;
      esc = false;
    } else if (c == '\\') {
      esc = true;
    } else if (c == '"') {
      break;
    } else {
      out += c;
    }
  }
  return out;
}

String tgRequest(const String &path) {
#if TELEGRAM_ENABLED
  WiFiClientSecure *secure = new WiFiClientSecure();
  if (!secure) {
    tgLastError = F("RAM yetersiz");
    return "";
  }
  secure->setInsecure();
  secure->setBufferSizes(1024, 1024);
  secure->setTimeout(6000);

  HTTPClient https;
  https.setTimeout(6000);
  https.setReuse(false);
  String url = "https://api.telegram.org" + path;

  String body = "";
  if (!https.begin(*secure, url)) {
    tgLastError = F("HTTPS baslatilamadi");
    delete secure;
    return "";
  }
  int code = https.GET();
  if (code == HTTP_CODE_OK) {
    body = https.getString();
  } else {
    tgLastError = "HTTP " + String(code);
    if (code > 0) body = https.getString();
  }
  https.end();
  secure->stop();
  delete secure;
  return body;
#else
  (void)path;
  return "";
#endif
}

String tgUsage(uint8_t cmd, const String &args) {
  return String(TT(TG_T_USAGE)) + ": /" + String(TG_CMD[tgLang()][cmd]) + " " + args;
}

String tgOnOffWords() {
  return String(TG_ON_W[tgLang()]) + "|" + String(TG_OFF_W[tgLang()]);
}

String tgHandleCommand(const String &raw) {
  String text = raw;
  text.trim();
  String low = text;
  low.toLowerCase();
  String body = low;
  if (body.startsWith("/")) body = body.substring(1);
  body.trim();

  if (tgCmdIs(body, TG_C_STATUS)) return statusSummary();

  if (tgCmdIs(body, TG_C_TEMP) || body.startsWith("sp")) {
    int sp = text.indexOf(' ');
    if (sp < 0) return tgUsage(TG_C_TEMP, "22.5");
    if (!applySetting("sp", text.substring(sp + 1))) return String(TT(TG_T_UNCHANGED));
    return String(TT(TG_T_TARGET)) + ": " + String(cfg.setpoint, 1) + "C";
  }

  if (tgCmdIs(body, TG_C_MODE)) {
    int8_t v = tgOnOff(low);
    if (v == 0) { applySetting("mode", "0"); return String(TT(TG_T_MODE_OFF)); }
    if (v == 1) { applySetting("mode", "1"); return String(TT(TG_T_MODE_ON)); }
    return tgUsage(TG_C_MODE, tgOnOffWords());
  }

  if (tgCmdIs(body, TG_C_BOOST)) {
    int sp = text.indexOf(' ');
    int m = (sp < 0) ? 30 : text.substring(sp + 1).toInt();
    applySetting("boost", String(m));
    if (m <= 0) return String(TT(TG_T_BOOST_OFF));
    return String(TT(TG_T_BOOST_ON)) + " (" + String(m) + " " + String(TT(TG_T_MIN)) + ")";
  }

  // Once /uykusicaklik ve /uykusaat: /uyku oneki ile karismasin
  if (tgCmdIs(body, TG_C_SLEEPTEMP)) {
    int sp = text.indexOf(' ');
    if (sp < 0) return tgUsage(TG_C_SLEEPTEMP, "18");
    applySetting("sleepsp", text.substring(sp + 1));
    return String(TT(TG_T_SLEEP_TARGET)) + " " + String(cfg.sleepSetpoint, 1) + "C";
  }

  if (tgCmdIs(body, TG_C_SLEEPTIME)) {
    int sp = text.indexOf(' ');
    if (sp < 0) return tgUsage(TG_C_SLEEPTIME, "23:00 07:00");
    String rest = text.substring(sp + 1);
    rest.trim();
    int sp2 = rest.indexOf(' ');
    if (sp2 < 0) return tgUsage(TG_C_SLEEPTIME, "23:00 07:00");
    applySetting("sleepstart", rest.substring(0, sp2));
    applySetting("sleepend", rest.substring(sp2 + 1));
    return String(TT(TG_T_SLEEP_TIME)) + " " + fmtHHMM(cfg.sleepStartMin) +
           " - " + fmtHHMM(cfg.sleepEndMin);
  }

  if (tgCmdIs(body, TG_C_SLEEP)) {
    int8_t v = tgOnOff(low);
    if (v == 0) { applySetting("sleep", "0"); return String(TT(TG_T_SLEEP_OFF)); }
    if (v == 1) { applySetting("sleep", "1"); return String(TT(TG_T_SLEEP_ON)); }
    return tgUsage(TG_C_SLEEP, tgOnOffWords());
  }

  if (tgCmdIs(body, TG_C_HELP) || body.startsWith("start")) {
    uint8_t li = tgLang();
    String h = String(TT(TG_T_COMMANDS)) + ":\n";
    h += "/" + String(TG_CMD[li][TG_C_STATUS]) + "\n";
    h += "/" + String(TG_CMD[li][TG_C_TEMP]) + " 22.5\n";
    h += "/" + String(TG_CMD[li][TG_C_MODE]) + " " + tgOnOffWords() + "\n";
    h += "/" + String(TG_CMD[li][TG_C_BOOST]) + " 30\n";
    h += "/" + String(TG_CMD[li][TG_C_SLEEP]) + " " + tgOnOffWords() + "\n";
    h += "/" + String(TG_CMD[li][TG_C_SLEEPTIME]) + " 23:00 07:00\n";
    h += "/" + String(TG_CMD[li][TG_C_SLEEPTEMP]) + " 18";
    return h;
  }

  return String(TT(TG_T_UNKNOWN)) + " /" + String(TG_CMD[tgLang()][TG_C_HELP]);
}

void tgSend(const String &text) {
  String t = text;
  if (t.length() > 900) t = t.substring(0, 900);
  String path = "/bot" + tgToken + "/sendMessage?chat_id=" + urlEncode(tgChat) +
                "&text=" + urlEncode(t);
  String body = tgRequest(path);
  if (body.length() == 0) {
    Serial.print(F("Telegram gonderilemedi: "));
    Serial.println(tgLastError);
  }
}

void handleTgUpdates(const String &body) {
  int pos = 0;
  while (true) {
    int u = body.indexOf("\"update_id\":", pos);
    if (u < 0) break;
    int next = body.indexOf("\"update_id\":", u + 12);
    String chunk = body.substring(u, next < 0 ? body.length() : next);
    pos = (next < 0) ? body.length() : next;

    long uid = chunk.substring(12).toInt();
    if (uid >= tgOffset) tgOffset = (int32_t)uid + 1;

    int ci = chunk.indexOf("\"chat\":{\"id\":");
    int ti = chunk.indexOf("\"text\":\"");
    if (ci < 0 || ti < 0) continue;

    String chat = chunk.substring(ci + 13);
    int cut = (int)chat.length();
    int c1 = chat.indexOf(',');
    int c2 = chat.indexOf('}');
    if (c1 >= 0 && c1 < cut) cut = c1;
    if (c2 >= 0 && c2 < cut) cut = c2;
    chat = chat.substring(0, cut);
    chat.trim();

    String text = jsonStringAt(chunk, ti + 8);

    if (chat != tgChat) {
      Serial.println(F("Telegram: yetkisiz sohbet yoksayildi."));
      continue;
    }

    Serial.print(F("Telegram komut: "));
    Serial.println(text);
    String reply = tgHandleCommand(text);
    if (reply.length() > 0) tgSend(reply);
  }
}

void telegramLoop() {
#if TELEGRAM_ENABLED
  if (tgToken.length() == 0 || tgChat.length() == 0) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastTgPollMs < TG_POLL_MS) return;
  lastTgPollMs = millis();

  if (ESP.getFreeHeap() < 24000) {
    tgLastError = F("RAM yetersiz");
    return;
  }

  String path = "/bot" + tgToken + "/getUpdates?timeout=0&limit=5&offset=" + String(tgOffset);
  String body = tgRequest(path);
  if (body.length() == 0) return;
  if (body.indexOf("\"ok\":true") < 0) {
    tgLastError = F("API hatasi");
    return;
  }

  tgLastOkMs = millis();
  tgLastError = "";
  handleTgUpdates(body);
#endif
}

// ============================ KOMBI TCP HATTI ==============================

float valueAfter(const String &s, const char *key, float def) {
  int i = s.indexOf(key);
  if (i < 0) return def;
  return s.substring(i + strlen(key)).toFloat();
}

bool boilerLinked() {
  return boilerClient && boilerClient.connected() &&
         lastBoilerMsgMs != 0 &&
         (millis() - lastBoilerMsgMs < 12000UL);
}

void parseBoilerLine(const String &line) {
  if (!line.startsWith("STATUS")) return;
  boilerRelay = (line.indexOf("relay=1") >= 0);
  boilerFlow = valueAfter(line, "flow=", -127.0f);
  boilerRssi = (int)valueAfter(line, "rssi=", 0);
  boilerUptimeSec = (unsigned long)valueAfter(line, "up=", 0);
  lastBoilerMsgMs = millis();
}

void handleBoilerTcp() {
  if (boilerClient && !boilerClient.connected()) {
    boilerClient.stop();
    boilerClient = WiFiClient();
    boilerRxLine = "";
    lastBoilerMsgMs = 0;
    Serial.println(F("Kombi role baglantisi koptu."));
  }

  if (!boilerClient || !boilerClient.connected()) {
    WiFiClient nc = boilerServer.accept();
    if (nc) {
      boilerClient = nc;
      boilerClient.setNoDelay(true);
      boilerRxLine = "";
      lastBoilerMsgMs = millis();
      forceSend = true;
      Serial.print(F("Kombi role baglandi: "));
      Serial.println(boilerClient.remoteIP());
    }
  }

  while (boilerClient && boilerClient.connected() && boilerClient.available()) {
    char c = (char)boilerClient.read();
    if (c == '\n') {
      parseBoilerLine(boilerRxLine);
      boilerRxLine = "";
    } else if (c != '\r' && boilerRxLine.length() < 180) {
      boilerRxLine += c;
    }
  }
}

void sendCommandLoop() {
  static unsigned long lastSendMs = 0;
  unsigned long now = millis();

  if (!boilerClient || !boilerClient.connected()) return;
  if (!forceSend && (now - lastSendMs < 2000UL)) return;
  lastSendMs = now;
  forceSend = false;

  char buf[112];
  snprintf(buf, sizeof(buf), "SET heat=%d sp=%.1f hyst=%.1f mode=%d fs=%d\n",
           demand ? 1 : 0, cfg.setpoint, cfg.hysteresis, cfg.mode, cfg.relayFailSafeOn ? 1 : 0);
  boilerClient.print(buf);
}

// ============================ WEB SUNUCUSU =================================

void fsStats(size_t &used, size_t &total) {
  FSInfo info;
  if (fsReady && LittleFS.info(info)) {
    used = info.usedBytes;
    total = info.totalBytes;
  } else {
    used = 0;
    total = 0;
  }
}

void handleApiStatus() {
  bool linked = boilerLinked();
  size_t fsUsed = 0, fsTotal = 0;
  fsStats(fsUsed, fsTotal);

  String j;
  j.reserve(1150);
  j = "{";
  j += "\"fw\":\"" FW_VERSION "\",";
  j += "\"lang\":\"" + uiLang + "\",";
  j += "\"ask\":";   j += String(langPrompt ? 1 : 0); j += ",";
  j += "\"theme\":\"" + String(themeStore.active().id) + "\",";
  j += "\"temp\":";    j += sensorOk ? String(currentTemp, 1) : String("null"); j += ",";
  j += "\"hum\":";     j += (sensorOk && !isnan(currentHum)) ? String(currentHum, 0) : String("null"); j += ",";
  j += "\"sp\":";      j += String(cfg.setpoint, 1); j += ",";
  j += "\"eff\":";     j += String(effectiveSetpoint(), 1); j += ",";
  j += "\"hyst\":";    j += String(cfg.hysteresis, 1); j += ",";
  j += "\"cal\":";     j += String(cfg.calibration, 1); j += ",";
  j += "\"frost\":";   j += String(cfg.frost, 1); j += ",";
  j += "\"mode\":";    j += String(cfg.mode); j += ",";
  j += "\"demand\":";  j += String(demand ? 1 : 0); j += ",";
  j += "\"relay\":";   j += String(boilerRelay ? 1 : 0); j += ",";
  j += "\"link\":";    j += String(linked ? 1 : 0); j += ",";
  j += "\"flow\":";    j += String(boilerFlow, 1); j += ",";
  j += "\"brssi\":";   j += String(boilerRssi); j += ",";
  j += "\"boost\":";   j += String(boostRemainSec()); j += ",";
  j += "\"boostMin\":"; j += String(boostMinutesSet); j += ",";
  j += "\"sensorOk\":"; j += String(sensorOk ? 1 : 0); j += ",";

  j += "\"sleepEn\":";  j += String(cfg.sleepEnabled); j += ",";
  j += "\"sleep\":";    j += String(cfg.sleepEnabled ? (sleepActive ? 2 : 1) : 0); j += ",";
  j += "\"sleepStart\":\"" + fmtHHMM(cfg.sleepStartMin) + "\",";
  j += "\"sleepEnd\":\"" + fmtHHMM(cfg.sleepEndMin) + "\",";
  j += "\"sleepSp\":";  j += String(cfg.sleepSetpoint, 1); j += ",";

  j += "\"failsafe\":"; j += String(cfg.relayFailSafeOn); j += ",";

  j += "\"mqtt\":";     j += String(mqttClient.connected() ? 1 : 0); j += ",";
  j += "\"mqttCfg\":\"" + jsonEscape(mqttHost) + "\",";
  j += "\"mqttPort\":"; j += String(mqttPort); j += ",";
  j += "\"mqttPrefix\":\"" + jsonEscape(mqttPrefix) + "\",";
  j += "\"tg\":";       j += String((tgToken.length() > 0 && tgChat.length() > 0) ? 1 : 0); j += ",";
  j += "\"tgOk\":";     j += String(tgLastOkMs ? (long)((millis() - tgLastOkMs) / 1000UL) : -1L); j += ",";
  j += "\"tgErr\":\"" + jsonEscape(tgLastError) + "\",";
  j += "\"tgChat\":\"" + jsonEscape(tgChat) + "\",";

  j += "\"timeOk\":";   j += String(timeReady() ? 1 : 0); j += ",";
  j += "\"now\":";      j += String((unsigned long)time(nullptr)); j += ",";
  j += "\"hist\":";     j += String(histLines); j += ",";
  j += "\"fsUsed\":";   j += String((unsigned long)fsUsed); j += ",";
  j += "\"fsTotal\":";  j += String((unsigned long)fsTotal); j += ",";

  j += "\"uptime\":";  j += String(millis() / 1000UL); j += ",";
  j += "\"ip\":\"" + jsonEscape(currentIp()) + "\",";
  j += "\"apip\":\"" + WiFi.softAPIP().toString() + "\",";
  j += "\"ssid\":\"" + jsonEscape(wifiSsid) + "\",";
  j += "\"setup\":";   j += String(setupNeeded() ? 1 : 0); j += ",";   // 1 = ilk kurulum karti gosterilsin
  j += "\"sta\":";     j += String(WiFi.status() == WL_CONNECTED ? 1 : 0); j += ",";
  j += "\"rssi\":";    j += String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0); j += ",";
  j += "\"heap\":";    j += String(ESP.getFreeHeap()); j += ",";
  j += "\"client\":\"" + (boilerClient && boilerClient.connected() ? boilerClient.remoteIP().toString() : String("")) + "\"";
  j += "}";

  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", j);
}

void handleApiSet() {
  bool changed = false;
  for (uint8_t i = 0; i < webServer.args(); i++) {
    if (applySetting(webServer.argName(i), webServer.arg(i))) changed = true;
  }

  if (changed) {
    updateControl();
    updateClock();
    markSettingsDirty();
    forceSend = true;
  }
  handleApiStatus();
}

void handleApiNet() {
  if (webServer.hasArg("mqtthost")) {
    mqttHost = webServer.arg("mqtthost");
    mqttHost.trim();
  }
  if (webServer.hasArg("mqttport")) {
    mqttPort = (uint16_t)constrain(webServer.arg("mqttport").toInt(), 1, 65535);
  }
  if (webServer.hasArg("mqttuser")) { mqttUser = webServer.arg("mqttuser"); mqttUser.trim(); }
  if (webServer.hasArg("mqttpass") && webServer.arg("mqttpass").length() > 0) {
    mqttPass = webServer.arg("mqttpass");
  }
  if (webServer.hasArg("mqttprefix")) {
    mqttPrefix = webServer.arg("mqttprefix");
    mqttPrefix.trim();
  }
  if (webServer.hasArg("tgtoken") && webServer.arg("tgtoken").length() > 0) {
    tgToken = webServer.arg("tgtoken");
    tgToken.trim();
  }
  if (webServer.hasArg("tgchat")) {
    tgChat = webServer.arg("tgchat");
    tgChat.trim();
  }
  if (webServer.hasArg("tgclear") && webServer.arg("tgclear").toInt()) {
    tgToken = "";
    tgChat = "";
  }
  if (webServer.hasArg("mqttclear") && webServer.arg("mqttclear").toInt()) {
    mqttHost = "";
  }

  sanitizeSettings();
  saveSettings();

  mqttClient.disconnect();
  lastMqttAttemptMs = 0;
  mqttWasConnected = false;
  tgOffset = 0;
  lastTgPollMs = 0;
  tgLastError = "";

  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", "{\"ok\":true}");
}

void handleApiHistory() {
  int hours = 24;
  if (webServer.hasArg("hours")) {
    hours = constrain(webServer.arg("hours").toInt(), 1, 720);
  }

  size_t fsUsed = 0, fsTotal = 0;
  fsStats(fsUsed, fsTotal);

  String j;
  j.reserve(9500);
  j  = "{\"timeOk\":";
  j += String(timeReady() ? 1 : 0);
  j += ",\"now\":";
  j += String((unsigned long)time(nullptr));
  j += ",\"hours\":";
  j += String(hours);
  j += ",\"count\":";
  j += String(histLines);
  j += ",\"fsUsed\":";
  j += String((unsigned long)fsUsed);
  j += ",\"fsTotal\":";
  j += String((unsigned long)fsTotal);
  j += ",\"points\":[";

  if (fsReady && LittleFS.exists(HIST_FILE)) {
    File f = LittleFS.open(HIST_FILE, "r");
    if (f) {
      time_t cutoff = time(nullptr) - (time_t)hours * 3600;
      int stride = (hours + 119) / 120;   // en fazla ~120 nokta
      if (stride < 1) stride = 1;
      int idx = 0;
      bool first = true;

      while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (line.length() < 10 || line[0] == '#') continue;

        int c1 = line.indexOf(',');
        if (c1 < 0) continue;
        long ts = line.substring(0, c1).toInt();
        if (timeReady() && ts < (long)cutoff) continue;
        if ((idx++ % stride) != 0) continue;

        int c2 = line.indexOf(',', c1 + 1);
        int c3 = line.indexOf(',', c2 + 1);
        int c4 = line.indexOf(',', c3 + 1);
        int c5 = line.indexOf(',', c4 + 1);
        if (c5 < 0) continue;

        if (!first) j += ",";
        first = false;
        j += "{\"t\":";
        j += String(ts);
        j += ",\"v\":";
        j += line.substring(c1 + 1, c2);
        j += ",\"h\":";
        j += line.substring(c2 + 1, c3);
        j += ",\"s\":";
        j += line.substring(c3 + 1, c4);
        j += ",\"d\":";
        j += line.substring(c4 + 1, c5);
        j += ",\"u\":";
        j += line.substring(c5 + 1);
        j += "}";
      }
      f.close();
    }
  }

  j += "]}";
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", j);
}

void handleApiSample() {
  bool saved = false;
  if (fsReady && timeReady() && sensorOk) {
    appendHistoryPoint();
    saved = true;
  }
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json",
                 String("{\"ok\":") + (saved ? "1" : "0") + ",\"count\":" + String(histLines) + "}");
}

void handleApiHistoryClear() {
  if (fsReady) {
    LittleFS.remove(HIST_FILE);
    histLines = 0;
    histBytes = 0;
  }
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", "{\"ok\":true}");
}

void handleApiWifi() {
  // "Simdi degil": kurulum karti bir daha acilmasin (AP modu bilincli secim)
  if (webServer.hasArg("skip")) {
    setupDone = true;
    saveSettings();
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send(200, "application/json", "{\"ok\":1,\"reboot\":0}");
    return;
  }

  if (!fsReady) {
    // Ayar dosyasi yazilamiyorsa sessizce basarili gorunmeyelim.
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send(200, "application/json", "{\"ok\":0,\"err\":\"fs\"}");
    return;
  }

  String ssid = webServer.arg("ssid");
  String pass = webServer.arg("pass");
  ssid.trim();

  String oldSsid = wifiSsid;
  if (ssid.length() == 0) {
    // AP-only mod: STA bilgilerini temizle
    wifiSsid = "";
    wifiPass = "";
    setupDone = true;            // bilincli olarak ev agi kullanilmiyor
  } else {
    wifiSsid = ssid;
    if (pass.length() > 0 || ssid != oldSsid) {
      wifiPass = pass;
    }
    // setupDone burada isaretlenmez: ancak ag gercekten baglanirsa tamamlanir.
  }
  saveSettings();

  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json",
                 "{\"ok\":1,\"reboot\":1,\"ssid\":\"" + jsonEscape(wifiSsid) + "\"}");
  restartAtMs = millis() + 1500UL;
}

// Tarama asenkron yurutulur: istek uzun sure bloklanmaz (AP+STA modunda
// bloklayan tarama istegi zaman asimina ve kopmalara yol acabiliyor).
// ?start=1 ile baslatilir, sonuc hazir olunca ayni uctan alinir.
static unsigned long scanStartMs = 0;

void handleApiWifiScan() {
  int16_t st;

  if (webServer.hasArg("start")) {
    WiFi.scanDelete();
    scanStartMs = millis();
    st = WiFi.scanNetworks(true, false);      // true = asenkron
    if (st < 0) st = -2;                      // -1 = tarama basladi (hazir degil)
  } else {
    st = WiFi.scanComplete();                 // -2 calisiyor, -1 yok, >=0 adet
  }

  if (st == -2 && millis() - scanStartMs > 20000UL) {
    WiFi.scanDelete();                        // takildi: birak
    st = -1;
  }

  if (st == -2) {
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send(200, "application/json", "{\"ok\":1,\"busy\":1}");
    return;
  }

  if (st < 0) {
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send(200, "application/json", "{\"ok\":0,\"err\":\"scan\"}");
    return;
  }

  String j;
  j.reserve(900);
  j  = "{\"ok\":1,\"busy\":0,\"state\":1,\"nets\":[";

  uint8_t added = 0;
  for (int i = 0; i < st && added < 12; i++) {
    String ssid = WiFi.SSID(i);
    ssid.trim();
    if (ssid.length() == 0) continue;     // gizli SSID'leri atla

    bool dup = false;
    for (int k = 0; k < i; k++) {
      if (WiFi.SSID(k) == WiFi.SSID(i)) { dup = true; break; }
    }
    if (dup) continue;                    // ayni adi bir kez goster

    if (added) j += ",";
    j += "{\"s\":\"" + jsonEscape(ssid) + "\",\"r\":" + String(WiFi.RSSI(i));
    j += ",\"e\":" + String(WiFi.encryptionType(i) == 0 ? 0 : 1);   // 0 = acik ag
    j += "}";
    added++;
  }
  j += "]}";
  WiFi.scanDelete();

  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", j);
}

void handleApiReboot() {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", "{\"ok\":true}");
  restartAtMs = millis() + 600UL;
}

void handleApiTheme() {
  bool changed = false;

  if (webServer.hasArg("set")) {
    changed = setThemeById(webServer.arg("set"));
    if (changed) {
      markSettingsDirty();
      forceSend = true;
    }
  }
  // /api/theme?step=1 (sonraki) / -1 (onceki): uzun basilan sag/sol tusu
  if (webServer.hasArg("step")) {
    long st = webServer.arg("step").toInt();
    themeStep(st < 0 ? -1 : 1);
    forceSend = true;
    changed = true;
  }
  if (webServer.hasArg("reload")) changed = themeStore.reload() || changed;

  String j;
  j.reserve(1400);
  j  = "{\"active\":\"";
  j += themeStore.active().id;
  j += "\",\"index\":";
  j += String(themeStore.index());
  j += ",\"count\":";
  j += String(OLED_THEME_COUNT);
  j += ",\"fs\":";
  j += String(themeStore.filesOk() ? 1 : 0);
  j += ",\"changed\":";
  j += String(changed ? 1 : 0);
  j += ",\"themes\":[";

  for (uint8_t i = 0; i < OLED_THEME_COUNT; i++) {
    const OledTheme &t = themeStore.builtin(i);
    if (i) j += ",";
    j += "{\"id\":\"";     j += t.id;
    j += "\",\"name\":\"";  j += t.name;
    j += "\",\"tempSize\":"; j += String(t.tempSize);
    j += ",\"header\":";   j += String(t.header);
    j += ",\"status\":";   j += String(t.statusStyle);
    j += ",\"separator\":"; j += String(t.separator);
    j += ",\"icons\":";     j += String(t.icons);
    j += ",\"font\":";      j += String(t.font);
    j += ",\"invert\":";    j += String(t.invert);
    j += "}";
  }
  j += "]}";

  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", j);
}

// ============================ CAPTIVE PORTAL ================================

// AP'ye baglanan cihazlarin (telefon, tablet, PC) web arayuzunu kendiliginden
// acmasi icin: tum alan adlari bu cihaza cozulur ve bilinmeyen adresler buraya
// yonlendirilir (klasik captive portal davranisi).
String portalUrl() {
  return String("http://") + WiFi.softAPIP().toString() + "/";
}

void sendPortalRedirect() {
  String url = portalUrl();
  String body;
  body.reserve(240);
  body  = F("<!DOCTYPE html><html><head><meta charset='utf-8'>");
  body += F("<meta http-equiv='refresh' content='0;url=");
  body += url;
  body += F("'></head><body><a href='");
  body += url;
  body += F("'>KombiPlus</a></body></html>");

  webServer.sendHeader("Location", url, true);
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(302, "text/html; charset=utf-8", body);
}

// Windows / Android / iOS / macOS'un "agda oturum acin" kontrol adresleri.
// 204 veya beklenen govde yerine 302 donmek portal penceresini tetikler.
static const char *const CAPTIVE_PATHS[] = {
  "/generate_204", "/gen_204", "/hotspot-detect.html",
  "/library/test/success.html", "/success.txt", "/ncsi.txt",
  "/connecttest.txt", "/redirect", "/canonical.html", "/portal"
};

String hostOnly(String host) {
  host.trim();
  int c = host.indexOf(':');
  if (c > 0) host = host.substring(0, c);
  host.toLowerCase();
  return host;
}

// ---- /test: panel simulasyonu ---------------------------------------------
// OLED tamponunu (1024 bayt, sayfa bazli) hex olarak verir. Ekran modulu
// bagli olmasa da cizim tamponu oldugu icin sayfa cihazin ne gosterecegini
// birebir yansitir; tema renkleri (invert) ayrica bildirilir.
void handleApiOled() {
  uint8_t *buf = display.getBuffer();
  if (!buf) {
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send(503, "application/json", "{\"ok\":0,\"err\":\"tampon yok\"}");
    return;
  }

  static const char *const HXD = "0123456789abcdef";
  static char out[2240];
  const OledTheme &th = themeStore.active();

  int n = snprintf(out, sizeof(out),
                   "{\"ok\":1,\"page\":%u,\"invert\":%u,\"dim\":%u,\"ask\":%u,\"px\":\"",
                   (unsigned)oledPage, (unsigned)(th.invert ? 1 : 0),
                   (unsigned)(th.dim ? 1 : 0), (unsigned)(langChosen() ? 0 : 1));
  if (n < 0 || n > (int)sizeof(out) - 2060) {
    webServer.send(500, "application/json", "{\"ok\":0,\"err\":\"tampon kucuk\"}");
    return;
  }
  for (uint16_t i = 0; i < 1024; i++) {
    out[n++] = HXD[buf[i] >> 4];
    out[n++] = HXD[buf[i] & 0x0F];
  }
  out[n++] = '"';
  out[n++] = '}';
  out[n] = 0;

  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", out);
}

// /test: enkoder cevirme. steps = tik sayisi (yon isaretli), her tik 4
// gecis uretir; handleEncoder bunlari 4'luk gruplar halinde isler.
void handleApiEncoder() {
  long steps = webServer.arg("steps").toInt();
  if (steps == 0) steps = webServer.arg("d").toInt();
  steps = constrain(steps, -20L, 20L);

  noInterrupts();
  encoderDelta = (int16_t)(encoderDelta + (int16_t)(steps * 4));
  interrupts();

  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", "{\"ok\":1}");
}

// /test: buton basisi. ms = basili kalma suresi; loop icinde gercek butonla
// ayni isleyici (applyButtonPress) calisir.
void handleApiButton() {
  long ms = webServer.arg("ms").toInt();
  if (ms < 40) ms = 120;
  if (ms > 6000) ms = 6000;
  pendingButtonMs = (unsigned long)ms;

  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json", "{\"ok\":1}");
}

void setupWeb() {
  webServer.on("/", HTTP_GET, []() {
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  });
  webServer.on("/api/status", HTTP_GET, handleApiStatus);
  webServer.on("/api/set", HTTP_GET, handleApiSet);
  webServer.on("/api/net", HTTP_POST, handleApiNet);
  webServer.on("/api/net", HTTP_GET, handleApiNet);
  webServer.on("/api/scan", HTTP_GET, handleApiWifiScan);
  webServer.on("/api/scan", HTTP_POST, handleApiWifiScan);
  webServer.on("/api/wifi", HTTP_POST, handleApiWifi);
  webServer.on("/api/wifi", HTTP_GET, handleApiWifi);
  webServer.on("/api/history", HTTP_GET, handleApiHistory);
  webServer.on("/api/history/sample", HTTP_GET, handleApiSample);
  webServer.on("/api/history/sample", HTTP_POST, handleApiSample);
  webServer.on("/api/history/clear", HTTP_GET, handleApiHistoryClear);
  webServer.on("/api/history/clear", HTTP_POST, handleApiHistoryClear);
  webServer.on("/api/reboot", HTTP_POST, handleApiReboot);
  webServer.on("/api/reboot", HTTP_GET, handleApiReboot);
  webServer.on("/api/theme", HTTP_GET, handleApiTheme);
  webServer.on("/api/theme", HTTP_POST, handleApiTheme);

  // Panel simulasyonu: /test sayfasi + OLED aynasi ve sanal kontroller
  webServer.on("/test", HTTP_GET, []() {
    webServer.sendHeader("Cache-Control", "no-store");
    webServer.send_P(200, "text/html; charset=utf-8", TEST_HTML);
  });
  webServer.on("/api/oled", HTTP_GET, handleApiOled);
  webServer.on("/api/encoder", HTTP_GET, handleApiEncoder);
  webServer.on("/api/encoder", HTTP_POST, handleApiEncoder);
  webServer.on("/api/button", HTTP_GET, handleApiButton);
  webServer.on("/api/button", HTTP_POST, handleApiButton);

  // LittleFS dosya yoneticisi: http://<ip>/fs
  fsExplorer.begin(&webServer, "/fs");

  for (uint8_t i = 0; i < sizeof(CAPTIVE_PATHS) / sizeof(CAPTIVE_PATHS[0]); i++) {
    webServer.on(CAPTIVE_PATHS[i], HTTP_GET, sendPortalRedirect);
  }

  webServer.onNotFound([]() {
    String host = hostOnly(webServer.hostHeader());
    String apIp = WiFi.softAPIP().toString();
    String staIp = WiFi.localIP().toString();
    if (host.length() == 0 || (host != apIp && host != staIp)) {
      sendPortalRedirect();     // captive portal yakalama
      return;
    }
    webServer.send(404, "text/plain; charset=utf-8", "Bulunamadi");
  });
  webServer.begin();
}

// ============================ OLED EKRAN ===================================

// ---- Ekran metinleri: en / tr / es / pt (ASCII, gomulu font) --------------
enum {
  L_MAIN, L_NET, L_INFO, L_SLEEP, L_THEME, L_LANG,
  L_SLEEP_TAG, L_TARGET, L_TARGET_SHORT,
  L_HEATING, L_WAITING, L_OFF, L_SENSOR_ERR, L_NO_BOILER,
  L_HUM, L_WATER, L_BOOST, L_MIN,
  L_BOILER, L_LINKED, L_NO, L_RELAY_IP, L_SIGNAL, L_AP_MODE,
  L_HYST, L_CAL, L_FROST, L_NONE, L_UPTIME,
  L_STATE, L_ACTIVE, L_RANGE, L_SLEEP_TARGET, L_NORMAL_TARGET,
  L_CLOCK, L_RELAY_FS, L_FS_HOLD, L_FS_CLOSE,
  L_NAME, L_ID, L_HEADER, L_STATUS, L_ENC_THEME,
  L_DAY, L_HOUR, L_MINUTE,
  L_TXT_COUNT
};

static const char *const OLED_TXT[4][L_TXT_COUNT] = {
  // en
  {"TERMOSTAT", "NETWORK", "INFO", "SLEEP", "THEME", "LANGUAGE",
   "SLEEP", "TARGET", "HD",
   "HEATING", "WAITING", "OFF/FROST", "SENSOR ERROR", "NO BOILER",
   "Hum", "Water", "BST", "min",
   "Boiler", "LINKED", "NO", "Relay IP", "Signal", "AP mode",
   "Hysteresis", "Calibration", "Frost", "none", "Uptime",
   "State", "ACTIVE", "Range", "Sleep target", "Normal target",
   "Clock", "Relay FS", "stays ON", "closes",
   "Name", "Id", "Header", "Status", "Turn: theme",
   "d", "h", "m"},
  // tr
  {"TERMOSTAT", "AG BILGISI", "BILGI", "UYKU MODU", "TEMA", "DIL SEC",
   "UYKU", "HEDEF", "HD",
   "ISITIYOR", "BEKLIYOR", "KAPALI/DON KOR.", "SENSOR HATASI", "KOMBI BAGLI DEGIL",
   "Nem", "Su", "BST", "dk",
   "Kombi", "BAGLI", "YOK", "Role IP", "Sinyal", "AP modu",
   "Histerezis", "Kalibrasyon", "Don koruma", "yok", "Calisma",
   "Durum", "AKTIF", "Aralik", "Uyku hedefi", "Normal hedef",
   "Saat", "Role FS", "ACIK kalir", "kapanir",
   "Ad", "Kimlik", "Ust", "Durum", "Enkoder: tema",
   "g", "s", "d"},
  // es
  {"TERMOSTATO", "RED", "INFO", "SUENO", "TEMA", "IDIOMA",
   "SUENO", "OBJETIVO", "OBJ",
   "CALENTANDO", "ESPERANDO", "APAG./ANTIHIELO", "FALLO SENSOR", "SIN CALDERA",
   "Hum", "Agua", "BST", "min",
   "Caldera", "CONECT.", "NO", "IP rele", "Senal", "modo AP",
   "Histeresis", "Calibracion", "Antihielo", "nada", "Encendido",
   "Estado", "ACTIVO", "Horario", "Obj. sueno", "Obj. normal",
   "Reloj", "FS rele", "queda ON", "cierra",
   "Nombre", "Id", "Cabecera", "Estado", "Giro: tema",
   "d", "h", "m"},
  // pt
  {"TERMOSTATO", "REDE", "INFO", "DORMIR", "TEMA", "IDIOMA",
   "DORMIR", "ALVO", "ALV",
   "AQUECENDO", "AGUARDANDO", "DESL./ANTIGELO", "ERRO SENSOR", "SEM CALDEIRA",
   "Umid", "Agua", "BST", "min",
   "Caldeira", "CONECT.", "NAO", "IP rele", "Sinal", "modo AP",
   "Histerese", "Calibracao", "Antigelo", "nada", "Ativo",
   "Estado", "ATIVO", "Horario", "Alvo dormir", "Alvo normal",
   "Relogio", "FS rele", "fica ON", "fecha",
   "Nome", "Id", "Cabecalho", "Status", "Giro: tema",
   "d", "h", "m"}
};

uint8_t oledLang() {
  uint8_t i = langIndex(uiLang);
  if (i == 1) return 1;   // tr
  if (i == 3) return 2;   // es
  if (i == 4) return 3;   // pt
  return 0;               // en (ru/ja/ko/zh icin de)
}

const char *OT(uint8_t id) {
  return OLED_TXT[oledLang()][id];
}

// Ust bant saati: NTP yoksa "--:--" gosterilir.
void oledClockText(char *out, size_t n) {
  if (!timeReady()) { snprintf(out, n, "--:--"); return; }
  time_t t = time(nullptr);
  struct tm tmv;
  localtime_r(&t, &tmv);
  snprintf(out, n, "%02d:%02d", tmv.tm_hour, tmv.tm_min);
}

void oledHeader() {
  uint8_t notes = 0;
  if (!sensorOk)                      notes |= OLED_NOTE_SENSOR;
  if (!boilerLinked())                notes |= OLED_NOTE_NOLINK;
  if (WiFi.status() != WL_CONNECTED)  notes |= OLED_NOTE_AP;
  if (sleepActive)                    notes |= OLED_NOTE_SLEEP;
  if (boostActive)                    notes |= OLED_NOTE_BOOST;
  if (!cfg.mode)                      notes |= OLED_NOTE_OFF;
  char hm[8];
  oledClockText(hm, sizeof(hm));
  oledThemeStatusBar(display, themeStore.active(), notes, oledPage,
                     OLED_PAGE_COUNT, hm);
}

// ---- Buyuk sicaklik yazisi -------------------------------------------------
// Tum temalar ayni yuvayi kullanir; yalnizca font degisir:
//   0 = Adafruit GFX yerlesik fontu (4x, uzun degerlerde 3x)
//   1 = yedi parcali (dijital) rakamlar
#define SEG_W   16
#define SEG_H   30
#define SEG_T   3
#define SEG_GAP 4

// A B C D E F G = 0x01..0x40
static const uint8_t SEG_DIGIT[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

static int segCharW(char ch) {
  if (ch == '-') return 10;
  if (ch == '.') return 4;
  if (ch == ' ') return 6;
  return SEG_W;
}

static void segDigit(int x, int y, uint8_t m) {
  const int hx = x + SEG_T;
  const int hw = SEG_W - 2 * SEG_T;
  if (m & 0x01) display.fillRect(hx, y, hw, SEG_T, SSD1306_WHITE);                       // A
  if (m & 0x02) display.fillRect(x + SEG_W - SEG_T, y + SEG_T, SEG_T, SEG_H / 2 - SEG_T, SSD1306_WHITE);
  if (m & 0x04) display.fillRect(x + SEG_W - SEG_T, y + SEG_H / 2, SEG_T, SEG_H / 2 - SEG_T, SSD1306_WHITE);
  if (m & 0x08) display.fillRect(hx, y + SEG_H - SEG_T, hw, SEG_T, SSD1306_WHITE);       // D
  if (m & 0x10) display.fillRect(x, y + SEG_H / 2, SEG_T, SEG_H / 2 - SEG_T, SSD1306_WHITE);
  if (m & 0x20) display.fillRect(x, y + SEG_T, SEG_T, SEG_H / 2 - SEG_T, SSD1306_WHITE);
  if (m & 0x40) display.fillRect(hx, y + SEG_H / 2 - SEG_T / 2, hw, SEG_T, SSD1306_WHITE);
}

// Degerin kapladigi genislik (ortalamak ve "C" konumu icin).
static int bigWidth(const char *s, uint8_t font, uint8_t tsize) {
  int n = (int)strlen(s);
  if (font == 0) {
    return n * 6 * (int)tsize;
  }
  int w = 0;
  for (int i = 0; i < n; i++) {
    w += segCharW(s[i]);
    if (i < n - 1) w += SEG_GAP;
  }
  return w;
}

static void bigText(const char *s, int x, int y, uint8_t font, uint8_t tsize) {
  if (font == 0) {
    display.setTextSize(tsize);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(x, y);
    display.print(s);
    if (tsize >= 5) {
      // 1 px saga ikinci gecis: ince GFX fontunu kalinlastirir.
      display.setCursor(x + 1, y);
      display.print(s);
    }
    display.setTextSize(1);
    return;
  }
  int cx = x;
  for (const char *p = s; *p; p++) {
    int cw = segCharW(*p);
    if (*p >= '0' && *p <= '9')      segDigit(cx, y, SEG_DIGIT[*p - '0']);
    else if (*p == '-')              display.fillRect(cx, y + SEG_H / 2 - SEG_T / 2, cw, SEG_T, SSD1306_WHITE);
    else if (*p == '.')              display.fillRect(cx + 1, y + SEG_H - SEG_T, 3, SEG_T, SSD1306_WHITE);
    cx += cw + SEG_GAP;
  }
}

// Ana ekran: oda sicakligi olabildigince buyuk (4x, ortada). Hedef deger
// normalde gizlidir; encoder cevrilince 10 sn boyunca ayni yuvada yanip soner.
// Alt satirda yalnizca veri varsa nem / kombi suyu. Durum metni yok.
void drawMainPage() {
  const OledTheme &th = themeStore.active();
  oledHeader();

  const int base  = oledThemeHeaderHeight(th);    // sari bantin hemen alti
  const int footY = OLED_BAND_H + 38;             // 54: en alt satir

  bool sensorReady = sensorOk && !isnan(currentTemp);
  // Don korumasi dahil her isi talebinde sicaklik yanip soner.
  bool heating = demand && boilerLinked();

  // Hedef gosterimi: son encoder cevirmesinden sonra SETPOINT_PEEK_MS.
  if (setpointPeekMs != 0 &&
      (millis() - setpointPeekMs) >= SETPOINT_PEEK_MS) {
    setpointPeekMs = 0;
  }
  bool editing = (setpointPeekMs != 0);

  // Alt satirda bir sey yazilacaksa buyuk yazi 4x kalir; bos oldugunda 5x
  // ile govdenin tamamini kullanir. (5x dort karakterde 120 px genislik.)
  bool footLeft  = editing || (th.showHum && sensorReady && !isnan(currentHum));
  bool footRight = (th.showBoost && boostActive) ||
                   (th.showWater && boilerFlow > -100.0f);
  bool footShown = footLeft || footRight;

  // Deger sabit tamponda tutulur: 4 Hz yenilemede String ayirmasi yapilmaz.
  char big[8];
  bool bigOn;
  if (editing) {
    snprintf(big, sizeof(big), "%.1f", cfg.setpoint);
    bigOn = ((millis() / 450UL) % 2 == 0);        // hedef yanip soner
  } else {
    if (sensorReady) snprintf(big, sizeof(big), "%.1f", currentTemp);
    else             snprintf(big, sizeof(big), "00.0");
    // Isitma talebi varken deger yanip soner: "ISITIYOR" yazisi yerine gorsel.
    bigOn = !heating || ((millis() / 600UL) % 2 == 0);
  }

  // font 0: 4x sigmayan uzun degerler (or. "-10.5") 3x cizilir.
  const uint8_t font = th.font;
  uint8_t tsize = 4;
  if (font == 0) {
    tsize = ((int)strlen(big) <= 4) ? (footShown ? 4 : 5) : 3;
  }
  int bigW  = bigWidth(big, font, tsize);
  int lineY = base + 1;
  int cOff  = SEG_H - 9;                          // kucuk "C" dikey konumu
  if (font == 0) {
    if (tsize == 5)      lineY = base + 4;        // 40 px: govdeye ortala
    else if (tsize == 3) lineY += 4;
    cOff = tsize * 8 - 9;
  }
  int x0 = (128 - (bigW + 8)) / 2;                // +8: kucuk "C" ve bosluk
  if (x0 < 0) x0 = 0;

  if (bigOn) {
    bigText(big, x0, lineY, font, tsize);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(x0 + bigW + 2, lineY + cOff);
    display.print(F("C"));
  }

  // Alt satir: solda hedef etiketi ya da nem, sagda boost / kombi suyu.
  if (footShown) oledThemeSeparator(display, th, footY - 2);

  display.setTextSize(1);
  if (editing) {
    display.setCursor(0, footY);
    display.print(OT(L_TARGET));
  } else if (footLeft) {
    // Nem yalnizca gecerli veri varsa yazilir; yoksa ekranda yer kaplamaz.
    display.setCursor(0, footY);
    display.print(OT(L_HUM));
    display.print(F(" %"));
    display.print((int)roundf(currentHum));
  }

  // Alt sag: boost kalan sure, yoksa kombi su sicakligi (veri varsa).
  if (th.showBoost && boostActive) {
    char b[16];
    snprintf(b, sizeof(b), "%s %u%s", OT(L_BOOST),
             (unsigned)((boostRemainSec() + 59) / 60), OT(L_MIN));
    display.setCursor(128 - (int)strlen(b) * 6, footY);
    display.print(b);
  } else if (th.showWater && boilerFlow > -100.0f) {
    char f[20];
    snprintf(f, sizeof(f), "%s %.1fC", OT(L_WATER), boilerFlow);
    display.setCursor(128 - (int)strlen(f) * 6, footY);
    display.print(f);
  }
}

void drawNetPage() {
  oledHeader();
  display.setTextSize(1);
  display.setCursor(0, OLED_BAND_H);
  display.print(F("AP: "));  display.print(WiFi.softAPIP());

  display.setCursor(0, OLED_BAND_H + 8);
  display.print(F("IP: "));  display.print(currentIp());

  String ssid = wifiSsid.length() ? wifiSsid : String("-");
  if (ssid.length() > 19) ssid = ssid.substring(0, 19);
  display.setCursor(0, OLED_BAND_H + 16);
  display.print(F("STA: ")); display.print(ssid);

  display.setCursor(0, OLED_BAND_H + 24);
  display.print(OT(L_BOILER));
  display.print(F(": "));
  display.print(OT(boilerLinked() ? L_LINKED : L_NO));
  if (boilerClient && boilerClient.connected()) {
    display.setCursor(0, OLED_BAND_H + 32);
    display.print(OT(L_RELAY_IP));
    display.print(F(": "));
    display.print(boilerClient.remoteIP());
  }

  display.setCursor(0, OLED_BAND_H + 40);
  if (WiFi.status() == WL_CONNECTED) {
    display.print(OT(L_SIGNAL));
    display.print(F(": "));
    display.print(WiFi.RSSI());
    display.print(F(" dBm"));
  } else {
    display.print(OT(L_SIGNAL));
    display.print(F(": "));
    display.print(OT(L_AP_MODE));
  }
}

void drawInfoPage() {
  oledHeader();

  display.setTextSize(1);
  // Sabit tamponlar: 4 Hz yenilemede String ayirmasi yapilmaz.
  display.setCursor(0, OLED_BAND_H);
  display.print(OT(L_HYST));
  display.print(F(": "));
  display.print(cfg.hysteresis, 1);
  display.print(F("C"));

  display.setCursor(0, OLED_BAND_H + 8);
  display.print(OT(L_CAL));
  display.print(F(": "));
  if (cfg.calibration >= 0) display.print(F("+"));
  display.print(cfg.calibration, 1);
  display.print(F("C"));

  display.setCursor(0, OLED_BAND_H + 16);
  display.print(OT(L_FROST));
  display.print(F(": "));
  display.print(cfg.frost, 1);
  display.print(F("C"));

  display.setCursor(0, OLED_BAND_H + 24);
  display.print(OT(L_BOOST));
  display.print(F(": "));
  if (boostActive) {
    display.print((boostRemainSec() + 59) / 60);
    display.print(' ');
    display.print(OT(L_MIN));
  } else {
    display.print(OT(L_NONE));
  }

  unsigned long up = millis() / 1000UL;
  unsigned long d = up / 86400UL;
  unsigned long h = (up % 86400UL) / 3600UL;
  unsigned long m = (up % 3600UL) / 60UL;
  display.setCursor(0, OLED_BAND_H + 32);
  display.print(OT(L_UPTIME));
  display.print(F(": "));
  if (d > 0) { display.print(d); display.print(OT(L_DAY)); display.print(' '); }
  display.print(h);
  display.print(OT(L_HOUR));
  display.print(' ');
  display.print(m);
  display.print(OT(L_MINUTE));

  display.setCursor(0, OLED_BAND_H + 40);
  display.print(F("FW " FW_VERSION "  Heap:"));
  display.print(ESP.getFreeHeap() / 1024);
  display.print(F("k"));
}

void drawSleepPage() {
  oledHeader();

  display.setTextSize(1);
  display.setCursor(0, OLED_BAND_H);
  display.print(OT(L_STATE));
  display.print(F(": "));
  if (!cfg.sleepEnabled) {
    display.print(OT(L_OFF));
  } else if (sleepActive) {
    display.print(OT(L_ACTIVE));
  } else {
    display.print(OT(L_WAITING));
  }

  char hmA[6], hmB[6];
  snprintf(hmA, sizeof(hmA), "%02u:%02u",
           (unsigned)((cfg.sleepStartMin / 60) % 24), (unsigned)(cfg.sleepStartMin % 60));
  snprintf(hmB, sizeof(hmB), "%02u:%02u",
           (unsigned)((cfg.sleepEndMin / 60) % 24), (unsigned)(cfg.sleepEndMin % 60));

  display.setCursor(0, OLED_BAND_H + 8);
  display.print(OT(L_RANGE));
  display.print(F(": "));
  display.print(hmA);
  display.print(F(" - "));
  display.print(hmB);

  display.setCursor(0, OLED_BAND_H + 16);
  display.print(OT(L_SLEEP_TARGET));
  display.print(F(": "));
  display.print(cfg.sleepSetpoint, 1);
  display.print(F("C"));

  display.setCursor(0, OLED_BAND_H + 24);
  display.print(OT(L_NORMAL_TARGET));
  display.print(F(": "));
  display.print(cfg.setpoint, 1);
  display.print(F("C"));

  display.setCursor(0, OLED_BAND_H + 32);
  display.print(OT(L_CLOCK));
  display.print(F(": "));
  if (timeReady()) display.print(F("NTP OK"));
  else             display.print(OT(L_NONE));
  if (timeReady()) {
    time_t t = time(nullptr);
    struct tm tmv;
    localtime_r(&t, &tmv);
    char b[8];
    snprintf(b, sizeof(b), " %02d:%02d", tmv.tm_hour, tmv.tm_min);
    display.print(b);
  }

  display.setCursor(0, OLED_BAND_H + 40);
  display.print(OT(L_RELAY_FS));
  display.print(F(": "));
  display.print(OT(cfg.relayFailSafeOn ? L_FS_HOLD : L_FS_CLOSE));
}

// Ilk acilis dil secimi ekrani: encoder cevir -> dil, kisa bas -> kaydet.
void drawLangPage() {
  oledHeader();
  display.setTextSize(1);

  display.setCursor(0, OLED_BAND_H);
  display.print(F("SELECT LANGUAGE"));

  char line[26];
  snprintf(line, sizeof(line), "%u/%u  %s", (unsigned)(langCursor + 1),
           (unsigned)LANG_COUNT, LANG_NAMES[langCursor]);
  int w = (int)strlen(line) * 6;
  display.setCursor(w < 128 ? (128 - w) / 2 : 0, 27);
  display.print(line);

  display.setCursor(0, 38);
  display.print(F("TURN: SELECT"));
  display.setCursor(0, 48);
  display.print(F("PRESS: SET"));

  display.setCursor(0, 57);
  display.print(F("WEB 192.168.4.1"));
}

void drawOled() {
  // Panel bagli olmasa da tampon cizilir: /test gercek ekran icerigini
  // birebir gosterebilsin (SSD1306 tamponu begin() icinde ayrilir).
  if (!display.getBuffer()) return;
  unsigned long now = millis();
  if (now - lastOledMs < OLED_REFRESH_MS) return;
  lastOledMs = now;

  const OledTheme &th = themeStore.active();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (!langChosen()) {
    drawLangPage();
  } else {
    switch (oledPage) {
      case 1:  drawNetPage();   break;
      case 2:  drawInfoPage();  break;
      case 3:  drawSleepPage(); break;
      default: drawMainPage();  break;
    }
  }

  if (displayReady) {
    static uint8_t appliedInvert = 0xFF;
    static uint8_t appliedDim = 0xFF;
    if (th.invert != appliedInvert) {
      display.invertDisplay(th.invert);
      appliedInvert = th.invert;
    }
    if (th.dim != appliedDim) {
      display.dim(th.dim);
      appliedDim = th.dim;
    }

    display.display();
  }
}
