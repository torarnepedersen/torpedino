// =====================================================================
//  TORPEDINO C5 – KJERNE  (ikke endre denne filen)
//  WiFi-provisjonering, OTA-oppdatering og webserver.
//
//  Din kode hører hjemme i torpedino.ino – ikke her.
//
//  Krever arduino-esp32 (Board Manager-pakken "esp32") versjon som
//  støtter ESP32-C5 – bruk nyeste 3.x-versjon.
// =====================================================================

#include <WiFi.h>
#include <Wire.h>

// Kildekode-nedlasting – oppdater URL ved ny GitHub-release
#define DOWNLOAD_URL "https://github.com/torarnepedersen/torpedino/releases/download/c5-v0.0003/torpedino-c5.zip"

// Definert i torpedino_png.ino / pinwizard_html.ino
// (kompileres etter denne filen – ekstern frem-deklarasjon nødvendig)
extern const uint8_t torpedino_png[];
extern const size_t  torpedino_png_len;
extern const uint8_t pinwizard_html[];
extern const size_t  pinwizard_html_len;
#include <WebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Update.h>

#define HOSTNAME        "torpedino-c5"
#define AP_SSID         "torpedino-c5-setup"
#define AP_PASS         ""          // Åpent AP – sett passord om ønskelig
#define CONNECT_TIMEOUT 15000       // ms

Preferences prefs;
WebServer   server(80);
bool        apMode = false;

// ─────────────────────────────────────────────────────────────
//  HTML-sider
// ─────────────────────────────────────────────────────────────

