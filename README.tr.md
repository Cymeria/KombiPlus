<div align="center">
  <img src="docs/images/banner.svg" alt="Kombi Termostat v1.0.0 — iki cihazlı ESP8266 oda termostatı" width="100%">
</div>

<h1 align="center">Kombi Termostat</h1>

<p align="center">
  <strong>Doğalgaz kombileri için iki cihazlı, Wi‑Fi tabanlı oda termostatı.</strong><br>
  NodeMCU v3 (ESP8266) · OLED · rotary encoder · gömülü web arayüzü · MQTT · Telegram · fail‑safe röle
</p>

<p align="center">
  <img alt="sürüm" src="https://img.shields.io/badge/s%C3%BCr%C3%BCm-1.0.0-ff8a3d?style=flat-square">
  <img alt="platform" src="https://img.shields.io/badge/platform-NodeMCU%20v3%20(ESP8266)-2b3d61?style=flat-square">
  <img alt="çekirdek" src="https://img.shields.io/badge/Arduino%20core-3.1.2-00979D?style=flat-square&logo=arduino&logoColor=white">
  <img alt="bulut" src="https://img.shields.io/badge/bulut-gerekmez-2dd47c?style=flat-square">
  <img alt="lisans" src="https://img.shields.io/badge/lisans-MIT-49b8ff?style=flat-square">
</p>

<p align="center">
  <strong>Diller:</strong>
  <a href="README.md">English</a> ·
  <a href="README.tr.md">Türkçe</a> ·
  <a href="README.ru.md">Русский</a> ·
  <a href="README.es.md">Español</a> ·
  <a href="README.pt.md">Português</a> ·
  <a href="README.ja.md">日本語</a> ·
  <a href="README.ko.md">한국어</a> ·
  <a href="README.zh.md">中文</a>
</p>

---

