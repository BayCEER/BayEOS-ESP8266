#include <ESP8266WebServer.h>
#include "js.h"
const char TABLE_STYLE[] PROGMEM = "<style>table{border-collapse:collapse;width:100%;}td,th{border:1px solid #ddd;padding: 8px;}tr:nth-child(even){background-color: #f2f2f2;}tr:hover {background-color: #ddd;}th{padding-top:12px;padding-bottom:12px;text-align:left;background-color:#4CAF50;color:white;}</style>";
const char BACK_MAINPAGE[] PROGMEM = "<form action=\"/\" method=\"get\"><button style=\"max-width:1200px;\">Back to main page</button></form>";

ESP8266WebServer server(80);
void handleRoot() {
  String message = FPSTR(HTTP_HEAD);
  message.replace("{v}", "BayEOS WIFI RF24 Router");
  message += FPSTR(HTTP_STYLE);
  message += FPSTR(TABLE_STYLE);
  message += FPSTR(HTTP_HEAD_END);
  message += String(F("<h1>BayEOS WIFI RF24 Router</h1>"));
  message += String(F("<p><table><tr><th colspan=2>Info</th></tr><tr><td>Name</td><td>"));
  message += cfg.bayeos_name;
  message += String(F("</td></tr><tr><td>Gateway</td><td><a href=\"http://"));
  message += cfg.bayeos_gateway;
  message += String(F("/gateway/\">"));
  message += cfg.bayeos_gateway;
  message += String(F("</a></td></tr><tr><td>Total TX</td><td>"));
  message += total_tx;
  message += String(F("</td></tr><tr><td>Total TX Errors</td><td>"));
  message += total_tx_error;
  message += String(F("</td></tr><tr><td>Free Space</td><td>"));
  message += myBuffer.freeSpace();
  message += String(F(" Byte</td></tr><tr><td>Unsent Data</td><td>"));
  message += myBuffer.available();
  message += String(F(" Byte</td></tr></table>"));
  message += String(F("<br/><table><tr><th colspan=2>RF24-Configuration</th></tr><tr><td>Channel</td><td>0x"));
  message += String(cfg.rf24_channel, HEX);
  message += String(F("</td></tr><tr><td>Total RX</td><td>"));
  message += total_rx;
  message += String(F("</td></tr><tr><td>Accept</td><td>"));
  if (WITH_RF24_CHECKSUM)
    message += String(F("Only frames with checksum"));
  else
    message += String(F("All"));

  message += String(F("</td></tr></table><br/><table><tr><th>Pipe</th><th>Address</th><th>RX</th>"));
  if (WITH_RF24_CHECKSUM)
    message += String(F("<th>CRC failed</th>"));
  message += String(F("</tr>"));

  char pipe_ends[6][3] = {"12", "24", "48", "96", "ab", "bf"};
  for (uint8_t i = 0; i < 6; i++) {
    message += "<tr><td><a href=\"/chart?p=";
    message += i;
    message += "\">P";
    message += i;
    message += "</a></td><td>0x";
    message += String(cfg.rf24_base, HEX);
    message += pipe_ends[i];
    message += "</td><td>";
    message += rx_per_pipe[i];
    if (WITH_RF24_CHECKSUM) {
      message += "</td><td>";
      message += rx_per_pipe_failed[i];
    }
    message += "</td></tr>";
  }
  message += "</table>";
  message += "</p>";

  message += String(F("<form action=\"/config\" method=\"get\"><button>Configure Router</button></form>"));
  message += FPSTR(HTTP_END);

  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/html", message);
}

