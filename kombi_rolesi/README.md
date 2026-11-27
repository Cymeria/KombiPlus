# Kombi Röle Ünitesi — NodeMCU v3

Oda termostatından gelen ısı talebine göre kombinin kuru kontağını süren,
bağlantı koptuğunda röleyi **otomatik olarak kapatan** güvenli röle ünitesi.

> Sürüm **1.3.0** · Diğer diller ve proje genel bakışı: [`README.md`](../README.md)
> (English, Türkçe, Русский, Español, Português, 日本語, 한국어, 中文)

> Bu klasördeki `kombi_rolesi.ino` tek dosyadır.
> Arduino IDE'de klasör adı ile sketch adı aynı olmalıdır: `kombi_rolesi`.

---

## ⚠️ GÜVENLİK UYARISI

- Kombi termostat girişleri genellikle **düşük gerilimli (kuru kontak)** olsa da
  cihaz içinde **230 V** bulunur. Kapağı açmayın, yetkisiz müdahale yapmayın.
- Röle kontağını yalnızca kombinin **termostat/oda termostatı** klemensine
  bağlayın; şebeke hattına değil.
- **Opto‑izolasyonlu**, kontakları uygun akımda olan kaliteli bir röle modülü
  kullanın. Gerekirse kombi üreticisinin önerdiği ara röle kullanılmalıdır.
- Montajı yetkili teknik servis yapmalıdır. Yanlış bağlantı cihaz ve can
  güvenliğini riske atar.
- Bu proje **hobi/iyileştirme** amaçlıdır; ürün garantisi kapsamında değildir.

---

## 1. Özellikler

- Oda ünitesinin `KombiPlus` ağına otomatik bağlanır (STA)
- TCP `8266` üzerinden `SET heat=0/1 ...` komutlarını alır
- Röle çıkışında **aktif‑low / aktif‑high** seçimi
- **Fail‑safe:** 5 dk boyunca komut gelmezse seçilen davranış uygulanır
  (röle kapanır **veya** son talep korunur)
- Fail‑safe tercihi oda ünitesinden (`fs=0/1`) gelir ve rölenin EEPROM'unda saklanır
- Açılışta röle **her zaman kapalı** başlar
- Opsiyonel **0.96" SSD1306 OLED** (SDA=D5, SCL=D6): sarı üst bantta başlık ve sürüm, gövdede
  röle durumu (2× yazı), oda bağlantısı, fail‑safe, su sıcaklığı, IP ve sinyal; ekran yoksa
  sessizce devre dışı kalır
- **D7 butonu ve ekran zaman aşımı:** butona basınca ekran 5 dakika daha açık kalır; 5 dakika
  boyunca basılmazsa panel kapanır, röle ve bağlantı çalışmaya devam eder
- Opsiyonel **DS18B20** ile kombi su sıcaklığını oda ünitesine bildirir
- Kart üzerindeki LED ile bağlantı durumu göstergesi
- Wi‑Fi koptuğunda 15 sn'de bir otomatik yeniden bağlanma
- Seri portta (115200) ayrıntılı log

---

## 2. Malzeme Listesi

| Adet | Malzeme |
|------|---------|
| 1 | NodeMCU v3 / ESP8266 (Lolin) |
| 1 | 1 kanal röle modülü (5 V, opto‑izolasyonlu, aktif‑low yaygın) |
| 1 | Opsiyonel: waterproof DS18B20 + 4.7 kΩ |
| 1 | Opsiyonel: 0.96" SSD1306 OLED (I²C, 128×64) |
| 1 | Buton (D7 → GND, ekran uyandırma) |
| 1 | 5 V USB adaptör + micro‑USB kablo |
| — | Klemens, sigorta (önerilir), uygun kesitli kablo |

---

## 3. Pin Bağlantıları

| NodeMCU | GPIO | Bağlantı | Not |
|---------|------|----------|-----|
| D1 | GPIO5 | Röle **IN** | Kontrol sinyali |
| VIN (5 V) | — | Röle **VCC** | USB beslemesinden |
| GND | — | Röle **GND** | Ortak GND |
| D2 | GPIO4 | DS18B20 **DATA** (opsiyonel) | 4.7 kΩ pull‑up 3V3'e |
| D5 | GPIO14 | OLED **SDA** (opsiyonel) | Oda ünitesiyle aynı pin |
| D6 | GPIO12 | OLED **SCL** (opsiyonel) | Oda ünitesiyle aynı pin |
| D7 | GPIO13 | Buton (diğer bacak GND) | Ekranı uyandırır, dahili pull‑up |
| 3V3 | — | DS18B20 **VCC** | 3.3 V |
| GND | — | DS18B20 **GND** | |
| — | — | Kart üzeri mavi LED | Durum göstergesi |

