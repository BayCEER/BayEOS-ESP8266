#include <ESP8266WebServer.h>
#include "BayEOSCommands.h"
#include "js.h"
const char TABLE_STYLE[] PROGMEM = "<style>table{border-collapse:collapse;width:100%;}td,th{border:1px solid #ddd;padding: 8px;}tr:nth-child(even){background-color: #f2f2f2;}tr:hover {background-color: #ddd;}th{padding-top:12px;padding-bottom:12px;text-align:left;background-color:#4CAF50;color:white;}</style>";
const char BACK_MAINPAGE[] PROGMEM = "<form action=\"/\" method=\"get\"><button style=\"max-width:1200px;\">Back to main page</button></form>";

ESP8266WebServer server(80);
void handleRoot() {
  String message = FPSTR(HTTP_HEADER);
  message.replace("{v}", router_name);
  message += FPSTR(HTTP_STYLE);
  message += FPSTR(TABLE_STYLE);
  message += FPSTR(HTTP_HEADER_END);
  message += String(F("<h1>Serial2BayEOS Router</h1>"));
  message += String(F("<p><table><tr><th colspan=2>Info</th></tr><tr><td>Name</td><td>"));
  message += cfg.bayeos_name;
  message += String(F("</td></tr><tr><td>Gateway</td><td><a href=\"http://"));
  message += cfg.bayeos_gateway;
  message += String(F("/gateway/\">"));
  message += cfg.bayeos_gateway;
  message += String(F("</a></td></tr><tr><td>RX Count</td><td><a href=\"/chart\">"));
  message += rx_count;
  message += String(F("</a></td></tr><tr><td>Free Space</td><td>"));
  message += myBuffer.freeSpace();
  message += String(F(" Byte</td></tr><tr><td>Unsent Data</td><td>"));
  message += myBuffer.available();
  message += String(F(" Byte</td></tr></table>"));

  message += String(F("<form action=\"/config\" method=\"get\"><button>Configure Router</button></form>"));
  message += FPSTR(HTTP_END);

  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/html", message);
}

void handleConfig() {
  String message = FPSTR(HTTP_HEADER);
  message.replace("{v}", router_name);
  message += FPSTR(HTTP_STYLE);
  message += String(F("<style>select{width:95%;padding:5px;font-size:1em;}</style>"));
  message += FPSTR(HTTP_HEADER_END);
  message += String(F("<form method='get' action='save'><h4>BayEOS-Gateway Configuration</h4><input id='bayeos_name' name='bayeos_name' maxlength=40 placeholder='Origin' value='"));
  message += cfg.bayeos_name;
  message += String(F("' ><br/><input id='server' name='server' maxlength=40 placeholder='BayEOS Gateway' value='"));
  message += cfg.bayeos_gateway;
  message += String(F("' ><br/><input id='bayeos_user' name='bayeos_user' maxlength=40 placeholder='BayEOS User' value='"));
  message += cfg.bayeos_user;
  message += String(F("' ><br/><input id='bayeos_pw' name='bayeos_pw' maxlength=40 placeholder='BayEOS Password' value='"));
  message += cfg.bayeos_pw;
  message += String(F("' >"));
  message += String(F("<br/><button type='submit'>save</button></form>"));
  message += String(F("<br/>"));
  message += FPSTR(BACK_MAINPAGE);
  message += FPSTR(HTTP_END);
  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/html", message);
}

void handleSave() {
  String message = FPSTR(HTTP_HEADER);
  message.replace("{v}", router_name);
  message += FPSTR(HTTP_STYLE);
  message += FPSTR(HTTP_HEADER_END);
  if (! digitalRead(0)) {
    message += String(F("<h4>New configuration saved</h4>"));
    server.arg(0).toCharArray(cfg.bayeos_name, 40);
    server.arg(1).toCharArray(cfg.bayeos_gateway, 40);
    server.arg(2).toCharArray(cfg.bayeos_user, 40);
    server.arg(3).toCharArray(cfg.bayeos_pw, 40);

    saveConfig();
    client.setConfig(cfg.bayeos_name, cfg.bayeos_gateway, port, path, cfg.bayeos_user, cfg.bayeos_pw);

  } else {
    message += String(F("<h4 style=\"color:#f00\">Configuration not saved! You have to hold PROG on save!!!</h4>"));

  }
  message += FPSTR(BACK_MAINPAGE);
  message += FPSTR(HTTP_END);
  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/html", message);

}


void handleBin() {
  unsigned long diff;
  uint8_t num_entries = 0;
  String message = "[";
  char tmp[150];
  char df_buffer[102];
  unsigned long m=0;
  if (server.args() > 0) {
    server.arg(0).toCharArray(tmp, 20);
    m = atol(tmp);
  }
  m++;

  unsigned long read_pos=myBuffer.readPos();
  myBuffer.seekReadPointer(myBuffer.endPos());
  while(myBuffer.available()){
    myBuffer.initNextPacket();
    diff = myBuffer.packetMillis()-m;
    if(diff<3600000 || m == 1){
      df_buffer[0] = BayEOS_DelayedFrame;
      *(unsigned long*)(df_buffer + 1) = millis()-myBuffer.packetMillis();
      myBuffer.readPacket((uint8_t*)df_buffer+5);
      base64_encode(tmp, df_buffer, myBuffer.packetLength() + 5);
      tmp[base64_enc_len(myBuffer.packetLength() + 5)] = 0;
      if (num_entries) message += ",";
      message += "[";
      message += myBuffer.packetMillis();
      message += ",\"";
      message += tmp;
      message += "\"]";
      num_entries++;
    }
    myBuffer.next();
  }
  myBuffer.seekReadPointer(read_pos);
  message += "]";
  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/json", message);
}


void handleChart() {
  String message = FPSTR(HTTP_HEADER);
  message.replace("{v}", router_name);
  message += FPSTR(HTTP_STYLE);
  message += FPSTR(HIGHCHART_JS1);
  message += FPSTR(HIGHCHART_JS2);
  message += FPSTR(HTTP_HEADER_END);
  message += String(F("<h1>Serial2BayEOS</h1></div>"));
  message += String(F("<div id=\"container\"></div>"));
  message += String(F("<div style=\"text-align:center; width:100%;\"><button style=\"max-width:1200px;\" id=\"button\">Hide series</button><br/><br/>"));
  message += FPSTR(HIGHCHART_BUTTON);
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
