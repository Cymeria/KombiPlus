/*
 * ============================================================================
 *  KOMBIPLUS - KOMBI ROLE UNITESI
 *  NodeMCU v3 (ESP8266) + role modulu (+ opsiyonel DS18B20 su sensoru)
 *
 *  Bu cihaz:
 *    - Oda unitesinin AP'sine (KombiPlus) baglanir
 *    - TCP 8266 uzerinden gelen isi talebine gore kombi kontagini acar/kapatir
 *    - 5 dk boyunca komut gelmezse secilen fail-safe davranisi uygulanir
 *      (role kapanir veya son talep korunur; tercih EEPROM'da saklanir)
 *    - Opsiyonel DS18B20 ile kombi su sicakligini oda unitesine bildirir
 *
 *  GUVENLIK: Kombi termostat girisleri genellikle dusuk gerilimdir.
 *  230 V ile calisiyorsaniz isi mutlaka yetkili bir teknisyene yaptirin.
 *  Role modulu opto-izolasyonlu ve sigortali olmalidir.
 * ============================================================================
 */

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ============================ KULLANICI AYARLARI ============================

#define FW_VERSION "1.1.1"

// Role modulu (aktif dusuk role kartlari yaygindir)
#define RELAY_PIN          D1        // GPIO5
#define RELAY_ACTIVE_LOW   1         // aktif-low kart icin 1, aktif-high icin 0

// Karti uzerindeki mavi LED (GPIO2 / D4)
#define STATUS_LED_PIN     LED_BUILTIN
#define LED_ACTIVE_LOW     1

// Opsiyonel kombi su sensoru (waterproof DS18B20)
#define FLOW_SENSOR_ENABLED 0
#define FLOW_PIN            D2        // GPIO4, 4.7k pull-up 3V3'e

// Oda unitesi AP bilgileri (oda kodu ile AYNI olmali!)
#define TERMOSTAT_AP_SSID  "KombiPlus"
#define TERMOSTAT_AP_PASS  "Kombi.1881"

// Oda unitesinin AP uzerindeki sabit adresi
#define SERVER_HOST        "192.168.4.1"
#define TCP_PORT           8266

// Guvenlik / zamanlama
#define LINK_TIMEOUT_MS    300000UL   // 5 dk komut gelmezse role kapanir
#define STATUS_PERIOD_MS   2000UL
#define WIFI_RETRY_MS      15000UL
#define CONNECT_RETRY_MS   5000UL

// ============================ GLOBAL =======================================

WiFiClient client;
bool relayOn = false;
bool heatRequest = false;
bool linkEverSeen = false;      // ilk komut geldi mi? (acilista role kapali kalsin)
bool failSafeOn = false;        // oda unitesi erisilemezse son talebi koru
unsigned long lastSetMs = 0;
unsigned long lastStatusMs = 0;
unsigned long lastConnectAttemptMs = 0;
String rxLine = "";

// ---- EEPROM: fail-safe tercihi ----
#define RLY_MAGIC 0x524C5931UL   // "RLY1"
struct RlyPersist {
  uint32_t magic;
  uint8_t  failSafeOn;
};
const int EEPROM_SIZE = 16;

#if FLOW_SENSOR_ENABLED
  OneWire oneWire(FLOW_PIN);
  DallasTemperature flowSensor(&oneWire);
  float flowTemp = -127.0f;
  bool flowPending = false;
  unsigned long flowRequestMs = 0;
  unsigned long flowLastStartMs = 0;
#endif

// ============================ ROLE =========================================

void loadFailSafe() {
  RlyPersist p;
  EEPROM.get(0, p);
  if (p.magic == RLY_MAGIC && p.failSafeOn <= 1) {
    failSafeOn = (p.failSafeOn == 1);
  } else {
    failSafeOn = false;
    saveFailSafe();
  }
  Serial.print(F("Fail-safe (baglanti yok): role "));
  Serial.println(failSafeOn ? F("ACIK kalir") : F("KAPANIR"));
}