> Röle modülünü **3.3 V** ile beslemeyin; çoğu modül 5 V ister. `VIN`/`5V` ve
> `GND` kullanın. IN girişi 3.3 V ile sürülebilir (yaygın modüller destekler).
> Röle aktif‑low ise kodda `RELAY_ACTIVE_LOW 1`, aktif‑high ise `0` yapın.
> Modül üzerindeki pin sırası üreticiye göre değişebilir; şemadaki etiketleri
> esas alın.

### Bağlantı şeması

![Kombi röle ünitesi bağlantı şeması](../docs/kombi_rolesi_sema.svg)

```
        +----------------------+          +-------------------+
        |                      |   D1     |  Röle Modülü      |
        |      NodeMCU v3      |--------->| IN                |
        |                      |   VIN    | VCC               |
        |                      |--------->|                   |
        |                      |   GND    | GND               |
        |                      |--------->|                   |
        |                      |          | COM ----+        |
        |                      |          | NO  ----+|        |
        +----------+-----------+          +---------|--------+
                   |                              |
               micro-USB 5V                 Kombi termostat
                                            klemensi (kuru kontak)
```

> **Kombi tarafı:** Rölenin `COM` ve `NO` uçları, kombinin oda termostatı
> girişine **seri** bağlanır. Kontak kapandığında kombi ısıtma talebi alır.
> Kombinin kendi termostat/limit korumaları devrede kalmalıdır.

---

## 4. Yazılım Kurulumu

### Kütüphaneler

| Kütüphane | Yazar |
|-----------|-------|
| OneWire | Paul Stoffregen |
| DallasTemperature | Miles Burton |
| Adafruit SSD1306 | Adafruit |
| Adafruit GFX Library | Adafruit (SSD1306 bağımlılığı) |

`ESP8266WiFi` kart paketiyle gelir.

### Arduino IDE 2 Ayarları

- **Board:** `NodeMCU 1.0 (ESP-12E Module)`
- **Upload Speed:** 115200
- **Flash Size:** 4MB
- **Port:** cihazın COM portu

### Yapılandırma (`kombi_rolesi.ino` başı)

```cpp
#define RELAY_PIN          D1
#define RELAY_ACTIVE_LOW   1      // aktif-low kart: 1, aktif-high: 0

#define FLOW_SENSOR_ENABLED 1     // DS18B20 yoksa 0 yapın
#define FLOW_PIN            D2

#define OLED_ENABLED        1     // OLED yoksa 0 yapın
#define PIN_OLED_SDA        D5    // oda ünitesiyle aynı
#define PIN_OLED_SCL        D6
#define BUTTON_PIN          D7    // ekran uyandırma butonu (GND'ye)
#define OLED_TIMEOUT_MS     300000UL  // 5 dk

#define TERMOSTAT_AP_SSID  "KombiPlus"
#define TERMOSTAT_AP_PASS  "Kombi.1881"

#define SERVER_HOST        "192.168.4.1"
#define TCP_PORT           8266
```

- **SSID/şifre** oda ünitesiyle birebir aynı olmalıdır. İki dosyada da değiştirin.
- Oda ünitesi AP modunda her zaman `192.168.4.1` adresindedir; `SERVER_HOST`
  genellikle değiştirilmez.
- DS18B20 kullanmıyorsanız `FLOW_SENSOR_ENABLED 0` yapın.
- OLED takmıyorsanız `OLED_ENABLED 0` yapabilirsiniz; kod ekran yoksa zaten sessizce devam eder.
- OLED, oda ünitesiyle **aynı pinlerdedir**: SDA = D5 (GPIO14), SCL = D6 (GPIO12), adres `0x3C`.

---

## 5. İlk Çalıştırma

1. Oda ünitesini çalıştırın (`KombiPlus` yayınlanıyor olmalı).
2. Röle ünitesine kodu yükleyin.
3. Seri portu 115200 ile izleyin:
   ```
   === KombiPlusi - Role Unitesi ===
   Wi-Fi baglaniyor: KombiPlus
   Wi-Fi OK. IP: 192.168.4.2
   TCP baglaniyor: 192.168.4.1:8266
   Oda unitesine baglandi.
   ```
