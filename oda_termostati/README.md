# Oda Termostatı — NodeMCU v3

Kombi ısıtmasını oda sıcaklığına göre yöneten, 0.96" OLED ekranlı ve mobil uyumlu
web arayüzüne sahip oda ünitesi.

> Sürüm **1.0.0** · Diğer diller ve proje genel bakışı: [`README.md`](../README.md)
> (English, Türkçe, Русский, Español, Português, 日本語, 한국어, 中文)

> Bu klasördeki `oda_termostati.ino` + `web_ui.h` birlikte derlenir.
> Arduino IDE'de klasör adı ile sketch adı aynı olmalıdır: `oda_termostati`.

---

## 1. Özellikler

- **Sensör seçimi:** DHT11, DHT22 veya DS18B20 (tek satır `#define` ile)
- **0.96" SSD1306 OLED** (128×64, I²C): sıcaklık, hedef, durum, ağ ve bilgi sayfaları
- **Rotary encoder (KY‑040):** döndürerek hedef sıcaklık, kısa basış sayfa değiştirme,
  uzun basış mod aç/kapat
- **AP + STA birlikte:** her zaman `TERMOSTAT-AP` yayını; istenirse ev ağına da bağlanır
- **Mobil uyumlu web arayüzü:** hedef sıcaklık, mod, boost, uyku modu, röle
  davranışı, MQTT/Telegram, sıcaklık geçmişi grafiği, histerezis, kalibrasyon,
  don koruması, Wi‑Fi ayarları, sistem bilgisi
- **Çoklu dil (8):** en, tr, ru, es, pt, ja, ko, zh — ilk açılışta OLED ve web
  arayüzünde dil seçimi; seçim `/config.txt` içinde `lang=` olarak saklanır
- **Uyku modu:** belirlenen saat aralığında farklı hedef sıcaklık (NTP saati)
- **MQTT:** durum yayını + `anahtar=değer` komutları
- **Telegram botu:** durum ve uzaktan kontrol (yalnızca tanımlı sohbet)
- **LittleFS:** ayarlar `/config.txt`, saatlik geçmiş `/history.csv`;
  `/fs` adresinde dosya yöneticisi (LittleFSExplorer)
- **OLED temaları:** 10 yerleşik tema, ilk açılışta `/themes/*.txt` olarak
  LittleFS'e yaratılır; encoder'ın tema sayfasından, `/api/theme` veya
  `theme=<id>` komutu ile seçilir
- **Histerezis + minimum açık/kapalı süre** ile kombinin kısa döngü yapması engellenir
- **Boost:** hedefi geçici olarak +1.5 °C yükseltir (5/15/30 dk)
- **Don koruması:** mod kapalıyken bile 5 °C altında ısıtma talebi üretir
- **Fail‑safe:** sensör hatası veya kombi bağlantısı kopması durumunda talep kesilir
- **Ayarlar kalıcıdır** (LittleFS; eski EEPROM yapısı ilk açılışta taşınır)
- **OTA** (yalnızca STA bağlantısı varken), mDNS: `http://termostat.local`
- Wi‑Fi bilgileri web arayüzünden girilir; yanlış bilgi girilse bile AP açık kalır

---

## 2. Malzeme Listesi

| Adet | Malzeme |
|------|---------|
| 1 | NodeMCU v3 / ESP8266 (Lolin) |
| 1 | 0.96" SSD1306 OLED, I²C, 128×64 |
| 1 | KY‑040 rotary encoder (veya 3 butonlu eşdeğer) |
| 1 | DHT11 **veya** DHT22 **veya** DS18B20 (+ 4.7 kΩ direnç) |
| 1 | 5 V USB adaptör + micro‑USB kablo |
| — | Jumper kablo, isteğe bağlı 100 nF dekuplaj |

---

## 3. Pin Bağlantıları