void handleConfig() {
  String message = FPSTR(HTTP_HEAD);
  message.replace("{v}", "BayEOS WIFI RF24 Router");
  message += FPSTR(HTTP_STYLE);
  message += String(F("<style>select{width:95%;padding:5px;font-size:1em;}</style>"));
  message += FPSTR(HTTP_HEAD_END);
  message += String(F("<form method='get' action='save'><h4>BayEOS-Gateway Configuration</h4><input id='bayeos_name' name='bayeos_name' maxlength=40 placeholder='Origin' value='"));
  message += cfg.bayeos_name;
  message += String(F("' ><br/><input id='server' name='server' maxlength=40 placeholder='BayEOS Gateway' value='"));
  message += cfg.bayeos_gateway;
  message += String(F("' ><br/><input id='bayeos_user' name='bayeos_user' maxlength=40 placeholder='BayEOS User' value='"));
  message += cfg.bayeos_user;
  message += String(F("' ><br/><input id='bayeos_pw' name='bayeos_pw' maxlength=40 placeholder='BayEOS Password' value='"));
  message += cfg.bayeos_pw;
  message += String(F("' ><br/><h4>RF24-Configuration</h4><input id='rf24_channel' name='rf24_channel' maxlength=2 placeholder='RF24 Channel (HEX)' value='"));
  message += String(RF24_CHANNEL, HEX);
  message += String(F("' ><br/><input id='rf24base' name='rf24base' maxlength=8 placeholder='RF24 pipe base (HEX)' value='"));
  message += String(cfg.rf24_base, HEX);
  message += String(F("' ><br/><select id='rf24_checksum' name='rf24_checksum'><option value=0>Accept all frames</option><option value=1"));
  if(WITH_RF24_CHECKSUM) message+= String(F(" selected"));
  message += String(F(">Accept only frames with Checksum</option></select>"));
  message += String(F("<p>Channel and rf24 pipe base must be given in hex numbers.<br/>The resulting listen pipes will be:</p><ul><li>0x_BASE_12</li><li>0x_BASE_24</li><li>0x_BASE_48</li><li>0x_BASE_96</li><li>0x_BASE_ab</li><li>0x_BASE_bf</li></ul>"));
  message += String(F("<h4>LED-Configuration</h4><select id='disable_leds' name='disable_leds'><option value=0>LEDs enabled</option><option value=1"));
  if(cfg.disable_leds) message+= String(F(" selected"));
  message += String(F(">LEDs disabled</option></select>"));
  message += String(F("<br/><h4 style=\"color:#f00\">Note: You must hold PROG on save!!!</h4><button type='submit'>save</button></form>"));
  message += String(F("<br/>"));
  message += FPSTR(BACK_MAINPAGE);
  message += FPSTR(HTTP_END);
  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/html", message);
}

void handleBin() {
  String message = "[";
  char tmp[60];
  char df_buffer[37];
  if (server.args() < 2) {
    strcpy(tmp, "0");
  } else {
    server.arg(0).toCharArray(tmp, 2);
  }
  uint8_t p = atoi(tmp);
  if (p > 5) p = 0;
  unsigned long m;
  if (server.args() > 1) {
    server.arg(1).toCharArray(tmp, 20);
    m = atol(tmp);
  } else m = 0;
  m++;
  uint8_t i = rx_i[p];
  uint8_t e = (i - 1);
  if (e >= NR_RX_BUFFER) e = NR_RX_BUFFER-1;
  unsigned long diff;
  uint8_t num_entries = 0;
  while (true) {
    if (i >= NR_RX_BUFFER) i = 0;
    diff = rx_time[p][i] - m;
    if ((diff < 3600000 || m == 1) && rx_length[p][i]) {
      diff = millis() - rx_time[p][i];
      df_buffer[0] = BayEOS_DelayedFrame;
      *(unsigned long*)(df_buffer + 1) = diff;
      memcpy(df_buffer + 5, payload[p][i], rx_length[p][i]);
      base64_encode(tmp, df_buffer, rx_length[p][i] + 5);
      tmp[base64_enc_len(rx_length[p][i] + 5)] = 0;
      if (num_entries) message += ",";
      message += "[";
      message += rx_time[p][i];
      message += ",\"";
      message += tmp;
      message += "\"]";
      num_entries++;
    }
    if (i == e) break;
    i++;
  }
  message += "]";
  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/json", message);
}

