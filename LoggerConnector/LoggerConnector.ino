/*
 * Logger Connector 
 * 
 * You have to upload the sketch _and_ the data in two steps
 * for the data use ESP8266 Sketch Data Upload tool
 * https://github.com/esp8266/arduino-esp8266fs-plugin
 * 
 * ESP opens a AP "LoggerConnector" 
 * Password is bayeos24
 * 
 * use a browser to connect to http://192.168.4.1
 * 
 * Install the following libraries via library manager:
 *  - WebSockets (by Markus Sattler)
 *  - ArduinoJSON
 *  - RF24
 * 
 */
#define RX_LED D1
#define TX_LED D2

#define WITH_BAT 1

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <Ticker.h>  //Ticker Library

#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#include <WebSocketsServer.h>
WebSocketsServer webSocket(81);    // create a websocket server on port 81

#include "config.h"
#include "rf24.h"
#include "socket.h"
#include "handler.h"
const char* ssid = "LoggerConnector";
const char* password = "bayeos24";  // set to "" for open access point w/o passwortd
Ticker rf_poll;

void setup(void) {
  Serial.begin(115200);
  Serial.println("Starting...");
  pinMode(RX_LED, OUTPUT);
  digitalWrite(RX_LED, HIGH);
  pinMode(TX_LED, OUTPUT);

  
  EEPROM.begin(sizeof(cfg));
  delay(1000);
  if (! digitalRead(0)) {
    uint8_t delete_count=0;
    while(! digitalRead(0) && delete_count<10){
      digitalWrite(TX_LED,LOW);
      delay(50);
      digitalWrite(TX_LED, HIGH);
      delay(100);
      delete_count++;
    }
    if(! digitalRead(0)){
      eraseConfig();
      digitalWrite(RX_LED, LOW);
      delay(200);
      digitalWrite(RX_LED, HIGH);
    }
    digitalWrite(TX_LED,LOW);    
  }
  
  SPIFFS.begin();
  Serial.println("SPIFFS ok");
  loadConfig();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println("WIFI is up");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  server.on("/download",handleDownload);
  server.on("/downloadFull",handleDownloadFull);
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists
  rf_poll.attach_ms(5, poll); //Use <strong>attach_ms</strong> if you need time in ms
  server.begin();
  digitalWrite(RX_LED, LOW);
  digitalWrite(TX_LED, HIGH);

}

void loop(void) {
  webSocket.loop();                           // constantly check for websocket events
  sendEvent();
  handleLogger();
  server.handleClient();
}