| NodeMCU | GPIO | Bağlantı | Not |
|---------|------|----------|-----|
| D1 | GPIO5 | OLED **SCL** | I²C saat |
| D2 | GPIO4 | OLED **SDA** | I²C veri |
| D4 | GPIO2 | Sensör **DATA** | DHT data veya DS18B20 data |
| 3V3 | — | OLED VCC, encoder +, sensör VCC | 3.3 V |
| GND | — | Ortak GND | Tüm modüller |
| D5 | GPIO14 | Encoder **CLK** | Interrupt |
| D6 | GPIO12 | Encoder **DT** | Interrupt |
| D7 | GPIO13 | Encoder **SW** | Buton, dahili pull‑up |
| VIN/USB | — | 5 V besleme | Kart üzerinden |

**DS18B20** kullanıyorsanız DATA–3V3 arasına **4.7 kΩ** pull‑up direnç takın.
DHT11/DHT22 modülleri genelde pull‑up dahil gelir; çıplak sensörde 10 kΩ gerekir.

> Modül üzerindeki pin sırası üreticiye göre değişebilir (özellikle OLED ve
> encoder). Bağlantıyı yaparken şemadaki **etiketleri** esas alın, sırayı değil.

### Bağlantı şeması

![Oda termostatı bağlantı şeması](../docs/oda_termostati_sema.svg)

```
             +----------------------+
   OLED      |                      |      DHT22 / DS18B20
  GND  ------| GND                  |           VCC ---- 3V3
  VCC  ------| 3V3                  |           DATA --- D4
  SCL  ------| D1  (GPIO5)          |           GND ---- GND
  SDA  ------| D2  (GPIO4)          |
             |      NodeMCU v3      |
  Encoder    |                      |
  GND  ------| GND                  |
  +    ------| 3V3                  |
  SW   ------| D7  (GPIO13)         |
  DT   ------| D6  (GPIO12)         |
  CLK  ------| D5  (GPIO14)         |
             +----------+-----------+
                        |
                    micro-USB 5V
```

> OLED modüllerinin çoğu **3.3 V** ile çalışır. 5 V beslemeyin.
> Encoder'ın `+` ucunu 3V3'e bağlayın; bazı KY‑040 kartlarında pull‑up zaten vardır,
> yazılımda ayrıca `INPUT_PULLUP` etkindir.

---

## 4. Sensör Seçimi

`oda_termostati.ino` başındaki satırı değiştirin:

```cpp
#define SENSOR_TYPE     SENSOR_DHT22      // <<<<< BURAYI DEGISTIRIN
```

| Değer | Sensör | Nem gösterimi |
|-------|--------|----------------|
| `SENSOR_DHT11` | DHT11 | Var |
| `SENSOR_DHT22` | DHT22 / AM2302 | Var |
| `SENSOR_DS18B20` | DS18B20 (1‑Wire) | Yok (`--`) |

DS18B20 seçildiğinde `D4` pinine 1‑Wire verisi bağlanır; DHT kütüphanesi
kullanılmaz ama derlemede sorun çıkarmaz.

---

## 5. Gerekli Kütüphaneler (Arduino IDE → Library Manager)

| Kütüphane | Yazar |
|-----------|-------|
| Adafruit SSD1306 | Adafruit |
| Adafruit GFX Library | Adafruit |
| DHT sensor library | Adafruit |
| Adafruit Unified Sensor | Adafruit (DHT bağımlılığı) |
| OneWire | Paul Stoffregen |
| DallasTemperature | Miles Burton |
| PubSubClient | Nick O'Leary (MQTT) |

`ESP8266WiFi`, `ESP8266WebServer`, `ESP8266mDNS`, `ArduinoOTA`, `Wire`,
`EEPROM`, `LittleFS`, `WiFiClientSecure`, `ESP8266HTTPClient`, `time` ESP8266
kart paketiyle birlikte gelir.

**LittleFSExplorer** (MIT, Cymeria) kütüphanesi projeye vendor edilmiştir:
`oda_termostati/src/LittleFSExplorer/`. Arduino IDE `src` alt klasörünü
otomatik derler; ayrıca Library Manager'dan kurmak gerekmez.