void handlePipe() {
  String message = FPSTR(HTTP_HEAD);
  char tmp[2];
  if (server.args() < 1) {
    strcpy(tmp, "0");
  } else {
    server.arg(0).toCharArray(tmp, 2);
  }
  uint8_t p = atoi(tmp);
  message.replace("{v}", "BayEOS WIFI RF24 Router");
  message += FPSTR(HTTP_STYLE);
  message += FPSTR(TABLE_STYLE);
  message += FPSTR(HTTP_HEAD_END);
  message += String(F("<h1>BayEOS WIFI RF24 Router</h1>"));
  message += String(F("<h3>Pipe "));
  message += p;
  message += String(F("</h3><table><tr><th>RX-Time</th><th>Size</th><th>Data</th></tr> "));
  uint8_t i = rx_i[p] - 1;
  uint8_t has_data = 0;
  while (true) {
    if (i >= NR_RX_BUFFER) i = NR_RX_BUFFER-1;
    if (rx_length[p][i]) {
      has_data = 1;
      message += String(F("<tr><td>"));
      message += (millis() - rx_time[p][i]) / 1000;
      message += String(F(" seconds before"));
      debug_client.startFrame();
      for (uint8_t j = 0; j < rx_length[p][i]; j++) {
        debug_client.addToPayload(payload[p][i][j]);
      }
      debug_client.sendPayload();
      message += String(F("</td><td>"));
      message += rx_length[p][i];
      message += String(F("</td><td><pre>"));
      message += debug_client.get();
      message += "</pre></td></tr>";
    }
    if (i == rx_i[p]) break;
    i--;
  }
  if (! has_data) message += String(F("<tr><td colspan=3>No data</td></tr>"));
  message += String(F("</table>"));
  if ( has_data) {
    message += String(F("<br/><form action=\"/chart\" method=\"get\"><input type=\"hidden\" name=\"p\" value=\""));
    message += p;
    message += String(F("\"><button>Chart</button></form>"));
  }
  message += String(F("<br/>"));
  message += FPSTR(BACK_MAINPAGE);
  message += FPSTR(HTTP_END);
  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/html", message);

}


void handleChart() {
  String message = FPSTR(HTTP_HEAD);
  char tmp[2];
  if (server.args() < 1) {
    strcpy(tmp, "0");
  } else {
    server.arg(0).toCharArray(tmp, 2);
  }
  uint8_t p = atoi(tmp);
  message.replace("{v}", "BayEOS WIFI RF24 Router");
  message += FPSTR(HTTP_STYLE);
  message += FPSTR(HIGHCHART_JS1);
  message += p;
  message += FPSTR(HIGHCHART_JS2);
  message += FPSTR(HTTP_HEAD_END);
  message += String(F("<h1>BayEOS WIFI RF24 Router</h1></div>"));
  message += String(F("<div id=\"container\"></div>"));
  message += String(F("<div style=\"text-align:center; width:100%;\">"));
  message += String(F("<form action=\"/pipe\" method=\"get\"><input type=\"hidden\" name=\"p\" value=\""));
  message += p;
  message += String(F("\"><button style=\"max-width:1200px;\">Frame Details</button></form><br/>"));
  message += FPSTR(BACK_MAINPAGE);
  message += FPSTR(HTTP_END);
  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/html", message);

}


void handleSave() {
  String message = FPSTR(HTTP_HEAD);
  message.replace("{v}", "BayEOS WIFI RF24 Router");
  message += FPSTR(HTTP_STYLE);
  message += FPSTR(HTTP_HEAD_END);
  if (! digitalRead(0)) {
    message += String(F("<h4>New configuration saved</h4>"));
    server.arg(0).toCharArray(cfg.bayeos_name, 40);
    server.arg(1).toCharArray(cfg.bayeos_gateway, 40);
    server.arg(2).toCharArray(cfg.bayeos_user, 40);
    server.arg(3).toCharArray(cfg.bayeos_pw, 40);

    char tmp[10];
    server.arg(4).toCharArray(tmp, 10);
    cfg.rf24_channel = strtol(tmp, 0, 16);
    RF24_CHANNEL = cfg.rf24_channel;
    server.arg(5).toCharArray(tmp, 10);
    cfg.rf24_base = strtol(tmp, 0, 16);
    *(long*)(pipe_0 + 1) = cfg.rf24_base;
    *(long*)(pipe_1 + 1) = cfg.rf24_base;
    server.arg(6).toCharArray(tmp, 10);
    cfg.rf24_checksum = atoi(tmp);
    WITH_RF24_CHECKSUM = cfg.rf24_checksum;
    server.arg(7).toCharArray(tmp, 10);
    cfg.disable_leds = atoi(tmp);
    saveConfig();
    client.setConfig(cfg.bayeos_name, cfg.bayeos_gateway, port, path, cfg.bayeos_user, cfg.bayeos_pw);
    initRF24();

  } else {
    message += String(F("<h4 style=\"color:#f00\">Configuration not saved! You have to hold PROG on save!!!</h4>"));

  }
  message += FPSTR(BACK_MAINPAGE);
  message += FPSTR(HTTP_END);
  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/html", message);

}

void handleBayEOSParser_JS() {
  String message = FPSTR(bayeosParser_js);
  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/javascript", message);

}
void handleBase64_min_JS() {
  String message = FPSTR(base64_min_js);
  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/javascript", message);

}


void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

