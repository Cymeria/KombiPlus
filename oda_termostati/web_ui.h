/*
 * Mobil uyumlu web arayuzu (PROGMEM).
 * HTML/CSS/JS tek dosyada, harici kaynak kullanmaz.
 */
#pragma once

static const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0b1220">
<title>Kombi Termostat</title>
<style>
:root{
  --bg:#0b1220; --bg2:#0e1729; --card:#141e33; --card2:#18243d;
  --line:#243252; --txt:#e8eefb; --mut:#94a6c3;
  --acc:#ff8a3d; --acc2:#ffb066; --ok:#2dd47c; --warn:#f4b740; --bad:#f25555;
  --blue:#49b8ff;
}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{margin:0;padding:0}
body{
  min-height:100vh;
  overflow-x:hidden;
  background:radial-gradient(1200px 600px at 80% -10%,#1b2a4a 0%,transparent 60%),
             radial-gradient(900px 500px at -10% 110%,#221a2c 0%,transparent 55%),
             var(--bg);
  color:var(--txt);
  font:15px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,Arial,sans-serif;
  padding:14px 12px 34px;
}
.wrap{max-width:760px;margin:0 auto}
header{display:flex;align-items:center;justify-content:space-between;gap:10px;margin:4px 2px 14px}
.brand{display:flex;align-items:center;gap:10px;min-width:0}
.logo{
  width:42px;height:42px;flex:0 0 42px;border-radius:14px;display:grid;place-items:center;
  font-size:22px;background:linear-gradient(145deg,#ff9a4d,#e8562a);
  box-shadow:0 6px 20px rgba(255,120,50,.35);
}
h1{font-size:18px;margin:0;letter-spacing:.2px}
.brand p{margin:1px 0 0;font-size:12px;color:var(--mut);white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.pill{
  display:flex;align-items:center;gap:7px;padding:7px 11px;border-radius:999px;
  background:var(--card);border:1px solid var(--line);font-size:12px;white-space:nowrap;color:var(--mut)
}
.pill .dot{width:8px;height:8px;border-radius:50%;background:var(--mut);box-shadow:0 0 0 3px rgba(148,166,195,.12)}
.pill.ok{border-color:rgba(45,212,124,.35);color:#bdf7d8}
.pill.ok .dot{background:var(--ok);box-shadow:0 0 0 3px rgba(45,212,124,.16),0 0 12px rgba(45,212,124,.7)}
.pill.warn{border-color:rgba(244,183,64,.35);color:#ffe6b0}
.pill.warn .dot{background:var(--warn);box-shadow:0 0 0 3px rgba(244,183,64,.16)}
.pill.bad{border-color:rgba(242,85,85,.4);color:#ffc9c9}
.pill.bad .dot{background:var(--bad);box-shadow:0 0 0 3px rgba(242,85,85,.16)}

.card{
  background:linear-gradient(180deg,var(--card),var(--bg2));
  border:1px solid var(--line);border-radius:18px;padding:16px;
  box-shadow:0 10px 30px rgba(0,0,0,.18);margin:0 0 12px;
}
.label{font-size:12px;letter-spacing:.5px;text-transform:uppercase;color:var(--mut);font-weight:600}
.hint{font-size:12px;color:var(--mut);margin-top:6px}
.meta{font-size:13px;color:var(--mut);display:flex;gap:8px;flex-wrap:wrap}
.sep{opacity:.5}

.hero{display:grid;grid-template-columns:1fr;gap:18px}
.room{text-align:center}
.room .meta{justify-content:center}
@media(min-width:620px){.hero{grid-template-columns:1.05fr .95fr;align-items:center}}
.big{font-size:62px;line-height:1;font-weight:750;letter-spacing:-2px;margin:6px 0 6px}
.big .deg{font-size:24px;font-weight:600;color:var(--mut);margin-left:4px;letter-spacing:0}
.big.cold{color:var(--blue)}
.big.hot{color:var(--acc2)}
.target{border-top:1px solid var(--line);padding-top:14px}
@media(min-width:620px){.target{border-top:0;border-left:1px solid var(--line);padding:0 0 0 18px}}
.targetRow{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-top:10px}
.sp{font-size:32px;font-weight:750;letter-spacing:-.5px}
.sp .unit2{font-size:16px;color:var(--mut);margin-left:2px;font-weight:600}
.circle{
  width:48px;height:48px;border-radius:50%;border:1px solid var(--line);
  background:var(--card2);color:var(--txt);font-size:24px;line-height:1;cursor:pointer;
  transition:transform .08s ease,background .2s;
}
.circle:active{transform:scale(.93);background:#223254}
input[type=range]{
  -webkit-appearance:none;appearance:none;width:100%;height:8px;border-radius:99px;
  margin:18px 0 4px;outline:none;cursor:pointer;
  background:linear-gradient(to right,var(--acc) var(--p,50%),#26324a var(--p,50%));
}
input[type=range]::-webkit-slider-thumb{
  -webkit-appearance:none;width:26px;height:26px;border-radius:50%;
  background:linear-gradient(145deg,#ffc07a,var(--acc));border:3px solid #0d1526;
  box-shadow:0 3px 12px rgba(255,138,61,.5);
}
input[type=range]::-moz-range-thumb{
  width:22px;height:22px;border-radius:50%;background:var(--acc);border:3px solid #0d1526;
}

.stats{display:grid;grid-template-columns:repeat(3,1fr);gap:10px;margin-bottom:12px}
.stat{padding:12px 12px;margin:0;text-align:center}
.stat b{display:block;font-size:15px;margin-top:5px;font-weight:650}
.stat b.ok{color:var(--ok)} .stat b.hot{color:var(--acc2)} .stat b.bad{color:var(--bad)}

.row{display:flex;align-items:center;justify-content:space-between;gap:14px}
.toggle{
  width:58px;height:32px;flex:0 0 58px;border-radius:99px;border:1px solid var(--line);
  background:#22304d;position:relative;cursor:pointer;transition:background .2s;padding:0;
}
.toggle span{
  position:absolute;top:3px;left:3px;width:24px;height:24px;border-radius:50%;
  background:#93a4bd;transition:.2s;
}
.toggle.on{background:linear-gradient(90deg,#e9622c,#ff9e55);border-color:transparent}
.toggle.on span{left:29px;background:#fff}

.btns{display:grid;grid-template-columns:repeat(4,1fr);gap:8px;margin-top:12px}
button{
  font:inherit;color:var(--txt);background:var(--card2);border:1px solid var(--line);
  border-radius:12px;padding:11px 8px;cursor:pointer;transition:filter .15s,transform .08s;
}
button:active{transform:scale(.97)}
button:hover{filter:brightness(1.12)}
button.primary{width:100%;margin-top:14px;background:linear-gradient(90deg,#e9622c,#ff9e55);border:0;font-weight:700;color:#1a0d04}
button.danger{width:100%;margin-top:14px;background:#3a1d24;border-color:#6b2b36;color:#ffb3b3}
button.ghost{background:transparent}

details summary{
  list-style:none;cursor:pointer;display:flex;align-items:center;justify-content:space-between;
  font-weight:650;font-size:15px;
}
details summary::-webkit-details-marker{display:none}
.chev{color:var(--mut);transition:transform .2s}
details[open] .chev{transform:rotate(180deg)}
.form{display:grid;grid-template-columns:1fr;gap:10px;margin-top:14px}
@media(min-width:520px){.form.two{grid-template-columns:1fr 1fr}}
.form label{font-size:12px;color:var(--mut);display:flex;flex-direction:column;gap:5px;font-weight:600}
.form input{
  font:inherit;color:var(--txt);background:#0d1628;border:1px solid var(--line);
  border-radius:11px;padding:11px 12px;outline:none;
}
.form input:focus{border-color:var(--acc)}
.kv{display:flex;justify-content:space-between;gap:12px;padding:8px 0;border-bottom:1px dashed rgba(36,50,82,.75);font-size:13px}
.kv:last-of-type{border-bottom:0}
.kv span{color:var(--mut)}
.kv b{font-weight:600;text-align:right;word-break:break-all}
.foot{text-align:center;color:var(--mut);font-size:11px;margin:18px 0 0}

.toast{
  position:fixed;left:50%;bottom:22px;transform:translate(-50%,20px);
  background:#1b2740;border:1px solid var(--line);border-radius:12px;
  padding:11px 16px;font-size:13px;box-shadow:0 12px 32px rgba(0,0,0,.45);
  opacity:0;pointer-events:none;transition:.22s;z-index:9;max-width:88vw;text-align:center
}
.toast.show{opacity:1;transform:translate(-50%,0)}
.toast.bad{border-color:#6b2b36;color:#ffc9c9}

.chips{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px}
.chip{padding:8px 14px;border-radius:999px;border:1px solid var(--line);background:var(--card2);font-size:13px;cursor:pointer;color:var(--txt)}
.chip.on{background:linear-gradient(90deg,#e9622c,#ff9e55);border-color:transparent;color:#1a0d04;font-weight:700}
canvas{width:100%;height:200px;display:block;border-radius:12px;background:#0d1628;border:1px solid var(--line);margin-top:12px}
.legend{display:flex;gap:14px;font-size:12px;color:var(--mut);margin-top:8px;flex-wrap:wrap}
.legend i{display:inline-block;width:10px;height:10px;border-radius:3px;margin-right:5px;vertical-align:-1px}
.mini{display:flex;gap:8px;margin-top:10px}
.mini button{flex:1;font-size:13px;padding:9px 6px}
.rowline{display:flex;align-items:center;justify-content:space-between;gap:14px}
.linkbtn{display:inline-block;margin-top:12px;color:var(--acc2);font-size:13px;text-decoration:none;font-weight:600}
.form input[type=time]{color-scheme:dark}
.netrow{display:flex;align-items:center;gap:10px;flex-wrap:wrap;margin-top:12px}
.netrow .hint{margin:0;flex:1 1 160px}
.netrow button{flex:0 0 auto;width:auto}
.netlist{display:none;flex-direction:column;gap:6px;margin-top:10px}
.netlist.on{display:flex}
.net{display:flex;align-items:center;justify-content:space-between;gap:10px;width:100%;padding:10px 12px;border-radius:12px;border:1px solid var(--line);background:var(--card2);color:var(--txt);font:inherit;font-size:13px;cursor:pointer;text-align:left}
.net b{font-weight:600;min-width:0;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.net span{color:var(--mut);font-size:12px;white-space:nowrap}
.net.sel{border-color:var(--acc);background:rgba(255,138,61,.14)}
</style>
</head>
<body>
<div class="wrap">
  <header>
    <div class="brand">
      <div class="logo">&#9832;</div>
      <div>
        <h1>Kombi Termostat</h1>
        <p id="net">Oda ünitesi</p>
      </div>
    </div>
    <div class="pill" id="pill"><span class="dot"></span><span id="pillTxt">Bağlanıyor</span></div>
  </header>

  <section class="card hero">
    <div class="room">
      <div class="label">Oda Sıcaklığı</div>
      <div class="big" id="bigWrap"><span id="temp">--</span><span class="deg">°C</span></div>
      <div class="meta">
        <span id="hum">Nem —</span>
        <span class="sep">•</span>
        <span id="flow">Kombi suyu —</span>
      </div>
    </div>
    <div class="target">
      <div class="label">Hedef Sıcaklık</div>
      <div class="targetRow">
        <button class="circle" onclick="step(-0.5)" aria-label="Azalt">−</button>
        <div class="sp"><span id="spVal">21.0</span><span class="unit2">°C</span></div>
        <button class="circle" onclick="step(0.5)" aria-label="Arttır">+</button>
      </div>
      <input id="range" type="range" min="5" max="30" step="0.5" value="21" aria-label="Hedef sıcaklık">
      <div class="hint" id="effHint"></div>
    </div>
  </section>

  <section class="stats">
    <div class="card stat"><span class="label">Durum</span><b id="state">—</b></div>
    <div class="card stat"><span class="label">Kombi</span><b id="relay">—</b></div>
    <div class="card stat"><span class="label">Boost</span><b id="boost">Kapalı</b></div>
  </section>

  <section class="card row">
    <div>
      <div class="label">Çalışma Modu</div>
      <div class="hint" id="modeHint">Açık: hedef sıcaklığa göre ısıtır.</div>
    </div>
    <button class="toggle" id="modeBtn" onclick="toggleMode()" aria-label="Mod değiştir"><span></span></button>
  </section>

  <section class="card">
    <div class="label">Hızlı Isıtma (Boost)</div>
    <div class="btns">
      <button onclick="boost(5)">5 dk</button>
      <button onclick="boost(15)">15 dk</button>
      <button onclick="boost(30)">30 dk</button>
      <button class="ghost" onclick="boost(0)">İptal</button>
    </div>
    <div class="hint" id="boostHint">Boost, hedefi geçici olarak +1.5 °C yükseltir.</div>
  </section>

  <section class="card">
    <div class="rowline">
      <div>
        <div class="label">Uyku Modu</div>
        <div class="hint" id="sleepHint">Belirlenen saat aralığında daha düşük hedef uygulanır.</div>
      </div>
      <button class="toggle" id="sleepBtn" onclick="toggleSleep()" aria-label="Uyku modu"><span></span></button>
    </div>
    <div class="form two" style="margin-top:12px">
      <label>Başlangıç
        <input id="sleepStart" type="time">
      </label>
      <label>Bitiş
        <input id="sleepEnd" type="time">
      </label>
      <label>Uyku Hedefi (°C)
        <input id="sleepSp" type="number" step="0.5" min="5" max="30">
      </label>
    </div>
    <button class="primary" onclick="saveSleep()">Uyku Ayarlarını Kaydet</button>
  </section>

  <section class="card rowline">
    <div>
      <div class="label">Röle Davranışı</div>
      <div class="hint" id="fsHint">Oda ünitesine 5 dakika boyunca ulaşılamazsa röle kapanır.</div>
    </div>
    <button class="toggle" id="fsBtn" onclick="toggleFailSafe()" aria-label="Röle fail-safe"><span></span></button>
  </section>

  <section class="card">
    <div class="label">Sıcaklık Geçmişi</div>
    <div class="chips">
      <button class="chip on" id="h24" onclick="setRange(24)">24 saat</button>
      <button class="chip" id="h168" onclick="setRange(168)">7 gün</button>
      <button class="chip" id="h720" onclick="setRange(720)">30 gün</button>
    </div>
    <canvas id="chart" height="200"></canvas>
    <div class="legend">
      <span><i style="background:#ff9e55"></i>Oda</span>
      <span><i style="background:#49b8ff"></i>Hedef</span>
      <span id="histInfo"></span>
    </div>
    <div class="mini">
      <button onclick="sampleNow()">Şimdi Kaydet</button>
      <button onclick="loadHistory(true)">Yenile</button>
      <button onclick="clearHistory()">Temizle</button>
    </div>
    <a class="linkbtn" href="/fs" target="_blank" rel="noopener">LittleFS Dosya Yöneticisi →</a>
    <div class="hint">Saatlik kayıtlar LittleFS'de <b>/history.csv</b> dosyasında tutulur (en fazla ~30 gün).</div>
  </section>

  <details class="card">
    <summary>MQTT &amp; Telegram <span class="chev">⌄</span></summary>
    <div class="hint">MQTT ile durum yayınlanır ve komut alınır. Telegram botu ile uzaktan kontrol edilebilir.
      Şifre/token alanlarını boş bırakırsanız mevcut değer korunur.</div>
    <div class="form two">
      <label>MQTT Sunucu
        <input id="mqtthost" type="text" placeholder="192.168.1.10" autocomplete="off">
      </label>
      <label>MQTT Port
        <input id="mqttport" type="number" min="1" max="65535" placeholder="1883">
      </label>
      <label>MQTT Kullanıcı
        <input id="mqttuser" type="text" autocomplete="off">
      </label>
      <label>MQTT Şifre
        <input id="mqttpass" type="password" placeholder="Değişmeyecekse boş" autocomplete="new-password">
      </label>
      <label>Konu Öneki
        <input id="mqttprefix" type="text" placeholder="kombi" autocomplete="off">
      </label>
      <label>Telegram Bot Token
        <input id="tgtoken" type="password" placeholder="Değişmeyecekse boş" autocomplete="new-password">
      </label>
      <label>Telegram Chat ID
        <input id="tgchat" type="text" autocomplete="off">
      </label>
    </div>
    <button class="primary" onclick="saveNet()">Bağlantı Ayarlarını Kaydet</button>
    <button class="ghost" style="width:100%;margin-top:8px" onclick="clearNet()">MQTT/Telegram Ayarlarını Sil</button>
    <div class="kv" style="margin-top:12px"><span>MQTT durumu</span><b id="mqttState">—</b></div>
    <div class="kv"><span>Telegram durumu</span><b id="tgState">—</b></div>
  </details>

  <details class="card">
    <summary>Ayarlar <span class="chev">⌄</span></summary>
    <div class="form two">
      <label>Histerezis (°C)
        <input id="hyst" type="number" step="0.1" min="0.1" max="3">
      </label>
      <label>Kalibrasyon (°C)
        <input id="cal" type="number" step="0.1" min="-5" max="5">
      </label>
      <label>Don Koruması (°C)
        <input id="frost" type="number" step="0.5" min="2" max="10">
      </label>
    </div>
    <button class="primary" onclick="saveSettings()">Ayarları Kaydet</button>
    <div class="hint">Histerezis: hedefin altında/üstünde açma-kapama aralığı. Kalibrasyon: sensör düzeltmesi.</div>
  </details>

  <details class="card">
    <summary>Wi-Fi Ayarları <span class="chev">⌄</span></summary>
    <div class="hint">Kombi röle ünitesi bu cihazın AP'sine bağlanır; AP her zaman açıktır.
      Ev ağına da bağlanmak için bilgileri girin. SSID'yi boş bırakırsanız yalnızca AP modunda çalışır.</div>
    <div class="netrow">
      <button class="ghost" id="scanBtn" onclick="scanWifi()">Ağları Tara</button>
      <span class="hint" id="scanHint">Çevredeki ağları tarayıp listeden seçebilirsiniz.</span>
    </div>
    <div class="netlist" id="netList"></div>
    <div class="form two">
      <label>Ev Ağı SSID
        <input id="wifiSsid" type="text" maxlength="32" autocapitalize="none" autocomplete="off">
      </label>
      <label>Şifre
        <input id="wifiPass" type="password" maxlength="64" autocomplete="new-password" placeholder="Değişmeyecekse boş bırakın">
      </label>
    </div>
    <button class="primary" onclick="saveWifi()">Kaydet ve Yeniden Başlat</button>
  </details>

  <section class="card">
    <div class="label">Sistem</div>
    <div class="kv"><span>IP adresi</span><b id="ip">—</b></div>
    <div class="kv"><span>AP adresi</span><b id="apip">—</b></div>
    <div class="kv"><span>Ev ağı</span><b id="ssidView">—</b></div>
    <div class="kv"><span>Sinyal</span><b id="rssi">—</b></div>
    <div class="kv"><span>Kombi bağlantısı</span><b id="client">—</b></div>
    <div class="kv"><span>Çalışma süresi</span><b id="uptime">—</b></div>
    <div class="kv"><span>Boş bellek</span><b id="heap">—</b></div>
    <div class="kv"><span>Sürüm</span><b id="fw">—</b></div>
    <div class="kv"><span>Saat (NTP)</span><b id="timeState">—</b></div>
    <div class="kv"><span>Geçmiş kayıt</span><b id="histCount">—</b></div>
    <div class="kv"><span>LittleFS</span><b id="fsUse">—</b></div>
    <button class="danger" onclick="reboot()">Yeniden Başlat</button>
  </section>

  <p class="foot">Kombi Termostat • NodeMCU v3 • AP: 192.168.4.1</p>
</div>
<div class="toast" id="toast"></div>

<script>
const $ = id => document.getElementById(id);
let S = null, timer = null, dragging = false, toastTimer = null, firstRender = true;
let histHours = 24, histData = null, lastHistCount = -1;

function clamp(v,a,b){ return Math.max(a,Math.min(b,v)); }
function fmt(v,d){ return (v===null||v===undefined||Number.isNaN(Number(v))) ? '—' : Number(v).toFixed(d===undefined?1:d); }

function setPill(kind, text){
  const p = $('pill');
  p.className = 'pill ' + (kind || '');
  $('pillTxt').textContent = text;
}

function toast(msg, bad){
  const t = $('toast');
  t.textContent = msg;
  t.className = 'toast show' + (bad ? ' bad' : '');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { t.className = 'toast'; }, 2600);
}

async function api(path, opts){
  const r = await fetch(path, Object.assign({cache:'no-store'}, opts || {}));
  if(!r.ok) throw new Error('HTTP ' + r.status);
  return r.json();
}

function paintRange(v){
  const pct = ((v - 5) / 25) * 100;
  $('range').style.setProperty('--p', pct.toFixed(1) + '%');
}

function uptimeText(sec){
  sec = Number(sec) || 0;
  const d = Math.floor(sec/86400), h = Math.floor((sec%86400)/3600), m = Math.floor((sec%3600)/60);
  if(d > 0) return d + ' gün ' + h + ' sa';
  if(h > 0) return h + ' sa ' + m + ' dk';
  return m + ' dk';
}

function render(){
  if(!S) return;
  setPill(S.link ? 'ok' : 'warn', S.link ? 'Kombi bağlı' : 'Kombi bağlı değil');

  const t = (S.sensorOk && S.temp !== null) ? Number(S.temp) : null;
  $('temp').textContent = t === null ? '--' : t.toFixed(1);
  const big = $('bigWrap');
  big.classList.toggle('cold', t !== null && t < S.sp - 1);
  big.classList.toggle('hot', t !== null && t > S.sp + 1);

  $('hum').textContent = 'Nem ' + (S.hum === null ? '—' : Math.round(S.hum) + '%');
  $('flow').textContent = 'Kombi suyu ' + (S.flow > -100 ? Number(S.flow).toFixed(1) + '°C' : '—');

  if(!dragging){
    $('spVal').textContent = Number(S.sp).toFixed(1);
    $('range').value = S.sp;
    paintRange(S.sp);
  }

  const state = $('state');
  if(!S.sensorOk){ state.textContent = 'Sensör hatası'; state.className = 'bad'; }
  else if(!S.link){ state.textContent = 'Bağlantı yok'; state.className = 'bad'; }
  else if(!S.mode){ state.textContent = 'Kapalı'; state.className = ''; }
  else if(S.demand){ state.textContent = 'Isıtıyor'; state.className = 'hot'; }
  else { state.textContent = 'Bekliyor'; state.className = 'ok'; }

  const relay = $('relay');
  relay.textContent = S.relay ? 'Aktif' : 'Kapalı';
  relay.className = S.relay ? 'hot' : '';

  $('boost').textContent = S.boost > 0 ? Math.ceil(S.boost/60) + ' dk' : 'Kapalı';
  $('boostHint').textContent = S.boost > 0
    ? 'Boost aktif: yaklaşık ' + Math.ceil(S.boost/60) + ' dk kaldı.'
    : 'Boost, hedefi geçici olarak +1.5 °C yükseltir.';

  const btn = $('modeBtn');
  btn.classList.toggle('on', !!S.mode);
  $('modeHint').textContent = S.mode ? 'Açık: hedef sıcaklığa göre ısıtır.' : 'Kapalı: yalnızca don koruması devrede.';

  $('ip').textContent = S.ip || '—';
  $('apip').textContent = S.apip || '—';
  $('ssidView').textContent = S.sta ? (S.ssid || '—') : 'AP modu';
  $('rssi').textContent = S.sta ? (S.rssi + ' dBm') : '—';
  $('client').textContent = S.client ? S.client : 'Bağlı değil';
  $('uptime').textContent = uptimeText(S.uptime);
  $('heap').textContent = Math.round(S.heap/1024) + ' KB';
  $('fw').textContent = S.fw;
  $('net').textContent = S.sta ? ('Ev ağı • ' + S.ip) : ('AP • ' + S.apip);

  // ---- Uyku / role fail-safe / baglanti ----
  const sleepOn = !!S.sleepEn;
  $('sleepBtn').classList.toggle('on', sleepOn);
  if(!sleepOn){
    $('sleepHint').textContent = 'Belirlenen saat aralığında daha düşük hedef uygulanır.';
  }else if(S.sleep === 2){
    $('sleepHint').textContent = 'AKTİF • hedef ' + fmt(S.sleepSp) + ' °C • ' + S.sleepStart + '–' + S.sleepEnd;
  }else{
    $('sleepHint').textContent = 'Bekliyor • ' + S.sleepStart + '–' + S.sleepEnd + ' • ' + fmt(S.sleepSp) + ' °C';
  }
  $('effHint').textContent = (S.sleep === 2)
    ? 'Uygulanan hedef: ' + fmt(S.eff) + ' °C (uyku modu)'
    : 'Uygulanan hedef: ' + fmt(S.eff) + ' °C';

  $('fsBtn').classList.toggle('on', !!S.failsafe);
  $('fsHint').textContent = S.failsafe
    ? 'Oda ünitesine 5 dk ulaşılamazsa röle son talebi korur (AÇIK kalabilir).'
    : 'Oda ünitesine 5 dk ulaşılamazsa röle kapanır (güvenli).';

  $('mqttState').textContent = S.mqtt
    ? 'Bağlı • konu: ' + (S.mqttPrefix || 'kombi')
    : (S.mqttCfg ? 'Bağlanıyor…' : 'Yapılandırılmadı');
  $('tgState').textContent = S.tg
    ? ((S.tgOk >= 0 ? 'Çalışıyor' : 'Bekliyor') + (S.tgErr ? ' • ' + S.tgErr : ''))
    : 'Yapılandırılmadı';

  $('timeState').textContent = S.timeOk ? 'Senkron' : 'Bekleniyor';
  $('histCount').textContent = S.hist + ' kayıt';
  $('fsUse').textContent = S.fsTotal
    ? (Math.round(S.fsUsed / 1024) + ' / ' + Math.round(S.fsTotal / 1024) + ' KB')
    : '—';

  if(S.hist !== lastHistCount){
    lastHistCount = S.hist;
    loadHistory();
  }

  if(firstRender){
    $('hyst').value = S.hyst;
    $('cal').value = S.cal;
    $('frost').value = S.frost;
    $('wifiSsid').value = S.ssid || '';
    $('sleepStart').value = S.sleepStart || '23:00';
    $('sleepEnd').value = S.sleepEnd || '07:00';
    $('sleepSp').value = S.sleepSp;
    $('mqtthost').value = S.mqttCfg || '';
    $('mqttport').value = S.mqttPort || 1883;
    $('mqttprefix').value = S.mqttPrefix || 'kombi';
    $('tgchat').value = S.tgChat || '';
    firstRender = false;
  }
}

async function poll(){
  try{
    S = await api('/api/status');
    render();
  }catch(e){
    setPill('bad', 'Bağlantı yok');
  }
  clearTimeout(timer);
  timer = setTimeout(poll, 2500);
}

async function sendSet(params, okMsg){
  try{
    S = await api('/api/set?' + new URLSearchParams(params).toString());
    firstRender = false;
    render();
    if(okMsg) toast(okMsg);
  }catch(e){ toast('İşlem başarısız', true); }
}

function step(d){
  if(!S) return;
  const v = clamp(Math.round((Number(S.sp) + d) * 2) / 2, 5, 30);
  $('spVal').textContent = v.toFixed(1);
  $('range').value = v;
  paintRange(v);
  sendSet({sp:v});
}

$('range').addEventListener('input', e => {
  dragging = true;
  const v = Number(e.target.value);
  $('spVal').textContent = v.toFixed(1);
  paintRange(v);
});
$('range').addEventListener('change', e => {
  dragging = false;
  sendSet({sp:Number(e.target.value)});
});

function toggleMode(){ if(S) sendSet({mode: S.mode ? 0 : 1}); }
function boost(m){ sendSet({boost:m}, m > 0 ? ('Boost ' + m + ' dk başlatıldı') : 'Boost iptal edildi'); }

function saveSettings(){
  const hyst = Number($('hyst').value);
  const cal = Number($('cal').value);
  const frost = Number($('frost').value);
  if(!isFinite(hyst) || !isFinite(cal) || !isFinite(frost)){ toast('Geçerli değer girin', true); return; }
  sendSet({hyst:hyst, cal:cal, frost:frost}, 'Ayarlar kaydedildi');
}

let scanning = false;
async function scanWifi(){
  if(scanning) return;
  scanning = true;
  const btn = $('scanBtn'), list = $('netList'), hint = $('scanHint');
  btn.disabled = true;
  btn.textContent = 'Taranıyor…';
  hint.textContent = 'Çevredeki ağlar aranıyor, birkaç saniye sürebilir.';
  try{
    const r = await api('/api/scan');
    const nets = (r && r.nets) ? r.nets : [];
    list.textContent = '';
    list.classList.add('on');
    if(nets.length === 0){
      hint.textContent = 'Ağ bulunamadı; SSID\'yi elle girebilirsiniz.';
    }else{
      hint.textContent = nets.length + ' ağ bulundu. Seçmek için dokunun.';
      nets.forEach(n => {
        const b = document.createElement('button');
        b.type = 'button';
        b.className = 'net';
        const nm = document.createElement('b');
        nm.textContent = n.s;
        const meta = document.createElement('span');
        meta.textContent = (n.e ? '🔒 ' : '') + n.r + ' dBm';
        b.appendChild(nm);
        b.appendChild(meta);
        b.onclick = () => {
          $('wifiSsid').value = n.s;
          Array.prototype.forEach.call(list.children, c => c.classList.remove('sel'));
          b.classList.add('sel');
          if(n.e) $('wifiPass').focus();
        };
        list.appendChild(b);
      });
    }
  }catch(e){
    hint.textContent = 'Tarama başarısız oldu; SSID\'yi elle girebilirsiniz.';
    toast('Ağ taraması başarısız', true);
  }
  btn.disabled = false;
  btn.textContent = 'Ağları Tara';
  scanning = false;
}

async function saveWifi(){
  const ssid = $('wifiSsid').value.trim();
  const pass = $('wifiPass').value;
  if(ssid.length === 0){
    if(!confirm('Ev ağı bilgileri silinsin ve yalnızca AP modunda çalışılsın mı?')) return;
  }
  try{
    await api('/api/wifi', {method:'POST', body:new URLSearchParams({ssid:ssid, pass:pass})});
    toast('Kaydedildi. Cihaz yeniden başlıyor…');
    setPill('warn', 'Yeniden başlıyor');
    setTimeout(() => { location.reload(); }, 9000);
  }catch(e){ toast('Kaydedilemedi', true); }
}

async function reboot(){
  if(!confirm('Cihaz yeniden başlatılsın mı?')) return;
  try{ await api('/api/reboot', {method:'POST'}); }catch(e){}
  toast('Yeniden başlıyor…');
  setTimeout(() => { location.reload(); }, 8000);
}

function toggleSleep(){
  if(S) sendSet({sleep: S.sleepEn ? 0 : 1}, S.sleepEn ? 'Uyku modu kapatıldı' : 'Uyku modu açıldı');
}

function saveSleep(){
  const a = $('sleepStart').value || '23:00';
  const b = $('sleepEnd').value || '07:00';
  const sp = Number($('sleepSp').value);
  if(!isFinite(sp) || sp < 5 || sp > 30){ toast('Geçerli uyku hedefi girin', true); return; }
  sendSet({sleep: (S && S.sleepEn) ? 1 : 0, sleepstart:a, sleepend:b, sleepsp:sp}, 'Uyku ayarları kaydedildi');
}

function toggleFailSafe(){
  if(S) sendSet({failsafe: S.failsafe ? 0 : 1},
    S.failsafe ? 'Bağlantı kopunca röle kapanacak' : 'Bağlantı kopunca röle son talebi koruyacak');
}

async function saveNet(){
  const body = new URLSearchParams({
    mqtthost: $('mqtthost').value.trim(),
    mqttport: String($('mqttport').value || 1883),
    mqttuser: $('mqttuser').value.trim(),
    mqttpass: $('mqttpass').value,
    mqttprefix: $('mqttprefix').value.trim(),
    tgtoken: $('tgtoken').value,
    tgchat: $('tgchat').value.trim()
  });
  try{
    await api('/api/net', {method:'POST', body:body});
    $('mqttpass').value = '';
    $('tgtoken').value = '';
    S = await api('/api/status');
    render();
    toast('Bağlantı ayarları kaydedildi');
  }catch(e){ toast('Kaydedilemedi', true); }
}

async function clearNet(){
  if(!confirm('MQTT ve Telegram ayarları silinsin mi?')) return;
  try{
    await api('/api/net', {method:'POST', body:new URLSearchParams({mqttclear:'1', tgclear:'1'})});
    S = await api('/api/status');
    render();
    toast('Bağlantı ayarları silindi');
  }catch(e){ toast('Silinemedi', true); }
}

// ---------------- Sicaklik gecmisi grafigi ----------------
function fmtClock(ts){
  const d = new Date(Number(ts) * 1000);
  return String(d.getHours()).padStart(2,'0') + ':' + String(d.getMinutes()).padStart(2,'0');
}
function fmtDay(ts){
  const d = new Date(Number(ts) * 1000);
  return String(d.getDate()).padStart(2,'0') + '.' + String(d.getMonth()+1).padStart(2,'0');
}

function setRange(h){
  histHours = h;
  ['h24','h168','h720'].forEach(id => $(id).classList.remove('on'));
  $(h === 24 ? 'h24' : h === 168 ? 'h168' : 'h720').classList.add('on');
  loadHistory();
}

async function loadHistory(manual){
  try{
    histData = await api('/api/history?hours=' + histHours);
    drawChart();
    if(manual) toast('Grafik yenilendi');
  }catch(e){ if(manual) toast('Geçmiş yüklenemedi', true); }
}

async function sampleNow(){
  try{
    await api('/api/history/sample', {method:'POST'});
    S = await api('/api/status');
    lastHistCount = S.hist;
    render();
    await loadHistory();
    toast('Örnek kaydedildi');
  }catch(e){ toast('Kaydedilemedi', true); }
}

async function clearHistory(){
  if(!confirm('Tüm sıcaklık geçmişi silinsin mi?')) return;
  try{
    await api('/api/history/clear', {method:'POST'});
    histData = {points:[]};
    drawChart();
    S = await api('/api/status');
    lastHistCount = S.hist;
    render();
    toast('Geçmiş silindi');
  }catch(e){ toast('Silinemedi', true); }
}

function drawChart(){
  const cv = $('chart');
  if(!cv) return;
  const w = Math.max(280, cv.clientWidth || 300), h = 200;
  const dpr = window.devicePixelRatio || 1;
  cv.width = Math.round(w * dpr);
  cv.height = Math.round(h * dpr);
  const g = cv.getContext('2d');
  g.setTransform(dpr, 0, 0, dpr, 0, 0);
  g.clearRect(0, 0, w, h);

  const padL = 36, padR = 10, padT = 12, padB = 22;
  const cw = w - padL - padR, ch = h - padT - padB;
  const pts = (histData && histData.points) || [];
  const info = $('histInfo');

  g.font = '11px system-ui,sans-serif';

  if(pts.length < 2){
    g.beginPath();
    g.moveTo(padL, padT); g.lineTo(padL, padT + ch); g.lineTo(padL + cw, padT + ch);
    g.strokeStyle = 'rgba(36,50,82,.9)'; g.stroke();
    g.fillStyle = '#94a6c3'; g.textAlign = 'center';
    g.fillText(histData ? 'Henüz yeterli kayıt yok' : 'Yükleniyor…', w / 2, h / 2);
    g.textAlign = 'left';
    if(info) info.textContent = histData ? (pts.length + ' nokta') : '';
    return;
  }

  let lo = 99, hi = -99;
  pts.forEach(p => {
    const v = Number(p.v), s = Number(p.s);
    if(isFinite(v)){ lo = Math.min(lo, v); hi = Math.max(hi, v); }
    if(isFinite(s)){ lo = Math.min(lo, s); hi = Math.max(hi, s); }
  });
  if(!isFinite(lo) || !isFinite(hi)){ lo = 5; hi = 30; }
  lo = Math.floor(lo - 1); hi = Math.ceil(hi + 1);
  if(hi - lo < 4) hi = lo + 4;

  const t0 = histData.timeOk ? Number(pts[0].t) : 0;
  const t1 = histData.timeOk ? Math.max(Number(pts[pts.length-1].t), Number(histData.now || 0)) : pts.length - 1;

  const X = i => {
    if(histData.timeOk){
      const tt = Number(pts[i].t);
      return padL + (t1 === t0 ? cw / 2 : ((tt - t0) / (t1 - t0)) * cw);
    }
    return padL + (pts.length === 1 ? cw / 2 : (i / (pts.length - 1)) * cw);
  };
  const Y = v => padT + ch - ((clamp(Number(v), lo, hi) - lo) / (hi - lo)) * ch;

  g.textAlign = 'right'; g.fillStyle = '#94a6c3';
  const steps = 4;
  for(let k = 0; k <= steps; k++){
    const val = lo + (hi - lo) * k / steps;
    const yy = padT + ch - (k / steps) * ch;
    g.beginPath(); g.moveTo(padL, yy); g.lineTo(padL + cw, yy);
    g.strokeStyle = 'rgba(36,50,82,.7)'; g.stroke();
    g.fillText(val.toFixed(0) + '°', padL - 6, yy + 4);
  }
  g.textAlign = 'left';

  const grad = g.createLinearGradient(0, padT, 0, padT + ch);
  grad.addColorStop(0, 'rgba(255,158,85,.35)');
  grad.addColorStop(1, 'rgba(255,158,85,0)');
  g.beginPath();
  pts.forEach((p, i) => { const xx = X(i), yy = Y(p.v); i ? g.lineTo(xx, yy) : g.moveTo(xx, yy); });
  g.lineTo(X(pts.length - 1), padT + ch);
  g.lineTo(X(0), padT + ch);
  g.closePath();
  g.fillStyle = grad; g.fill();

  g.beginPath();
  pts.forEach((p, i) => { const xx = X(i), yy = Y(p.v); i ? g.lineTo(xx, yy) : g.moveTo(xx, yy); });
  g.strokeStyle = '#ff9e55'; g.lineWidth = 2; g.lineJoin = 'round'; g.stroke();

  g.beginPath();
  pts.forEach((p, i) => { const xx = X(i), yy = Y(p.s); i ? g.lineTo(xx, yy) : g.moveTo(xx, yy); });
  g.setLineDash([5, 4]); g.strokeStyle = '#49b8ff'; g.lineWidth = 1.5; g.stroke();
  g.setLineDash([]);

  g.fillStyle = '#94a6c3'; g.textAlign = 'center';
  const ticks = 3;
  for(let k = 0; k <= ticks; k++){
    const idx = Math.round((pts.length - 1) * k / ticks);
    const p = pts[idx];
    const label = histData.timeOk
      ? (histHours > 48 ? fmtDay(p.t) : fmtClock(p.t))
      : ('#' + (idx + 1));
    g.fillText(label, X(idx), h - 6);
  }
  g.textAlign = 'left';

  if(info){
    const vs = pts.map(p => Number(p.v)).filter(v => isFinite(v));
    if(vs.length){
      info.textContent = pts.length + ' nokta • min ' + Math.min.apply(null, vs).toFixed(1) +
                         '° • max ' + Math.max.apply(null, vs).toFixed(1) + '°';
    }
  }
}

let resizeTimer = null;
window.addEventListener('resize', () => {
  clearTimeout(resizeTimer);
  resizeTimer = setTimeout(drawChart, 150);
});

poll();
</script>
</body>
</html>
)HTML";