### Arduino IDE 2 Ayarları

- **Board:** `NodeMCU 1.0 (ESP-12E Module)`
- **Upload Speed:** 115200
- **Flash Size:** 4MB (FS:2MB) — varsayılan yeterli
- **CPU Frequency:** 80 MHz
- **Port:** cihazın COM portu

> İlk yüklemede encoder/OLED takılı olması sorun çıkarmaz; ancak D3/D8 pinlerine
> harici bir şey bağlamayın (boot strapping).

---

## 6. İlk Çalıştırma ve Wi‑Fi

1. Kodu yükleyin, seri portu **115200** ile açın.
2. Telefon/PC ile `TERMOSTAT-AP` ağına bağlanın.
   - Şifre: `termostat123`
3. Tarayıcıda **http://192.168.4.1** adresini açın.
4. OLED'de sıcaklık görünmeye başlar (sensör okuma ~3 sn).
5. Ev ağına bağlamak için **Wi‑Fi Ayarları** bölümünü kullanın.
   Kaydettiğinizde cihaz yeniden başlar; STA bağlanırsa aynı arayüze
   `http://termostat.local` veya ev ağı IP'si ile de erişebilirsiniz.

**Önemli:** Kombi röle ünitesi oda ünitesinin AP'sine bağlanır. AP kapatılamaz.
AP SSID/şifresi `kombi_rolesi.ino` ile **birebir aynı** olmalıdır.

AP + STA aynı anda çalışırken ESP8266 tek kanal kullanır; STA bağlantısı
kanal değiştirirse röle ünitesi birkaç saniye içinde yeniden bağlanır.
Bu süre fail‑safe süresinden (5 dk) kısadır.

---

## 7. Kullanım

### Encoder

| Hareket | İşlev |
|---------|-------|
| Sağa / sola döndürme | Hedef sıcaklığı 0.5 °C adımlarla değiştirir |
| Kısa basış | OLED sayfası: Ana → Ağ → Bilgi → Uyku → Tema |
| Uzun basış (≥1.2 sn) | Mod: Açık ↔ Kapalı (don koruması) |
| Tema sayfasında döndürme | Aktif OLED temasını değiştirir |

Yön ters geliyorsa koddaki `ENCODER_REVERSE` değerini `1` yapın.

### Web arayüzü

- **Hedef sıcaklık:** +/− butonları veya kaydırıcı
- **Mod:** Açık / Kapalı
- **Boost:** 5/15/30 dk; kalan süre gösterilir
- **Ayarlar:** histerezis, kalibrasyon, don koruması
- **Uyku modu:** aç/kapat, başlangıç/bitiş saati, uyku hedefi
- **Röle davranışı:** bağlantı kopunca kapan / son talebi koru
- **Sıcaklık geçmişi:** 24 saat / 7 gün / 30 gün grafiği, örnek kaydet, temizle,
  `/fs` LittleFS dosya yöneticisi bağlantısı
- **MQTT & Telegram:** sunucu, port, kullanıcı/şifre, konu öneki, bot token,
  chat ID; durum göstergeleri
- **Wi‑Fi:** ev ağı SSID/şifre; boş SSID = yalnızca AP
- **Sistem:** IP, sinyal, boş bellek, sürüm, saat, geçmiş, LittleFS doluluk,
  yeniden başlatma, **dil seçimi** (`lang=en|tr|ru|es|pt|ja|ko|zh`)

