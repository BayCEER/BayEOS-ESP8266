/*
   Logger Connector

   You have to upload the sketch _and_ the data in two steps
   for the data use ESP8266 Sketch Data Upload tool
   https://github.com/esp8266/arduino-esp8266fs-plugin

   ESP opens a AP "LoggerConnector"
   Password is bayeos24

   use a browser to connect to http://192.168.4.1

   Install the following libraries via library manager:
    - WebSockets (by Markus Sattler)
    - ArduinoJSON
    - RF24

*/
#define SOFTWARE_VERSION "1.0.4"
const char* ssid = "LoggerConnector2";
const char* password = "bayeos24";  // set to "" for open access point w/o password
#define WITH_BAT 1
#define WITH_SERIAL 0


#define LED_GREEN D1
#define LED_RED D2


#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
DNSServer dns;
#define DNS_PORT 53

#include <Ticker.h>  //Ticker Library

#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#include <WebSocketsServer.h>
WebSocketsServer webSocket(81);    // create a websocket server on port 81

#include "config.h"
#include "logger.h"
#include "socket.h"
#include "handler.h"
Ticker rf_poll;

void setup(void) {
  Serial.begin(115200);
  Serial.println("Starting...");
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, HIGH);
  pinMode(LED_GREEN, OUTPUT);


  EEPROM.begin(sizeof(cfg));
  delay(1000);
  if (! digitalRead(0)) {
    uint8_t delete_count = 0;
    while (! digitalRead(0) && delete_count < 10) {
      digitalWrite(LED_GREEN, LOW);
      delay(50);
      digitalWrite(LED_GREEN, HIGH);
      delay(100);
      delete_count++;
    }
    if (! digitalRead(0)) {
      eraseConfig();
      digitalWrite(LED_RED, LOW);
      delay(200);
      digitalWrite(LED_RED, HIGH);
    }
    digitalWrite(LED_GREEN, LOW);
  }

  SPIFFS.begin();
  Serial.println("SPIFFS ok");
  loadConfig();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  /* Setup the DNS server redirecting all the domains to the apIP */
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.println("WIFI is up");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  server.on("/download", handleDownload);
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists
  rf_poll.attach_ms(3, poll); //call in backgroud every 5 ms
  server.begin();
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);

}

void loop(void) {
  webSocket.loop();                           // constantly check for websocket events
  //Handle DNS-Request
  dns.processNextRequest();
  sendEvent();
  handleLogger();
  server.handleClient();
}
