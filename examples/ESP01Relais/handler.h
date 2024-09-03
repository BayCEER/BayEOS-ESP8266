#include <ESP8266WebServer.h>
const char TABLE_STYLE[] PROGMEM = "<style>table{border-collapse:collapse;width:100%;}td,th{border:1px solid #ddd;padding: 8px;}tr:nth-child(even){background-color: #f2f2f2;}tr:hover {background-color: #ddd;}th{padding-top:12px;padding-bottom:12px;text-align:left;background-color:#4CAF50;color:white;}</style>";
const char BACK_MAINPAGE[] PROGMEM = "<form action=\"/\" method=\"get\"><button style=\"max-width:1200px;\">Back to main page</button></form>";


ESP8266WebServer server(80);
void handleRoot() {
  String message = FPSTR(HTTP_HEADER);
  message.replace("{v}", "ESP01Relais");
  message += FPSTR(HTTP_STYLE);
  message += FPSTR(TABLE_STYLE);
  message += FPSTR(HTTP_HEADER_END);
  message +="<h1>";
  message += String(F("</h1>"));
  message += String(F("<table><tr><th colspan=2>Info</th></tr><tr><td>Relais</td><td>"));
  if(relais) message += "on";
  else message+="off";
  message += String(F("</td></tr><tr><td>Ontime remaining</td><td>"));
  message += relais_on;
  message += String(F(" min</td></tr></table>"));

  uint8_t c, t;
  char on_off[3];
  if(relais){
	  strcpy(on_off,"ff");
	  c=0;
	  t=0;
  } else {
	  strcpy(on_off,"n");
	  c=1;
	  t=12;
  }
  message += String(F("<form action=\"/command\"><input type=\"hidden\" name=\"c\" value=\""));
  message += c;
  message += String(F("\">"));
  if(relais) message += String(F("<input type=\"hidden\" name=\"t\" value=\"0\">"));
  else {
    message +=String(F("<select name=\"t\" style=\"width:100%;padding:3px;\">"));
    for(uint8_t i=1;i<25;i++){
      message += String(F("<option value=\""));
      message +=i*SWITCH_OFF_FACTOR;
      message += String(F("\">"));
      message +=i*SWITCH_OFF_FACTOR;
      message += String(F(" min</option>"));
    }
     message += String(F("<option value=\"65535\">no limit</option></select>"));
  }
  message += String(F("<input type=\"hidden\" name=\"h\" value=\"1\">"));
  message += String(F("<button>Switch o"));
  message += on_off;
  message += String(F("</button></form>"));
  
  message += FPSTR(HTTP_END);

  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/html", message);
}


void handleCommand(){
  char buffer[6];
  server.arg(0).toCharArray(buffer,6);
  uint8_t command=atoi(buffer);
  server.arg(1).toCharArray(buffer,6);
  relais_on=atoi(buffer);
  relais_minutes=relais_on;
  relais_millis=millis();
  digitalWrite(0, command);
  relais=command;

  String message=""; 
  if(server.args()>2){
  message +=FPSTR(HTTP_HEADER);
  message.replace("{v}", "ESP01Relais");
  message += FPSTR(HTTP_STYLE);
  message += FPSTR(HTTP_HEADER_END);
  message += String(F("<h4>Command successful</h4>"));
  message += FPSTR(BACK_MAINPAGE);
  message += FPSTR(HTTP_END);
  server.sendHeader("Content-Length", String(message.length()));
  server.send(200, "text/html", message);
    
  } else {
    message+=relais;
    server.send(200, "text/plain", message);
  }
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