**Dil seçimi:** Yalnızca **yeni cihazda** (LittleFS'de `/config.txt` yokken) OLED dil
seçimi ekranı gelir: encoder ile diller arasında dolaşın, kısa basışla onaylayın. Aynı
anda web arayüzü de tam ekran bir dil seçici açar. Seçim `lang=` ve `langset=1`
olarak kalıcıdır; sonradan web arayüzündeki **Dil** kartından veya
`/api/set?lang=tr` ile değiştirilebilir.

Daha eski bir sürümden güncellenen cihazlarda ayar dosyası zaten bulunduğu için dil
sorulmaz: ayarlar korunur, arayüz İngilizce açılır ve web arayüzü siz Dil kartından
seçim yapana kadar tarayıcı dilini kullanır (`/api/status` içindeki `ask` alanı `0`
olur). Yeni cihazda seçim yapılmazsa soru sonraki açılışlarda tekrarlanır.

OLED'in gömülü fontu yalnızca ASCII bastığı için ekran metinleri **en/tr/es/pt**
olduğu gibi yerelleşir (Türkçe karakterler ASCII karşılıklarıyla: `ISITIYOR`,
`BAGLI`). Rusça, Japonca, Korece ve Çince seçildiğinde web arayüzü tam çevrilidir,
OLED ise İngilizce etiket kullanır — 128×64 ekranda CJK glifleri için yer yoktur.
Telegram komutları ve yanıtları seçilen dili izler (CJK dilleri dahil): komut adları o
dilde çalışır, İngilizce ve eski Türkçe adlar her zaman kabul edilir; `/yardim` listesi de
seçilen dilde gelir.

### OLED sayfaları

1. **Ana:** büyük oda sıcaklığı, hedef, durum, nem / kombi su sıcaklığı / boost;
   uyku aktifken üstte `UYKU` etiketi
2. **Ağ:** AP IP, STA IP, SSID, kombi bağlantısı, sinyal
3. **Bilgi:** histerezis, kalibrasyon, don koruması, boost, çalışma süresi, FW
4. **Uyku:** durum, saat aralığı, uyku/normal hedef, NTP saati, röle fail‑safe
5. **Tema:** aktif tema adı/kimliği, dosya kaynağı, üst çizgi ve durum stili;
   encoder ile tema seçimi

### OLED temaları

Ekran yerleşimi `src/OledTheme/` modülünden gelir. Tema, ekranın üst çizgisi,
hedef kolonu, sıcaklık boyutu, ayırıcı, durum şeridi ve alt bilgi satırını
belirleyen küçük bir veri yapısıdır. Hedef kolonu olan temalarda hedef
sıcaklık, üstünde `HEDEF` etiketi olmadan çerçeve içinde 2× boyutta gösterilir;
durum satırı (`ISITIYOR` / `BEKLIYOR`) her temada yatay ortalanır.

İlk açılışta (veya dosya silinmişse) `LittleFS` üzerinde şunlar oluşturulur:

```
/themes/klasik.txt    /themes/minimal.txt   /themes/ters.txt
/themes/kutu.txt      /themes/genis.txt     /themes/gece.txt
/themes/retro.txt     /themes/durum.txt     /themes/kompakt.txt
/themes/dev.txt       /themes/active.txt
```

| Tema | Karakter |
|------|----------|
| `klasik` | Özgün yerleşim: çizgi başlık, 2× sıcaklık, talep barı |
| `minimal` | Başlıksız, ortada 3× sıcaklık, sade durum metni |
| `ters` | Negatif ekran, dolu başlık şeridi, ters durum şeridi |
| `kutu` | Kutu başlık, çift ayırıcı, çerçeveli durum |
| `genis` | 3× sıcaklık, çift ayırıcı, etiketsiz |
| `gece` | Düşük kontrast (dim), çift çizgi, noktalı ayırıcı |
| `retro` | Çift çizgi, noktalı ayırıcı, çerçeveli durum |
| `durum` | Durum şeridi odaklı, hedef kolonu yok |
| `kompakt` | Dolu başlık şeridi, kısa etiketler (`HD`) |
| `dev` | Yalnızca dev sıcaklık + durum, sade |

Dosya biçimi `anahtar=değer` (web'de `/fs` üzerinden düzenlenebilir):

```ini
id=klasik
adi=Klasik
header=1        # 0 yok, 1 tek çizgi, 2 çift çizgi, 3 kutu, 4 dolu şerit
tempsize=2      # 2 veya 3 (yazı boyutu)
tempcenter=0    # 1 = ortala
setpointcol=1   # sağda çerçeveli büyük hedef (tempcenter=0 iken)
separator=1     # 0 yok, 1 tek, 2 çift, 3 noktalı
status=1        # 0 metin, 1 talep barı, 2 çerçeve, 3 ters şerit
nem=1           # alt satırda nem
boost=1         # alt satırda boost
su=1            # alt satırda kombi su sıcaklığı
sayfa=1         # sağ üstte sayfa sayacı
etiket=1        # HEDEF/Nem etiketleri (ortalanmış sıcaklık yerleşiminde)
ters=0          # tüm ekranı negatif yap
dim=0           # düşük kontrast
```

`/themes/active.txt` yalnızca son seçilen temanın kimliğini (`id=...`) tutar;
kalıcı seçim `/config.txt` içindeki `theme=` satırındadır. Tema dosyasını
elle düzenledikten sonra cihazı yeniden başlatın veya `/api/theme?reload=1`
çağırın.

---

## 8. Kontrol Algoritması

```
Hedef (efektif) = mod AÇIK  → uyku aktifse uyku hedefi, değilse setpoint
                              (+ boost sırasında 1.5 °C)
                  mod KAPALI → don koruması (varsayılan 5 °C)

Talep AÇIK : sıcaklık ≤ hedef − histerezis  VE  son kapanmadan ≥ 180 sn geçti
Talep KAPALI: sıcaklık ≥ hedef + histerezis  VE  son açmadan ≥ 60 sn geçti
```

- Varsayılan histerezis **0.3 °C**'dir (0.1–3.0 arası ayarlanabilir).
- Isıtmanın başlaması için hedefin, ölçülen sıcaklığın **en az histerezis kadar**
  üzerinde olması gerekir (örn. 21.4 °C'de 0.3 histerezisle hedef ≥ 21.7 °C).
- Röle kapanması minimum açık süre (60 sn), açılması minimum kapalı süre
  (180 sn) tamamlanmadan gerçekleşmez; test ederken bu gecikme normaldir.
- Sensör okunamazsa veya 30 sn güncellenmezse talep kesilir.
- Röle ünitesi 5 dk komut almazsa seçilen fail‑safe davranışını uygular.
- Mod kapalıyken bile sıcaklık don korumasının altına düşerse ısıtma açılır.
- Uyku aralığı gece yarısını aşabilir (örn. 23:00–07:00). NTP saati yoksa uyku
  modu devreye girmez.

---

## 9. Web API (isteğe bağlı entegrasyon)

| Metot | Yol | Örnek |
|-------|-----|-------|
| GET | `/api/status` | Durum JSON |
| GET | `/api/set?sp=22.0` | Hedef sıcaklık |
| GET | `/api/set?mode=0` | Mod kapat |
| GET | `/api/set?hyst=0.4&cal=-0.2&frost=6` | Ayarlar |
| GET | `/api/set?boost=15` | 15 dk boost (`0` = iptal) |
| GET | `/api/set?sleep=1&sleepstart=23:00&sleepend=07:00&sleepsp=18` | Uyku modu |
| GET | `/api/set?failsafe=1` | Röle fail‑safe: son talebi koru |
| GET | `/api/set?theme=gece` | OLED temasını seç (10 tema) |
| GET | `/api/theme` | Tema listesi + aktif tema (JSON) |
| GET | `/api/theme?set=retro` | Tema seç |
| GET | `/api/theme?reload=1` | Tema dosyalarını diskten yeniden oku |
| POST | `/api/net` | MQTT/Telegram ayarları |
| GET | `/api/history?hours=24` | Geçmiş noktaları (JSON) |
| POST | `/api/history/sample` | Anlık örnek kaydet |
| POST | `/api/history/clear` | Geçmişi sil |
| GET | `/fs` | LittleFSExplorer dosya yöneticisi |
| GET | `/api/scan` | Çevredeki Wi‑Fi ağları (JSON: `nets[]`) |
| POST | `/api/wifi` | `ssid`, `pass` form alanları |
| POST | `/api/reboot` | Yeniden başlat |

`/api/status` alanları: `fw, theme, temp, hum, sp, eff, hyst, cal, frost, mode, demand,
relay, link, flow, brssi, boost, boostMin, sensorOk, sleepEn, sleep, sleepStart,
sleepEnd, sleepSp, failsafe, mqtt, mqttCfg, mqttPort, mqttPrefix, tg, tgOk,
tgErr, tgChat, timeOk, now, hist, fsUsed, fsTotal, uptime, ip, apip, ssid, sta,
rssi, heap, client`.

`/api/net` alanları: `mqtthost, mqttport, mqttuser, mqttpass, mqttprefix,
tgtoken, tgchat`; `mqttclear=1` / `tgclear=1` ilgili ayarları siler. Boş
`mqttpass` veya `tgtoken` mevcut değeri korur.

**MQTT konuları:** `kombi/status` (retained JSON), `kombi/set` (komut),
`kombi/result` (`ok`/`hata`). **Telegram:** `/durum`, `/sicaklik 22.5`,
`/mod ac|kapat`, `/boost 30`, `/uyku ac|kapat`, `/uykusaat 23:00 07:00`,
`/uykusicaklik 18`, `/yardim`.

---

## 10. Sorun Giderme

| Belirti | Çözüm |
|---------|-------|
| OLED boş | Adres 0x3C mi? SDA/SCL ters mi? 3.3 V besleme? |
| Sensör `--` | GPIO D4, pull‑up, sensör tipi `SENSOR_TYPE` doğru mu? |
| Encoder atlıyor | KY‑040 beslemesi 3V3, kablolar kısa; `ENCODER_REVERSE` deneyin |
| Web açılmıyor | AP'ye bağlı mısınız? `192.168.4.1`; seri logda IP'yi kontrol edin |
| Ev ağına bağlanmıyor | 2.4 GHz ağ gerekir; SSID/şifre; AP her zaman açık kalır |
| Kombi bağlı değil | Röle ünitesi açık mı, AP şifresi aynı mı? Röle seri loguna bakın |
| Sıcaklık sapmalı | Kalibrasyon ayarı ile düzeltin; sensörü radyatör yakınına koymayın |
| Rastgele reset | Yetersiz 5 V adaptör; OLED + encoder akımı; kaliteli kablo |
| LittleFS yazılamıyor | İlk açılışta otomatik formatlanır; `/fs` ile doluluk kontrol edin |
| MQTT bağlanmıyor | 2.4 GHz ağ, doğru port/kullanıcı; `mqttCfg` dolu mu? |
| Telegram yanıt yok | Token/chat ID doğru mu? `tgErr` alanına bakın; RAM < 24 KB'de poll atlanır |
| Grafik boş | Saat (NTP) alınmadan kayıt yapılmaz; `/api/history/sample` deneyin |

---

## 11. Teknik Notlar

- I²C adresi `0x3C`; farklıysa `display.begin(..., 0x3D)` yapın.
- DHT okuma periyodu 2.5 sn'dir (DHT11 için de güvenli).
- Sıcaklık filtresi: son 5 geçerli okumanın ortalaması.
- Ayarlar 1.5 sn içinde LittleFS'ye yazılır (flash ömrü korunur).
- Geçmiş dosyası saatlik büyür; 720 satırı aşınca en eski kayıtlar kırpılır.
- Telegram/TLS `setInsecure()` kullanır (sertifika doğrulaması yoktur); güvenlik
  için bot token'ını gizli tutun ve yalnızca güvenli ağda kullanın.
- IRAM kullanımı ~%95'tir. Yer açmak için `TELEGRAM_ENABLED 0` yapılabilir.
- OTA portu **3232**, şifre `termo1234` (kodu değiştirin).
- mDNS yalnızca STA bağlantısında router tarafından çözümlenir; AP modunda IP kullanın.
- Üretimde AP şifresini ve OTA şifresini mutlaka değiştirin.