void saveFailSafe() {
  RlyPersist p;
  p.magic = RLY_MAGIC;
  p.failSafeOn = failSafeOn ? 1 : 0;
  EEPROM.put(0, p);
  EEPROM.commit();
}

void setRelay(bool on) {
  relayOn = on;
#if RELAY_ACTIVE_LOW
  digitalWrite(RELAY_PIN, on ? LOW : HIGH);
#else
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
#endif
}

// ============================ WI-FI ========================================

void connectWiFi() {
  Serial.print(F("Wi-Fi baglaniyor: "));
  Serial.println(TERMOSTAT_AP_SSID);
  WiFi.disconnect();
  WiFi.begin(TERMOSTAT_AP_SSID, TERMOSTAT_AP_PASS);
}

void updateWiFi() {
  static bool wasUp = false;
  static unsigned long lastAttempt = 0;

  if (WiFi.status() == WL_CONNECTED) {
    if (!wasUp) {
      wasUp = true;
      Serial.print(F("Wi-Fi OK. IP: "));
      Serial.println(WiFi.localIP());
      lastConnectAttemptMs = 0;   // hemen TCP baglanmayi dene
    }
    return;
  }

  if (wasUp) {
    wasUp = false;
    Serial.println(F("Wi-Fi koptu, yeniden baglanilacak."));
  }

  if (lastAttempt == 0 || (millis() - lastAttempt > WIFI_RETRY_MS)) {
    lastAttempt = millis();
    connectWiFi();
  }
}

// ============================ TCP ==========================================

void sendStatus() {
  if (!client.connected()) return;
#if FLOW_SENSOR_ENABLED
  float ft = flowTemp;
#else
  float ft = -127.0f;
#endif
  char buf[130];
  snprintf(buf, sizeof(buf), "STATUS relay=%d flow=%.2f rssi=%d up=%lu fs=%d fw=%s\n",
           relayOn ? 1 : 0, ft, WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0,
           millis() / 1000UL, failSafeOn ? 1 : 0, FW_VERSION);
  client.print(buf);
}

void parseLine(const String &line) {
  if (!line.startsWith("SET")) return;

  bool heat = (line.indexOf("heat=1") >= 0);
  if (heat != heatRequest) {
    Serial.print(F("Isi talebi: "));
    Serial.println(heat ? F("ACIK") : F("KAPALI"));
  }
  heatRequest = heat;
  linkEverSeen = true;
  lastSetMs = millis();

  int fi = line.indexOf("fs=");
  if (fi >= 0) {
    int v = line.substring(fi + 3).toInt();
    bool want = (v != 0);
    if (want != failSafeOn) {
      failSafeOn = want;
      saveFailSafe();
      Serial.print(F("Fail-safe ayari: role "));
      Serial.println(failSafeOn ? F("ACIK kalir") : F("KAPANIR"));
    }
  }
}

void handleClient() {
  if (client && !client.connected()) {
    client.stop();
    client = WiFiClient();
    rxLine = "";
    Serial.println(F("Oda unitesi baglantisi koptu."));
  }

  if (!client.connected()) {
    if (WiFi.status() != WL_CONNECTED) return;
    if (millis() - lastConnectAttemptMs < CONNECT_RETRY_MS) return;
    lastConnectAttemptMs = millis();

    Serial.print(F("TCP baglaniyor: "));
    Serial.print(SERVER_HOST);
    Serial.print(':');
    Serial.println(TCP_PORT);

    if (client.connect(SERVER_HOST, TCP_PORT)) {
      client.setNoDelay(true);
      rxLine = "";
      Serial.println(F("Oda unitesine baglandi."));
      sendStatus();
    }
    return;
  }

  while (client.available()) {
    char c = (char)client.read();
    if (c == '\n') {
      parseLine(rxLine);
      rxLine = "";
    } else if (c != '\r' && rxLine.length() < 160) {
      rxLine += c;
    }
  }
}

