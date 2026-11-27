# Arayuz demosunu uretir.
#   oda_termostati/web_ui.h  ->  demo/web_ui_demo.html (tek basina calisan demo)
#                            ->  demo/demo.html        (demo.src.html + gomulu arayuz)
# Kullanim: powershell -ExecutionPolicy Bypass -File demo\build_demo.ps1
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

$headerPath = Join-Path $root 'oda_termostati\web_ui.h'
$srcPath    = Join-Path $PSScriptRoot 'demo.src.html'
$outDemo    = Join-Path $PSScriptRoot 'demo.html'
$outUi      = Join-Path $PSScriptRoot 'web_ui_demo.html'

$raw = [System.IO.File]::ReadAllText($headerPath, [System.Text.Encoding]::UTF8)
$m = [regex]::Match($raw, '(?s)R"HTML\((.*)\)HTML"')
if (-not $m.Success) { throw 'web_ui.h icinde R"HTML(...)HTML" bulunamadi.' }
$ui = $m.Groups[1].Value -replace "`r`n", "`n"

# Gercek donanim yerine sahte API sunan demo katmani
$stub = @'
<script>
/* === DEMO KATMANI: gercek donanim yerine sahte API === */
(function () {
  var MOCK = {
    link: true, sensorOk: true, mode: true, sta: true,
    temp: 21.4, sp: 22.0, eff: 22.0, hum: 55, flow: 42.5,
    demand: true, relay: true, boost: 0,
    ip: '192.168.1.57', apip: '192.168.4.1', ssid: 'EvWifi',
    rssi: -58, client: '192.168.4.2', uptime: 8040, heap: 38200,
    fw: '1.1.0', lang: '', ask: 1, setup: 1, hyst: 0.3, cal: 0, frost: 5.0,
    sleepEn: 1, sleep: 1, sleepStart: '23:00', sleepEnd: '07:00', sleepSp: 18.0,
    failsafe: 0, mqtt: 1, mqttCfg: '192.168.1.10', mqttPort: 1883, mqttPrefix: 'kombi',
    tg: 1, tgOk: 12, tgErr: '', tgChat: '123456789',
    timeOk: 1, now: Math.floor(Date.now() / 1000), hist: 0,
    fsUsed: 4096, fsTotal: 1048576
  };

  function mins(hhmm) {
    var p = String(hhmm || '0:0').split(':');
    return (Number(p[0]) || 0) * 60 + (Number(p[1]) || 0);
  }
  function sleepNow() {
    if (!MOCK.sleepEn) return false;
    var a = mins(MOCK.sleepStart), b = mins(MOCK.sleepEnd);
    var d = new Date(), n = d.getHours() * 60 + d.getMinutes();
    if (a === b) return false;
    return a < b ? (n >= a && n < b) : (n >= a || n < b);
  }

  var HIST = [];
  (function seed() {
    var now = Math.floor(Date.now() / 1000);
    for (var k = 719; k >= 0; k--) {
      var ts = now - k * 3600;
      var hr = new Date(ts * 1000).getHours();
      var slp = hr >= 23 || hr < 7;
      var base = 21 + Math.sin(k / 12) * 2.1 + (k % 5) * 0.08;
      HIST.push({
        t: ts,
        v: Math.round((base - (slp ? 1.6 : 0)) * 10) / 10,
        h: 55,
        s: slp ? 18 : 22,
        d: slp ? 0 : 1,
        u: slp ? 1 : 0
      });
    }
    MOCK.hist = HIST.length;
  })();

  function tick() {
    var act = sleepNow();
    MOCK.sleep = MOCK.sleepEn ? (act ? 2 : 1) : 0;
    var target = (act ? MOCK.sleepSp : MOCK.sp) + (MOCK.boost > 0 ? 1.5 : 0);
    MOCK.eff = Math.round(target * 10) / 10;
    if (MOCK.boost > 0) MOCK.boost = Math.max(0, MOCK.boost - 3);
    MOCK.demand = !!(MOCK.mode && MOCK.sensorOk && MOCK.link && MOCK.temp < target - MOCK.hyst / 2);
    MOCK.relay = MOCK.demand;
    MOCK.temp = Math.round((MOCK.temp + (MOCK.demand ? 0.12 : -0.04)) * 10) / 10;
    MOCK.temp = Math.max(15, Math.min(27, MOCK.temp));
    MOCK.uptime += 3;
    MOCK.now = Math.floor(Date.now() / 1000);
    MOCK.heap = 36800 + Math.round(Math.random() * 1200);
  }
  setInterval(tick, 3000);

  function ok(obj) {
    return Promise.resolve({
      ok: true, status: 200,
      json: function () { return Promise.resolve(obj); },
      text: function () { return Promise.resolve(''); }
    });
  }

  window.fetch = function (url, opts) {
    var u = String(url);
    var i = u.indexOf('?');
    var path = i < 0 ? u : u.slice(0, i);
    var q = new URLSearchParams(i < 0 ? '' : u.slice(i + 1));
    var body = (opts && opts.body instanceof URLSearchParams) ? opts.body : new URLSearchParams();

    if (path === '/api/set') {
      if (q.has('sp')) MOCK.sp = Number(q.get('sp'));
      if (q.has('hyst')) MOCK.hyst = Number(q.get('hyst'));
      if (q.has('cal')) MOCK.cal = Number(q.get('cal'));
      if (q.has('frost')) MOCK.frost = Number(q.get('frost'));
      if (q.has('mode')) MOCK.mode = Number(q.get('mode')) !== 0;
      if (q.has('boost')) MOCK.boost = Number(q.get('boost')) * 60;
      if (q.has('sleep')) MOCK.sleepEn = Number(q.get('sleep')) ? 1 : 0;
      if (q.has('sleepstart')) MOCK.sleepStart = q.get('sleepstart');
      if (q.has('sleepend')) MOCK.sleepEnd = q.get('sleepend');
      if (q.has('sleepsp')) MOCK.sleepSp = Number(q.get('sleepsp'));
      if (q.has('failsafe')) MOCK.failsafe = Number(q.get('failsafe')) ? 1 : 0;
      if (q.has('lang')) { MOCK.lang = q.get('lang'); MOCK.ask = 0; }
      tick();
      return ok(MOCK);
    }
    if (path === '/api/net') {
      if (body.has('mqtthost')) MOCK.mqttCfg = body.get('mqtthost');
      if (body.has('mqttport')) MOCK.mqttPort = Number(body.get('mqttport')) || 1883;
      if (body.has('mqttprefix')) MOCK.mqttPrefix = body.get('mqttprefix');
      if (body.has('tgchat')) MOCK.tgChat = body.get('tgchat');
      if (body.get('mqttclear') === '1' || body.get('tgclear') === '1') {
        if (body.get('mqttclear') === '1') { MOCK.mqttCfg = ''; MOCK.mqtt = 0; }
        if (body.get('tgclear') === '1') { MOCK.tg = 0; MOCK.tgChat = ''; }
      } else {
        MOCK.mqtt = MOCK.mqttCfg ? 1 : 0;
        MOCK.tg = MOCK.tgChat ? 1 : 0;
      }
      return ok({ ok: true });
    }
    if (path === '/api/history') {
      var hours = Number(q.get('hours') || 24);
      var now = Math.floor(Date.now() / 1000);
      var cut = now - hours * 3600;
      var stride = Math.max(1, Math.ceil(hours / 200));
      var out = [];
      for (var k = 0; k < HIST.length; k++) {
        if (HIST[k].t < cut) continue;
        if (k % stride !== 0) continue;
        out.push(HIST[k]);
      }
      return ok({
        timeOk: 1, now: now, hours: hours, count: HIST.length,
        fsUsed: MOCK.fsUsed, fsTotal: MOCK.fsTotal, points: out
      });
    }
    if (path === '/api/history/sample') {
      var t = Math.floor(Date.now() / 1000);
      HIST.push({ t: t, v: MOCK.temp, h: MOCK.hum, s: MOCK.eff, d: MOCK.demand ? 1 : 0, u: MOCK.sleep === 2 ? 1 : 0 });
      MOCK.hist = HIST.length;
      return ok({ ok: 1, count: HIST.length });
    }
    if (path === '/api/history/clear') {
      HIST.length = 0;
      MOCK.hist = 0;
      return ok({ ok: true });
    }
    if (path === '/api/scan') {
      return ok({ ok: 1, busy: 0, state: 1, nets: [
        { s: 'EvWifi', r: -58, e: 1 },
        { s: 'EvWifi-5G', r: -64, e: 1 },
        { s: 'KOMSU_2.4G', r: -71, e: 1 },
        { s: 'KOMBI-TEST', r: -77, e: 1 },
        { s: 'MISAFIR', r: -83, e: 0 },
        { s: 'TP-LINK_9F2C', r: -88, e: 1 }
      ] });
    }
    if (path === '/api/wifi') {
      if (q.has('skip')) { MOCK.setup = 0; return ok({ ok: 1, reboot: 0 }); }
      MOCK.ssid = body.get('ssid') || '';
      MOCK.setup = MOCK.ssid ? 0 : 1;
      return ok({ ok: 1, reboot: 1, ssid: MOCK.ssid });
    }
    if (path === '/api/reboot') return ok({ ok: true });
    return ok(MOCK);
  };

  window.__demoState = MOCK;
})();
</script>
'@

# Stub, sayfanin kendi script'inden once calismali (ilk poll basarisiz olmasin)
$marker = "<script>`nconst `$ = id"
if (-not $ui.Contains($marker)) { throw 'web_ui.h icindeki script baslangici bulunamadi.' }
$uiDemo = $ui.Replace($marker, $stub + "`n" + $marker)

$enc = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($outUi, $uiDemo, $enc)

# demo.html: arayuzu JS string olarak gom (file:// altinda iframe sorunsuz calissin)
$tpl = [System.IO.File]::ReadAllText($srcPath, [System.Text.Encoding]::UTF8)
if (-not $tpl.Contains('__UI_HTML__')) { throw 'demo.src.html icinde __UI_HTML__ isaretcisi yok.' }
$json = ConvertTo-Json -InputObject $uiDemo -Compress
$json = $json -replace '</', '<\/'
$out = $tpl.Replace('__UI_HTML__', $json)
[System.IO.File]::WriteAllText($outDemo, $out, $enc)

Write-Host ('web_ui_demo.html : ' + (Get-Item $outUi).Length + ' bayt')
Write-Host ('demo.html        : ' + (Get-Item $outDemo).Length + ' bayt')
