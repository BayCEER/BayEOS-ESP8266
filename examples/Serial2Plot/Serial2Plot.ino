/*
   This Sketch reads serial data an stores it on SD card

   default SSID: SerialPlotter
   default Password: bayeos24

   Webfrontend on http://192.168.4.1

   Commands via WebSocket
   setConf
   getConf
   setTime
   getAll

   Install the following libraries via library manager:
   - WebSockets (by Markus Sattler)
   - ArduinoJSON


*/
#define SW_VERSION "1.0.0"

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

#define WITH_DNS 1
#if WITH_DNS
#define DNS_PORT 53
#include <DNSServer.h>
DNSServer dns;
#endif

#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#include <WebSocketsServer.h>
WebSocketsServer webSocket(81);    // create a websocket server on port 81

#include "config.h"
#include "serial.h"
#include "socket.h"
#include "handler.h"


void setup(void) {
  Serial.setRxBufferSize(1024);

  EEPROM.begin(sizeof(cfg));
  SPIFFS.begin();
  loadConfig();
  Serial.begin(cfg.baud);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(cfg.ssid, cfg.password);
  delay(100);
  IPAddress Ip(192, 168, 4, 1);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists
  server.begin();
  Serial.println();
  Serial.println("End of setup");
#if WITH_DNS
  /* Setup the DNS server redirecting all the domains to the apIP */
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(DNS_PORT, "serial.plot", WiFi.softAPIP());
#endif
}

void loop(void) {
  webSocket.loop();                           // constantly check for websocket events
  server.handleClient();
  handleSerial();
#if WITH_DNS
  dns.processNextRequest();
#endif
}
