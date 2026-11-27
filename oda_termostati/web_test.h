/*
 * /test - panel simulasyonu (PROGMEM).
 *
 * Ekran, cihazin gercek SSD1306 framebuffer'inden gelir (/api/oled), yani
 * temalar, sayfalar ve dil secimi birebir ayni gorunur. Ekrandaki enkoder ve
 * buton, cihazin gercek enkoder/buton isleyicilerini tetikler
 * (/api/encoder, /api/button): hedef sicaklik, sayfa, tema ve mod degisir.
 */
#pragma once

static const char TEST_HTML[] PROGMEM = R"TEST(<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0b1220">
<title>KombiPlus · Panel testi</title>
<style>
:root{
  --bg:#0b1220; --bg2:#0e1729; --card:#141e33; --line:#243252;
  --txt:#e8eefb; --mut:#94a6c3; --acc:#ff8a3d; --ok:#2dd47c; --bad:#f25555;
}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{margin:0;padding:0}
body{
  min-height:100vh;
  background:radial-gradient(1200px 600px at 80% -10%,#1b2a4a 0%,transparent 60%),
             radial-gradient(900px 500px at -10% 110%,#221a2c 0%,transparent 55%),
             var(--bg);
  color:var(--txt);
  font:15px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,Arial,sans-serif;
  padding:14px 12px 30px;
}
.wrap{max-width:520px;margin:0 auto}
header{display:flex;align-items:center;gap:10px;margin:4px 2px 14px}
.back{
  width:38px;height:38px;flex:0 0 38px;border-radius:12px;display:grid;place-items:center;
  background:var(--card);border:1px solid var(--line);color:var(--txt);
  text-decoration:none;font-size:18px;line-height:1
}
h1{font-size:17px;margin:0;letter-spacing:.2px}
.sub{font-size:12px;color:var(--mut);margin-top:1px}
.pill{
  margin-left:auto;display:flex;align-items:center;gap:7px;padding:7px 11px;border-radius:999px;
  background:var(--card);border:1px solid var(--line);font-size:12px;color:var(--mut);white-space:nowrap
}
.pill .dot{width:8px;height:8px;border-radius:50%;background:var(--mut)}
.pill.ok{border-color:rgba(45,212,124,.35);color:#bdf7d8}
.pill.ok .dot{background:var(--ok);box-shadow:0 0 0 3px rgba(45,212,124,.16),0 0 12px rgba(45,212,124,.7)}
.pill.bad{border-color:rgba(242,85,85,.4);color:#ffc9c9}
.pill.bad .dot{background:var(--bad);box-shadow:0 0 0 3px rgba(242,85,85,.16)}

.card{
  background:linear-gradient(180deg,var(--card),var(--bg2));
  border:1px solid var(--line);border-radius:18px;padding:16px;
  box-shadow:0 10px 30px rgba(0,0,0,.18);margin:0 0 12px;
}
.label{font-size:12px;letter-spacing:.5px;text-transform:uppercase;color:var(--mut);font-weight:600}
.hint{font-size:12px;color:var(--mut);margin-top:8px}
.kv{display:flex;justify-content:space-between;gap:10px;font-size:13px;padding:5px 0}
.kv span{color:var(--mut)}
.kv b{font-weight:650}

.device{
  display:flex;flex-direction:column;align-items:center;gap:16px;
  background:linear-gradient(160deg,#1a2338,#0c1220);
  border:1px solid var(--line);border-radius:20px;padding:18px 16px;
}
.screen{
  position:relative;width:100%;max-width:330px;padding:12px;border-radius:14px;
  background:#05070c;border:1px solid #1d2942;
  box-shadow:inset 0 0 22px rgba(73,184,255,.10),0 8px 22px rgba(0,0,0,.45)
}
canvas{
  display:block;width:100%;height:auto;image-rendering:pixelated;border-radius:2px;
  background:#000;box-shadow:0 0 14px rgba(120,200,255,.14)
}
.screen.dead canvas{opacity:.18}
.screen .off{
  position:absolute;inset:0;display:none;place-items:center;text-align:center;
  font-size:12px;color:var(--mut);padding:18px;line-height:1.5
}
.screen.dead .off{display:grid}
.knobrow{display:flex;align-items:center;gap:18px}
.rbtn{
  width:54px;height:54px;border-radius:16px;border:1px solid var(--line);
  background:#1b2740;color:var(--txt);font-size:23px;line-height:1;cursor:pointer;
  user-select:none;touch-action:none;padding:0;
  transition:transform .08s ease,background .15s ease;
}
.rbtn:active{transform:scale(.93);background:#24345a}
.knob{
  --p:0;position:relative;width:92px;height:92px;border-radius:50%;padding:0;cursor:pointer;
  border:1px solid #2b3a5c;user-select:none;touch-action:none;
  background:radial-gradient(circle at 35% 30%,#2b3a5c,#111a2c 70%);
  box-shadow:inset 0 2px 6px rgba(255,255,255,.06),0 6px 18px rgba(0,0,0,.45);
  transition:transform .08s ease;
}
.knob::before{
  content:'';position:absolute;left:50%;top:9px;width:4px;height:15px;border-radius:2px;
  background:var(--acc);transform:translateX(-50%);opacity:.85
}
.knob .ring{
  position:absolute;inset:-1px;border-radius:50%;display:none;
  background:conic-gradient(var(--ok) calc(var(--p)*1%),rgba(255,255,255,.06) 0);
  -webkit-mask:radial-gradient(circle,transparent 64%,#000 65%);
  mask:radial-gradient(circle,transparent 64%,#000 65%);
}
.knob.hold .ring{display:block}
.knob:active{transform:scale(.97)}
.devcap{font-size:12px;color:var(--mut);text-align:center}
.devcap b{color:var(--txt);font-weight:650}
</style>
</head>
<body>
<div class="wrap">
  <header>
    <a class="back" href="/" aria-label="Geri">&larr;</a>
    <div>
      <h1>Panel testi</h1>
      <div class="sub">Oda termostat&#305; sim&#252;lasyonu</div>
    </div>
    <div class="pill" id="pill"><span class="dot"></span><span id="pillTxt">Ba&#287;lan&#305;yor</span></div>
  </header>

  <div class="card">
    <div class="device">
      <div class="screen" id="scr">
        <canvas id="oled" width="128" height="64"></canvas>
        <div class="off">Cihazdan ekran al&#305;namad&#305;.<br>Ayn&#305; a&#287;da oldu&#287;unuzu kontrol edin.</div>
      </div>
      <div class="knobrow">
        <button class="rbtn" id="rotL" title="Sola &#231;evir / bas&#305;l&#305; tut: &#246;nceki tema">&#8634;</button>
        <button class="knob" id="knob" title="Bas">
          <span class="ring"></span>
        </button>
        <button class="rbtn" id="rotR" title="Sa&#287;a &#231;evir / bas&#305;l&#305; tut: sonraki tema">&#8635;</button>
      </div>
      <div class="devcap">Ekran: <b id="pageTxt">&mdash;</b></div>
    </div>
  </div>

  <div class="card">
    <div class="label">Kontroller</div>
    <div class="kv"><span>&#199;evir</span><b>Hedef s&#305;cakl&#305;k</b></div>
    <div class="kv"><span>K&#305;sa bas</span><b>Sonraki sayfa</b></div>
    <div class="kv"><span>Uzun bas (1.2 sn)</span><b>Modu a&#231; / kapat</b></div>
    <div class="kv"><span>Sol / sa&#287; tu&#351;u bas&#305;l&#305; tut</span><b>&#214;nceki / sonraki tema</b></div>
    <div class="kv"><span>Cihazda: buton bas&#305;l&#305; + &#231;evir</span><b>Tema de&#287;i&#351;tir</b></div>
    <div class="hint" id="tip">
      Ekran cihaz&#305;n ger&#231;ek OLED i&#231;eri&#287;idir; d&#252;&#287;meler cihaz&#305;n kendi
      enkoder ve buton mant&#305;&#287;&#305;n&#305; &#231;al&#305;&#351;t&#305;r&#305;r.
    </div>
  </div>
</div>

<script>
(function () {
  var $ = function (id) { return document.getElementById(id); };
  var cv = $('oled'), ctx = cv.getContext('2d');
  var W = 128, H = 64;
  var img = ctx.createImageData(W, H);
  var ON = [236, 247, 255, 255], OFF = [6, 10, 18, 255];
  var NAMES = ['ANA EKRAN', 'A\u011e B\u0130LG\u0130S\u0130', 'B\u0130LG\u0130', 'UYKU'];

  // SSD1306 framebuffer: 8 sayfa x 128 bayt, her baytin bit 0'i ust piksel.
  function paint(hex, inv) {
    var on = inv ? OFF : ON, off = inv ? ON : OFF;
    var d = img.data;
    for (var i = 0; i < 1024; i++) {
      var v = parseInt(hex.slice(i * 2, i * 2 + 2), 16) || 0;
      var x = i & 127, y0 = (i >> 7) << 3;
      for (var b = 0; b < 8; b++) {
        var o = ((y0 + b) * W + x) * 4;
        var c = ((v >> b) & 1) ? on : off;
        d[o] = c[0]; d[o + 1] = c[1]; d[o + 2] = c[2]; d[o + 3] = 255;
      }
    }
    ctx.putImageData(img, 0, 0);
  }

  function setLink(ok) {
    var p = $('pill');
    p.className = 'pill ' + (ok ? 'ok' : 'bad');
    $('pillTxt').textContent = ok ? 'Ba\u011fl\u0131' : 'Ba\u011flant\u0131 yok';
  }

  var inFlight = false;
  function tick() {
    if (inFlight) return;
    inFlight = true;
    fetch('/api/oled', { cache: 'no-store' }).then(function (r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      return r.json();
    }).then(function (j) {
      if (!j || !j.ok) throw new Error('bad frame');
      paint(j.px, j.invert);
      $('scr').className = 'screen';
      setLink(true);
      $('pageTxt').textContent = j.ask
        ? 'D\u0130L SE\u00c7\u0130M\u0130'
        : ((j.page + 1) + '/' + NAMES.length + ' \u00b7 ' + (NAMES[j.page] || NAMES[0]));
      $('tip').textContent = j.ask
        ? 'Dil se\u00e7im ekran\u0131: \u00e7evirerek dili se\u00e7in, k\u0131sa basarak onaylay\u0131n.'
        : ('Bu kare cihazdan 1:1 al\u0131nd\u0131' + (j.dim ? ' \u00b7 tema lo\u015f \u0131\u015f\u0131k' : '') + '.');
    }).catch(function () {
      $('scr').className = 'screen dead';
      setLink(false);
    }).then(function () { inFlight = false; });
  }

  function act(url) {
    fetch(url, { method: 'POST', cache: 'no-store' }).then(function (r) {
      if (!r.ok) throw new Error('HTTP ' + r.status);
      setLink(true);
    }).catch(function () { setLink(false); });
    setTimeout(tick, 320);   // OLED yenileme araligindan (250 ms) sonra
  }

  function rotate(n) { act('/api/encoder?steps=' + n); }
  function press(ms) { act('/api/button?ms=' + Math.round(ms)); }
  function themeStep(n) { act('/api/theme?step=' + n); }

  // ---- cevirme dugmeleri ----
  // Kisa dokunus = enkoder cevirme (hedef). 450 ms basili tutunca tema
  // degisir: sol onceki, sag sonraki; ekran ayni sayfada canli guncellenir.
  function wireRotate(el, dir) {
    var hold = 0, rep = 0, downAt = 0, themed = false;
    function stop() {
      clearTimeout(hold); clearInterval(rep);
      hold = 0; rep = 0;
    }
    el.addEventListener('pointerdown', function (e) {
      e.preventDefault();
      try { el.setPointerCapture(e.pointerId); } catch (err) {}
      themed = false;
      downAt = Date.now();
      hold = setTimeout(function () {
        themed = true;
        themeStep(dir);
        rep = setInterval(function () { themeStep(dir); }, 700);
      }, 450);
    });
    function release() {
      if (!hold && !rep) return;
      stop();
      if (!themed && Date.now() - downAt < 450) rotate(dir);
    }
    el.addEventListener('pointerup', release);
    el.addEventListener('pointercancel', release);
  }
  wireRotate($('rotL'), -1);
  wireRotate($('rotR'), 1);

  // ---- enkoder dugmesi: kisa bas = sayfa, 1.2 sn = mod ----
  var knob = $('knob'), t0 = 0, raf = 0;
  function endHold() {
    if (!t0) return;
    var ms = performance.now() - t0;
    t0 = 0;
    knob.className = 'knob';
    knob.style.setProperty('--p', 0);
    cancelAnimationFrame(raf);
    press(Math.max(60, Math.min(ms, 4000)));
  }
  knob.addEventListener('pointerdown', function (e) {
    e.preventDefault();
    try { knob.setPointerCapture(e.pointerId); } catch (err) {}
    knob.className = 'knob hold';
    t0 = performance.now();
    (function loop() {
      if (!t0) return;
      knob.style.setProperty('--p', Math.min(100, (performance.now() - t0) / 1200 * 100).toFixed(1));
      raf = requestAnimationFrame(loop);
    })();
  });
  knob.addEventListener('pointerup', endHold);
  knob.addEventListener('pointercancel', endHold);

  // ---- ekran uzerinde tekerlek ----
  var wheelAcc = 0, wheelT = 0;
  $('scr').addEventListener('wheel', function (e) {
    e.preventDefault();
    wheelAcc += e.deltaY;
    clearTimeout(wheelT);
    wheelT = setTimeout(function () { wheelAcc = 0; }, 300);
    if (Math.abs(wheelAcc) >= 50) {
      rotate(wheelAcc > 0 ? 1 : -1);
      wheelAcc = 0;
    }
  }, { passive: false });

  // ---- klavye ----
  window.addEventListener('keydown', function (e) {
    if (e.shiftKey && !e.repeat && (e.key === 'ArrowLeft' || e.key === 'ArrowRight')) {
      e.preventDefault(); themeStep(e.key === 'ArrowRight' ? 1 : -1);
    }
    else if (e.key === 'ArrowLeft') { e.preventDefault(); rotate(-1); }
    else if (e.key === 'ArrowRight') { e.preventDefault(); rotate(1); }
    else if (e.key === ' ' || e.key === 'Enter') { e.preventDefault(); press(120); }
  });

  setInterval(tick, 900);
  tick();
})();
</script>
</body>
</html>
)TEST";
