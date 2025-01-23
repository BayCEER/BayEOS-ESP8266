/*
   Logger Connector

   You have to upload the sketch _and_ the data in two steps
   for the data use ESP8266 Sketch Data Upload tool
   https://github.com/esp8266/arduino-esp8266fs-plugin

   ESP opens a AP "BayEOSLogger"
   Password is bayeos24

   use a browser to connect to http://192.168.4.1

   Install the following libraries via library manager:
    - WebSockets (by Markus Sattler)
    - ArduinoJSON

*/
#define SOFTWARE_VERSION "1.0.0"
const long baud=38400;
const char* ssid = "BayEOSLogger02";
const char* password = "bayeos24";  // set to "" for open access point w/o password




#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
DNSServer dns;
#define DNS_PORT 53


#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#include <WebSocketsServer.h>
WebSocketsServer webSocket(81);    // create a websocket server on port 81

#include "config.h"
#include "logger.h"
#include "socket.h"
#include "handler.h"

void setup(void) {


  EEPROM.begin(sizeof(cfg));
  delay(1000);
  loadConfig();
  client.begin(cfg.baud);

  SPIFFS.begin();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(cfg.ssid, cfg.password);
  /* Setup the DNS server redirecting all the domains to the apIP */
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.println("WIFI is up");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  server.on("/download", handleDownload);
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists
  server.begin();


}

void loop(void) {
  webSocket.loop();                           // constantly check for websocket events
  //Handle DNS-Request
  dns.processNextRequest();
  sendEvent();
  handleLogger();
  server.handleClient();
}
