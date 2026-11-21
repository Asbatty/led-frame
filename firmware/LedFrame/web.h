#pragma once
// ---- Watch-face style control app (http://matrix.local), JSON API and wireless updates ----
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <Update.h>

WebServer server(80);

static const char PAGE[] PROGMEM = R"HTML(<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0c0d10"><meta name="mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-capable" content="yes"><meta name="apple-mobile-web-app-title" content="LED Frame">
<link rel="manifest" href="/manifest.json"><title>LED Frame</title>
<style>
:root{--bg:#0c0d10;--card:#16181d;--card2:#1d2027;--line:#2a2e36;--tx:#ecebe6;--mut:#9a9a94;--ac:#ffc478;--ac2:#00bec8;--ok:#7bc77f;--err:#ff6b6b}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{margin:0;background:var(--bg);color:var(--tx);font:16px/1.45 system-ui,-apple-system,"Segoe UI",sans-serif}
main{max-width:560px;margin:0 auto;padding:14px 16px 40px}
header{display:flex;align-items:center;gap:10px;margin:2px 0 12px}
header h1{font-size:19px;font-weight:650;margin:0;flex:1}
.pill{font-size:12px;padding:3px 9px;border-radius:99px;border:1px solid var(--line);color:var(--mut)}
.pill.ok{color:var(--ok);border-color:#2f4d31}
button{font:inherit;font-size:14px;padding:8px 13px;border-radius:10px;border:1px solid var(--line);background:var(--card);color:var(--tx);cursor:pointer}
button.pri{background:var(--ac);border-color:var(--ac);color:#1a1306;font-weight:650}
button.on{border-color:var(--ac);color:var(--ac)}
.panel{background:#050505;border:8px solid #3a2a1e;outline:6px solid #ece6da;margin:6px 6px 12px;line-height:0}
.panel canvas{width:100%;height:auto;display:block}
.row{display:flex;align-items:center;gap:10px}
.sub{color:var(--mut);font-size:13px}
input[type=range]{flex:1;accent-color:var(--ac)}
h2{font-size:13px;letter-spacing:.06em;text-transform:uppercase;color:var(--mut);margin:22px 0 10px;font-weight:600;display:flex;align-items:center;gap:8px}
h2 .sp{flex:1}
.carousel{display:flex;gap:10px;overflow-x:auto;scroll-snap-type:x mandatory;padding:2px 2px 8px;margin:0 -16px;padding-left:16px;scrollbar-width:none}
.carousel::-webkit-scrollbar{display:none}
.face{flex:0 0 46%;max-width:210px;scroll-snap-align:start;background:var(--card);border:2px solid var(--line);border-radius:14px;padding:8px;cursor:pointer;position:relative}
.face.sel{border-color:var(--ac)}
.face canvas{width:100%;display:block;border-radius:6px;background:#050505}
.face .nm{font-size:14px;font-weight:600;margin:7px 2px 1px}
.face .st{font-size:12px;color:var(--mut);margin:0 2px}
.face .live{position:absolute;top:12px;right:12px;font-size:10px;font-weight:700;background:var(--ac);color:#1a1306;padding:1px 6px;border-radius:6px}
.card{background:var(--card);border:1px solid var(--line);border-radius:14px;padding:14px}
.card h3{margin:0 0 10px;font-size:17px;display:flex;align-items:center;gap:8px}
.card h3 span{flex:1}
.opt{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:10px 0;border-top:1px solid var(--line)}
.opt:first-of-type{border-top:0}
.opt label{font-size:15px}
.seg{display:flex;flex-wrap:wrap;gap:6px;justify-content:flex-end}
.seg button{padding:6px 10px;font-size:13px}
.sw{position:relative;width:46px;height:28px;flex:0 0 46px}
.sw input{opacity:0;width:0;height:0}
.sw i{position:absolute;inset:0;background:#3a3d45;border-radius:99px;transition:.15s}
.sw i:before{content:"";position:absolute;width:22px;height:22px;left:3px;top:3px;background:#fff;border-radius:50%;transition:.15s}
.sw input:checked+i{background:var(--ac)}
.sw input:checked+i:before{transform:translateX(18px)}
input[type=text],input[type=date]{font:inherit;width:100%;padding:9px 11px;border-radius:10px;border:1px solid var(--line);background:var(--card2);color:var(--tx)}
.colors{display:flex;gap:8px;flex-wrap:wrap}
.dot{width:30px;height:30px;border-radius:50%;padding:0;border:3px solid transparent}
.dot.on{border-color:var(--tx)}
.note{font-size:13px;color:var(--mut);margin:8px 0 0}
.msg{font-size:13px;min-height:18px;margin-top:6px}.msg.ok{color:var(--ok)}.msg.err{color:var(--err)}
.list{background:var(--card);border:1px solid var(--line);border-radius:14px;overflow:hidden}
.li{display:flex;align-items:center;gap:8px;padding:9px 12px;border-top:1px solid var(--line)}
.li:first-child{border-top:0}
.li .n{flex:1;font-size:15px}.li .n small{color:var(--mut);margin-left:6px}
.li button{padding:4px 9px}
footer{margin-top:26px;font-size:12px;color:var(--mut);text-align:center}
footer a{color:var(--ac2)}
</style></head><body><main>
<header><h1>LED Frame</h1><span class="pill" id="ha">…</span><button id="power">On</button></header>
<div class="panel"><canvas id="live" width="640" height="320"></canvas></div>
<div class="row"><span class="sub" id="showing">Loading…</span></div>
<div class="row" style="margin-top:8px"><span class="sub">Brightness</span><input type="range" id="bri" min="1" max="255" step="1"><span class="sub" id="briv" style="min-width:34px;text-align:right"></span></div>

<h2>Faces<span class="sp"></span><button id="rot">Rotate</button></h2>
<div class="carousel" id="faces"></div>

<div class="card" id="detail"></div>

<h2>Rotation order</h2>
<div class="list" id="order"></div>
<p class="note">Rotation skips faces that have nothing to show (no message, no countdown set, Basil above 10 days).</p>

<h2>Test</h2>
<div class="row"><button id="ta">Test alert</button><button id="tr">Rain alert</button></div>
<footer id="foot"></footer>
</main><script>
const OPTS={
 clock:[["clock24","24-hour time","toggle"],["clockColor","Colour","choice",["Amber","White","Cyan","Green","Pink"]],["clockStrip","Bottom line","choice",["Date","High / low","Alternate"]]],
 weather:[["wxTemp","Show temperature","toggle"]],
 message:[["msgSpeed","Scroll speed","choice",["Slow","Normal","Fast"]]],
 art:[["artPalette","Palette","choice",["Ocean","Sunset","Forest","Rainbow"]],["artClock","Show clock","toggle"]],
 countdown:[],basil:[]};
const NOTES={weather:"A stick figure acts out the weather Home Assistant sends.",basil:"Joins the rotation only when Basil has under 10 days of food left.",art:"Slow colour waves, a calm screen for the evening."};
const COLS=[["Rainbow","conic-gradient(red,yellow,lime,cyan,blue,magenta,red)"],["White","#fff"],["Pink","#ff3c8c"],["Cyan","#00c8ff"],["Amber","#ffaa00"],["Green","#30ff60"],["Red","#ff2020"]];
const $=id=>document.getElementById(id);
let st=null,sel=null,msgColor=null,busy=false,thumbs={},draft={},detailFor=null;

async function api(p,b){const r=await fetch(p,b!==undefined?{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(b)}:{});if(!r.ok)throw new Error(await r.text());return r.headers.get("content-type").includes("json")?r.json():r.arrayBuffer()}
async function save(p){try{st=await api("/api/state",p);render()}catch(e){$("showing").textContent="Couldn't reach the frame. Try again."}}

function drawLED(cv,buf){const x=cv.getContext("2d"),px=new Uint16Array(buf),s=cv.width/64;x.fillStyle="#050505";x.fillRect(0,0,cv.width,cv.height);
 for(let i=0;i<2048;i++){const c=px[i];if(c){x.fillStyle=`rgb(${(c>>11)*255/31|0},${(c>>5&63)*255/63|0},${(c&31)*255/31|0})`}else x.fillStyle="#141414";
 x.beginPath();x.arc((i%64+.5)*s,((i>>6)+.5)*s,s*.4,0,7);x.fill()}}

async function liveLoop(){if(!document.hidden&&!busy){busy=true;try{drawLED($("live"),await api("/api/frame.bin"))}catch(e){}busy=false}setTimeout(liveLoop,350)}
async function thumb(k){try{const b=await api("/api/preview?face="+k);thumbs[k]=b;const c=document.querySelector(`[data-k="${k}"] canvas`);if(c)drawLED(c,b)}catch(e){}}

function face(k){return st.faces.find(f=>f.key===k)}
function render(){
 $("power").textContent=st.on?"On":"Off";$("power").className=st.on?"on":"";
 $("ha").textContent=st.mqtt?"Home Assistant ✓":"Home Assistant offline";$("ha").className="pill"+(st.mqtt?" ok":"");
 if(document.activeElement!==$("bri")){$("bri").value=st.brightness}$("briv").textContent=Math.round(st.brightness/2.55)+"%";
 const rotating=st.mode==="Auto",shown=face(st.showing);
 $("showing").textContent=(rotating?"Rotating · now ":"Pinned · ")+(shown?shown.name:"");
 $("rot").className=rotating?"on":"";$("rot").textContent=rotating?"Rotating":"Rotate";
 if(!sel)sel=st.showing;
 const fc=$("faces");
 if(fc.children.length!==st.faces.length){fc.innerHTML="";st.faces.forEach(f=>{const d=document.createElement("div");d.className="face";d.dataset.k=f.key;
   d.innerHTML='<canvas width="256" height="128"></canvas><div class="nm"></div><div class="st"></div><span class="live">LIVE</span>';
   d.onclick=()=>{sel=f.key;render()};fc.appendChild(d);thumb(f.key)})}
 st.faces.forEach(f=>{const d=fc.querySelector(`[data-k="${f.key}"]`);d.classList.toggle("sel",f.key===sel);d.querySelector(".nm").textContent=f.name;
   d.querySelector(".st").textContent=f.enabled?(f.available?"In rotation":"In rotation · waiting"):"Not in rotation";
   d.querySelector(".live").style.display=f.key===st.showing?"":"none";if(thumbs[f.key])drawLED(d.querySelector("canvas"),thumbs[f.key])});
 // don't rebuild the options while they're being used (typing, sliding)
 const dc=$("detail");if(detailFor!==sel||!dc.contains(document.activeElement))renderDetail();
 renderOrder();
 $("foot").innerHTML="Firmware "+st.fw+" · "+st.ip+" · Wi-Fi "+st.rssi+" dBm · <a href='/update'>Update firmware</a>";
}

function sw(checked,on){const l=document.createElement("label");l.className="sw";l.innerHTML='<input type="checkbox"><i></i>';const i=l.firstChild;i.checked=checked;i.onchange=()=>on(i.checked);return l}
function opt(label,ctrl){const d=document.createElement("div");d.className="opt";const l=document.createElement("label");l.textContent=label;d.append(l,ctrl);return d}
function seg(list,cur,on){const d=document.createElement("div");d.className="seg";list.forEach(n=>{const b=document.createElement("button");b.textContent=n;if(n===cur)b.className="on";b.onclick=()=>on(n);d.appendChild(b)});return d}
async function setOpt(k,v){await save({opts:{[k]:v}});thumb(sel)}

function renderDetail(){
 const f=face(sel),c=$("detail");if(!f)return;c.innerHTML="";detailFor=sel;
 const h=document.createElement("h3");h.innerHTML="<span></span>";h.firstChild.textContent=f.name;
 const show=document.createElement("button");const isShown=st.showing===f.key&&st.mode!=="Auto";
 show.textContent=isShown?"Showing":"Show now";show.className=isShown?"on":"pri";show.onclick=()=>save({show:f.key});h.appendChild(show);c.appendChild(h);
 if(NOTES[f.key]){const p=document.createElement("p");p.className="note";p.style.margin="0 0 6px";p.textContent=NOTES[f.key];c.appendChild(p)}
 if(f.key==="message"){
   const i=document.createElement("input");i.type="text";i.maxLength=100;i.placeholder="WELCOME HOME";i.value=draft.msg??st.message;
   const cols=document.createElement("div");cols.className="colors";cols.style.margin="10px 0";msgColor=msgColor||st.color;
   COLS.forEach(([n,css])=>{const b=document.createElement("button");b.className="dot"+(n===msgColor?" on":"");b.style.background=css;b.title=n;b.setAttribute("aria-label",n);b.onclick=()=>{msgColor=n;renderDetail()};cols.appendChild(b)});
   const go=document.createElement("button");go.className="pri";go.textContent="Send to frame";go.style.width="100%";
   const m=document.createElement("div");m.className="msg";
   i.oninput=()=>{m.textContent="";draft.msg=i.value};
   go.onclick=async()=>{const v=i.value.trim();if(!v){m.className="msg err";m.textContent="Enter a message first";return}delete draft.msg;await save({message:v,color:msgColor});thumb("message");
     const m2=$("detail").querySelector(".msg");if(m2){m2.className="msg ok";m2.textContent="Sent"}};
   c.append(i,cols,go,m)}
 if(f.key==="countdown"){
   const n=document.createElement("input");n.type="text";n.maxLength=15;n.placeholder="BEACH TRIP";n.value=draft.cdn??st.cd_name;
   const d=document.createElement("input");d.type="date";d.value=draft.cdd??st.cd_date;d.style.marginTop="8px";
   n.oninput=()=>{draft.cdn=n.value};d.oninput=()=>{draft.cdd=d.value};
   const go=document.createElement("button");go.className="pri";go.textContent="Save countdown";go.style.cssText="width:100%;margin-top:10px";
   const m=document.createElement("div");m.className="msg";
   go.onclick=async()=>{if(!d.value){m.className="msg err";m.textContent="Pick a date first";return}delete draft.cdn;delete draft.cdd;await save({cd_name:n.value.trim(),cd_date:d.value});thumb("countdown");
     const m2=$("detail").querySelector(".msg");if(m2){m2.className="msg ok";m2.textContent="Saved"}};
   c.append(n,d,go,m)}
 (OPTS[f.key]||[]).forEach(([k,label,type,list])=>{c.appendChild(opt(label,type==="toggle"?sw(st.opts[k],v=>setOpt(k,v)):seg(list,st.opts[k],v=>setOpt(k,v))))});
 c.appendChild(opt("In rotation",sw(f.enabled,v=>save({faces:{[f.key]:{enabled:v}}}))));
 const r=document.createElement("div");r.className="row";const sl=document.createElement("input");sl.type="range";sl.min=f.key==="message"?10:3;sl.max=120;sl.step=1;sl.value=f.dur;
 const sv=document.createElement("span");sv.className="sub";sv.style.minWidth="40px";sv.textContent=f.dur+" s";
 sl.oninput=()=>{sv.textContent=sl.value+" s"};sl.onchange=()=>save({faces:{[f.key]:{dur:+sl.value}}});r.append(sl,sv);
 c.appendChild(opt(f.key==="message"?"Show for up to":"Show for",r));
}

function renderOrder(){
 const o=$("order");o.innerHTML="";const keys=st.faces.map(f=>f.key);
 st.faces.forEach((f,i)=>{const d=document.createElement("div");d.className="li";
  const n=document.createElement("div");n.className="n";n.textContent=f.name;const s=document.createElement("small");s.textContent=f.dur+" s";n.appendChild(s);
  const up=document.createElement("button");up.textContent="↑";up.setAttribute("aria-label","Move "+f.name+" up");up.disabled=i===0;
  const dn=document.createElement("button");dn.textContent="↓";dn.setAttribute("aria-label","Move "+f.name+" down");dn.disabled=i===keys.length-1;
  const mv=j=>{const k=keys.slice();[k[i],k[j]]=[k[j],k[i]];save({order:k})};up.onclick=()=>mv(i-1);dn.onclick=()=>mv(i+1);
  d.append(n,up,dn,sw(f.enabled,v=>save({faces:{[f.key]:{enabled:v}}})));o.appendChild(d)})}

$("power").onclick=()=>save({on:!st.on});
$("bri").oninput=()=>{$("briv").textContent=Math.round($("bri").value/2.55)+"%"};$("bri").onchange=()=>save({brightness:+$("bri").value});
$("rot").onclick=()=>save({rotate:true});
$("ta").onclick=()=>api("/api/alert",{text:"TEST ALERT",icon:"bell",duration:8});
$("tr").onclick=()=>api("/api/alert",{text:"RAIN IN 30 MIN",icon:"rain",color:"#3399ff",duration:10});
(async()=>{st=await api("/api/state");render();liveLoop()})();
setInterval(()=>{if(!document.hidden)api("/api/state").then(s=>{st=s;render()}).catch(()=>{})},2000);
setInterval(()=>{if(!document.hidden)thumb("clock")},60000);
</script></body></html>)HTML";

static const char MANIFEST[] PROGMEM = R"JSON({"name":"LED Frame","short_name":"LED Frame","start_url":"/","display":"standalone","background_color":"#0c0d10","theme_color":"#0c0d10"})JSON";

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

// Raw RGB565 pixels (64x32x2 bytes) of whatever is in the canvas
void sendCanvasBinary() {
  server.setContentLength(PANEL_W * PANEL_H * 2);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/octet-stream", "");
  server.sendContent((const char*)canvas.getBuffer(), PANEL_W * PANEL_H * 2);
}

bool updateAuth() {
  if (server.authenticate("admin", OTA_PASSWORD)) return true;
  server.requestAuthentication();
  return false;
}

void setupWeb() {
  server.on("/", HTTP_GET, [] { server.send_P(200, "text/html", PAGE); });
  server.on("/manifest.json", HTTP_GET, [] { server.send_P(200, "application/manifest+json", MANIFEST); });
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
  // Live view: the frame currently on the panel
  server.on("/api/frame.bin", HTTP_GET, sendCanvasBinary);
  // Gallery thumbnail: draw one face off-screen (sample data where real data is missing),
  // send it, then put the live frame back.
  server.on("/api/preview", HTTP_GET, [] {
    int s = faceByKey(server.arg("face"));
    if (s < 0) { server.send(404, "text/plain", "Unknown face"); return; }
    static uint16_t saved[PANEL_W * PANEL_H];
    memcpy(saved, canvas.getBuffer(), sizeof saved);
    bool demo = wxDemo;
    wxDemo = false;
    previewing = true;
    canvas.fillScreen(0);
    drawFace(s, 2600);
    previewing = false;
    wxDemo = demo;
    sendCanvasBinary();
    memcpy(canvas.getBuffer(), saved, sizeof saved);
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