**İçindekiler:** [Nasıl çalışır](#nasıl-çalışır) · [Ekranlar](#ekranlar) · [Özellikler](#özellikler) · [Donanım](#donanım) · [Klasör yapısı](#klasör-yapısı) · [Hızlı başlangıç](#hızlı-başlangıç) · [Kontrol mantığı](#kontrol-mantığı) · [Fail‑safe](#fail-safe-röle) · [Protokol](#protokol) · [Web API](#web-api) · [MQTT ve Telegram](#mqtt-ve-telegram) · [Depolama ve temalar](#depolama-ve-oled-temaları) · [Güvenlik](#güvenlik) · [Sürüm](#sürüm)

---

## Nasıl çalışır?

```
┌─────────────────────────┐        Wi-Fi AP (TERMOSTAT-AP)        ┌──────────────────────┐
│  ODA ÜNİTESİ            │◄─────────────────────────────────────►│  KOMBİ RÖLE ÜNİTESİ  │
│  NodeMCU v3             │   TCP 8266: SET heat=0/1 sp=...      │  NodeMCU v3 + röle   │
│  DHT11/22 veya DS18B20  │◄──────────────────────────────────────│  ops. DS18B20        │
│  OLED + encoder + web   │        STATUS relay=... flow=...     │  COM/NO → kombi      │
└───────────┬─────────────┘                                      └──────────┬───────────┘
            │                                                               │
   Telefon / PC (http://192.168.4.1)                            Kombi termostat klemensi
   İsteğe bağlı ev Wi-Fi (STA)                                  (kuru kontak, düşük gerilim)
```

- Oda ünitesi **her zaman** kendi Wi‑Fi ağını (AP) yayınlar. Telefonla bu ağa bağlanıp
  `192.168.4.1` adresinden kontrol edilir; uygulama, bulut veya hesap gerekmez.
- İstenirse oda ünitesi ev ağına da (STA) bağlanır. AP kapanmaz, çünkü röle ünitesi ona bağlıdır.
- Oda ünitesi sıcaklık + hedef + histerezise göre ısı talebini hesaplar ve her 2 saniyede
  röle ünitesine gönderir.
- Röle ünitesi talebi uygular; **5 dakika** komut gelmezse seçilen fail‑safe davranışına döner.
- Opsiyonel su geçirmez DS18B20, kombi su sıcaklığını oda ünitesine bildirir.
- **Uyku modu**, **don koruması**, **boost**, **10 OLED teması**, **saatlik geçmiş**,
  **LittleFS dosya yöneticisi** ve **OTA** yerleşik olarak gelir.

<p align="center">
  <img src="docs/sistem_mimari.svg" alt="Sistem mimarisi" width="90%">
</p>

---

## Ekranlar

<table>
<tr>
<td width="50%"><img src="docs/images/demo-oled.png" alt="OLED sayfaları ve temalar"></td>
<td width="50%"><img src="docs/images/demo-web.png" alt="Mobil web arayüzü"></td>
</tr>
<tr>
<td align="center"><sub>On temada beş OLED sayfası — firmware ile aynı çizim kodundan</sub></td>
<td align="center"><sub>Gerçek gömülü web arayüzü, <code>web_ui.h</code> içinden</sub></td>
</tr>
</table>

İki görüntü de donanımsız tarayıcı demosundan alınmıştır: **[`demo/demo.html`](demo/demo.html)**
dosyasına çift tıklamak yeterlidir. Demo, tarayıcı içinde sahte bir API çalıştırır.

---

## Özellikler

### Oda ünitesi (`oda_termostati/`)

- **Sensör seçimi:** DHT11, DHT22 veya DS18B20 (tek `#define`)
- **0.96" SSD1306 OLED** (128×64, I²C): sıcaklık, hedef, durum, ağ ve bilgi sayfaları
- **KY‑040 rotary encoder:** döndürerek hedef, kısa basış sayfa, uzun basış mod
- **10 OLED teması** — LittleFS üzerinde düzenlenebilir `/themes/*.txt` dosyaları
- **Mobil öncelikli web arayüzü:** hedef, mod, boost, uyku, röle davranışı, MQTT/Telegram,
  geçmiş grafiği, histerezis, kalibrasyon, don koruması, **Wi‑Fi ağ tarayıcı**, sistem bilgisi
- **8 arayüz dili** — İngilizce, Türkçe, Rusça, İspanyolca, Portekizce, Japonca, Korece ve Çince.
  İlk açılışta oda ünitesi dil sorar (OLED: çevir–seç, bas–onayla; web: tam ekran seçim) ve `lang=`
  olarak saklar; sonradan **Dil** kartından değiştirilir. OLED etiketleri gömülü font ASCII
  olduğu için en/tr/es/pt kapsar, diğer dillerde ekran İngilizceye düşer.
- **Uyku modu:** saat aralığında farklı hedef (NTP saati)
- **MQTT:** retained durum JSON'u + `anahtar=değer` komutları
- **Telegram botu:** durum ve kontrol, yalnızca tanımlı sohbet
- **LittleFS:** ayarlar `/config.txt`, saatlik örnekler `/history.csv`, `/fs` dosya yöneticisi
- **Histerezis + minimum açık/kapalı süre** ile kısa döngü engellenir
- **Boost:** hedefi 5/15/30 dk boyunca +1.5 °C yükseltir
- **Don koruması:** mod kapalıyken bile 5 °C altında ısıtır
- **Fail‑safe:** sensör hatasında veya röle bağlantısı koptuğunda talep kesilir
- **OTA** (yalnızca STA) ve mDNS: `http://termostat.local`

### Röle ünitesi (`kombi_rolesi/`)

- Oda ünitesinin AP'sine otomatik bağlanır
- Kombinin **kuru kontağını** opto‑izolasyonlu röle modülüyle sürer
- **Aktif‑low / aktif‑high** tek satırla seçilir
- **Fail‑safe:** 5 dakika komut gelmezse röle kapanır (varsayılan) veya son talep korunur
- Fail‑safe tercihi oda ünitesinden (`fs=0/1`) gelir ve EEPROM'da saklanır
- Açılışta röle **her zaman kapalı** başlar
- Opsiyonel DS18B20 ile kombi su sıcaklığı
- Kart üstü LED bağlantı göstergesi ve 115200 ayrıntılı seri log

---

## Donanım

| Adet | Malzeme |
|-----:|---------|
| 2 | NodeMCU v3 / ESP8266 (Lolin) |
| 1 | 0.96" SSD1306 OLED, I²C, 128×64 |
| 1 | KY‑040 rotary encoder |
| 1 | DHT11 / DHT22 / DS18B20 (+ DS18B20 için 4.7 kΩ) |
| 1 | Opto‑izolasyonlu 5 V röle modülü (1 kanal) |
| 1 | Opsiyonel su geçirmez DS18B20 (kombi suyu) |
| 2 | 5 V USB adaptör, jumper, klemens |

<table>
<tr>
<td width="50%"><img src="docs/oda_termostati_sema.svg" alt="Oda ünitesi bağlantı şeması"></td>
<td width="50%"><img src="docs/kombi_rolesi_sema.svg" alt="Röle ünitesi bağlantı şeması"></td>
</tr>
<tr>
<td align="center"><sub>Oda ünitesi — OLED, encoder, sensör</sub></td>
<td align="center"><sub>Röle ünitesi — kombi kuru kontak</sub></td>
</tr>
</table>

| Oda ünitesi | GPIO | Bağlantı |
|-------------|------|----------|
| D1 | GPIO5 | OLED SCL |
| D2 | GPIO4 | OLED SDA |
| D4 | GPIO2 | Sensör DATA (DHT veya DS18B20) |
| D5 | GPIO14 | Encoder CLK |
| D6 | GPIO12 | Encoder DT |
| D7 | GPIO13 | Encoder SW (buton) |

| Röle ünitesi | GPIO | Bağlantı |
|--------------|------|----------|
| D1 | GPIO5 | Röle IN |
| VIN | — | Röle VCC (5 V) |
| GND | — | Röle GND |
| D2 | GPIO4 | Opsiyonel DS18B20 DATA |

Ayrıntılı pin tabloları, notlar ve sorun giderme:
[`oda_termostati/README.md`](oda_termostati/README.md) ve [`kombi_rolesi/README.md`](kombi_rolesi/README.md).

---

## Klasör yapısı

```
kombi/
├── README.md                  ← İngilizce ana dosya + 7 çeviri
├── CHANGELOG.md               ← sürüm geçmişi
├── LICENSE                    ← MIT
├── oda_termostati/
│   ├── oda_termostati.ino     ← oda ünitesi yazılımı
│   ├── web_ui.h               ← gömülü HTML/CSS/JS
│   ├── src/LittleFSExplorer/  ← vendor dosya yöneticisi (MIT)
│   ├── src/OledTheme/         ← OLED tema motoru (10 tema)
│   └── README.md
├── kombi_rolesi/
│   ├── kombi_rolesi.ino       ← röle ünitesi yazılımı
│   └── README.md
├── demo/
│   ├── demo.html              ← donanımsız arayüz demosu
│   ├── demo.src.html          ← demo şablonu
│   ├── web_ui_demo.html       ← üretilen tek dosya web arayüzü
│   └── build_demo.ps1         ← demoyu web_ui.h'den üretir
└── docs/
    ├── images/                ← banner, ekran görüntüleri
    └── *.svg                  ← bağlantı ve mimari şemaları
```

---

## Hızlı başlangıç

### 1. Arduino ortamı

1. **Arduino IDE 2.x** → *Preferences* → *Additional boards manager URLs*:

   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```

2. *Boards Manager* → **esp8266** `3.1.2` kurun.
3. *Library Manager* → kurun:

   `Adafruit SSD1306`, `Adafruit GFX Library`, `DHT sensor library`,
   `Adafruit Unified Sensor`, `OneWire`, `DallasTemperature`, `PubSubClient`.

   `LittleFSExplorer` ve OLED tema motoru `oda_termostati/src/` altında **vendor**
   edilmiştir; ayrıca kurulum gerekmez.

4. Kart: **NodeMCU 1.0 (ESP‑12E Module)**, upload **115200**, flash 4 MB.

### 2. Yapılandırma

**İki dosyada da aynı AP bilgilerini kullanın:**

```cpp
// oda_termostati.ino
#define AP_SSID  "TERMOSTAT-AP"
#define AP_PASS  "termostat123"

// kombi_rolesi.ino
#define TERMOSTAT_AP_SSID  "TERMOSTAT-AP"
#define TERMOSTAT_AP_PASS  "termostat123"
```

Oda ünitesinde sensörü seçin:

```cpp
#define SENSOR_TYPE  SENSOR_DHT22   // SENSOR_DHT11 / SENSOR_DHT22 / SENSOR_DS18B20
```

Aktif‑high röle kartı:

```cpp
#define RELAY_ACTIVE_LOW  0
```

### 3. Yükleme ve test

1. Önce **röle ünitesini** yükleyin (kontağı kombiye bağlamayın).
2. Sonra **oda ünitesini** yükleyin.
3. `TERMOSTAT-AP` ağına bağlanın (şifre `termostat123`), `http://192.168.4.1` adresini açın.
4. Röle seri logunda `Oda unitesine baglandi.` yazmalı; arayüzde **Kombi bağlı** görünür.
5. Testten sonra röle kontağını kombinin termostat klemensine **yetkili servise** bağlatın.

---

## Kontrol mantığı

```
Efektif hedef = mod AÇIK  → uyku aktifse uyku hedefi, değilse hedef
                            (+ boost sırasında 1.5 °C)
                mod KAPALI → don koruması (varsayılan 5 °C)

Talep AÇIK : sıcaklık ≤ hedef − histerezis  VE  son kapanmadan ≥ 180 sn geçti
Talep KAPALI: sıcaklık ≥ hedef + histerezis  VE  son açmadan ≥ 60 sn geçti
```

| Parametre | Varsayılan | Aralık |
|-----------|-----------|--------|
| Hedef sıcaklık | 21.0 °C | 5–30 °C |
| Histerezis | 0.3 °C | 0.1–3.0 °C |
| Minimum açık süre | 60 sn | sabit |
| Minimum kapalı süre | 180 sn | sabit |
| Don koruması | 5.0 °C | 2–10 °C |
| Boost | +1.5 °C | 5/15/30 dk |
| Uyku modu | kapalı | 23:00–07:00, 18.0 °C |
| Röle fail‑safe | kapanır | kapanır / son talebi korur |
| Saatlik geçmiş | 720 kayıt | ~30 gün |

30 saniyeden eski sensör verisi talebi keser. Mod kapalıyken bile don koruması altında ısıtma açılır.

---

## Fail‑safe röle

Röle yalnızca `heat=1` **ve** son komut 5 dakikadan yeni olduğunda açılır.

- `fs=0` **(varsayılan)** — röle kapanır, kombi durur. Güvenli seçim.
- `fs=1` — son talep korunur. Donma riskini azaltır fakat aşırı ısınma riski taşır; yalnızca
  kombinin kendi güvenlik termostatı ve limit koruması varsa kullanın.
- Röle her zaman kontak açık başlar; enerji kesilip gelirse ısıtma kendiliğinden başlamaz.

---

## Protokol

Oda ünitesi **TCP 8266** portunu dinler; röle ünitesi istemcidir. Satır tabanlı ASCII:

| Yön | Satır |
|-----|-------|
| Oda → Röle | `SET heat=1 sp=21.0 hyst=0.3 mode=1 fs=0\n` |
| Röle → Oda | `STATUS relay=1 flow=42.50 rssi=-58 up=123 fs=0 fw=1.0.0\n` |

İki yön de 115200 baud seri loglarda izlenebilir.

---

## Web API

Gömülü web sunucusu 80 portunda JSON sunar — Home Assistant, betikler veya curl için uygundur.

| Metot | Yol | Örnek |
|-------|-----|-------|
| GET | `/api/status` | Tam durum JSON'u |
| GET | `/api/set?sp=22.0` | Hedef sıcaklık |
| GET | `/api/set?mode=0` | Mod kapat |
| GET | `/api/set?hyst=0.4&cal=-0.2&frost=6` | Ayarlar |
| GET | `/api/set?boost=15` | 15 dk boost (`0` iptal) |
| GET | `/api/set?sleep=1&sleepstart=23:00&sleepend=07:00&sleepsp=18` | Uyku modu |
| GET | `/api/set?failsafe=1` | Röle: son talebi koru |
| GET | `/api/set?lang=ja` | Arayüz dili: en/tr/ru/es/pt/ja/ko/zh |
| GET | `/api/set?theme=gece` | OLED teması seç |
| GET | `/api/theme` | Tema listesi + aktif tema |
| GET | `/api/theme?set=retro` | Tema seç |
| GET | `/api/theme?reload=1` | Tema dosyalarını yeniden oku |
| POST | `/api/net` | MQTT / Telegram ayarları |
| GET | `/api/history?hours=24` | Geçmiş noktaları |
| POST | `/api/history/sample` | Anlık örnek kaydet |
| POST | `/api/history/clear` | Geçmişi sil |
| GET | `/api/scan` | Çevredeki Wi‑Fi ağları (`nets[]`) |
| POST | `/api/wifi` | `ssid`, `pass` form alanları |
| POST | `/api/reboot` | Yeniden başlat |
| GET | `/fs` | LittleFS dosya yöneticisi |

`/api/status` alanları: `fw, theme, temp, hum, sp, eff, hyst, cal, frost, mode, demand, relay,
link, flow, brssi, boost, boostMin, sensorOk, sleepEn, sleep, sleepStart, sleepEnd, sleepSp,
failsafe, mqtt, mqttCfg, mqttPort, mqttPrefix, tg, tgOk, tgErr, tgChat, timeOk, now, hist,
fsUsed, fsTotal, uptime, ip, apip, ssid, sta, rssi, heap, client`.

---

## MQTT ve Telegram

**MQTT** (web → *MQTT & Telegram*):

- `kombi/status` — retained JSON (`temp`, `sp`, `eff`, `mode`, `demand`, `relay`, `link`, `sleep`, `failsafe`, `boost`, …)
- `kombi/set` — `sp=22.5`, `mode=1`, `sleepsp=18`, `sleepstart=23:00&sleepend=07:00`, `failsafe=1`
- `kombi/result` — `ok` / hata

**Telegram**: `/durum`, `/sicaklik 22.5`, `/mod ac|kapat`, `/boost 30`, `/uyku ac|kapat`,
`/uykusaat 23:00 07:00`, `/uykusicaklik 18`, `/yardim`. Yalnızca tanımlı sohbet yanıt alır.

---

## Depolama ve OLED temaları

- `/config.txt` — tüm ayarlar (`anahtar=değer`); eski EEPROM yapısı ilk açılışta taşınır
- `/history.csv` — saatlik kayıtlar: `zaman, oda, nem, hedef, talep, uyku`
- `/themes/*.txt` — tema tanımları (`id, header, tempsize, separator, status, nem, boost, su,
  sayfa, etiket, ters, dim`); dosya yoksa ilk açılışta yeniden yaratılır
- `/themes/active.txt` — son seçilen tema kimliği
- Web arayüzü 24 saat / 7 gün / 30 gün grafiği çizer; `/fs` ile dosya listeleme, indirme,
  yükleme, silme ve düzenleme yapılır

Yerleşik temalar: `klasik`, `minimal`, `ters`, `kutu`, `genis`, `gece`, `retro`, `durum`,
`kompakt`, `dev`.

---

## Güvenlik

- Kombi içinde **230 V** vardır; kapağını yalnızca yetkili servis açmalıdır.
- Röle, kombinin termostat girişine **gerilimsiz kuru kontak** olarak bağlanır.
- **Opto‑izolasyonlu** röle modülü, uygun sigorta ve kaliteli 5 V adaptör kullanın.
- Üretimde AP ve OTA şifrelerini değiştirin; ağ misafirlere açık olmasın.
- Bu proje hobi/iyileştirme amaçlıdır, **garanti kapsamında değildir**; sorumluluk kullanıcıdadır.
- Kombinin kendi güvenlik termostatı ve limit koruması her zaman devrede kalmalıdır.

---

## Sürüm

- **v1.0.0** — ilk kararlı sürüm ([CHANGELOG.md](CHANGELOG.md))
- Test edilen platform: ESP8266 Arduino core 3.1.2, Arduino IDE 2.x
- Oda ünitesi IRAM ~%95 doludur (gerekirse `TELEGRAM_ENABLED 0` ile yer açılır)
- Değişiklik yaparken iki cihazın `FW_VERSION` ve AP bilgilerini uyumlu tutun

---

## Lisans

[MIT](LICENSE) — kullanmak, değiştirmek ve paylaşmak serbesttir. `LittleFSExplorer` kendi MIT
lisansını korur.