void updateRelay() {
  bool linkFresh = (lastSetMs != 0) && (millis() - lastSetMs < LINK_TIMEOUT_MS);

  bool want;
  if (linkFresh) {
    want = heatRequest;
  } else if (linkEverSeen && failSafeOn) {
    // Oda unitesi erisilemiyor ve kullanici "acik kal" sectiyse son talep korunur.
    want = heatRequest;
  } else {
    want = false;
  }

  if (want != relayOn) {
    setRelay(want);
    Serial.print(F("ROLE: "));
    Serial.println(want ? F("ACIK") : F("KAPALI"));
  }

  static bool fsReported = false;
  if (!linkFresh && linkEverSeen && !fsReported) {
    fsReported = true;
    Serial.println(failSafeOn
      ? F("FAIL-SAFE: Baglanti yok, son talep korunuyor (fs=1).")
      : F("FAIL-SAFE: Baglanti yok, role kapatildi (fs=0)."));
  }
  if (linkFresh) fsReported = false;
}

// ============================ SU SENSORU ===================================

#if FLOW_SENSOR_ENABLED
void updateFlowSensor() {
  unsigned long now = millis();

  if (!flowPending && (now - flowLastStartMs >= 3000UL)) {
    flowSensor.requestTemperatures();
    flowPending = true;
    flowRequestMs = now;
    flowLastStartMs = now;
  } else if (flowPending && (now - flowRequestMs >= 800UL)) {
    flowPending = false;
    float t = flowSensor.getTempCByIndex(0);
    if (t == DEVICE_DISCONNECTED_C || t < -55.0f || t > 125.0f) {
      flowTemp = -127.0f;
    } else {
      flowTemp = t;
    }
  }
}
#endif

// ============================ LED ==========================================

void updateLed() {
  bool link = (lastSetMs != 0) && (millis() - lastSetMs < LINK_TIMEOUT_MS) && client.connected();
  bool wifi = (WiFi.status() == WL_CONNECTED);
  unsigned long now = millis();

  uint8_t onLevel = LED_ACTIVE_LOW ? LOW : HIGH;
  uint8_t offLevel = LED_ACTIVE_LOW ? HIGH : LOW;

  if (link) {
    digitalWrite(STATUS_LED_PIN, onLevel);                 // surekli yanik
  } else if (wifi) {
    digitalWrite(STATUS_LED_PIN, ((now / 500) % 2) ? onLevel : offLevel);  // 1 Hz
  } else {
    digitalWrite(STATUS_LED_PIN, ((now / 100) % 2) ? onLevel : offLevel);  // 5 Hz
  }
}

// ============================ SETUP / LOOP =================================

void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println();
  Serial.println(F("=== KombiPlusi - Role Unitesi ==="));
  Serial.println(F("FW " FW_VERSION));

  pinMode(RELAY_PIN, OUTPUT);
  setRelay(false);                       // HER ZAMAN kapali basla

  EEPROM.begin(EEPROM_SIZE);
  loadFailSafe();

  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LED_ACTIVE_LOW ? HIGH : LOW);

#if FLOW_SENSOR_ENABLED
  flowSensor.begin();
  flowSensor.setResolution(12);
  flowSensor.setWaitForConversion(false);
  Serial.println(F("DS18B20 su sensoru etkin."));
#endif

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);
  WiFi.hostname("kombi-role");

  connectWiFi();

  Serial.println(F("Hazir. Oda unitesi bekleniyor..."));
}

void loop() {
  updateWiFi();
  handleClient();
  updateRelay();
#if FLOW_SENSOR_ENABLED
  updateFlowSensor();
#endif
  updateLed();

  if (client.connected() && (millis() - lastStatusMs >= STATUS_PERIOD_MS)) {
    lastStatusMs = millis();
    sendStatus();
  }

  delay(2);
}
