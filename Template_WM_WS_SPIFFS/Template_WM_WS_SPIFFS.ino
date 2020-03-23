/*
 * Template for a Application with
 * WIFI-Manager
 * Webserver
 * SPIFFS
 * Websocket
 * 
 * You have to upload the sketch _AND_ the data in two steps
 * for the data use ESP8266 Sketch Data Upload tool
 * https://github.com/esp8266/arduino-esp8266fs-plugin
 * 
 * 
 */
#define RX_LED D1
#define TX_LED D2

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>

#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#include <WebSocketsServer.h>
WebSocketsServer webSocket(81);    // create a websocket server on port 81
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
const char app_name[]="MyApplication";

#include "config.h"
#include "socket.h"
#include "handler.h"

void setup(void) {
  Serial.begin(115200);
  Serial.println("Starting...");
  pinMode(RX_LED, OUTPUT);
  digitalWrite(TX_LED, HIGH);
  pinMode(TX_LED, OUTPUT);

  
  EEPROM.begin(sizeof(cfg));
  delay(1000);
  //WiFiManager
  WiFiManager wifiManager;

  if (! digitalRead(0)) {
    uint8_t delete_count=0;
    while(! digitalRead(0) && delete_count<10){
      digitalWrite(RX_LED,LOW);
      delay(50);
      digitalWrite(RX_LED, HIGH);
      delay(100);
      delete_count++;
    }
    if(! digitalRead(0)){
      eraseConfig();
      wifiManager.resetSettings();
      digitalWrite(TX_LED, LOW);
      delay(200);
      digitalWrite(TX_LED, HIGH);
    }
    digitalWrite(RX_LED,LOW);    
  }
  //set static ip
  wifiManager.setAPStaticIPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 254), IPAddress(255, 255, 255, 0));
  wifiManager.setConfigPortalTimeout(180);
  if (!wifiManager.autoConnect(app_name, "bayeos24")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  
  SPIFFS.begin();
  Serial.println("SPIFFS ok");
  loadConfig();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists
  server.begin();
  digitalWrite(RX_LED, HIGH);
  digitalWrite(TX_LED, LOW);

}

void loop(void) {
  webSocket.loop();                           // constantly check for websocket events
  sendEvent();
  server.handleClient();
}
