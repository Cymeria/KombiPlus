<div align="center">
  <img src="docs/images/banner.svg" alt="KombiPlus v1.1.0 — termostato de habitación Wi-Fi de dos módulos ESP8266" width="100%">
</div>

<h1 align="center">KombiPlus</h1>

<p align="center">
  <strong>Termostato de habitación Wi-Fi de dos módulos para calderas de gas.</strong><br>
  NodeMCU v3 (ESP8266) · OLED · encoder rotativo · interfaz web integrada · MQTT · Telegram · relé fail-safe
</p>

<p align="center">
  <img alt="versión" src="https://img.shields.io/badge/versi%C3%B3n-1.1.0-ff8a3d?style=flat-square">
  <img alt="plataforma" src="https://img.shields.io/badge/platform-NodeMCU%20v3%20(ESP8266)-2b3d61?style=flat-square">
  <img alt="núcleo" src="https://img.shields.io/badge/Arduino%20core-3.1.2-00979D?style=flat-square&logo=arduino&logoColor=white">
  <img alt="nube" src="https://img.shields.io/badge/nube-no%20necesaria-2dd47c?style=flat-square">
  <img alt="licencia" src="https://img.shields.io/badge/licencia-MIT-49b8ff?style=flat-square">
</p>

<p align="center">
  <strong>Idiomas:</strong>
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

