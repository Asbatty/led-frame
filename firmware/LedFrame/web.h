#pragma once
// ---- Phone control page (http://matrix.local), JSON API and wireless updates ----
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Update.h>

WebServer server(80);

static const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>LED Frame</title>
<style>
:root{--bg:#f5f4ef;--card:#fff;--tx:#1d1d1b;--mut:#6b6a65;--bd:#dddcd5;--ac:#2f6fdc;--ok:#2e7d32}
@media(prefers-color-scheme:dark){:root{--bg:#1c1c1a;--card:#262624;--tx:#ecebe6;--mut:#a3a29c;--bd:#3a3a37;--ac:#6ea0ff;--ok:#7bc77f}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--tx);font:16px/1.45 system-ui,sans-serif}
main{max-width:460px;margin:0 auto;padding:16px}h1{font-size:20px;font-weight:600;margin:4px 0 2px}
.mut{color:var(--mut);font-size:13px}.card{background:var(--card);border:1px solid var(--bd);border-radius:12px;padding:14px;margin:12px 0}
h2{font-size:14px;font-weight:600;margin:0 0 10px;color:var(--mut)}.row{display:flex;gap:8px;align-items:center;flex-wrap:wrap}
button{font:inherit;font-size:14px;padding:8px 12px;border-radius:8px;border:1px solid var(--bd);background:transparent;color:var(--tx)}
button.on{background:var(--ac);border-color:var(--ac);color:#fff}button.main{width:100%;margin-top:10px;background:var(--ac);border-color:var(--ac);color:#fff}
input[type=text],input[type=date]{font:inherit;width:100%;padding:8px 10px;border-radius:8px;border:1px solid var(--bd);background:transparent;color:var(--tx)}
input[type=range]{flex:1}.sw{width:30px;height:30px;border-radius:50%;padding:0;border:3px solid transparent}.sw.on{border-color:var(--tx)}
.msg{font-size:13px;min-height:18px;margin-top:6px}.ok{color:var(--ok)}.err{color:#d33}
</style></head><body><main>
<h1>LED Frame</h1><div class="mut" id="status">Loading…</div>
<div class="card"><h2>Display</h2>
<div class="row"><button id="power">On</button><span class="mut">Brightness</span><input type="range" id="bri" min="1" max="255" step="1"><span id="briv" class="mut"></span></div></div>
<div class="card"><h2>Screen</h2><div class="row" id="modes"></div></div>
<div class="card"><h2>Message</h2><input type="text" id="msg" maxlength="100" placeholder="WELCOME HOME">
<div class="row" id="colors" style="margin-top:10px"></div>
<button class="main" id="send">Send to frame</button><div class="msg" id="msgr"></div></div>
<div class="card"><h2>Countdown</h2><input type="text" id="cdn" maxlength="15" placeholder="BEACH TRIP">
<input type="date" id="cdd" style="margin-top:8px"><button class="main" id="cds">Save countdown</button><div class="msg" id="cdr"></div></div>
<div class="card"><h2>Test</h2><div class="row"><button id="ta">Show test alert</button><button id="tr">Rain alert</button></div></div>
<div class="mut" id="foot"></div>
</main><script>
const MODES=["Auto","Clock","Message","Countdown","Art"];
const COLS=[["Rainbow","conic-gradient(red,yellow,lime,cyan,blue,magenta,red)"],["White","#fff"],["Pink","#ff3c8c"],["Cyan","#00c8ff"],["Amber","#ffaa00"],["Green","#30ff60"],["Red","#ff2020"]];
let st={},color="Rainbow";const $=id=>document.getElementById(id);
async function api(path,body){const r=await fetch(path,body?{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(body)}:{});if(!r.ok)throw new Error(await r.text());return r.json()}
function render(){
 $("power").textContent=st.on?"On":"Off";$("power").className=st.on?"on":"";
 $("bri").value=st.brightness;$("briv").textContent=Math.round(st.brightness/2.55)+"%";
 $("modes").innerHTML="";MODES.forEach(m=>{const b=document.createElement("button");b.textContent=m;b.className=st.mode===m?"on":"";b.onclick=()=>save({mode:m});$("modes").appendChild(b)});
 $("colors").innerHTML="";COLS.forEach(([n,c])=>{const b=document.createElement("button");b.className="sw"+(color===n?" on":"");b.style.background=c;b.title=n;b.setAttribute("aria-label",n);b.onclick=()=>{color=n;render()};$("colors").appendChild(b)});
 const w=st.weather||{};$("status").textContent=(st.mqtt?"Home Assistant connected":"Home Assistant not connected")+(w.cond?" · "+w.cond+" "+Math.round(w.temp)+"°":"");
 $("foot").innerHTML="Firmware "+st.fw+" · "+st.ip+" · Wi-Fi "+st.rssi+" dBm · <a href='/update'>Update firmware</a>";
}
async function load(){st=await api("/api/state");color=st.color;$("msg").value=st.message;$("cdn").value=st.cd_name;$("cdd").value=st.cd_date;render()}
async function save(p){try{st=await api("/api/state",p);render()}catch(e){$("status").textContent="Couldn't reach the frame. Try again."}}
$("power").onclick=()=>save({on:!st.on});
$("bri").oninput=()=>{$("briv").textContent=Math.round($("bri").value/2.55)+"%"};$("bri").onchange=()=>save({brightness:+$("bri").value});
$("msg").oninput=()=>{$("msgr").textContent=""};
$("send").onclick=async()=>{const m=$("msg").value.trim();if(!m){$("msgr").className="msg err";$("msgr").textContent="Enter a message first";return}
 await save({message:m,color:color});$("msgr").className="msg ok";$("msgr").textContent="Sent"};
$("cds").onclick=async()=>{const d=$("cdd").value;if(!d){$("cdr").className="msg err";$("cdr").textContent="Pick a date first";return}
 await save({cd_name:$("cdn").value.trim(),cd_date:d});$("cdr").className="msg ok";$("cdr").textContent="Saved"};
$("ta").onclick=()=>api("/api/alert",{text:"TEST ALERT",icon:"bell",duration:8});
$("tr").onclick=()=>api("/api/alert",{text:"RAIN IN 30 MIN",icon:"rain",color:"#3399ff",duration:10});
load();setInterval(()=>api("/api/state").then(s=>{st=s;render()}).catch(()=>{}),15000);
</script></body></html>)HTML";

static const char UPDATE_PAGE[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Update firmware</title></head>
<body style="font:16px system-ui;max-width:460px;margin:24px auto;padding:0 16px">
<h1 style="font-size:20px">Update firmware</h1><p>Choose the <code>LedFrame.ino.bin</code> file.</p>
<form method="POST" action="/update" enctype="multipart/form-data"><input type="file" name="fw" accept=".bin">
<p><button>Upload</button></p></form><p><a href="/">Back</a></p></body></html>)HTML";

void sendState() {
  JsonDocument d;
  buildStateJson(d);
  String out;
  serializeJson(d, out);
  server.send(200, "application/json", out);
}

bool updateAuth() {
  if (server.authenticate("admin", OTA_PASSWORD)) return true;
  server.requestAuthentication();
  return false;
}

void setupWeb() {
  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", PAGE); });
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/state", HTTP_POST, [] {
    JsonDocument d;
    if (deserializeJson(d, server.arg("plain")) || !d.is<JsonObject>()) { server.send(400, "text/plain", "Bad JSON"); return; }
    applyPatch(d.as<JsonObjectConst>());
    sendState();
  });
  server.on("/api/alert", HTTP_POST, [] {
    alertFromPayload(server.arg("plain"));
    server.send(200, "application/json", "{\"ok\":true}");
  });
  // Debug: the last drawn frame as text, '.' = black, otherwise the RGB565 value in hex
  server.on("/api/frame", HTTP_GET, [] {
    String out;
    out.reserve(PANEL_W * PANEL_H * 5 + 40);
    uint16_t* buf = canvas.getBuffer();
    for (int y = 0; y < PANEL_H; y++) {
      for (int x = 0; x < PANEL_W; x++) {
        uint16_t c = buf[y * PANEL_W + x];
        if (c == 0) out += "   .";
        else { char h[6]; snprintf(h, sizeof h, "%4x", c); out += h; }
      }
      out += '\n';
    }
    server.send(200, "text/plain", out);
  });
  // Panel tuning while running, e.g. /api/tune?lat=3 (latch blanking 1-8, fixes ghosting)
  server.on("/api/tune", HTTP_GET, [] {
    String out = "ok";
    if (server.hasArg("lat")) {
      int n = constrain(server.arg("lat").toInt(), 1, 8);
      display->setLatBlanking(n);
      out += " lat=" + String(n);
    }
    if (server.hasArg("flip")) {  // takes effect immediately, saved for next boot
      panelFlip = server.arg("flip") == "1";
      prefs.begin("panel", false);
      prefs.putBool("flip", panelFlip);
      prefs.end();
      out += panelFlip ? " flipped" : " normal";
    }
    if (server.hasArg("wxdemo")) {  // cycle all weather scenes on screen
      String v = server.arg("wxdemo");  // 1 = cycle all, 0 = off, or a weather state to hold
      wxDemo = v != "0";
      wxDemoCond = (v == "1" || v == "0") ? String("") : v;
      wxDemoStart = millis();
      out += wxDemo ? " weather demo " + (wxDemoCond.length() ? wxDemoCond : String("cycling")) : " weather demo off";
    }
    if (server.hasArg("freeze")) {
      artFrozen = server.arg("freeze") == "1";
      artFrozenAt = millis();
      out += artFrozen ? " frozen" : " moving";
    }
    // Colour depth and refresh need a restart to take effect
    if (server.hasArg("depth") || server.hasArg("refresh") || server.hasArg("clkphase")) {
      prefs.begin("panel", false);
      if (server.hasArg("clkphase")) prefs.putBool("clkphase", server.arg("clkphase") == "1");
      if (server.hasArg("depth")) prefs.putUChar("depth", constrain(server.arg("depth").toInt(), 3, 8));
      if (server.hasArg("refresh")) prefs.putUShort("refresh", constrain(server.arg("refresh").toInt(), 30, 240));
      prefs.end();
      server.send(200, "text/plain", "saved, restarting");
      delay(300);
      ESP.restart();
    }
    server.send(200, "text/plain", out);
  });
  server.on("/update", HTTP_GET, [] { if (updateAuth()) server.send_P(200, "text/html", UPDATE_PAGE); });
  server.on("/update", HTTP_POST,
    [] {
      if (!updateAuth()) return;
      bool ok = !Update.hasError();
      server.send(200, "text/html", ok ? "Updated. Restarting… <a href='/'>Back</a>" : "Update failed. <a href='/update'>Try again</a>");
      if (ok) { delay(500); ESP.restart(); }
    },
    [] {
      if (!server.authenticate("admin", OTA_PASSWORD)) return;
      HTTPUpload& up = server.upload();
      if (up.status == UPLOAD_FILE_START) { showStatus("UPDATING", "DON'T UNPLUG"); Update.begin(UPDATE_SIZE_UNKNOWN); }
      else if (up.status == UPLOAD_FILE_WRITE) Update.write(up.buf, up.currentSize);
      else if (up.status == UPLOAD_FILE_END) Update.end(true);
    });
  server.onNotFound([] { server.send(404, "text/plain", "Not found"); });
  server.begin();

  MDNS.begin(HOSTNAME);
  MDNS.addService("http", "tcp", 80);

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setMdnsEnabled(false);  // mDNS already started above
  ArduinoOTA.onStart([] { showStatus("UPDATING", "DON'T UNPLUG"); });
  ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
    canvas.fillRect(4, 26, 56, 3, c565(0x1E1E1E));
    canvas.fillRect(4, 26, (int)(56ULL * done / total), 3, c565(0x00C88C));
    pushFrame();
  });
  ArduinoOTA.onError([](ota_error_t) { showStatus("UPDATE", "FAILED", "", 0xFF2020); });
  ArduinoOTA.begin();
}