4. Oda ünitesinin web arayüzünde **Kombi bağlı** yazmalıdır.
5. OLED takılıysa sarı bantta `KOMBI ROLESI` ve `FW 1.3.0`, gövdede `ROLE ACIK/KAPALI`,
   `Oda : BAGLI/YOK`, `fs0/fs1`, su sıcaklığı ve IP/sinyal görünür. 5 dakika butona
   basılmazsa ekran kapanır; D7 butonuna basmak onu 5 dakika daha açar.
6. Röle testi: hedef sıcaklığı oda sıcaklığının üstüne çıkarın; röle klik sesi
   ve modül LED'i ile aktifleşir. Hedefi altına indirin, histerezis ve minimum
   süre sonrası kapanır.

---

## 6. Çalışma Mantığı ve Fail‑Safe

```
Oda ünitesi her 2 sn:  SET heat=0/1 sp=21.0 hyst=0.3 mode=1 fs=0
Röle ünitesi yanıt  :  STATUS relay=1 flow=42.5 rssi=-58 up=123 fs=0 fw=1.1.0
```

- Röle yalnızca şu koşullarda açılır:
  1. `heat=1` komutu geldi **ve**
  2. son komut 5 dk'dan yeni
- Bağlantı tazeyken `heat` değeri doğrudan uygulanır.
- Komut 5 dk boyunca gelmezse (`fs` ile seçilir):
  - **`fs=0` (varsayılan):** röle kapanır (güvenli).
  - **`fs=1`:** son talep korunur; `heat=1` ise röle açık kalır. Bu seçenek
    donma riskini azaltır ama aşırı ısınma riski taşır.
- `fs` değeri ilk `SET` satırıyla birlikte EEPROM'a yazılır; oda ünitesi
  erişilemezken bile yeniden başlatmada korunur.
- Açılışta röle **her zaman kapalı** başlar; ilk geçerli komut gelene kadar
  fail‑safe uygulanmaz (enerji kesilip gelirse ısıtma kendiliğinden başlamaz).

### LED anlamları

| LED | Anlam |
|-----|-------|
| Sürekli yanık | Oda ünitesine bağlı ve komut akışı sağlıklı |
| 1 Hz yavaş yanıp sönme | Wi‑Fi bağlı, TCP/oda ünitesi bekleniyor |
| 5 Hz hızlı yanıp sönme | Wi‑Fi bağlantısı yok |
| Kapalı | Cihaz kapalı veya başlatılıyor |

---

## 7. Sorun Giderme

| Belirti | Çözüm |
|---------|-------|
| Wi‑Fi bağlanmıyor | Oda ünitesi açık mı? SSID/şifre aynı mı? 2.4 GHz? |
| Bağlanıyor ama TCP yok | Oda ünitesinde `Kombi bağlı değil`; IP `192.168.4.1` doğru mu? |
| Röle ters çalışıyor | `RELAY_ACTIVE_LOW` değerini tersine çevirin |
| Röle hiç çekmiyor | VIN 5 V, GND ortak, IN D1; modül LED'i yanıyor mu? |
| Su sıcaklığı `--` | DS18B20 kablosu/pull‑up; `FLOW_SENSOR_ENABLED 1` mi? |
| OLED boş | Adres `0x3C` mi? SDA D5 / SCL D6, 3.3 V besleme; `OLED_ENABLED 1` mi? |
| OLED 5 dk sonra kapanıyor | Normal davranış; D7 butonuna basınca 5 dk daha açılır |
| Sürekli açıp kapatıyor | Histerezisi artırın; minimum süreler zaten devrede |
| Rastgele reset | Yetersiz adaptör; röle bobini akım darbesi; ayrı 5 V besleme deneyin |

---

## 8. Notlar

- Röle kontağı **kuru kontak**tır; kombiya gerilim göndermez.
- `fs=1` ("son talebi koru") yalnızca kombinin kendi güvenlik termostatı varsa
  kullanılmalıdır; aksi halde varsayılan `fs=0` bırakın.
- Fail‑safe tercihi web arayüzündeki **Röle Davranışı** anahtarıyla değiştirilir
  ve bir sonraki `SET` ile röleye aktarılır.
- Oda ünitesiyle haberleşme şifrelenmemiştir; yerel AP ağında çalışır.
  AP şifresini değiştirin ve misafirlere açık bırakmayın.
- Birden fazla oda ünitesi/röle kullanacaksanız her sisteme farklı AP adı ve
  `SERVER_HOST`/port planı gerekir (şu an tek sistem için tasarlanmıştır).
- Oda ünitesi yeniden başlarken röle en fazla 5 dk son talimatla çalışır;
  bu süre boyunca kombinin kendi güvenlik termostatı devrede olmalıdır.