**Contenido:** [Cómo funciona](#cómo-funciona) · [Pantallas](#pantallas) · [Funciones](#funciones) · [Hardware](#hardware) · [Estructura](#estructura-del-repositorio) · [Inicio rápido](#inicio-rápido) · [Lógica](#lógica-de-control) · [Fail-safe](#relé-fail-safe) · [Protocolo](#protocolo) · [API web](#api-web) · [MQTT y Telegram](#mqtt-y-telegram) · [Memoria y temas](#memoria-y-temas-oled) · [Seguridad](#seguridad) · [Versión](#versión)

---

## Cómo funciona

```
┌─────────────────────────┐        Wi-Fi AP (KombiPlus)        ┌──────────────────────┐
│  UNIDAD DE HABITACIÓN   │◄─────────────────────────────────────►│  UNIDAD DE RELÉ      │
│  NodeMCU v3             │   TCP 8266: SET heat=0/1 sp=...      │  NodeMCU v3 + relé   │
│  DHT11/22 o DS18B20     │◄──────────────────────────────────────│  DS18B20 opcional    │
│  OLED + encoder + web   │        STATUS relay=... flow=...     │  COM/NO → caldera    │
└───────────┬─────────────┘                                      └──────────┬───────────┘
            │                                                               │
   Móvil / PC (http://192.168.4.1)                              Bornera del termostato
   Wi-Fi de casa (STA, opcional)                                (contacto seco, baja tensión)
```

- La unidad de habitación **siempre** emite su propia red Wi-Fi. Conéctate a ella y abre
  `192.168.4.1` — sin app, sin nube y sin cuenta.
- Opcionalmente también se une a la red de casa (STA). El punto de acceso nunca se apaga
  porque la unidad de relé depende de él.
- Cada 2 segundos calcula la demanda de calor (temperatura, consigna e histéresis) y la
  envía a la unidad de relé.
- La unidad de relé aplica la demanda y, si no llega ningún comando en **5 minutos**, usa
  el comportamiento fail-safe configurado.
- Un DS18B20 estanco opcional informa la temperatura del agua de la caldera.
- Incluye **modo noche**, **protección antihielo**, **boost**, **10 temas OLED**,
  **histórico horario**, **gestor de archivos LittleFS** y **OTA**.

<p align="center">
  <img src="docs/sistem_mimari.svg" alt="Arquitectura del sistema" width="90%">
</p>

---

## Pantallas

<table>
<tr>
<td width="50%"><img src="docs/images/demo-oled.png" alt="Páginas y temas OLED"></td>
<td width="50%"><img src="docs/images/demo-web.png" alt="Interfaz web móvil"></td>
</tr>
<tr>
<td align="center"><sub>Cinco páginas OLED en diez temas, dibujadas por el mismo código del firmware</sub></td>
<td align="center"><sub>La interfaz web real, servida desde <code>web_ui.h</code></sub></td>
</tr>
</table>

Ambas capturas vienen de la demo sin hardware: abre **[`demo/demo.html`](demo/demo.html)**
con doble clic. La demo ejecuta una API simulada dentro del navegador.

---

## Funciones

### Unidad de habitación (`oda_termostati/`)

- **Sensor seleccionable:** DHT11, DHT22 o DS18B20 con un solo `#define`
- **OLED 0.96" SSD1306** (128×64, I²C): temperatura, consigna, estado, red e información
- **Encoder KY‑040:** girar cambia la consigna, pulsación corta cambia de página, larga el modo
- **10 temas OLED** en archivos editables `/themes/*.txt` de LittleFS
- **8 idiomas de interfaz** — inglés, turco, ruso, español, portugués, japonés, coreano y
  chino. En el primer arranque la unidad pide el idioma (OLED: girar para elegir, pulsar para
  confirmar; web: selector a pantalla completa) y lo guarda en `lang=`. En el OLED se admiten
  en/tr/es/pt (fuente solo ASCII); el resto se muestra en inglés.
- **Interfaz web móvil:** consigna, modo, boost, modo noche, comportamiento del relé,
  MQTT/Telegram, gráfico histórico, histéresis, calibración, antihielo,
  **escáner de redes Wi-Fi** e información del sistema
- **Modo noche:** otra consigna dentro de una franja horaria (reloj NTP)
- **MQTT:** JSON de estado retenido + comandos `clave=valor`
- **Bot de Telegram:** estado y control solo para el chat configurado
- **LittleFS:** ajustes en `/config.txt`, histórico horario en `/history.csv`, gestor en `/fs`
- **Histéresis + tiempos mínimos de encendido/apagado** contra ciclos cortos
- **Boost:** +1.5 °C durante 5/15/30 minutos
- **Antihielo:** calienta por debajo de 5 °C incluso con el modo apagado
- **Fail-safe:** corta la demanda si falla el sensor o se pierde el relé
- **OTA** (solo con STA) y mDNS: `http://termostat.local`

### Unidad de relé (`kombi_rolesi/`)

- Se conecta sola al punto de acceso de la unidad de habitación
- Acciona el **contacto seco** de la caldera con un módulo de relé optoacoplado
- **Activo bajo / activo alto** en una línea
- **Fail-safe:** tras 5 minutos sin comandos abre el relé (por defecto) o mantiene la última demanda
- La preferencia llega desde la unidad de habitación (`fs=0/1`) y se guarda en EEPROM
- El relé **siempre arranca abierto** al energizarse
- DS18B20 opcional para la temperatura del agua
- LED de enlace y registro serie detallado a 115200

---

## Hardware

| Cant. | Componente |
|------:|------------|
| 2 | NodeMCU v3 / ESP8266 (Lolin) |
| 1 | OLED 0.96" SSD1306, I²C, 128×64 |
| 1 | Encoder rotativo KY‑040 |
| 1 | DHT11 / DHT22 / DS18B20 (+ 4.7 kΩ para DS18B20) |
| 1 | Módulo de relé 5 V optoacoplado (1 canal) |
| 1 | DS18B20 estanco opcional (agua de la caldera) |
| 2 | Fuentes USB de 5 V, cables, bornes |

<table>
<tr>
<td width="50%"><img src="docs/oda_termostati_sema.svg" alt="Cableado de la unidad de habitación"></td>
<td width="50%"><img src="docs/kombi_rolesi_sema.svg" alt="Cableado de la unidad de relé"></td>
</tr>
<tr>
<td align="center"><sub>Unidad de habitación — OLED, encoder, sensor</sub></td>
<td align="center"><sub>Unidad de relé — contacto seco de la caldera</sub></td>
</tr>
</table>

| Unidad de habitación | GPIO | Conexión |
|----------------------|------|----------|
| D5 | GPIO14 | OLED SCL |
| D6 | GPIO12 | OLED SDA |
| D4 | GPIO2 | DATA del sensor (DHT o DS18B20) |
| D1 | GPIO5 | Encoder CLK |
| D2 | GPIO4 | Encoder DT |
| D7 | GPIO13 | Encoder SW (pulsador) |

| Unidad de relé | GPIO | Conexión |
|----------------|------|----------|
| D1 | GPIO5 | Entrada IN del relé |
| VIN | — | VCC del relé (5 V) |
| GND | — | GND común |
| D2 | GPIO4 | DS18B20 DATA (opcional) |

Tablas de pines, notas y solución de problemas en
[`oda_termostati/README.md`](oda_termostati/README.md) y [`kombi_rolesi/README.md`](kombi_rolesi/README.md).

---

## Estructura del repositorio

```
kombi/
├── README.md                  ← inglés (principal) + 7 traducciones
├── CHANGELOG.md               ← historial de versiones
├── LICENSE                    ← MIT
├── oda_termostati/
│   ├── oda_termostati.ino     ← firmware de la unidad de habitación
│   ├── web_ui.h               ← HTML/CSS/JS integrado
│   ├── src/LittleFSExplorer/  ← gestor de archivos (MIT)
│   ├── src/OledTheme/         ← motor de temas OLED (10 temas)
│   └── README.md
├── kombi_rolesi/
│   ├── kombi_rolesi.ino       ← firmware de la unidad de relé
│   └── README.md
├── demo/
│   ├── demo.html              ← demo de la interfaz sin hardware
│   ├── demo.src.html          ← plantilla de la demo
│   ├── web_ui_demo.html       ← interfaz web generada
│   └── build_demo.ps1         ← regenera la demo desde web_ui.h
└── docs/
    ├── images/                ← banner, capturas
    └── *.svg                  ← esquemas de cableado y arquitectura
```

---

## Inicio rápido

### 1. Entorno Arduino

1. **Arduino IDE 2.x** → *Preferences* → *Additional boards manager URLs*:

   ```
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```

2. *Boards Manager* → instala **esp8266** `3.1.2`.
3. *Library Manager* → instala:

   `Adafruit SSD1306`, `Adafruit GFX Library`, `DHT sensor library`,
   `Adafruit Unified Sensor`, `OneWire`, `DallasTemperature`, `PubSubClient`.

   `LittleFSExplorer` y el motor de temas ya están incluidos en `oda_termostati/src/`.

4. Placa: **NodeMCU 1.0 (ESP‑12E Module)**, velocidad **115200**, flash 4 MB.

### 2. Configuración

**Usa las mismas credenciales del punto de acceso en ambos sketches:**

```cpp
// oda_termostati.ino
#define AP_SSID  "KombiPlus"
#define AP_PASS  "Kombi.1881"

// kombi_rolesi.ino
#define TERMOSTAT_AP_SSID  "KombiPlus"
#define TERMOSTAT_AP_PASS  "Kombi.1881"
```

Elige el sensor en la unidad de habitación:

```cpp
#define SENSOR_TYPE  SENSOR_DHT22   // SENSOR_DHT11 / SENSOR_DHT22 / SENSOR_DS18B20
```

Módulo de relé activo alto:

```cpp
#define RELAY_ACTIVE_LOW  0
```

### 3. Flashear y probar

1. Flashea primero la **unidad de relé** (sin conectar el contacto a la caldera).
2. Flashea la **unidad de habitación**.
3. Conéctate a `KombiPlus` (contraseña `Kombi.1881`) y abre `http://192.168.4.1`.
4. El registro del relé mostrará `Oda unitesine baglandi.` y la web **Kombi bağlı**.
5. Solo después de probar, pide a un **técnico autorizado** que conecte el contacto del relé
   a la bornera del termostato de la caldera.

---

## Lógica de control

```
Consigna efectiva = modo ON  → consigna nocturna si el modo noche está activo, si no la normal
                               (+1.5 °C durante boost)
                    modo OFF → protección antihielo (5 °C)

Demanda ON : T ≤ consigna − histéresis  Y  ≥ 180 s desde el último apagado
Demanda OFF: T ≥ consigna + histéresis  Y  ≥ 60 s desde el último encendido
```

| Parámetro | Por defecto | Rango |
|-----------|-------------|-------|
| Consigna | 21.0 °C | 5–30 °C |
| Histéresis | 0.3 °C | 0.1–3.0 °C |
| Tiempo mínimo encendido | 60 s | fijo |
| Tiempo mínimo apagado | 180 s | fijo |
| Antihielo | 5.0 °C | 2–10 °C |
| Boost | +1.5 °C | 5/15/30 min |
| Modo noche | apagado | 23:00–07:00, 18.0 °C |
| Fail-safe del relé | abre | abre / mantiene la demanda |
| Histórico horario | 720 muestras | ~30 días |

Datos del sensor con más de 30 segundos cortan la demanda. Con el modo apagado, el antihielo sigue activo.

---

## Relé fail-safe

El relé solo se cierra mientras `heat=1` **y** el último comando tiene menos de 5 minutos.

- `fs=0` **(por defecto)** — el relé abre y la caldera se detiene. Opción segura.
- `fs=1` — mantiene la última demanda. Reduce el riesgo de congelación pero puede
  sobrecalentar; úsalo **solo** si la caldera tiene su propio termostato de seguridad.
- El relé siempre arranca abierto, así que un corte de luz no inicia la calefacción.

---

## Protocolo

La unidad de habitación escucha en **TCP 8266**; la de relé es el cliente. ASCII por líneas:

| Dirección | Línea |
|-----------|-------|
| Habitación → Relé | `SET heat=1 sp=21.0 hyst=0.3 mode=1 fs=0\n` |
| Relé → Habitación | `STATUS relay=1 flow=42.50 rssi=-58 up=123 fs=0 fw=1.1.0\n` |

Ambas direcciones se ven en los registros serie a 115200.

---

## API web

El servidor integrado entrega JSON en el puerto 80 — útil para Home Assistant, scripts o curl.

| Método | Ruta | Ejemplo |
|--------|------|---------|
| GET | `/api/status` | Estado completo en JSON |
| GET | `/api/set?sp=22.0` | Consigna |
| GET | `/api/set?mode=0` | Apagar modo |
| GET | `/api/set?hyst=0.4&cal=-0.2&frost=6` | Ajustes |
| GET | `/api/set?boost=15` | Boost 15 min (`0` cancela) |
| GET | `/api/set?sleep=1&sleepstart=23:00&sleepend=07:00&sleepsp=18` | Modo noche |
| GET | `/api/set?failsafe=1` | Relé: mantener demanda |
| GET | `/api/set?theme=gece` | Elegir tema OLED |
| GET | `/api/theme` | Lista de temas + activo |
| GET | `/api/theme?set=retro` | Elegir tema |
| GET | `/api/theme?reload=1` | Releer archivos de temas |
| POST | `/api/net` | Ajustes MQTT / Telegram |
| GET | `/api/history?hours=24` | Puntos del histórico |
| POST | `/api/history/sample` | Guardar una muestra |
| POST | `/api/history/clear` | Borrar histórico |
| GET | `/api/scan` | Redes Wi-Fi cercanas (`nets[]`) |
| POST | `/api/wifi` | Campos `ssid`, `pass` |
| POST | `/api/reboot` | Reiniciar |
| GET | `/fs` | Gestor de archivos LittleFS |

Campos de `/api/status`: `fw, theme, temp, hum, sp, eff, hyst, cal, frost, mode, demand, relay,
link, flow, brssi, boost, boostMin, sensorOk, sleepEn, sleep, sleepStart, sleepEnd, sleepSp,
failsafe, mqtt, mqttCfg, mqttPort, mqttPrefix, tg, tgOk, tgErr, tgChat, timeOk, now, hist,
fsUsed, fsTotal, uptime, ip, apip, ssid, sta, rssi, heap, client`.

---

## MQTT y Telegram

**MQTT** (web → *MQTT & Telegram*):

- `kombi/status` — JSON retenido (`temp`, `sp`, `eff`, `mode`, `demand`, `relay`, `link`, `sleep`, `failsafe`, `boost`, …)
- `kombi/set` — `sp=22.5`, `mode=1`, `sleepsp=18`, `sleepstart=23:00&sleepend=07:00`, `failsafe=1`
- `kombi/result` — `ok` / error

**Telegram**: `/durum`, `/sicaklik 22.5`, `/mod ac|kapat`, `/boost 30`, `/uyku ac|kapat`,
`/uykusaat 23:00 07:00`, `/uykusicaklik 18`, `/yardim`. Solo responde al chat configurado.

---

## Memoria y temas OLED

- `/config.txt` — todos los ajustes (`clave=valor`), migración automática desde EEPROM
- `/history.csv` — muestras horarias: `hora, habitación, humedad, consigna, demanda, noche`
- `/themes/*.txt` — definiciones de tema (`id, header, tempsize, separator, status, nem, boost,
  su, sayfa, etiket, ters, dim`); se recrean al primer arranque si faltan
- `/themes/active.txt` — id del último tema
- La web dibuja gráficos de 24 h / 7 d / 30 d; `/fs` permite listar, descargar, subir,
  borrar y editar archivos

Temas incluidos: `klasik`, `ince`, `dolu`, `kutu`, `nokta`, `dijital`, `neon`, `panel`,
`minimal`, `retro`.

---

## Seguridad

- Una caldera de gas contiene **230 V**; solo un técnico autorizado debe abrirla o cablear el contacto.
- El relé se conecta como **contacto seco** a la entrada del termostato de la caldera.
- Usa un módulo de relé **optoacoplado**, un fusible adecuado y una fuente de 5 V de calidad.
- Cambia las contraseñas de AP y OTA en producción; no dejes la red abierta a invitados.
- Proyecto de aficionado, **sin garantía**; el riesgo es del usuario.
- El termostato de seguridad y la protección de límite de la caldera deben seguir activos siempre.

---

## Versión

- **v1.1.0** — ten OLED themes, a room-temperature-first main screen and web-based first setup ([CHANGELOG.md](CHANGELOG.md))
- **v1.0.0** — primera versión estable ([CHANGELOG.md](CHANGELOG.md))
- Probado con: ESP8266 Arduino core 3.1.2, Arduino IDE 2.x
- La IRAM de la unidad de habitación está al ~95 % (`TELEGRAM_ENABLED 0` libera espacio)
- Mantén `FW_VERSION` y los datos AP de ambos dispositivos sincronizados al cambiar algo

---

## Licencia

[MIT](LICENSE) — libre para usar, modificar y compartir. `LittleFSExplorer` conserva su licencia MIT.