static const char INDEX_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="no">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>torpedino</title>
<style>
  body{font-family:sans-serif;max-width:480px;margin:40px auto;padding:0 16px}
  h1{margin-bottom:4px}
  .card{background:#f5f5f5;border-radius:8px;padding:16px;margin:16px 0}
  .status{font-weight:bold;color:%SC%}
  a.btn{display:inline-block;padding:10px 18px;background:#0d6efd;color:#fff;
        border-radius:6px;text-decoration:none;margin:4px 2px}
  a.btn.green{background:#198754}
  a.btn.grey{background:#6c757d}
  details{background:#f5f5f5;border-radius:8px;padding:12px 16px;margin:16px 0}
  summary{cursor:pointer;font-weight:bold}
  ol{margin:10px 0 4px 18px;padding:0;line-height:1.7}
  code{background:#e9ecef;padding:1px 5px;border-radius:3px;font-size:13px}
</style>
</head>
<body>
<h1>torpedino</h1>
<div class="card">
  <p>Modus:&nbsp;<span class="status">%MODE%</span></p>
  <p>IP:&nbsp;<strong>%IP%</strong></p>
  %EXTRA%
</div>
<a class="btn"       href="/wifi"      >WiFi-innstillinger</a>
<a class="btn green" href="/update"  >Oppdater firmware</a>
<a class="btn" style="background:#6f42c1" href="/pinwizard">&#128268; Pin-veiviser</a>
<a class="btn grey"  href="%DLURL%" target="_blank">Last ned kildekode</a>
<details>
<summary>Hvordan bruke kildekoden</summary>
<ol>
  <li>Klikk <strong>Last ned kildekode</strong> – zip-filen lastes ned fra GitHub.</li>
  <li>Pakk ut zip-filen – du får en mappe som heter <code>torpedino</code> med fem filer inni.</li>
  <li>Dobbelklikk <code>torpedino.ino</code> for å åpne sketchen i Arduino IDE.<br>
      Alle filene åpnes automatisk som faner – de auto-genererte filene skal ikke endres.</li>
  <li>Velg riktig kort:<br>
      <code>Verktøy → Kort → esp32 → ESP32C5 Dev Module</code></li>
  <li>Velg nettverksporten under <code>Verktøy → Port → torpedino-c5</code> og trykk Last opp.<br>
      Brettet oppdateres trådløst – ingen USB nødvendig.</li>
  <li>Skriv din egen kode i <code>userSetup()</code> og <code>userLoop()</code> i <code>torpedino.ino</code>.<br>
      <code>core.ino</code> skal ikke endres.</li>
</ol>
</details>
</body>
</html>)html";

static const char WIFI_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="no">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>WiFi – torpedino</title>
<style>
  body{font-family:sans-serif;max-width:480px;margin:40px auto;padding:0 16px}
  input{width:100%;padding:10px;margin:6px 0;box-sizing:border-box;
        border:1px solid #ccc;border-radius:4px;font-size:15px}
  button{padding:10px 18px;border:none;border-radius:6px;cursor:pointer;
         font-size:15px;margin:4px 2px}
  .primary{background:#0d6efd;color:#fff}
  .secondary{background:#6c757d;color:#fff}
  .net{display:flex;justify-content:space-between;padding:10px;
       border:1px solid #ddd;border-radius:4px;margin:4px 0;cursor:pointer}
  .net:hover{background:#e9ecef}
  #msg{padding:10px;border-radius:4px;margin:10px 0;display:none}
  .ok{background:#d1e7dd;color:#0a3622}
  .err{background:#f8d7da;color:#58151c}
  a{color:#0d6efd}
</style>
</head>
<body>
<h1>WiFi-innstillinger</h1>
<button class="secondary" onclick="scan()">&#128268; Søk etter nettverk</button>
<div id="list"></div>
<form onsubmit="connect(event)">
  <input id="ssid" placeholder="SSID" required>
  <input id="pass" type="password" placeholder="Passord (la stå tom for åpent nettverk)">
  <button class="primary" type="submit">Koble til</button>
</form>
<div id="msg"></div>
<br><a href="/">&#8592; Tilbake</a>
<script>
function scan(){
  document.getElementById('list').innerHTML='<p>Søker...</p>';
  fetch('/scan').then(r=>r.json()).then(nets=>{
    const d=document.getElementById('list');
    if(!nets.length){d.innerHTML='<p>Ingen nettverk funnet.</p>';return;}
    d.innerHTML=nets.sort((a,b)=>b.rssi-a.rssi).map(n=>
      `<div class="net" onclick="document.getElementById('ssid').value='${n.ssid.replace(/'/g,"\\'")}';document.getElementById('pass').focus()">
        <span>${n.ssid}</span>
        <span>${n.rssi}&nbsp;dBm${n.secure?' &#128274;':''}</span>
      </div>`
    ).join('');
  }).catch(()=>document.getElementById('list').innerHTML='<p>Søk feilet.</p>');
}
function connect(e){
  e.preventDefault();
  const msg=document.getElementById('msg');
  msg.style.display='none';
  fetch('/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(document.getElementById('ssid').value)
        +'&pass='+encodeURIComponent(document.getElementById('pass').value)
  }).then(r=>r.json()).then(d=>{
    msg.className=d.ok?'ok':'err';
    msg.textContent=d.msg;
    msg.style.display='block';
  });
}
</script>
</body>
</html>)html";

static const char UPDATE_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="no">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Firmware – torpedino</title>
<style>
  body{font-family:sans-serif;max-width:480px;margin:40px auto;padding:0 16px}
  input[type=file]{margin:14px 0;font-size:15px}
  button{padding:10px 18px;background:#198754;color:#fff;border:none;
         border-radius:6px;cursor:pointer;font-size:15px}
  #wrap{background:#e9ecef;border-radius:4px;margin:10px 0;overflow:hidden;display:none}
  #bar{width:0;height:22px;background:#0d6efd;transition:width .2s;text-align:center;
       color:#fff;font-size:13px;line-height:22px}
  #status{margin:8px 0}
  a{color:#0d6efd}
</style>
</head>
<body>
<h1>Firmware-oppdatering</h1>
<p>Last opp en <code>.bin</code>-fil kompilert for torpedino.</p>
<form id="form">
  <input type="file" id="file" accept=".bin" required>
  <button type="submit">&#8593; Last opp</button>
</form>
<div id="wrap"><div id="bar">0%</div></div>
<div id="status"></div>
<br><a href="/">&#8592; Tilbake</a>
<script>
document.getElementById('form').onsubmit=function(e){
  e.preventDefault();
  const file=document.getElementById('file').files[0];
  if(!file)return;
  const xhr=new XMLHttpRequest();
  const status=document.getElementById('status');
  const wrap=document.getElementById('wrap');
  const bar=document.getElementById('bar');
  wrap.style.display='block';
  xhr.upload.onprogress=function(ev){
    if(ev.lengthComputable){
      const p=Math.round(ev.loaded/ev.total*100)+'%';
      bar.style.width=p;bar.textContent=p;
    }
  };
  xhr.onload=function(){
    if(xhr.status===200){
      bar.style.background='#198754';bar.textContent='100%';
      status.textContent='Suksess! Enheten starter på nytt...';
    } else {
      bar.style.background='#dc3545';
      status.textContent='Feil: '+xhr.responseText;
    }
  };
  xhr.onerror=()=>status.textContent='Nettverksfeil under opplasting.';
  const fd=new FormData();
  fd.append('firmware',file);
  xhr.open('POST','/do-update');
  xhr.send(fd);
};
</script>
</body>
</html>)html";

// ─────────────────────────────────────────────────────────────
//  WiFi
// ─────────────────────────────────────────────────────────────

bool connectToWiFi(const String& ssid, const String& pass) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < CONNECT_TIMEOUT)
        delay(200);
    return WiFi.status() == WL_CONNECTED;
}

void startAP() {
    apMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.printf("AP: %s  IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
}

// ─────────────────────────────────────────────────────────────
//  Web-handlere
// ─────────────────────────────────────────────────────────────

void handleRoot() {
    String html = FPSTR(INDEX_HTML);
    if (apMode) {
        html.replace("%MODE%", "Tilgangspunkt (AP)");
        html.replace("%SC%",   "#e67e22");
        html.replace("%IP%",   WiFi.softAPIP().toString());
        html.replace("%EXTRA%","<p>Ikke koblet til WiFi – konfigurer nedenfor.</p>");
    } else {
        html.replace("%MODE%", "Tilkoblet WiFi");
        html.replace("%SC%",   "#198754");
        html.replace("%IP%",   WiFi.localIP().toString());
        html.replace("%EXTRA%","<p>Nettverk:&nbsp;<strong>" + WiFi.SSID() + "</strong></p>");
    }
    html.replace("%DLURL%", DOWNLOAD_URL);
    server.send(200, "text/html; charset=utf-8", html);
}

void handleWifi() {
    server.send(200, "text/html; charset=utf-8", FPSTR(WIFI_HTML));
}

void handleScan() {
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i) json += ',';
        String ssid = WiFi.SSID(i);
        ssid.replace("\"", "\\\"");
        json += "{\"ssid\":\"" + ssid + "\""
              + ",\"rssi\":"   + WiFi.RSSI(i)
              + ",\"secure\":" + (WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false")
              + "}";
    }
    json += "]";
    WiFi.scanDelete();
    server.send(200, "application/json", json);
}

void handleConnect() {
    if (!server.hasArg("ssid") || server.arg("ssid").isEmpty()) {
        server.send(400, "application/json", "{\"ok\":false,\"msg\":\"Mangler SSID.\"}");
        return;
    }
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();

    server.send(200, "application/json",
        "{\"ok\":true,\"msg\":\"Lagret! torpedino starter på nytt og kobler til \\\"" + ssid + "\\\".\"}");
    delay(1500);
    ESP.restart();
}

void handleUpdate() {
    server.send(200, "text/html; charset=utf-8", FPSTR(UPDATE_HTML));
}

void handleDoUpdateUpload() {
    HTTPUpload& up = server.upload();
    if (up.status == UPLOAD_FILE_START) {
        Serial.printf("OTA start: %s\n", up.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN))
            Update.printError(Serial);
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (Update.write(up.buf, up.currentSize) != up.currentSize)
            Update.printError(Serial);
    } else if (up.status == UPLOAD_FILE_END) {
        if (Update.end(true))
            Serial.printf("OTA ferdig: %u bytes\n", up.totalSize);
        else
            Update.printError(Serial);
    }
}

void handlePinwizard() {
    server.sendHeader("Content-Encoding", "gzip");
    server.sendHeader("Cache-Control", "no-cache");
    server.send_P(200, "text/html; charset=utf-8",
                  (const char*)pinwizard_html, pinwizard_html_len);
}

void handleTorpedinoPng() {
    server.sendHeader("Cache-Control", "max-age=3600");
    server.send_P(200, "image/png",
                  (const char*)torpedino_png, torpedino_png_len);
}

void handleDoUpdateFinish() {
    if (Update.hasError()) {
        server.send(500, "text/plain", String("OTA feilet: ") + Update.errorString());
    } else {
        server.send(200, "text/plain", "OK");
        delay(500);
        ESP.restart();
    }
}

// ─────────────────────────────────────────────────────────────
//  Pin-veiviser – live demo-handler
// ─────────────────────────────────────────────────────────────

static int demoP[6];

static bool demoPins(int n) {
    for (int i = 0; i < n; i++) {
        String k = "p" + String(i);
        if (!server.hasArg(k)) return false;
        demoP[i] = server.arg(k).toInt();
    }
    return true;
}

static void demoOk(const String& msg) {
    server.send(200, "application/json",
        "{\"ok\":true,\"msg\":\"" + msg + "\"}");
}

static void demoErr(const String& msg) {
    server.send(200, "application/json",
        "{\"ok\":false,\"msg\":\"" + msg + "\"}");
}

static void servoMove(int pin, int angle) {
    int pulse = 500 + angle * 2000 / 180;  // µs, 500–2500
    pinMode(pin, OUTPUT);
    for (int i = 0; i < 30; i++) {
        portDISABLE_INTERRUPTS();
        digitalWrite(pin, HIGH);
        delayMicroseconds(pulse);
        digitalWrite(pin, LOW);
        portENABLE_INTERRUPTS();
        delayMicroseconds(20000 - pulse);
    }
}

void handleDemo() {
    String dev    = server.arg("dev");
    String action = server.arg("action");

    if (dev == "led") {
        if (!demoPins(1)) { demoErr("Mangler pin"); return; }
        pinMode(demoP[0], OUTPUT);
        if      (action == "on")    { digitalWrite(demoP[0], HIGH); demoOk("LED GPIO " + String(demoP[0]) + " er PÅ"); }
        else if (action == "off")   { digitalWrite(demoP[0], LOW);  demoOk("LED GPIO " + String(demoP[0]) + " er AV"); }
        else if (action == "blink") {
            for (int i = 0; i < 5; i++) { digitalWrite(demoP[0], HIGH); delay(200); digitalWrite(demoP[0], LOW); delay(200); }
            demoOk("LED blinket 5 ganger");
        }
        return;
    }
    if (dev == "rgb") {
        if (!demoPins(3)) { demoErr("Mangler pinner"); return; }
        for (int i = 0; i < 3; i++) pinMode(demoP[i], OUTPUT);
        bool r = false, g = false, b = false;
        if      (action == "red")    { r=1; }
        else if (action == "green")  { g=1; }
        else if (action == "blue")   { b=1; }
        else if (action == "yellow") { r=1; g=1; }
        else if (action == "cyan")   { g=1; b=1; }
        else if (action == "white")  { r=1; g=1; b=1; }
        digitalWrite(demoP[0], r); digitalWrite(demoP[1], g); digitalWrite(demoP[2], b);
        demoOk("RGB: " + action);
        return;
    }
    if (dev == "servo") {
        if (!demoPins(1)) { demoErr("Mangler pin"); return; }
        int angle = (action == "left") ? 0 : (action == "right") ? 180 : 90;
        servoMove(demoP[0], angle);
        demoOk("Servo GPIO " + String(demoP[0]) + " -> " + String(angle) + " grader");
        return;
    }
    if (dev == "stepper_uln2003") {
        if (!demoPins(4)) { demoErr("Mangler pinner"); return; }
        for (int i = 0; i < 4; i++) pinMode(demoP[i], OUTPUT);
        static const uint8_t seq[8][4] = {
            {1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},
            {0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}
        };
        int dir = (action == "ccw") ? -1 : 1;
        for (int s = 0; s < 512; s++) {
            int idx = ((s * dir) % 8 + 8) % 8;
            for (int i = 0; i < 4; i++) digitalWrite(demoP[i], seq[idx][i]);
            delay(2);
        }
        for (int i = 0; i < 4; i++) digitalWrite(demoP[i], LOW);
        demoOk("512 half-steg " + String(action == "ccw" ? "mot klokken" : "med klokken"));
        return;
    }
    if (dev == "stepper_step") {
        if (!demoPins(2)) { demoErr("Mangler pinner"); return; }
        pinMode(demoP[0], OUTPUT); pinMode(demoP[1], OUTPUT);
        digitalWrite(demoP[1], action == "ccw" ? HIGH : LOW);
        delayMicroseconds(5);
        for (int i = 0; i < 200; i++) {
            digitalWrite(demoP[0], HIGH); delayMicroseconds(500);
            digitalWrite(demoP[0], LOW);  delayMicroseconds(500);
        }
        demoOk("200 steg " + String(action == "ccw" ? "mot klokken" : "med klokken"));
        return;
    }
    if (dev == "dc_l298n") {
        if (!demoPins(3)) { demoErr("Mangler pinner"); return; }
        pinMode(demoP[0], OUTPUT); pinMode(demoP[1], OUTPUT); pinMode(demoP[2], OUTPUT);
        if (action == "stop") {
            digitalWrite(demoP[0], LOW); demoOk("Motor stoppet");
        } else {
            bool fwd = (action == "forward");
            digitalWrite(demoP[1], fwd ? HIGH : LOW);
            digitalWrite(demoP[2], fwd ? LOW  : HIGH);
            digitalWrite(demoP[0], HIGH); delay(1500); digitalWrite(demoP[0], LOW);
            demoOk("Motor " + String(fwd ? "fremover" : "bakover") + " i 1.5 sekund");
        }
        return;
    }
    if (dev == "hcsr04") {
        if (!demoPins(2)) { demoErr("Mangler pinner"); return; }
        pinMode(demoP[0], OUTPUT); pinMode(demoP[1], INPUT);
        digitalWrite(demoP[0], LOW);  delayMicroseconds(2);
        digitalWrite(demoP[0], HIGH); delayMicroseconds(10);
        digitalWrite(demoP[0], LOW);
        long dur = pulseIn(demoP[1], HIGH, 30000);
        if (dur == 0) { demoErr("Ingen respons – sjekk kobling (spenningsdeler på ECHO!)"); return; }
        float cm = dur * 0.034f / 2.0f;
        server.send(200, "application/json",
            "{\"ok\":true,\"msg\":\"Avstand: " + String(cm, 1) + " cm  (" + String(dur) + " µs)\"}");
        return;
    }
    if (dev == "pot") {
        if (!demoPins(1)) { demoErr("Mangler pin"); return; }
        analogReadResolution(12);
        int val = analogRead(demoP[0]);
        float volt = val * 3.3f / 4095.0f;
        server.send(200, "application/json",
            "{\"ok\":true,\"msg\":\"ADC: " + String(val) + " / 4095  (" +
            String(volt, 2) + " V)  " + String(val * 100 / 4095) + "%\"}");
        return;
    }
    if (dev == "button") {
        if (!demoPins(1)) { demoErr("Mangler pin"); return; }
        pinMode(demoP[0], INPUT_PULLUP);
        int s = digitalRead(demoP[0]);
        server.send(200, "application/json",
            "{\"ok\":true,\"msg\":\"Knapp: " +
            String(s == LOW ? "TRYKKET (LOW)" : "ikke trykket (HIGH)") + "\"}");
        return;
    }
    if (dev == "pir") {
        if (!demoPins(1)) { demoErr("Mangler pin"); return; }
        pinMode(demoP[0], INPUT);
        int s = digitalRead(demoP[0]);
        server.send(200, "application/json",
            "{\"ok\":true,\"msg\":\"PIR: " +
            String(s == HIGH ? "BEVEGELSE OPPDAGET!" : "ingen bevegelse (LOW)") + "\"}");
        return;
    }
    if (dev == "i2c_scan") {
        if (!demoPins(2)) { demoErr("Mangler pinner (SDA, SCL)"); return; }
        Wire.begin(demoP[0], demoP[1]);
        String found = "";
        int count = 0;
        for (uint8_t addr = 8; addr < 120; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                if (count++) found += ", ";
                char buf[6];
                snprintf(buf, sizeof(buf), "0x%02X", addr);
                found += buf;
            }
        }
        Wire.end();
        if (count == 0)
            demoErr("Ingen I²C-enheter funnet på SDA=GPIO" + String(demoP[0]) + " SCL=GPIO" + String(demoP[1]));
        else
            demoOk("Funnet: " + found);
        return;
    }
    demoErr("Ukjent enhet: " + dev);
}

// ─────────────────────────────────────────────────────────────
//  Setup & loop  –  kaller inn i brukerens kode
// ─────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\ntorpedino starter...");

    prefs.begin("wifi", true);
    String ssid = prefs.getString("ssid", "");
    String pass = prefs.getString("pass", "");
    prefs.end();

    bool connected = false;
    if (ssid.length() > 0) {
        Serial.printf("Kobler til '%s'...\n", ssid.c_str());
        connected = connectToWiFi(ssid, pass);
        if (!connected) Serial.println("Tilkobling feilet.");
    }

    if (!connected) {
        Serial.println("Starter AP-modus.");
        startAP();
    } else {
        apMode = false;
        Serial.printf("Tilkoblet! IP: %s\n", WiFi.localIP().toString().c_str());
        if (MDNS.begin(HOSTNAME))
            Serial.println("mDNS klar: http://torpedino-c5.local");

        ArduinoOTA.setHostname(HOSTNAME);
        ArduinoOTA.onStart([]() { Serial.println("ArduinoOTA: starter"); });
        ArduinoOTA.onEnd([]()   { Serial.println("\nArduinoOTA: ferdig"); });
        ArduinoOTA.onProgress([](unsigned int prog, unsigned int total) {
            Serial.printf("OTA: %u%%\r", prog * 100 / total);
        });
        ArduinoOTA.onError([](ota_error_t e) { Serial.printf("OTA feil [%u]\n", e); });
        ArduinoOTA.begin();
    }

    server.on("/",              HTTP_GET,  handleRoot);
    server.on("/pinwizard",     HTTP_GET,  handlePinwizard);
    server.on("/torpedino.png", HTTP_GET,  handleTorpedinoPng);
    server.on("/demo",          HTTP_GET,  handleDemo);
    server.on("/wifi",          HTTP_GET,  handleWifi);
    server.on("/scan",      HTTP_GET,  handleScan);
    server.on("/connect",   HTTP_POST, handleConnect);
    server.on("/update",    HTTP_GET,  handleUpdate);
    server.on("/do-update", HTTP_POST, handleDoUpdateFinish, handleDoUpdateUpload);
    server.begin();
    Serial.println("Webserver klar.");

    userSetup();
}

void loop() {
    server.handleClient();
    if (!apMode)
        ArduinoOTA.handle();

    userLoop();
}
